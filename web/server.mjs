// server.mjs — zero-dependency Node HTTP server for the Gifscythe web build.
//
// Serves the static web UI (index.html / app.js / style.css / command.mjs) and
// exposes two API endpoints:
//
//   POST /optimize?settings=<urlencoded JSON>   body = raw GIF bytes
//     -> 200 image/gif  (optimized GIF) with X-Gifscythe-* metadata headers
//     -> 422 JSON       { ok:false, exitCode, stderr, command }
//     (single-file Auto mode; kept for compatibility and the transport suite)
//
//   POST /run    body = JSON { settings, files: [{ name, data(base64) }] }
//     -> 200 JSON  { ok:true, mode, outputs:[{name,bytes,data(base64)}],
//                    commands:[...], inBytes, outBytes }
//     -> 400 JSON  usage errors (unknown mode, wrong file count, bad payload)
//     -> 422 JSON  { ok:false, exitCode?, stderr?/error, issues?, command(s)? }
//     All four desktop modes (audit U-41 / fix-order P2-11):
//       auto    exactly 1 file  -> one run, output <stem>_opt.gif
//       batch   N files         -> a per-file Auto run each (the desktop batch
//                                  semantics — never one -b run), targets
//                                  <stem>_opt.gif, collisions REFUSED up front
//       merge   N files         -> one -m run over all inputs -> merged.gif
//       explode exactly 1 file  -> one -e/-E run against <stem>_frame, frames
//                                  VERIFIED (rc=0 with zero new GIF frames is
//                                  a 422, mirroring the desktop P1-19 check)
//
// The engine runs as a subprocess (spawn, argv array — never a shell), exactly
// like the desktop ProcessRunner. The command line is built by command.mjs,
// the same builder the browser live-pane uses.
//
//   node web/server.mjs [port]      (default 8000; binds 127.0.0.1)
//   GS_WEB_HOST=0.0.0.0 node web/server.mjs   (expose on the network)

import http from "node:http";
import { spawn } from "node:child_process";
import {
  readFile, writeFile, mkdtemp, rm, readdir, access, stat, open,
} from "node:fs/promises";
import { constants } from "node:fs";
import { tmpdir } from "node:os";
import { join, extname, resolve, dirname, basename } from "node:path";
import { realpath } from "node:fs/promises";
import { fileURLToPath } from "node:url";
import { buildArgs, shellQuote } from "./command.mjs";
import { validate } from "./validate.mjs";
import { hasGifMagic, snapshotOutput, verifyOutput } from "./output-verify.mjs";
import { uploadNameError, outputNameKey, requestPath, assertContainedPath } from "./run-paths.mjs";

const ROOT = dirname(fileURLToPath(import.meta.url)); // web/
const PRODUCT = resolve(ROOT, "..", "working_code", "gifscythe");
// U-85 / P3-13: `Number("abc")` is NaN and `listen(NaN)` threw a raw
// ERR_SOCKET_BAD_PORT RangeError at module top level — no usage message and no
// caller-error exit code, which is what a self-hosting panel that sets PORT hit
// first. Validate once, name the reason, exit 2 (the CLI's caller-error shape).
const PORT = (() => {
  const raw = process.argv[2] || process.env.PORT || "8000";
  const text = String(raw).trim();
  if (!/^\d+$/.test(text)) {
    process.stderr.write(`ERROR: port must be a decimal integer 0..65535, got "${raw}"\n`);
    process.stderr.write("       usage: node web/server.mjs [port]   (or set PORT)\n");
    process.exit(2);
  }
  const n = Number(text);
  if (n > 65535) {
    process.stderr.write(`ERROR: port must be a decimal integer 0..65535, got "${raw}"\n`);
    process.stderr.write("       usage: node web/server.mjs [port]   (or set PORT)\n");
    process.exit(2);
  }
  return n;
})();
// U-06: the demo used to bind 0.0.0.0 unconditionally, so anyone on the network
// could submit 64 MB bodies and 120 s engine runs to an endpoint with no auth
// and no concurrency cap. Default to loopback; opt in explicitly.
const HOST = process.env.GS_WEB_HOST || "127.0.0.1";
// MAX_BODY caps the raw HTTP request body. For /optimize the body IS the GIF, so
// the cap is the GIF cap. For /run the GIF travels base64-encoded inside a JSON
// envelope, so base64's ~33% overhead means the effective decoded-GIF cap is
// ~48 MB at the default 64 MB envelope limit (NF-11 / U-68 — the old "64 MB"
// claim was the envelope, not the GIF). Override with GS_MAX_BODY (a positive
// integer number of bytes); web/test/body-limit.test.mjs injects a tiny limit so
// the 413 path is exercised deterministically without shipping a 64 MB body.
const MAX_BODY = (() => {
  const fromEnv = Number(process.env.GS_MAX_BODY);
  return Number.isInteger(fromEnv) && fromEnv > 0 ? fromEnv : 64 * 1024 * 1024;
})();
// U-93 / P2-20: the stderr a single engine run may contribute to a response.
// Override with GS_MAX_STDERR; 0 means "cap at 0" is refused, so the floor is 1.
const MAX_STDERR = (() => {
  const fromEnv = Number(process.env.GS_MAX_STDERR);
  return Number.isInteger(fromEnv) && fromEnv > 0 ? fromEnv : 16 * 1024;
})();
// The engine-run bound (U-06 / P1-5). It existed as a hard 120 s, which is a
// long time to hold a socket for a GIF optimizer and impossible to size from a
// reverse proxy. Now configurable, still on by default; 0 disables it.
const ENGINE_TIMEOUT_MS = (() => {
  const fromEnv = Number(process.env.GS_ENGINE_TIMEOUT_MS);
  return Number.isInteger(fromEnv) && fromEnv >= 0 ? fromEnv : 120_000;
})();

// ---- U-06 / P1-5: concurrency + per-client request bounds ----
// The finding was "no auth, no concurrency cap, 64 MB bodies, 120 s engine
// runs": every accepted request spawned a process and a temp tree, so N clients
// meant N+1 gifsicles competing for CPU. S8 landed the loopback bind; these are
// the remaining two bounds.
//   * at most GS_MAX_CONCURRENT engine runs at once (default 2 — a self-hosted
//     box is usually doing one job at a time), excess requests queue up to
//     GS_MAX_QUEUED (default 8) and beyond that get 429 with Retry-After;
//   * each client address gets GS_RATE_LIMIT_PER_MIN requests per minute
//     (default 300; 0 disables). Generous on purpose: the transport suite is a
//     burst of ~70 requests from loopback, and a limit that trips the product's
//     own test suite is a wrong limit.
const MAX_CONCURRENT = (() => {
  const v = Number(process.env.GS_MAX_CONCURRENT);
  return Number.isInteger(v) && v >= 1 ? v : 2;
})();
const MAX_QUEUED = (() => {
  const v = Number(process.env.GS_MAX_QUEUED);
  return Number.isInteger(v) && v >= 0 ? v : 8;
})();
const RATE_LIMIT_PER_MIN = (() => {
  const v = Number(process.env.GS_RATE_LIMIT_PER_MIN);
  return Number.isInteger(v) && v >= 0 ? v : 300;
})();

const engineSlots = { running: 0, waiters: [] };

function acquireEngineSlot() {
  if (engineSlots.running < MAX_CONCURRENT) {
    engineSlots.running += 1;
    return Promise.resolve(true);
  }
  if (engineSlots.waiters.length >= MAX_QUEUED) return Promise.resolve(false);
  return new Promise((done) => {
    engineSlots.waiters.push(() => { engineSlots.running += 1; done(true); });
  });
}

function releaseEngineSlot() {
  engineSlots.running -= 1;
  const next = engineSlots.waiters.shift();
  if (next) next();
}

const requestWindow = new Map();   // address -> [timestamps]

function rateLimited(addr, now = Date.now()) {
  if (!RATE_LIMIT_PER_MIN) return false;
  const hits = (requestWindow.get(addr) || []).filter((t) => now - t < 60_000);
  if (hits.length >= RATE_LIMIT_PER_MIN) {
    requestWindow.set(addr, hits);
    return true;
  }
  hits.push(now);
  requestWindow.set(addr, hits);
  return false;
}

// Test seam: the window has to be resettable, or a rate-limit case would make
// every later case in the same process a 429.
export function resetRateLimit() { requestWindow.clear(); }

const INFO_UNSUPPORTED = "info=true is not supported by the web API; use the CLI for --info text output";

const MIME = {
  ".html": "text/html; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".mjs": "text/javascript; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".gif": "image/gif",
  ".png": "image/png",
  ".svg": "image/svg+xml",
  ".ico": "image/x-icon",
  ".json": "application/json",
};

async function isExecutableFile(file) {
  try {
    if (!(await stat(file)).isFile()) return false;
    await access(file, process.platform === "win32" ? constants.F_OK : constants.X_OK);
    return true;
  } catch { return false; }
}

// Structured resolution distinguishes an invalid explicit override from absent
// automatic discovery. A non-empty GS_ENGINE is exact (relative to CWD), never
// a PATH lookup. Empty/unset keeps discovery. Launch errors never cause fallback.
async function findEngine() {
  if (process.env.GS_ENGINE) {
    const override = process.env.GS_ENGINE;
    const file = resolve(override);
    if (await isExecutableFile(file)) return { path: file, source: "GS_ENGINE", error: null };
    return { path: null, source: "GS_ENGINE", error:
      `GS_ENGINE override is not an executable regular file: ${JSON.stringify(override)}; refusing automatic fallback` };
  }
  const rel = join(PRODUCT, "release");
  // U-66: `release/current` is the shared pin both surfaces honour FIRST, so a
  // VERSION.md bump cannot leave the desktop looking for a directory the web
  // server has already moved past. Mirrors GS_ENGINE_CURRENT in EngineLocator.h.
  for (const name of ["gifsicle", "gifsicle.exe"]) {
    const pinned = join(rel, "current", name);
    if (await isExecutableFile(pinned)) {
      return { path: await realpath(pinned).catch(() => pinned), source: "release/current", error: null };
    }
  }
  let versions = [];
  try { versions = await readdir(rel); } catch { versions = []; }
  // U-26: a lexicographic sort ranks 0.9.0 above 0.10.0. Compare numerically
  // per dotted component, falling back to a string compare for anything odd.
  versions.sort((a, b) => {
    const pa = String(a).split(".").map((n) => parseInt(n, 10));
    const pb = String(b).split(".").map((n) => parseInt(n, 10));
    for (let i = 0; i < Math.max(pa.length, pb.length); i += 1) {
      const na = Number.isNaN(pa[i]) ? -1 : (pa[i] ?? -1);
      const nb = Number.isNaN(pb[i]) ? -1 : (pb[i] ?? -1);
      if (na !== nb) return nb - na;   // newest first
    }
    return String(b).localeCompare(String(a));
  });
  for (const v of versions) {
    for (const name of ["gifsicle", "gifsicle.exe"]) {
      const p = join(rel, v, name);
      if (await isExecutableFile(p)) return { path: p, source: "release", error: null };
    }
  }
  return { path: null, source: "none", error:
    "Gifscythe engine (gifsicle) not found. Run ./build.sh first or set GS_ENGINE." };
}

function run(argv) {
  return new Promise((resolvePromise) => {
    // U-86 / P3-16: this promise could resolve TWICE — the timeout resolves with
    // 124 and the `close` event then resolves again with the real code. The first
    // resolution wins today only because that is how promises work, which is a
    // trap for any future close-time logic. Make the single resolution explicit.
    let settled = false;
    const settleOnce = (value) => { if (!settled) { settled = true; resolvePromise(value); } };
    const child = spawn(argv[0], argv.slice(1), {
      stdio: ["ignore", "ignore", "pipe"],
    });
    // U-93 / P2-20: stderr was accumulated without bound and then echoed whole
    // in 422 bodies, so a chatty engine (or a batch that warns per frame) turned
    // one request into unbounded server memory plus a wire response larger than
    // the upload that produced it. Cap the capture and SAY it was capped, so the
    // client can tell a complete message from a truncated one.
    let stderr = "";
    let stderrCapped = false;
    child.stderr.on("data", (d) => {
      if (stderr.length >= MAX_STDERR) { stderrCapped = true; return; }
      stderr += d.toString();
      if (stderr.length > MAX_STDERR) {
        stderr = stderr.slice(0, MAX_STDERR);
        stderrCapped = true;
      }
    });
    const timer = setTimeout(() => {
      child.kill("SIGKILL");
      settleOnce({ code: 124, stderr: "engine timed out", killed: true });
    }, ENGINE_TIMEOUT_MS);
    child.on("error", (err) => {
      clearTimeout(timer);
      settleOnce({ code: 127, stderr: String(err), failed: true });
    });
    child.on("close", (code) => {
      clearTimeout(timer);
      resolvePromise({
        code: code ?? 1,
        stderr: stderrCapped
          ? `${stderr}\n[stderr truncated at ${MAX_STDERR} characters]`
          : stderr,
        stderrCapped,
      });
    });
  });
}

// U-68 / NF-11: an oversized body used to reject with a bare Error and destroy
// the socket, so the caller could not tell it from a JSON parse failure (both
// landed in one catch -> 400 "bad JSON request body") and the client often saw a
// reset instead of any status. Tag the rejection with 413 and stop accumulating
// WITHOUT tearing down the socket; the handler writes the 413 and destroys the
// request only after the response has flushed.
class BodyTooLargeError extends Error {
  constructor(limit) {
    super(`request body too large (limit ${limit} bytes)`);
    this.name = "BodyTooLargeError";
    this.statusCode = 413;
  }
}

function readBody(req, limit) {
  return new Promise((resolveBody, reject) => {
    const chunks = [];
    let size = 0;
    let stopped = false;
    req.on("data", (c) => {
      if (stopped) return;
      size += c.length;
      if (size > limit) {
        stopped = true;
        reject(new BodyTooLargeError(limit));
        return;
      }
      chunks.push(c);
    });
    req.on("end", () => { if (!stopped) resolveBody(Buffer.concat(chunks)); });
    req.on("error", (err) => { if (!stopped) reject(err); });
  });
}

// U-06 / P1-5: back-pressure is a status code, not a hang. A caller that cannot
// be queued gets 429 + Retry-After so a proxy or script can back off.
function sendTooMany(req, res) {
  sendJson(res, 429, {
    ok: false,
    error: `too many engine runs in flight (cap ${MAX_CONCURRENT}, queue ${MAX_QUEUED}); retry later`,
    retryAfterMs: 250,
  });
  res.once("finish", () => { /* keep the socket; the caller may retry on it */ });
}

// Write the 413 and only then close the socket, so the status actually reaches
// the client instead of being cut off mid-flight.
function sendTooLarge(res, req, limit) {
  sendJson(res, 413, { ok: false, error: `request body too large (limit ${limit} bytes)` });
  res.once("finish", () => { req.destroy(); });
}

// U-67 / NF-10, narrowed by measurement in S21. The shipped UI is a closed set:
// index.html -> style.css + app.js -> command.mjs (a leaf module), and app.js
// posts to /run. web/wasm/ is experimental and not shippable (owner decision
// S21), so it is deliberately NOT routable here — serve that page with any
// static server rooted at web/, per web/wasm/README.md.
//
// The old handler had no allow-list, so it served the whole tree: server.mjs
// itself (26 KB, disclosing the loopback bind and GS_ENGINE handling — which
// matters because GS_WEB_HOST=0.0.0.0 is a documented opt-in), the test suite,
// run-paths/validate/output-verify, and the docs.
const STATIC_FILES = new Map([
  ["/", "index.html"],
  ["/index.html", "index.html"],
  ["/style.css", "style.css"],
  ["/app.js", "app.js"],
  ["/command.mjs", "command.mjs"],
  // U-54 / P1-34: app.js imports this module, so it MUST be routable — an
  // allow-list that lags the UI's import list breaks the shipped page with a
  // 404 on module load. static-hygiene.test.mjs now asserts the two lists agree.
  ["/request-guard.mjs", "request-guard.mjs"],
]);

const STATIC_NOT_FOUND = Buffer.from("not found");
const STATIC_FORBIDDEN = Buffer.from("forbidden");

// One response path for every static outcome, so 200/403/404 share a single HEAD
// contract. Node already suppresses a HEAD body on the wire; not writing one here
// makes that contract ours rather than incidental, and pins Content-Length either
// way. static-hygiene.test.mjs asserts it.
function sendStatic(req, res, status, contentType, body) {
  res.writeHead(status, {
    "Content-Type": contentType,
    "Content-Length": body.length,
    "Cache-Control": "no-store",
  });
  res.end(req.method === "HEAD" ? undefined : body);
}

async function serveStatic(req, res, urlPath) {
  const name = STATIC_FILES.get(urlPath);
  if (!name) {
    sendStatic(req, res, 404, "text/plain", STATIC_NOT_FOUND);
    return;
  }
  // Defence in depth. The allow-list already pins these names, so this cannot
  // fire today; it exists so a future entry cannot reintroduce the raw
  // `startsWith(ROOT)` prefix check that this replaces. Note the old check was
  // NOT a demonstrated traversal: new URL() normalises dot-segments before we see
  // them, and /../STATUS.md and /../../etc/hostname already returned 404. What it
  // was is brittle hygiene — a sibling like /tmp/web-x passes a string prefix.
  // assertContainedPath is the same resolved-path check run-paths.mjs enforces on
  // every engine output target, and transport.test.mjs already unit-tests it.
  let file;
  try {
    file = assertContainedPath(ROOT, join(ROOT, name));
  } catch {
    sendStatic(req, res, 403, "text/plain", STATIC_FORBIDDEN);
    return;
  }
  let data;
  try { data = await readFile(file); } catch {
    sendStatic(req, res, 404, "text/plain", STATIC_NOT_FOUND);
    return;
  }
  sendStatic(req, res, 200, MIME[extname(file)] || "application/octet-stream", data);
}

async function handleOptimize(req, res, url) {
  let settings = {};
  try {
    // U-49: `searchParams.get()` has ALREADY percent-decoded the value, so the
    // extra decodeURIComponent() decoded twice. Any setting containing a literal
    // "%" broke it — verified: `{"comments":["100%"]}` arrived as `100` plus a
    // stray escape and answered HTTP 400 "bad settings JSON".
    const raw = url.searchParams.get("settings") || "{}";
    settings = JSON.parse(raw);
    if (!settings || typeof settings !== "object" || Array.isArray(settings)) {
      res.writeHead(400, { "Content-Type": "application/json" });
      res.end(JSON.stringify({ ok: false, error: "bad settings JSON" }));
      return;
    }
  } catch {
    res.writeHead(400, { "Content-Type": "application/json" });
    res.end(JSON.stringify({ ok: false, error: "bad settings JSON" }));
    return;
  }

  // U-64 / NF-07: the web API is GIF-output only. `--info` writes text, so
  // letting it fall through to GIF verification produced a misleading 422 that
  // blamed the engine even though it exited 0 honestly. Reject it clearly here.
  if (settings.info) {
    sendJson(res, 400, { ok: false, error: INFO_UNSUPPORTED });
    return;
  }

  // U-79: /optimize returns ONE gif. Explode writes frames as <prefix>.NNN, so
  // it can only ever land in the single-file verifier as a misleading "no
  // output" 422 — the U-64 class. batch/merge of one file are fine (verified
  // 200); explode is the only contract breaker. /run carries all four modes.
  const optMode = settings.mode === undefined || settings.mode === null || settings.mode === ""
    ? "auto" : String(settings.mode);
  if (optMode === "explode") {
    sendJson(res, 400, {
      ok: false,
      error: "mode=explode is not supported by /optimize (single GIF output); use POST /run with mode=explode",
    });
    return;
  }

  // U-30: the desktop app surfaces out-of-range settings before running; the
  // demo used to pass them straight to gifsicle, so identical settings produced
  // a clear message on one client and a raw engine error on the other. Both now
  // use the same rules (validate.mjs mirrors src/core/Validate.h, cross-checked
  // against the real C++ validate() by web/test/validate.test.mjs).
  // `inputs` is faked as non-empty here: the real input is the request body,
  // and an empty upload is rejected separately below.
  const issues = validate({ ...settings, inputs: ["<upload>"] });
  if (issues.length) {
    res.writeHead(422, { "Content-Type": "application/json" });
    res.end(JSON.stringify({
      ok: false,
      error: issues.map((i) => `${i.field}=${i.value}: ${i.reason}`).join("; "),
      issues,
    }));
    return;
  }

  // U-84 / P3-14: the body is read BEFORE the engine is discovered, exactly like
  // handleRun. This handler used to discover the engine first, so an oversized
  // upload to an engine-less server answered 503 "engine not found" instead of
  // 413 — identical clients got a different body-cap contract per endpoint.
  // Reading first also means a refused upload never takes an engine slot.
  let body;
  try {
    body = await readBody(req, MAX_BODY);
  } catch (err) {
    if (err && err.statusCode === 413) { sendTooLarge(res, req, MAX_BODY); return; }
    sendJson(res, 400, { ok: false, error: "could not read request body" });
    return;
  }
  if (!body.length) {
    sendJson(res, 400, { ok: false, error: "empty upload" });
    return;
  }
  // U-92 / P2-19: for /optimize the body IS the GIF, so admission is a signature
  // check on it — a named 400 instead of relaying whatever the engine said about
  // somebody else's bytes.
  if (!hasGifMagic(body)) {
    sendJson(res, 400, {
      ok: false,
      error: "invalid upload: not a GIF (expected a GIF87a or GIF89a signature)",
    });
    return;
  }

  const resolution = await findEngine();
  if (!resolution.path) {
    res.writeHead(503, { "Content-Type": "application/json" });
    res.end(JSON.stringify({
      ok: false, error: resolution.error,
    }));
    return;
  }

  const engine = resolution.path;
  // U-06 / P1-5: the bounded resource is the ENGINE RUN, so the slot is taken
  // here — after validation AND after the body has been read and admitted, so a
  // refused request never consumes capacity — and before the temp tree exists.
  if (!(await acquireEngineSlot())) { sendTooMany(req, res); return; }
  let slotHeld = true;
  const dir = await mkdtemp(join(tmpdir(), "gsweb-"));
  try {
    const inFile = join(dir, "in.gif");
    const outFile = join(dir, "out.gif");
    await writeFile(inFile, body);

    const s = { ...settings, inputs: [inFile], output: outFile };
    const argv = [engine, ...buildArgs(s)];
    const before = await snapshotOutput(outFile);
    if (before.error) { sendJson(res, 422, { ok: false, error: before.error }); return; }
    const result = await run(argv);

    if (result.code !== 0) {
      res.writeHead(422, { "Content-Type": "application/json" });
      res.end(JSON.stringify({
        ok: false, exitCode: result.code, stderr: result.stderr.trim(),
        command: argv.map(shellQuote).join(" "),
      }));
      return;
    }

    const verified = await verifyOutput(outFile, before);
    if (verified.error) {
      sendJson(res, 422, { ok: false, exitCode: 0, stderr: verified.error,
        command: argv.map(shellQuote).join(" ") });
      return;
    }
    const outBytes = verified.data;  // serve the exact verified buffer
    res.writeHead(200, {
      "Content-Type": "image/gif",
      "Content-Disposition": 'attachment; filename="gifscythe-opt.gif"',
      "Access-Control-Expose-Headers":
        "X-Gifscythe-Command, X-Gifscythe-In-Bytes, X-Gifscythe-Out-Bytes",
      // U-50: HTTP header values must be latin1. A comment or engine path with
      // non-ASCII text made Node throw while writing this header, turning a
      // SUCCESSFUL engine run into a 500 — verified: {"comments":["作品"]}
      // returned HTTP 500 with the GIF never delivered. Percent-encode so the
      // header is always ASCII; app.js decodes it.
      "X-Gifscythe-Command": encodeURIComponent(argv.map(shellQuote).join(" ")),
      "X-Gifscythe-In-Bytes": String(body.length),
      "X-Gifscythe-Out-Bytes": String(outBytes.length),
    });
    res.end(outBytes);
  } catch (err) {
    // U-68 / NF-11: an oversized upload is a 413, not a generic 500. Since U-84
    // this handler reads the body BEFORE discovering the engine, so the 413 is
    // mapped above and reachable with no engine present — same contract as /run.
    // This branch is now defence in depth for any later read.
    if (err && err.statusCode === 413) { sendTooLarge(res, req, MAX_BODY); return; }
    res.writeHead(500, { "Content-Type": "application/json" });
    res.end(JSON.stringify({ ok: false, error: String(err && err.message ? err.message : err) }));
  } finally {
    if (slotHeld) { releaseEngineSlot(); slotHeld = false; }
    await rm(dir, { recursive: true, force: true }).catch(() => {});
  }
}

// ---------------------------------------------------------------------------
// POST /run — the multi-file, multi-mode endpoint (audit U-41 / P2-11).
// The command BUILDER was always mode-generic (command.mjs mirrors
// GifsicleCommand.h); what was missing was a transport that can carry several
// inputs and several outputs, and the desktop's honesty rules per mode:
// planned batch targets with collision refusal, and explode frame verification.
// ---------------------------------------------------------------------------

function sendJson(res, status, obj) {
  const body = JSON.stringify(obj);
  res.writeHead(status, {
    "Content-Type": "application/json",
    "Content-Length": Buffer.byteLength(body),
  });
  res.end(body);
}

// QFileInfo::completeBaseName semantics (strip after the LAST dot), which is
// what the desktop naming uses for <stem>_opt.gif / <stem>_frame.
const stemOf = (name) => {
  const i = String(name).lastIndexOf(".");
  return i > 0 ? String(name).slice(0, i) : String(name);
};

const isGifMagic = async (path) => {
  let fh;
  try {
    fh = await open(path, "r");
    const buf = Buffer.alloc(6);
    const { bytesRead } = await fh.read(buf, 0, 6, 0);
    return bytesRead === 6 && hasGifMagic(buf);
  } catch {
    return false;
  } finally {
    if (fh) await fh.close().catch(() => {});
  }
};

// Explode verification, mirroring src/core/ExplodeVerify.h: snapshot the
// "<prefix name>.*" candidates BEFORE the run, then require at least one NEW
// or CHANGED file with GIF87a/GIF89a magic. rc=0 with zero frames is a lie
// and must be refused (desktop P1-19 parity).
async function snapshotPrefix(dir, prefixName) {
  const snap = new Map();
  let ents;
  try { ents = await readdir(dir, { withFileTypes: true }); } catch { return snap; }
  for (const ent of ents) {
    if (!ent.isFile()) continue;
    if (!ent.name.startsWith(prefixName + ".") || ent.name.length <= prefixName.length + 1) continue;
    try {
      const st = await stat(requestPath(dir, ent.name));
      snap.set(ent.name, { size: st.size, mtimeMs: st.mtimeMs });
    } catch { /* vanished mid-snapshot; treat as absent */ }
  }
  return snap;
}

async function listWrittenFrames(dir, prefixName, before) {
  const frames = [];
  const suspicious = [];
  let ents;
  try { ents = await readdir(dir, { withFileTypes: true }); } catch { return { frames, suspicious }; }
  for (const ent of ents.sort((a, b) => a.name.localeCompare(b.name))) {
    if (!ent.isFile()) continue;
    if (!ent.name.startsWith(prefixName + ".") || ent.name.length <= prefixName.length + 1) continue;
    let st;
    try { st = await stat(requestPath(dir, ent.name)); } catch { continue; }
    const prev = before.get(ent.name);
    if (prev && prev.size === st.size && prev.mtimeMs === st.mtimeMs) continue; // stale leftover
    if (await isGifMagic(requestPath(dir, ent.name))) frames.push(ent.name);
    else suspicious.push(ent.name);
  }
  return { frames, suspicious };
}

// U-92 / P2-19: `Buffer.from(str, "base64")` is forgiving — it silently decodes
// truncated and foreign payloads, so "the client sent base64" was never actually
// checked. A strict shape test plus a canonical round trip makes it a real claim.
const BASE64_SHAPE = /^(?:[A-Za-z0-9+/]{4})*(?:[A-Za-z0-9+/]{2}==|[A-Za-z0-9+/]{3}=)?$/;
function isStrictBase64(text) {
  if (typeof text !== "string" || text.length === 0 || text.length % 4 !== 0) return false;
  if (!BASE64_SHAPE.test(text)) return false;
  return Buffer.from(text, "base64").toString("base64") === text;
}

async function handleRun(req, res) {
  let payload;
  // U-68 / NF-11: reading the body and parsing it are separate failures and must
  // not share one catch. Oversized -> 413; unreadable -> 400; malformed JSON ->
  // 400 "bad JSON request body" (unchanged). This runs before findEngine(), so
  // the 413 is reachable with no engine present.
  let raw;
  try {
    raw = await readBody(req, MAX_BODY);
  } catch (err) {
    if (err && err.statusCode === 413) { sendTooLarge(res, req, MAX_BODY); return; }
    sendJson(res, 400, { ok: false, error: "could not read request body" });
    return;
  }
  try {
    payload = JSON.parse(raw.toString("utf8"));
    if (!payload || typeof payload !== "object" || Array.isArray(payload)) {
      sendJson(res, 400, { ok: false, error: "bad JSON request body" });
      return;
    }
  } catch {
    sendJson(res, 400, { ok: false, error: "bad JSON request body" });
    return;
  }
  const settings = (payload && typeof payload.settings === "object" && payload.settings && !Array.isArray(payload.settings))
    ? payload.settings : {};
  const files = Array.isArray(payload.files) ? payload.files : [];
  const mode = settings.mode === undefined || settings.mode === null || settings.mode === ""
    ? "auto" : String(settings.mode);

  if (!["auto", "batch", "merge", "explode"].includes(mode)) {
    sendJson(res, 400, { ok: false, error: `unknown mode "${mode}" (auto|batch|merge|explode)` });
    return;
  }
  if (!files.length) {
    sendJson(res, 400, { ok: false, error: "no files uploaded (files: [{name, data(base64)}])" });
    return;
  }
  for (const f of files) {
    if (!f || typeof f.name !== "string" || !f.name.trim() || typeof f.data !== "string") {
      sendJson(res, 400, { ok: false, error: "every file needs a non-empty name and base64 data" });
      return;
    }
    const problem = uploadNameError(f.name);
    if (problem) {
      sendJson(res, 400, { ok: false, error: `invalid upload name: ${problem}`, file: f.name });
      return;
    }
  }
  // U-92 / P2-19: admission — GS-205's web twin (that row names the Qt picker and
  // drop filter only). Both endpoints used to accept arbitrary bytes and let the
  // engine's stderr be the error message; a non-GIF upload is now a named 400
  // BEFORE any engine discovery, slot or temp tree, and the decoded buffers are
  // kept so nothing is decoded twice.
  const uploads = [];
  for (const f of files) {
    if (!isStrictBase64(f.data)) {
      sendJson(res, 400, {
        ok: false, error: `invalid upload: ${f.name} is not valid base64`, file: f.name });
      return;
    }
    const buf = Buffer.from(f.data, "base64");
    if (!buf.length) {
      sendJson(res, 400, {
        ok: false, error: `invalid upload: ${f.name} decodes to zero bytes`, file: f.name });
      return;
    }
    if (!hasGifMagic(buf)) {
      sendJson(res, 400, {
        ok: false,
        error: `invalid upload: ${f.name} is not a GIF (expected a GIF87a or GIF89a signature)`,
        file: f.name });
      return;
    }
    uploads.push(buf);
  }

  // Mode usage rules, mirroring what the desktop enforces in MainWindow:
  if (mode === "auto" && files.length !== 1) {
    sendJson(res, 400, {
      ok: false,
      error: "Auto mode processes exactly one file — use Batch to optimize each of them, or Merge to combine them",
    });
    return;
  }
  if (mode === "explode" && files.length !== 1) {
    sendJson(res, 400, { ok: false, error: "Explode processes exactly one file per run" });
    return;
  }

  // U-64 / NF-07: the web API only returns GIFs/JSON. `info:true` is text-mode,
  // so reject it clearly before any engine spawn instead of misreporting a 422.
  if (settings.info) {
    sendJson(res, 400, { ok: false, error: INFO_UNSUPPORTED });
    return;
  }

  // Same validation layer as /optimize (audit U-30): out-of-range settings
  // are refused before the engine ever sees them.
  const issues = validate({ ...settings, mode, inputs: files.map((f) => f.name) });
  if (issues.length) {
    sendJson(res, 422, {
      ok: false,
      error: issues.map((i) => `${i.field}=${i.value}: ${i.reason}`).join("; "),
      issues,
    });
    return;
  }

  const resolution = await findEngine();
  if (!resolution.path) {
    sendJson(res, 503, {
      ok: false,
      error: resolution.error,
    });
    return;
  }

  const engine = resolution.path;
  // U-06 / P1-5: one gifscyle at a time, per server, past the cap a caller is
  // told to come back rather than queued onto an unbounded heap.
  if (!(await acquireEngineSlot())) { sendTooMany(req, res); return; }
  let slotHeld = true;
  const dir = await mkdtemp(join(tmpdir(), "gsweb-"));
  try {
    // ---- plan + refuse BEFORE anything runs (desktop batch parity, U-01) ----
    const targets = mode === "merge" ? ["merged.gif"]
      : mode === "explode" ? [`${stemOf(files[0].name)}_frame`]
      : files.map((f) => `${stemOf(f.name)}_opt.gif`);
    // Contain ALL targets (including the explode prefix) before writing uploads
    // or starting the first subprocess. Never rely on admission alone.
    let targetPaths;
    try {
      targetPaths = targets.map((name) => requestPath(dir, name));
    } catch (err) {
      sendJson(res, 422, { ok: false, error: err.message });
      return;
    }
    if (mode === "batch") {
      const names = files.map((f) => f.name);
      const sourceKeys = new Set(names.map(outputNameKey));
      const seen = new Map();
      for (let i = 0; i < targets.length; i += 1) {
        const key = outputNameKey(targets[i]);
        if (seen.has(key)) {
          sendJson(res, 422, {
            ok: false,
            error: `refusing to run: "${names[seen.get(key)]}" and "${names[i]}" would both write ${targets[i]}, destroying one result`,
          });
          return;
        }
        seen.set(key, i);
        if (sourceKeys.has(key)) {
          sendJson(res, 422, {
            ok: false,
            // U-86 / P3-16: this message claimed it protects "the uploaded file
            // of the same name", but uploads are renamed inN.gif in the temp tree —
            // what is actually protected is a PLANNED OUTPUT colliding with a
            // user-facing upload name. Say what the check really does.
            error: `refusing to run: the planned output ${targets[i]} collides with an upload name in this request`,
          });
          return;
        }
      }
    }

    // Decode uploads to neutral on-disk names; the user-facing names stay in
    // the response (only validated names enter the separately contained output plan).
    const paths = [];
    let inBytes = 0;
    for (let i = 0; i < files.length; i += 1) {
      const buf = uploads[i];  // U-92: already admitted (strict base64 + GIF magic)
      if (!buf.length) {
        sendJson(res, 400, { ok: false, error: `file "${files[i].name}" has empty data` });
        return;
      }
      inBytes += buf.length;
      const p = requestPath(dir, `in${i}.gif`);
      await writeFile(p, buf);
      paths.push(p);
    }
    const quote = (argv) => argv.map(shellQuote).join(" ");

    const outputs = [];   // { name, path }
    const commands = [];  // quoted command lines, in run order
    let outBytes = 0;

    const runOne = async (s, outPath, label) => {
      const before = await snapshotOutput(outPath);
      if (before.error) {
        sendJson(res, 422, { ok: false, error: before.error, file: label });
        return false;
      }
      const argv = [engine, ...buildArgs(s)];
      commands.push(quote(argv));
      const result = await run(argv);
      if (result.code !== 0) {
        sendJson(res, 422, {
          ok: false, exitCode: result.code, stderr: result.stderr.trim(),
          file: label, command: quote(argv),
        });
        return false;
      }
      const verified = await verifyOutput(outPath, before);
      if (verified.error) {
        sendJson(res, 422, { ok: false, exitCode: 0, file: label,
          command: quote(argv), stderr: verified.error });
        return false;
      }
      outputs.push({ name: basename(outPath), path: outPath, data: verified.data });
      outBytes += verified.data.length;
      return true;
    };

    let done = true;
    if (mode === "batch") {
      for (let i = 0; i < files.length && done; i += 1) {
        const outPath = targetPaths[i];
        // The desktop batch runs one Auto command PER FILE (never one -b run);
        // mirror that exactly so the commands list matches the desktop pane.
        done = await runOne(
          { ...settings, mode: "auto", inputs: [paths[i]], output: outPath },
          outPath, files[i].name);
      }
    } else if (mode === "merge") {
      const outPath = targetPaths[0];
      done = await runOne({ ...settings, mode, inputs: paths, output: outPath },
                          outPath, "merged.gif");
    } else if (mode === "explode") {
      const prefixName = targets[0];
      const outPath = targetPaths[0];
      const before = await snapshotPrefix(dir, prefixName);
      const argv = [engine, ...buildArgs({ ...settings, mode, inputs: paths, output: outPath })];
      commands.push(quote(argv));
      const result = await run(argv);
      if (result.code !== 0) {
        sendJson(res, 422, {
          ok: false, exitCode: result.code, stderr: result.stderr.trim(),
          file: files[0].name, command: quote(argv),
        });
        return;
      }
      const { frames, suspicious } = await listWrittenFrames(dir, prefixName, before);
      if (!frames.length) {
        sendJson(res, 422, {
          ok: false, exitCode: 0, file: files[0].name, command: quote(argv),
          stderr: `the engine exited 0 but wrote no frames under the prefix ${prefixName}`
            + (suspicious.length
              ? ` (${suspicious.length} new file(s) there, none a valid GIF)` : ""),
        });
        return;
      }
      for (const name of frames) {
        const p = requestPath(dir, name);
        const st = await stat(p);
        outputs.push({ name, path: p });
        outBytes += st.size;
      }
    } else { // auto
      const outPath = targetPaths[0];
      done = await runOne({ ...settings, mode: "auto", inputs: paths, output: outPath },
                          outPath, files[0].name);
    }
    if (!done) return; // the honest 422 was already sent by runOne

    const out = [];
    for (const o of outputs) {
      const data = o.data ?? await readFile(o.path);
      out.push({ name: o.name, bytes: data.length, data: data.toString("base64") });
    }
    sendJson(res, 200, { ok: true, mode, outputs: out, commands, inBytes, outBytes });
  } catch (err) {
    sendJson(res, 500, { ok: false, error: String(err && err.message ? err.message : err) });
  } finally {
    if (slotHeld) { releaseEngineSlot(); slotHeld = false; }
    await rm(dir, { recursive: true, force: true }).catch(() => {});
  }
}

const server = http.createServer(async (req, res) => {
  const url = new URL(req.url || "/", "http://localhost");
  try {
    if (req.method === "GET" || req.method === "HEAD") {
      // U-93 / P2-20: browsers request /favicon.ico on every page load and the
      // allow-list had no entry, so each visit produced a 404 in the log. index.html
      // now carries a data-URI icon (no request at all); this answers the ones
      // that arrive anyway. 204, not a served file: nothing to disclose.
      if (url.pathname === "/favicon.ico") {
        res.writeHead(204, { "Cache-Control": "no-store" });
        res.end();
        return;
      }
      await serveStatic(req, res, url.pathname);
      return;
    }
    if (req.method === "POST") {
      // U-06 / P1-5: a per-client bound on the endpoints that cost anything.
      // Static is exempt — a page load is several GETs by design.
      if (rateLimited(req.socket?.remoteAddress || "unknown")) {
        res.writeHead(429, {
          "Content-Type": "application/json",
          "Retry-After": "1",
          "Content-Length": 0,
        });
        res.end();
        return;
      }
      if (url.pathname !== "/optimize" && url.pathname !== "/run") {
        res.writeHead(405, { "Content-Type": "text/plain" });
        res.end("method not allowed");
        return;
      }
    }
    if (req.method === "POST" && url.pathname === "/optimize") {
      await handleOptimize(req, res, url);
      return;
    }
    if (req.method === "POST" && url.pathname === "/run") {
      await handleRun(req, res);
      return;
    }
    res.writeHead(405, { "Content-Type": "text/plain" });
    res.end("method not allowed");
  } catch (err) {
    res.writeHead(500, { "Content-Type": "text/plain" });
    res.end(String(err));
  }
});

const engineResolution = await findEngine();
server.listen(PORT, HOST, () => {
  console.log(`Gifscythe web server on http://${HOST}:${PORT}`);
  if (HOST === "127.0.0.1") {
    console.log("  (loopback only — set GS_WEB_HOST=0.0.0.0 to expose it on the network)");
  }
  console.log(`Engine [${engineResolution.source}]: ${engineResolution.path || `ERROR: ${engineResolution.error}`}`);
});
