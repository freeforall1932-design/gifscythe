// transport.test.mjs — end-to-end regression net for the web HTTP transport
// (audit U-49 / U-50 / U-30 / U-41, and register items P2-8 / P2-9 / P2-10 /
// P2-11).
//
// The command-builder and validation parity tests exercise pure functions. This
// one starts the REAL server and pushes tricky values through the actual
// `POST /optimize?settings=<urlencoded JSON>` and `POST /run` (JSON) paths,
// because that is where percent-decoding, latin1 header limits and multi-file
// mode semantics bit us:
//
//   U-49  searchParams.get() had already decoded, and the server decoded again,
//         so a comment containing "%" answered HTTP 400 "bad settings JSON".
//   U-50  the full command went into X-Gifscythe-Command verbatim; non-ASCII
//         text made Node throw while writing the header, so a SUCCESSFUL engine
//         run became an HTTP 500 and the GIF was never delivered.
//   U-30  out-of-range settings reached the engine instead of being refused.
//   U-41  /run carries all four desktop modes with the desktop's honesty rules:
//         planned batch targets with collision refusal, one -m merge run,
//         explode frame verification (rc=0 with zero frames is a 422).
//
// Every case asserts the status code AND, for successes, that the command the
// server actually ran still contains the exact value that was sent.
//
// Run:  node web/test/transport.test.mjs   (after ./build.sh)

import { spawn } from "node:child_process";
import assert from "node:assert/strict";
import { snapshotOutput, verifyOutput } from "../output-verify.mjs";
import { once } from "node:events";
import { mkdtemp, mkdir, readFile, writeFile, readdir, rm, copyFile, chmod, utimes } from "node:fs/promises";
import { tmpdir } from "node:os";
import path from "node:path";
import { assertContainedPath, requestPath, uploadNameError } from "../run-paths.mjs";
import { createServer } from "node:net";
import { dirname, join } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

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

/** POST /run — the multi-file JSON endpoint (audit U-41 / P2-11). */
async function postRun(port, settings, files) {
  const r = await fetch(`http://127.0.0.1:${port}/run`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      settings,
      files: files.map((f) => ({ name: f.name, data: f.buf.toString("base64") })),
    }),
  });
  let json = null;
  try { json = await r.json(); } catch { json = null; }
  return { status: r.status, json };
}

const gifFile = (name) => ({ name, buf: GIF });
const GIF_MAGIC = Buffer.from("GIF8");

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

  // --- U-42: independent X/Y scale factors reach the engine per axis ---
  { name: "U-42 asymmetric scale runs per axis (0.5x2 on the 1x1 GIF -> 1x2)",
    settings: { resize_kind: "scale", scale_x: 0.5, scale_y: 2 },
    expect: { status: 200, commandIncludes: "--scale 0.5x2" } },

  // --- U-64: info=true is unsupported on the web API and must fail clearly ---
  { name: "U-64 optimize rejects info=true clearly",
    settings: { info: true },
    expect: { status: 400, errorIncludes: "info=true is not supported" } },

  // --- malformed / empty requests keep their distinct codes ---
  { name: "sane settings still succeed", settings: { optimize_level: 2 },
    expect: { status: 200 } },
];

// Isolate even the PRE-FIX traversal reproduction inside this test's temp root.
// Instrument actual spawn calls using a Node preload (no shell wrapper, no fake
// engine for GS-202): refusals must precede ANY engine launch, including batch item 1.
const testRoot = await mkdtemp(join(tmpdir(), "gsweb-security-test-"));
const requestRoot = join(testRoot, "requests");
await mkdir(requestRoot);
const launchLog = join(testRoot, "launches.jsonl");
await writeFile(launchLog, "");
// DS-13 fixture is a REAL child process writing controlled output. Only the
// test preload redirects marked requests to it; production has no test hook.
const gif87 = Buffer.concat([GIF.subarray(0, 19), GIF.subarray(27)]);
gif87.write("GIF87a", 0, "ascii");
const outputCases = [
  { name: "text", bytes: Buffer.from("not a GIF"), status: 422 },
  { name: "PNG", bytes: Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), status: 422 },
  { name: "truncated signature", bytes: Buffer.from("GIF89"), status: 422 },
  { name: "wrong version", bytes: Buffer.from("GIF88a..."), status: 422 },
  { name: "wrong signature suffix", bytes: Buffer.from("GIF89b..."), status: 422 },
  { name: "lowercase signature", bytes: Buffer.from("gif89a..."), status: 422 },
  { name: "valid GIF87a", bytes: gif87, status: 200 },
  { name: "valid GIF89a", bytes: GIF, status: 200 },
  { name: "missing output", bytes: null, status: 422, noOutput: true },
  { name: "empty output", bytes: Buffer.alloc(0), status: 422, noOutput: true },
  { name: "nonzero exit with valid GIF", bytes: GIF, status: 422, exitCode: 7 },
];
const outputFixture = join(testRoot, "output-engine.mjs");
await writeFile(outputFixture, `
import { writeFile } from "node:fs/promises";
const fixtures = ${JSON.stringify(outputCases.map((c) => ({ data: c.bytes?.toString("base64") ?? null, code: c.exitCode || 0 })))};
const [outPath, index] = process.argv.slice(2);
const fixture = fixtures[Number(index)];
if (fixture.data !== null) await writeFile(outPath, Buffer.from(fixture.data, "base64"));
if (fixture.code) process.stderr.write("fixture engine failed");
process.exitCode = fixture.code;
`);
const preload = join(testRoot, "record-spawn.mjs");
await writeFile(preload, `
import cp from "node:child_process";
import { appendFileSync } from "node:fs";
import { syncBuiltinESMExports } from "node:module";
const spawn = cp.spawn;
cp.spawn = (...args) => {
  appendFileSync(process.env.GS_TEST_SPAWN_LOG, JSON.stringify(args.slice(0, 2)) + "\\n");
  const argv = args[1] || [];
  const comment = argv[argv.indexOf("--comment") + 1];
  if (argv.includes("--comment") && /^DS13-output:[0-9]+$/.test(comment)) {
    return spawn(process.execPath, [process.env.GS_TEST_OUTPUT_FIXTURE,
      argv[argv.indexOf("-o") + 1], comment.slice("DS13-output:".length)], args[2]);
  }
  return spawn(...args);
};
syncBuiltinESMExports();
`);
const launches = () => readFile(launchLog, "utf8");
// res.end precedes the server's asynchronous finally/rm. Wait for that cleanup,
// rather than racing it and treating a transient request directory as a leak.
async function leftoversExcept(allowed) {
  const deadline = Date.now() + 3000;
  let extra;
  do {
    extra = (await readdir(requestRoot)).filter((name) => !allowed.includes(name));
    if (!extra.length) return extra;
    await new Promise((resolve) => setTimeout(resolve, 20));
  } while (Date.now() < deadline);
  return extra;
}
const port = await freePort();
const child = spawn(process.execPath, ["--import", pathToFileURL(preload).href, SERVER, String(port)], {
  stdio: ["ignore", "pipe", "pipe"],
  env: { ...process.env, GS_WEB_HOST: "127.0.0.1", GS_TEST_SPAWN_LOG: launchLog, GS_TEST_OUTPUT_FIXTURE: outputFixture,
         TMPDIR: requestRoot, TMP: requestRoot, TEMP: requestRoot },
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
    if (e.errorIncludes && !String(r.json?.error || r.json?.stderr || "").includes(e.errorIncludes)) {
      problems.push(`error lacks ${JSON.stringify(e.errorIncludes)} (got ${JSON.stringify(r.json)})`);
    }
    if (problems.length) {
      failures++;
      console.log(`FAIL ${c.name}\n       ${problems.join("\n       ")}`);
    } else {
      console.log(`PASS ${c.name}`);
    }
  }

  // ---- U-41 (P2-11): the multi-file /run endpoint, all four modes ----
  // Same honesty rules as the desktop: per-file Auto runs for batch with
  // collision refusal up front, one -m run for merge, frame verification for
  // explode, validation shared with /optimize, and distinct 400/422 codes.
  {
    const t = (name, problems) => {
      if (problems.length) {
        failures++;
        console.log(`FAIL ${name}\n       ${problems.join("\n       ")}`);
      } else console.log(`PASS ${name}`);
    };
    const decode = (b64) => Buffer.from(b64, "base64");

    // GS-202 / P0-6: independently exercise the final containment guard, even
    // though name admission makes these escapes unreachable through the API.
    {
      const p = [];
      try {
        for (const [paths, root, childName, escapes] of [
          [path.posix, "/tmp/gsweb-private", "/tmp/gsweb-private/out.gif",
            ["/tmp/gsweb-private", "/tmp/gsweb-private-other/out.gif", "/tmp/out.gif",
             "/tmp/gsweb-private/../out.gif"]],
          [path.win32, "C:\\Temp\\gsweb-private", "C:\\Temp\\gsweb-private\\out.gif",
            ["C:\\Temp\\gsweb-private", "C:\\Temp\\gsweb-private-other\\out.gif",
             "C:\\Temp\\gsweb-private\\..\\out.gif", "D:\\out.gif", "\\\\server\\share\\out.gif"]],
          [path.win32, "\\\\server\\share\\gsweb-private", "\\\\server\\share\\gsweb-private\\out.gif",
            ["\\\\server\\other\\out.gif", "\\\\server\\share\\gsweb-private-other\\out.gif"]],
        ]) {
          assert.equal(assertContainedPath(root, childName, paths), paths.resolve(childName));
          for (const target of escapes) assert.throws(() => assertContainedPath(root, target, paths));
        }
        assert.equal(uploadNameError("作品 café.gif"), null);
        assert.equal(requestPath(requestRoot, "作品 café_opt.gif"), join(requestRoot, "作品 café_opt.gif"));
        for (const name of ["../escape.gif", "..\\escape.gif", "C:escape.gif", "a\0.gif", ".", ".."]) {
          assert.throws(() => requestPath(requestRoot, name));
        }
      } catch (err) { p.push(String(err)); }
      t("GS-202 final containment guard (POSIX, Windows drives and UNC)", p);
    }

    // Sentinels are OUTSIDE every gsweb-* request dir, but inside testRoot.
    // The old server overwrites the first two with Auto/Batch or Explode.
    const sentinels = ["gs202-victim_opt.gif", "gs202-victim_frame.000"];
    const sentinelBytes = Buffer.from("DO NOT OVERWRITE: GS-202 outside request directory");
    for (const name of sentinels) await writeFile(join(requestRoot, name), sentinelBytes);
    const unsafeNames = [
      "../gs202-victim.gif", "..\\gs202-victim.gif", "nested/clip.gif", "nested\\clip.gif",
      join(requestRoot, "absolute.gif"), "C:\\Temp\\clip.gif", "C:clip.gif", "\\\\server\\share\\clip.gif",
      ".", "..", "clip\0.gif", "clip\n.gif", "clip.gif:stream", "clip.gif.", "clip.gif ",
      "CON.gif", "LPT1.gif", "COM¹.gif", "clip<1>.gif", "clip?.gif", "clip*.gif",
      "clip|x.gif", 'clip"x.gif', "clip\u007f.gif", "clip\ud800.gif",
    ];
    for (const mode of ["auto", "batch", "merge", "explode"]) {
      for (const name of sentinels) await writeFile(join(requestRoot, name), sentinelBytes);
      const p = [];
      const beforeLaunches = await launches();
      for (const name of unsafeNames) {
        // Put the malicious item LAST: the entire batch must be refused before
        // even the first, valid item's engine can run.
        const files = mode === "batch" || mode === "merge"
          ? [gifFile("safe.gif"), gifFile(name)] : [gifFile(name)];
        const r = await postRun(port, { mode }, files);
        if (r.status !== 400 || !String(r.json?.error || "").includes("invalid upload name")) {
          p.push(`${JSON.stringify(name)}: expected named 400, got ${r.status} ${JSON.stringify(r.json)}`);
        }
      }
      if (await launches() !== beforeLaunches) p.push("invalid request launched the engine");
      for (const name of sentinels) {
        if (!(await readFile(join(requestRoot, name))).equals(sentinelBytes)) p.push(`outside sentinel changed: ${name}`);
      }
      const leftovers = await leftoversExcept(sentinels);
      if (leftovers.length) p.push(`uncontained writes / leaked temp directories: ${leftovers.join(", ")}`);
      t(`GS-202 ${mode}: unsafe names refused before engine, no outside writes`, p);
      // Keep mutations independent when running this net against the old server.
      for (const name of leftovers) await rm(join(requestRoot, name), { recursive: true, force: true });
    }

    for (const [label, names, error] of [
      ["case-only target collision", ["Clip.gif", "clip.GIF"], "would both write"],
      ["case-only source collision", ["Clip.gif", "CLIP_OPT.GIF"], "overwrite the uploaded file"],
      ["Unicode normalized target collision", ["café.gif", "cafe\u0301.gif"], "would both write"],
    ]) {
      const beforeLaunches = await launches();
      const r = await postRun(port, { mode: "batch" }, names.map(gifFile));
      const p = [];
      if (r.status !== 422 || !String(r.json?.error || "").includes(error)) p.push(`response ${r.status}: ${JSON.stringify(r.json)}`);
      if (await launches() !== beforeLaunches) p.push("collision launched the engine");
      t(`GS-202 batch: ${label} refused before any run`, p);
    }

    // Valid names (spaces, Unicode, multiple dots, literal percent encoding,
    // leading dash) stay usable; settings cannot replace server-owned paths.
    for (const mode of ["auto", "batch", "merge", "explode"]) {
      const beforeLaunches = await launches();
      const files = mode === "batch"
        ? ["作品 café.v2.gif", "%2e%2e%2fclip.gif", "-clip.gif", "🎉.gif"].map(gifFile)
        : [gifFile("作品 café.v2.gif")];
      const r = await postRun(port, { mode, output: "../forbidden.gif", inputs: ["../forbidden.gif"] }, files);
      const expected = mode === "merge" ? ["merged.gif"]
        : mode === "explode" ? ["作品 café.v2_frame.000"]
        : files.map((f) => f.name.slice(0, -4) + "_opt.gif");
      const p = [];
      if (r.status !== 200 || JSON.stringify(r.json?.outputs?.map((o) => o.name)) !== JSON.stringify(expected)) {
        p.push(`response ${r.status}: ${JSON.stringify(r.json)}`);
      }
      const log = (await launches()).slice(beforeLaunches.length).trim().split("\n").filter(Boolean);
      if (log.length !== (mode === "batch" ? files.length : 1)) p.push("unexpected engine invocation count");
      for (const line of log) {
        const [, argv] = JSON.parse(line);
        const target = argv[argv.indexOf("-o") + 1];
        const rel = path.relative(requestRoot, target);
        if (path.isAbsolute(rel) || rel.startsWith("..") || !rel.startsWith("gsweb-") || rel.split(path.sep).length !== 2) {
          p.push(`uncontained engine output/prefix: ${target}`);
        }
      }
      if ((await leftoversExcept(sentinels)).length) p.push("request files not cleaned up");
      t(`GS-202 ${mode}: valid names preserved and engine target contained`, p);
    }

    // auto: exactly one file -> one <stem>_opt.gif output
    let r = await postRun(port, { mode: "auto", optimize_level: 2 }, [gifFile("in.gif")]);
    let p = [];
    if (r.status !== 200) p.push(`status ${r.status}`);
    if (r.json?.outputs?.length !== 1) p.push(`outputs ${JSON.stringify(r.json?.outputs?.map((o) => o.name))}`);
    if (r.json?.outputs?.[0]?.name !== "in_opt.gif") p.push(`name ${r.json?.outputs?.[0]?.name}`);
    if (!decode(r.json?.outputs?.[0]?.data || "").subarray(0, 4).equals(GIF_MAGIC)) p.push("output is not a GIF");
    if (r.json?.commands?.length !== 1 || !r.json.commands[0].includes("-O2")) p.push(`commands ${JSON.stringify(r.json?.commands)}`);
    t("U-41 auto single file -> in_opt.gif", p);

    // merge: N files -> one merged.gif, one -m command
    r = await postRun(port, { mode: "merge" }, [gifFile("a.gif"), gifFile("b.gif")]);
    p = [];
    if (r.status !== 200) p.push(`status ${r.status}`);
    if (r.json?.outputs?.length !== 1 || r.json.outputs[0].name !== "merged.gif") p.push(`outputs ${JSON.stringify(r.json?.outputs?.map((o) => o.name))}`);
    if (!decode(r.json?.outputs?.[0]?.data || "").subarray(0, 4).equals(GIF_MAGIC)) p.push("merged output is not a GIF");
    if (r.json?.commands?.length !== 1 || !r.json.commands[0].includes("-m ")) p.push(`commands ${JSON.stringify(r.json?.commands)}`);
    t("U-41 merge two files -> merged.gif via one -m run", p);

    // batch: one Auto run PER FILE (never a single -b), derived targets
    r = await postRun(port, { mode: "batch", optimize_level: 1 }, [gifFile("a.gif"), gifFile("bee.gif")]);
    p = [];
    if (r.status !== 200) p.push(`status ${r.status}`);
    const names = (r.json?.outputs || []).map((o) => o.name);
    if (names.join(",") !== "a_opt.gif,bee_opt.gif") p.push(`outputs ${JSON.stringify(names)}`);
    if (r.json?.commands?.length !== 2) p.push(`expected 2 per-file commands, got ${r.json?.commands?.length}`);
    if ((r.json?.commands || []).some((c) => c.includes(" -b "))) p.push("batch used a single -b run (desktop runs per-file Auto)");
    t("U-41 batch -> per-file Auto runs with <stem>_opt.gif targets", p);

    // batch collision: two uploads, one target -> refused BEFORE any run
    r = await postRun(port, { mode: "batch" }, [gifFile("x.gif"), gifFile("x.gif")]);
    p = [];
    if (r.status !== 422) p.push(`status ${r.status}`);
    if (!String(r.json?.error || "").includes("would both write")) p.push(`error ${JSON.stringify(r.json?.error)}`);
    t("U-41 batch target collision is refused (desktop planner parity)", p);

    // batch target == an uploaded source name -> refused
    r = await postRun(port, { mode: "batch" }, [gifFile("y_opt.gif"), gifFile("y.gif")]);
    p = [];
    if (r.status !== 422) p.push(`status ${r.status}`);
    if (!String(r.json?.error || "").includes("overwrite the uploaded file")) p.push(`error ${JSON.stringify(r.json?.error)}`);
    t("U-41 batch target-equals-source is refused", p);

    // explode: frames verified; the 1x1 GIF yields exactly clip_frame.000
    r = await postRun(port, { mode: "explode" }, [gifFile("clip.gif")]);
    p = [];
    if (r.status !== 200) p.push(`status ${r.status}`);
    if (r.json?.outputs?.length !== 1 || r.json.outputs[0].name !== "clip_frame.000") p.push(`outputs ${JSON.stringify(r.json?.outputs?.map((o) => o.name))}`);
    if (!decode(r.json?.outputs?.[0]?.data || "").subarray(0, 4).equals(GIF_MAGIC)) p.push("frame is not a GIF");
    if (r.json?.commands?.length !== 1 || !r.json.commands[0].includes("-e ")) p.push(`commands ${JSON.stringify(r.json?.commands)}`);
    t("U-41 explode -> verified <stem>_frame.NNN frames", p);

    // explode by name passes -E through the shared builder
    r = await postRun(port, { mode: "explode", explode_by_name: true }, [gifFile("clip.gif")]);
    p = [];
    if (r.status !== 200) p.push(`status ${r.status}`);
    if (!String(r.json?.commands?.[0] || "").includes("-E ")) p.push(`commands ${JSON.stringify(r.json?.commands)}`);
    t("U-41 explode by name emits -E", p);

    // usage rules: wrong file counts and unknown modes are 400s with honest text
    r = await postRun(port, { mode: "explode" }, [gifFile("a.gif"), gifFile("b.gif")]);
    t("U-41 explode with two files -> 400", r.status === 400 ? [] : [`status ${r.status}`]);

    r = await postRun(port, { mode: "auto" }, [gifFile("a.gif"), gifFile("b.gif")]);
    p = r.status === 400 && String(r.json?.error || "").includes("Batch") ? [] : [`status ${r.status} error ${JSON.stringify(r.json?.error)}`];
    t("U-41 auto with two files -> 400 pointing at Batch/Merge", p);

    r = await postRun(port, { mode: "shred" }, [gifFile("a.gif")]);
    t("U-41 unknown mode -> 400", r.status === 400 ? [] : [`status ${r.status}`]);

    r = await postRun(port, { mode: "batch" }, []);
    t("U-41 /run with no files -> 400", r.status === 400 ? [] : [`status ${r.status}`]);

    // the shared validation layer (U-30) guards /run exactly like /optimize
    r = await postRun(port, { mode: "merge", color_count: 900 }, [gifFile("a.gif"), gifFile("b.gif")]);
    p = [];
    if (r.status !== 422) p.push(`status ${r.status}`);
    if (!(r.json?.issues || []).some((i) => i.field === "colors")) p.push(`issues ${JSON.stringify(r.json?.issues)}`);
    t("U-41 /run refuses out-of-range settings before any run", p);

    // U-64: info=true is not a GIF-producing web API mode. Reject it clearly
    // before any spawn instead of falling through to GIF verification.
    {
      const beforeLaunches = await launches();
      r = await postRun(port, { mode: "auto", info: true }, [gifFile("clip.gif")]);
      p = [];
      if (r.status !== 400) p.push(`status ${r.status}`);
      if (!String(r.json?.error || "").includes("info=true is not supported")) {
        p.push(`error ${JSON.stringify(r.json?.error)}`);
      }
      if (await launches() !== beforeLaunches) p.push("info=true request launched the engine");
      t("U-64 /run rejects info=true clearly before any run", p);
    }
  }

  // DS-13: test the actual HTTP response, not only a predicate. Invalid output
  // must never carry successful GIF headers/body; missing/empty and exit failures
  // keep their established diagnostics. The existing cases use the real engine.
  for (const [index, c] of outputCases.entries()) {
    const r = await post(port, { comments: [`DS13-output:${index}`] });
    const problems = [];
    if (r.status !== c.status) problems.push(`status ${r.status} != ${c.status}`);
    if (c.status === 200) {
      if (!r.type.startsWith("image/gif")) problems.push(`type ${r.type}`);
      if (!r.body.equals(c.bytes)) problems.push("valid output buffer changed");
      if (!r.command?.includes(`DS13-output:${index}`)) problems.push("missing success metadata");
    } else {
      if (!r.type.startsWith("application/json") || r.json?.ok !== false) problems.push("failure is not ok:false JSON");
      if (r.json?.exitCode !== (c.exitCode || 0)) problems.push(`exitCode ${r.json?.exitCode}`);
      const expected = c.exitCode ? "fixture engine failed"
        : c.noOutput ? "produced no output file" : "invalid GIF output";
      if (!String(r.json?.stderr || "").includes(expected)) problems.push(`stderr ${r.json?.stderr}`);
      if (!r.json?.command?.includes(`DS13-output:${index}`)) problems.push("missing diagnostic command");
      if (r.command) problems.push("failure exposes success command header");
    }
    if (problems.length) {
      failures++;
      console.log(`FAIL DS-13 ${c.name}\n       ${problems.join("; ")}`);
    } else console.log(`PASS DS-13 ${c.name}`);
  }

  // GS-203 shared JS postcondition: including unchanged existing outputs,
  // which fresh per-request directories normally make unreachable via HTTP.
  try {
    const file = join(testRoot, "verify.gif");
    const absent = await snapshotOutput(file);
    assert.ok((await verifyOutput(file, absent)).error);
    await writeFile(file, GIF);
    assert.equal((await verifyOutput(file, absent)).error, null);
    const before = await snapshotOutput(file);
    assert.match((await verifyOutput(file, before)).error, /unchanged/);
    const future = new Date(Date.now() + 5000);
    await utimes(file, future, future);
    assert.equal((await verifyOutput(file, before)).error, null);
    assert.ok((await snapshotOutput(testRoot)).error);
    await rm(file);
    console.log("PASS GS-203 snapshot/unchanged/refreshed metadata rule");
  } catch (err) { failures++; console.log(`FAIL GS-203 snapshot rule: ${err}`); }
  for (const mode of ["auto", "batch", "merge"]) {
    const problems = [];
    for (const [index, c] of outputCases.entries()) {
      const r = await postRun(port, { mode, comments: [`DS13-output:${index}`] }, [gifFile("clip.gif")]);
      if (r.status !== c.status) problems.push(`${c.name}: ${r.status} != ${c.status}`);
      if (c.status === 422 && (r.json?.ok !== false || r.json?.exitCode !== (c.exitCode || 0)))
        problems.push(`${c.name}: dishonest failure ${JSON.stringify(r.json)}`);
      if (c.status === 200 && !Buffer.from(r.json?.outputs?.[0]?.data || "", "base64").equals(c.bytes))
        problems.push(`${c.name}: response differs from verified buffer`);
    }
    if (problems.length) { failures++; console.log(`FAIL GS-203 ${mode}: ${problems.join("; ")}`); }
    else console.log(`PASS GS-203 ${mode} output fixtures (11 cases)`);
  }

  // GS-207: a good fallback engine is available throughout these tests. Each
  // server gets its own explicit environment; the spawn log proves invalid
  // overrides never launch either the requested path OR a fallback.
  const actualEngine = JSON.parse((await launches()).trim().split("\n")[0])[0];
  const engineDir = join(testRoot, "engine choice");
  await mkdir(engineDir);
  const selectedEngine = join(engineDir, process.platform === "win32" ? "selected engine.exe" : "selected engine");
  await copyFile(actualEngine, selectedEngine);
  await chmod(selectedEngine, 0o755);
  const noExec = join(testRoot, "non-executable");
  await writeFile(noExec, "not executable");
  await chmod(noExec, 0o600);

  async function withEngine(override, check) {
    const p = await freePort();
    const env = { ...process.env, GS_WEB_HOST: "127.0.0.1", GS_TEST_SPAWN_LOG: launchLog,
      GS_TEST_OUTPUT_FIXTURE: outputFixture, TMPDIR: requestRoot, TMP: requestRoot, TEMP: requestRoot };
    if (override === undefined) delete env.GS_ENGINE;
    else env.GS_ENGINE = override;
    const server = spawn(process.execPath, ["--import", pathToFileURL(preload).href, SERVER, String(p)],
      { cwd: testRoot, env, stdio: ["ignore", "pipe", "pipe"] });
    let log = "";
    server.stdout.on("data", (d) => { log += d; });
    server.stderr.on("data", (d) => { log += d; });
    try {
      await waitForServer(p);
      await check(p, () => log);
    } finally {
      const closed = once(server, "close");
      server.kill("SIGKILL");
      if (server.exitCode === null && server.signalCode === null) await closed;
    }
  }
  async function engineCheck(name, override, check) {
    try {
      await withEngine(override, check);
      console.log(`PASS GS-207 ${name}`);
    } catch (err) {
      failures++;
      console.log(`FAIL GS-207 ${name}\n       ${err}`);
    }
  }
  async function refused(p, override) {
    const before = await launches();
    const responses = [await post(p, {}), await postRun(p, { mode: "auto" }, [gifFile("clip.gif")])];
    for (const r of responses) {
      assert.equal(r.status, 503, JSON.stringify(r.json));
      assert.equal(r.json?.ok, false);
      assert.match(r.json?.error || "", /GS_ENGINE.*refusing automatic fallback/);
      assert.ok(r.json.error.includes(JSON.stringify(override)));
    }
    assert.equal(await launches(), before, "invalid override launched an engine");
  }
  for (const [name, override] of [
    ["missing override", join(testRoot, "missing-engine")],
    ["directory override", engineDir],
    ["bare override is exact, not a PATH search", "gifsicle"],
    ["whitespace is not an empty override", "   "],
    ...(process.platform === "win32" ? [] : [["non-executable override", noExec]]),
  ]) {
    await engineCheck(name, override, async (p, log) => {
      await refused(p, override);
      assert.match(log(), /Engine \[GS_ENGINE\]: ERROR:/);
      assert.ok(log().includes(JSON.stringify(override)), "startup diagnostic omits the override");
    });
  }
  for (const [name, override, source] of [
    ["valid absolute override with spaces", selectedEngine, "GS_ENGINE"],
    ["valid relative override with spaces", path.relative(testRoot, selectedEngine), "GS_ENGINE"],
    ["unset override preserves release discovery", undefined, "release"],
    ["empty override preserves release discovery", "", "release"],
  ]) {
    await engineCheck(name, override, async (p, log) => {
      const before = await launches();
      const responses = [await post(p, {}), await postRun(p, { mode: "auto" }, [gifFile("clip.gif")])];
      for (const r of responses) assert.equal(r.status, 200, JSON.stringify(r.json));
      const entries = (await launches()).slice(before.length).trim().split("\n").map(JSON.parse);
      assert.equal(entries.length, 2);
      if (source === "GS_ENGINE") for (const [engine] of entries) assert.equal(engine, selectedEngine);
      assert.ok(log().includes(`Engine [${source}]:`), "startup log omits the selected source");
    });
  }
  await engineCheck("override removed after startup still refuses fallback", selectedEngine, async (p) => {
    await rm(selectedEngine);
    await refused(p, selectedEngine);
  });

  // Malformed JSON must stay a 400 and must NOT be swallowed into a 500.
  for (const [name, raw] of [
    ["malformed settings JSON -> 400", "{not json"],
    ["truncated settings JSON -> 400", '{"comments":'],
    ["null settings JSON -> 400 (audit U-77)", "null"],
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

  // U-77: POST /run with a JSON null body must return 400, not 500.
  {
    const r = await fetch(`http://127.0.0.1:${port}/run`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: "null",
    });
    if (r.status === 400) {
      console.log("PASS U-77 /run with JSON null body -> 400");
    } else {
      failures++;
      console.log(`FAIL U-77 /run with JSON null body (got ${r.status})`);
    }
  }

  // U-79: POST /optimize with mode:"explode" must return 400, not misleading 422.
  {
    const r = await fetch(
      `http://127.0.0.1:${port}/optimize?settings=${encodeURIComponent(JSON.stringify({ mode: "explode" }))}`,
      { method: "POST", body: GIF },
    );
    if (r.status === 400) {
      console.log("PASS U-79 /optimize with mode:explode -> 400");
    } else {
      failures++;
      console.log(`FAIL U-79 /optimize with mode:explode (got ${r.status})`);
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
  const closed = once(child, "close");
  child.kill("SIGKILL");
  if (child.exitCode === null && child.signalCode === null) await closed;
  await rm(testRoot, { recursive: true, force: true });
}

if (failures === 0) {
  console.log("ALL WEB TRANSPORT TESTS PASSED");
  process.exit(0);
}
console.log(`${failures} WEB TRANSPORT TEST(S) FAILED`);
process.exit(1);
