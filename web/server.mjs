// server.mjs — zero-dependency Node HTTP server for the Gifscythe web build.
//
// Serves the static web UI (index.html / app.js / style.css / command.mjs) and
// exposes one API endpoint:
//
//   POST /optimize?settings=<urlencoded JSON>   body = raw GIF bytes
//     -> 200 image/gif  (optimized GIF) with X-Gifscythe-* metadata headers
//     -> 422 JSON       { ok:false, exitCode, stderr, command }
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
  readFile, writeFile, mkdtemp, rm, readdir, access,
} from "node:fs/promises";
import { existsSync } from "node:fs";
import { tmpdir } from "node:os";
import { join, extname, normalize, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { buildArgs, shellQuote } from "./command.mjs";
import { validate } from "./validate.mjs";

const ROOT = dirname(fileURLToPath(import.meta.url)); // web/
const PRODUCT = resolve(ROOT, "..", "working_code", "gifscythe");
const PORT = Number(process.argv[2] || process.env.PORT || 8000);
// U-06: the demo used to bind 0.0.0.0 unconditionally, so anyone on the network
// could submit 64 MB bodies and 120 s engine runs to an endpoint with no auth
// and no concurrency cap. Default to loopback; opt in explicitly.
const HOST = process.env.GS_WEB_HOST || "127.0.0.1";
const MAX_BODY = 64 * 1024 * 1024;
const ENGINE_TIMEOUT_MS = 120_000;

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

async function findEngine() {
  if (process.env.GS_ENGINE) {
    try { await access(process.env.GS_ENGINE); return process.env.GS_ENGINE; } catch {}
  }
  const rel = join(PRODUCT, "release");
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
      if (existsSync(p)) return p;
    }
  }
  return null;
}

function run(argv) {
  return new Promise((resolvePromise) => {
    const child = spawn(argv[0], argv.slice(1), {
      stdio: ["ignore", "ignore", "pipe"],
    });
    let stderr = "";
    child.stderr.on("data", (d) => { stderr += d.toString(); });
    const timer = setTimeout(() => {
      child.kill("SIGKILL");
      resolvePromise({ code: 124, stderr: "engine timed out", killed: true });
    }, ENGINE_TIMEOUT_MS);
    child.on("error", (err) => {
      clearTimeout(timer);
      resolvePromise({ code: 127, stderr: String(err), failed: true });
    });
    child.on("close", (code) => {
      clearTimeout(timer);
      resolvePromise({ code: code ?? 1, stderr });
    });
  });
}

function readBody(req, limit) {
  return new Promise((resolveBody, reject) => {
    const chunks = [];
    let size = 0;
    req.on("data", (c) => {
      size += c.length;
      if (size > limit) {
        reject(new Error("request body too large"));
        req.destroy();
        return;
      }
      chunks.push(c);
    });
    req.on("end", () => resolveBody(Buffer.concat(chunks)));
    req.on("error", reject);
  });
}

async function serveStatic(res, urlPath) {
  const rel = urlPath === "/" ? "index.html" : urlPath;
  const file = normalize(join(ROOT, rel));
  if (!file.startsWith(ROOT)) {
    res.writeHead(403, { "Content-Type": "text/plain" });
    res.end("forbidden");
    return;
  }
  let data;
  try { data = await readFile(file); } catch {
    res.writeHead(404, { "Content-Type": "text/plain" });
    res.end("not found");
    return;
  }
  res.writeHead(200, {
    "Content-Type": MIME[extname(file)] || "application/octet-stream",
    "Cache-Control": "no-store",
  });
  res.end(data);
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
  } catch {
    res.writeHead(400, { "Content-Type": "application/json" });
    res.end(JSON.stringify({ ok: false, error: "bad settings JSON" }));
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

  const engine = await findEngine();
  if (!engine) {
    res.writeHead(503, { "Content-Type": "application/json" });
    res.end(JSON.stringify({
      ok: false, error:
        "Gifscythe engine (gifsicle) not found. Run ./build.sh first or set GS_ENGINE.",
    }));
    return;
  }

  const dir = await mkdtemp(join(tmpdir(), "gsweb-"));
  try {
    const body = await readBody(req, MAX_BODY);
    if (!body.length) {
      res.writeHead(400, { "Content-Type": "application/json" });
      res.end(JSON.stringify({ ok: false, error: "empty upload" }));
      return;
    }
    const inFile = join(dir, "in.gif");
    const outFile = join(dir, "out.gif");
    await writeFile(inFile, body);

    const s = { ...settings, inputs: [inFile], output: outFile };
    const argv = [engine, ...buildArgs(s)];
    const result = await run(argv);

    if (result.code !== 0) {
      res.writeHead(422, { "Content-Type": "application/json" });
      res.end(JSON.stringify({
        ok: false, exitCode: result.code, stderr: result.stderr.trim(),
        command: argv.map(shellQuote).join(" "),
      }));
      return;
    }

    // U-24: rc=0 does not prove a GIF was written. Reading a missing file used
    // to throw ENOENT into the generic catch and surface as a 500; the desktop
    // app has verified its output before claiming success since S4, and the
    // demo has to be just as honest.
    let outBytes;
    try {
      outBytes = await readFile(outFile);
    } catch {
      outBytes = null;
    }
    if (!outBytes || outBytes.length === 0) {
      res.writeHead(422, { "Content-Type": "application/json" });
      res.end(JSON.stringify({
        ok: false, exitCode: 0,
        stderr: "the engine exited 0 but produced no output file",
        command: argv.map(shellQuote).join(" "),
      }));
      return;
    }
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
    res.writeHead(500, { "Content-Type": "application/json" });
    res.end(JSON.stringify({ ok: false, error: String(err && err.message ? err.message : err) }));
  } finally {
    await rm(dir, { recursive: true, force: true }).catch(() => {});
  }
}

const server = http.createServer(async (req, res) => {
  const url = new URL(req.url || "/", "http://localhost");
  try {
    if (req.method === "GET" || req.method === "HEAD") {
      await serveStatic(res, url.pathname);
      return;
    }
    if (req.method === "POST" && url.pathname === "/optimize") {
      await handleOptimize(req, res, url);
      return;
    }
    res.writeHead(405, { "Content-Type": "text/plain" });
    res.end("method not allowed");
  } catch (err) {
    res.writeHead(500, { "Content-Type": "text/plain" });
    res.end(String(err));
  }
});

const enginePath = await findEngine();
server.listen(PORT, HOST, () => {
  console.log(`Gifscythe web server on http://${HOST}:${PORT}`);
  if (HOST === "127.0.0.1") {
    console.log("  (loopback only — set GS_WEB_HOST=0.0.0.0 to expose it on the network)");
  }
  console.log(`Engine: ${enginePath || "NOT FOUND (build with ./build.sh or set GS_ENGINE)"}`);
});
