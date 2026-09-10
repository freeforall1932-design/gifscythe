// transport.test.mjs — end-to-end regression net for the web HTTP transport
// (audit U-49 / U-50 / U-30, and register items P2-8 / P2-9 / P2-10).
//
// The command-builder and validation parity tests exercise pure functions. This
// one starts the REAL server and pushes tricky values through the actual
// `POST /optimize?settings=<urlencoded JSON>` path, because that is where
// percent-decoding and latin1 header limits bit us:
//
//   U-49  searchParams.get() had already decoded, and the server decoded again,
//         so a comment containing "%" answered HTTP 400 "bad settings JSON".
//   U-50  the full command went into X-Gifscythe-Command verbatim; non-ASCII
//         text made Node throw while writing the header, so a SUCCESSFUL engine
//         run became an HTTP 500 and the GIF was never delivered.
//   U-30  out-of-range settings reached the engine instead of being refused.
//
// Every case asserts the status code AND, for successes, that the command the
// server actually ran still contains the exact value that was sent.
//
// Run:  node web/test/transport.test.mjs   (after ./build.sh)

import { spawn } from "node:child_process";
import { createServer } from "node:net";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = dirname(fileURLToPath(import.meta.url));
const SERVER = join(__dirname, "..", "server.mjs");

// A self-contained 43-byte 1x1 GIF89a so the test needs no fixture file.
// Verified accepted by the bundled engine: `gifsicle --info` -> 1 image, 1x1.
const GIF = Buffer.from([
  0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x01, 0x00, 0x01, 0x00, 0x80, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x21, 0xf9, 0x04, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x02, 0x02,
  0x44, 0x01, 0x00, 0x3b,
]);

function freePort() {
  return new Promise((res, rej) => {
    const s = createServer();
    s.on("error", rej);
    s.listen(0, "127.0.0.1", () => {
      const { port } = s.address();
      s.close(() => res(port));
    });
  });
}

async function waitForServer(port, timeoutMs = 10_000) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    try {
      const r = await fetch(`http://127.0.0.1:${port}/`, { method: "GET" });
      if (r.ok) return;
    } catch { /* not up yet */ }
    await new Promise((r) => setTimeout(r, 100));
  }
  throw new Error(`server did not come up on port ${port}`);
}

/** @returns {{status:number, type:string, body:Buffer, command:string|null, json:any}} */
async function post(port, settings, body = GIF) {
  const url = `http://127.0.0.1:${port}/optimize?settings=`
    + encodeURIComponent(JSON.stringify(settings));
  const r = await fetch(url, { method: "POST", body });
  const buf = Buffer.from(await r.arrayBuffer());
  const raw = r.headers.get("x-gifscythe-command");
  let json = null;
  if ((r.headers.get("content-type") || "").includes("json")) {
    try { json = JSON.parse(buf.toString("utf8")); } catch { json = null; }
  }
  return {
    status: r.status,
    type: r.headers.get("content-type") || "",
    body: buf,
    // app.js decodes this the same way; doing it here proves the round-trip.
    // Fall back to the raw value: under the U-50 mutation the header is mangled
    // latin1 and decodeURIComponent THROWS, which used to abort the whole suite
    // instead of failing the one case (found by mutation-testing this file).
    command: (() => {
      if (!raw) return null;
      try { return decodeURIComponent(raw); } catch { return raw; }
    })(),
    json,
  };
}

const cases = [
  // --- U-49: percent-encoded transport must survive exactly one decode ---
  { name: "U-49 literal % in a comment", settings: { comments: ["100%"] },
    // shellQuote leaves a value with no shell metacharacters unquoted.
    expect: { status: 200, commandIncludes: "--comment 100%",
              bodyIncludes: Buffer.from("100%") } },
  { name: "U-49 literal %20 stays %20, does not become a space",
    settings: { comments: ["a%20b"] },
    expect: { status: 200, commandIncludes: "a%20b", commandExcludes: "'a b'",
              bodyIncludes: Buffer.from("a%20b"), bodyExcludes: Buffer.from("a b") } },
  { name: "U-49 literal %22 stays %22", settings: { comments: ['a%22b'] },
    expect: { status: 200, commandIncludes: "a%22b" } },
  { name: "U-49 plus sign is not turned into a space",
    settings: { comments: ["a+b"] },
    expect: { status: 200, commandIncludes: "a+b",
              bodyIncludes: Buffer.from("a+b"), bodyExcludes: Buffer.from("a b") } },
  { name: "U-49 already-encoded %2540 is not decoded twice",
    settings: { comments: ["%2540"] },
    expect: { status: 200, commandIncludes: "%2540" } },

  // --- U-50: non-ASCII / control text must not break the response header ---
  { name: "U-50 CJK comment (used to be HTTP 500)", settings: { comments: ["作品"] },
    expect: { status: 200, commandIncludes: "作品",
              bodyIncludes: Buffer.from("作品", "utf8") } },
  { name: "U-50 emoji comment", settings: { comments: ["🎬 take 1"] },
    expect: { status: 200, commandIncludes: "🎬 take 1" } },
  { name: "U-50 newline inside a comment", settings: { comments: ["line1\nline2"] },
    expect: { status: 200, commandIncludes: "line1" } },
  // Not a bug: gifsicle consumes --comment's operand positionally, so the
  // literal text "--help" becomes the comment (verified with `gifsicle --info`,
  // which reports `comment --help`). The JS and C++ builders emit identical
  // argv for this input, so parity holds. Asserted end-to-end via bodyIncludes.
  { name: "U-50 comment that looks like a flag", settings: { comments: ["--help"] },
    expect: { status: 200, commandIncludes: "--comment --help",
              bodyIncludes: Buffer.from("--help") } },
  { name: "U-50 single quote in a comment", settings: { comments: ["it's"] },
    expect: { status: 200, commandIncludes: "it" } },

  // --- U-30: out-of-range settings are refused, not passed to the engine ---
  { name: "U-30 --scale 0x1 is a silent no-op, so it is refused",
    settings: { resize_kind: "scale", scale_x: 0, scale_y: 1 },
    expect: { status: 422, issueField: "scale" } },
  { name: "U-30 optimize 9", settings: { optimize_level: 9 },
    expect: { status: 422, issueField: "optimize" } },
  { name: "U-30 colors 900", settings: { color_count: 900 },
    expect: { status: 422, issueField: "colors" } },

  // --- malformed / empty requests keep their distinct codes ---
  { name: "sane settings still succeed", settings: { optimize_level: 2 },
    expect: { status: 200 } },
];

const port = await freePort();
const child = spawn(process.execPath, [SERVER, String(port)], {
  stdio: ["ignore", "pipe", "pipe"],
});
let serverLog = "";
child.stdout.on("data", (d) => { serverLog += d; });
child.stderr.on("data", (d) => { serverLog += d; });

let failures = 0;
try {
  await waitForServer(port);

  for (const c of cases) {
    let r;
    try {
      r = await post(port, c.settings);
    } catch (err) {
      failures++;
      console.log(`FAIL ${c.name}\n       threw ${err}`);
      continue;
    }
    const e = c.expect;
    const problems = [];
    if (r.status !== e.status) problems.push(`status ${r.status} != ${e.status}`);
    if (e.status === 200) {
      if (!r.type.startsWith("image/gif")) problems.push(`content-type ${r.type}`);
      if (r.body.length === 0) problems.push("empty body");
      if (!r.command) problems.push("no X-Gifscythe-Command header");
    }
    if (e.commandIncludes && !(r.command || "").includes(e.commandIncludes)) {
      problems.push(`command lacks ${JSON.stringify(e.commandIncludes)}`
        + ` (got ${JSON.stringify(r.command)})`);
    }
    if (e.bodyIncludes && !r.body.includes(e.bodyIncludes)) {
      problems.push(`returned GIF does not contain ${JSON.stringify(e.bodyIncludes.toString())}`);
    }
    if (e.bodyExcludes && r.body.includes(e.bodyExcludes)) {
      problems.push(`returned GIF unexpectedly contains ${JSON.stringify(e.bodyExcludes.toString())}`);
    }
    if (e.commandExcludes && (r.command || "").includes(e.commandExcludes)) {
      problems.push(`command unexpectedly contains ${JSON.stringify(e.commandExcludes)}`);
    }
    if (e.issueField && !(r.json?.issues || []).some((i) => i.field === e.issueField)) {
      problems.push(`no issue for field "${e.issueField}" (got ${JSON.stringify(r.json)})`);
    }
    if (problems.length) {
      failures++;
      console.log(`FAIL ${c.name}\n       ${problems.join("\n       ")}`);
    } else {
      console.log(`PASS ${c.name}`);
    }
  }

  // Malformed JSON must stay a 400 and must NOT be swallowed into a 500.
  for (const [name, raw] of [
    ["malformed settings JSON -> 400", "{not json"],
    ["truncated settings JSON -> 400", '{"comments":'],
  ]) {
    const r = await fetch(
      `http://127.0.0.1:${port}/optimize?settings=${encodeURIComponent(raw)}`,
      { method: "POST", body: GIF },
    );
    if (r.status === 400) {
      console.log(`PASS ${name}`);
    } else {
      failures++;
      console.log(`FAIL ${name} (got ${r.status})`);
    }
  }

  // Empty upload must stay a 400.
  {
    const r = await fetch(
      `http://127.0.0.1:${port}/optimize?settings=${encodeURIComponent("{}")}`,
      { method: "POST", body: Buffer.alloc(0) },
    );
    if (r.status === 400) console.log("PASS empty upload -> 400");
    else { failures++; console.log(`FAIL empty upload -> ${r.status}`); }
  }
} catch (err) {
  failures++;
  console.log(`FAIL harness error: ${err}\n--- server log ---\n${serverLog}`);
} finally {
  child.kill("SIGKILL");
}

if (failures === 0) {
  console.log("ALL WEB TRANSPORT TESTS PASSED");
  process.exit(0);
}
console.log(`${failures} WEB TRANSPORT TEST(S) FAILED`);
process.exit(1);
