// body-limit.test.mjs — regression for U-68 / NF-11 (audit F, fable 5.1 low WINNER).
//
// Finding: an oversized request body answered 400 "bad JSON request body" on
// /run (and 500 on /optimize) instead of 413 Payload Too Large. readBody()
// rejected with a generic Error and destroyed the socket, so the size rejection
// was indistinguishable from a JSON parse failure (and often never reached the
// client at all).
//
// Fix under test: readBody() tags the rejection with statusCode 413 and stops
// accumulating without destroying the socket; both handlers map that tag to a
// real 413 (with Connection: close, destroying the socket only AFTER the
// response flushes), while a genuine JSON parse error stays 400.
//
// WHY THIS TEST NEEDS NO REAL ENGINE: in handleRun() the body is read BEFORE
// findEngine(), so /run's 413 is reachable with no gifsicle at all. /optimize
// (U-84 note: it no longer does — /optimize reads the body first, exactly like
// /run, and case F below proves the 413 with NO engine at all. The stub stays so
// the U-68 cases keep exercising the post-admission path.) Historically it
// discovered the engine first, so to reach ITS readBody we pointed GS_ENGINE at an
// inert executable — process.execPath (node itself). That stub is NEVER executed
// by these cases: the oversize rejection happens in readBody, before run() is
// ever called, so node-as-"engine" only satisfies discovery (isExecutableFile).
// This is cross-platform (node exists and is executable everywhere the suite
// runs). The limit is injected via GS_MAX_BODY so the test stays small and
// deterministic instead of shipping a 64 MB body.
//
// Run:  node web/test/body-limit.test.mjs

import { spawn } from "node:child_process";
import { createServer } from "node:http";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = dirname(fileURLToPath(import.meta.url));
const SERVER = join(__dirname, "..", "server.mjs");
const LIMIT = 64; // bytes; injected as GS_MAX_BODY

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

// POST a raw body to a path; tolerate a server that resets the connection
// (the pre-fix behaviour destroyed the socket mid-request).
async function postRaw(port, path, bodyStr) {
  try {
    const r = await fetch(`http://127.0.0.1:${port}${path}`, {
      method: "POST",
      body: bodyStr,
      headers: { "Content-Type": "application/octet-stream" },
    });
    const text = await r.text().catch(() => "");
    let json = null;
    try { json = JSON.parse(text); } catch { json = null; }
    return { status: r.status, text, json, threw: null };
  } catch (err) {
    return { status: null, text: "", json: null, threw: String(err && err.message ? err.message : err) };
  }
}

let failures = 0;
function check(name, ok, detail) {
  if (ok) { console.log(`PASS ${name}`); }
  else { failures++; console.log(`FAIL ${name}\n       ${detail}`); }
}

const port = await freePort();
const child = spawn(process.execPath, [SERVER, String(port)], {
  stdio: ["ignore", "pipe", "pipe"],
  // GS_ENGINE points at an inert executable (node itself) purely so /optimize's
  // findEngine() succeeds and execution reaches readBody; it is never run by the
  // oversize cases below. GS_MAX_BODY keeps the limit small and deterministic.
  env: { ...process.env, GS_WEB_HOST: "127.0.0.1", GS_MAX_BODY: String(LIMIT),
         GS_ENGINE: process.execPath },
});
let serverLog = "";
child.stdout.on("data", (d) => { serverLog += d; });
child.stderr.on("data", (d) => { serverLog += d; });

try {
  await waitForServer(port);

  // A) /run with a body larger than the limit -> 413 (was 400 / connection reset).
  {
    const big = "x".repeat(LIMIT + 128);
    const r = await postRaw(port, "/run", big);
    check("U-68 /run oversized body -> 413",
      r.status === 413,
      r.threw ? `request threw ${r.threw} (no HTTP status)` : `status ${r.status} != 413 (body ${JSON.stringify(r.text.slice(0, 80))})`);
    check("U-68 /run 413 body is honest JSON {ok:false}",
      r.status === 413 && r.json && r.json.ok === false && /too large/i.test(r.json.error || ""),
      `json=${JSON.stringify(r.json)}`);
  }

  // B) /run with a SMALL but invalid-JSON body -> 400 (size and parse are distinct).
  {
    const r = await postRaw(port, "/run", "{not json");
    check("U-68 /run small malformed JSON -> 400 (not 413)",
      r.status === 400,
      r.threw ? `threw ${r.threw}` : `status ${r.status} != 400`);
    check("U-68 /run malformed-JSON error wording preserved",
      r.status === 400 && r.json && /bad JSON request body/i.test(r.json.error || ""),
      `json=${JSON.stringify(r.json)}`);
  }

  // C) /run with a small VALID envelope but no files -> 400 usage error (the
  //    normal path is untouched by the body-limit change; in particular NOT 413).
  {
    const r = await postRaw(port, "/run", JSON.stringify({ settings: {}, files: [] }));
    check("U-68 /run valid envelope, no files -> 400 usage (not 413)",
      r.status === 400 && r.json && /no files/i.test(r.json.error || ""),
      r.threw ? `threw ${r.threw}` : `status ${r.status} json=${JSON.stringify(r.json)}`);
  }

  // D) /optimize with an oversized body -> 413. findEngine() succeeds via the
  //    inert GS_ENGINE stub, then readBody() rejects BEFORE run() — so the stub
  //    is never executed and no real gifsicle is needed. (Pre-fix this was a 500.)
  {
    const big = "x".repeat(LIMIT + 128);
    const r = await postRaw(port, "/optimize?settings=%7B%7D", big);
    check("U-68 /optimize oversized body -> 413 (was 500)",
      r.status === 413,
      r.threw ? `threw ${r.threw}` : `status ${r.status} != 413 (body ${JSON.stringify(r.text.slice(0, 80))})`);
    check("U-68 /optimize 413 body is honest JSON {ok:false}",
      r.status === 413 && r.json && r.json.ok === false && /too large/i.test(r.json.error || ""),
      `json=${JSON.stringify(r.json)}`);
  }

  // E) /optimize with a normal-sized body is NOT wrongly rejected as 413. Since
  //    U-92 the body is also admitted on its GIF signature, so a non-GIF body is
  //    a named 400 before any engine relay — still "not 413", now pinned exactly.
  {
    const r = await postRaw(port, "/optimize?settings=%7B%7D", "GIF89a-small-body");
    check("U-68 /optimize normal-sized body is not 413",
      r.status !== 413,
      r.threw ? `threw ${r.threw}` : `status ${r.status} (expected != 413)`);
    // NOTE: the body above DOES carry a GIF89a signature, so it passes admission
    // and fails later, honestly, at output verification. The admission gate needs
    // a body without the magic:
    const notGif = await postRaw(port, "/optimize?settings=%7B%7D", "PNG-not-a-gif");
    check("U-92 /optimize non-GIF body -> named 400 admission (no engine relay)",
      notGif.status === 400 && /not a GIF/.test(notGif.json?.error || ""),
      `status ${notGif.status} json=${JSON.stringify(notGif.json)}`);
    check("U-92 /optimize a GIF-signed body still passes admission (not 400)",
      r.status !== 400,
      `status ${r.status} json=${JSON.stringify(r.json)}`);
  }

  // F) U-84 / P3-14: an ENGINE-LESS server. /optimize used to discover the engine
  //    before reading the body, so an oversized upload answered 503 "engine not
  //    found" instead of 413 — a different body-cap contract per endpoint. (The
  //    GS_ENGINE=node stub above existed only to work around that ordering; these
  //    cases need no engine at all, which is the point.)
  {
    const port2 = await freePort();
    const noEngine = spawn(process.execPath, [SERVER, String(port2)], {
      stdio: ["ignore", "pipe", "pipe"],
      env: { ...process.env, GS_WEB_HOST: "127.0.0.1", GS_MAX_BODY: String(LIMIT),
             GS_ENGINE: join(__dirname, "gs-no-such-engine-binary") },
    });
    try {
      await waitForServer(port2);
      const big = await postRaw(port2, "/optimize?settings=%7B%7D", "x".repeat(LIMIT + 128));
      check("U-84 /optimize oversized body with NO engine -> 413 (was 503)",
        big.status === 413,
        big.threw ? `threw ${big.threw}` : `status ${big.status} json=${JSON.stringify(big.json)}`);
      const runBig = await postRaw(port2, "/run", "x".repeat(LIMIT + 128));
      check("U-84 /run and /optimize now share one cap contract (both 413, no engine)",
        runBig.status === 413 && big.status === 413,
        `run=${runBig.status} optimize=${big.status}`);
    } finally {
      noEngine.kill("SIGKILL");
    }
  }
} catch (err) {
  failures++;
  console.log(`FAIL harness error: ${err && err.stack ? err.stack : err}`);
} finally {
  child.kill("SIGKILL");
}

if (failures) {
  console.log(`\n--- server log ---\n${serverLog.trim()}\n`);
  console.log(`${failures} BODY-LIMIT TEST(S) FAILED`);
  process.exit(1);
}
console.log("body-limit: all cases passed");
process.exit(0);
