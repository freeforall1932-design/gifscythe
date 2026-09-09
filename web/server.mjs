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
//   node web/server.mjs [port]      (default 8000; binds 0.0.0.0)

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

const ROOT = dirname(fileURLToPath(import.meta.url)); // web/
const PRODUCT = resolve(ROOT, "..", "working_code", "gifscythe");
const PORT = Number(process.argv[2] || process.env.PORT || 8000);
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
  versions.sort().reverse();
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
    const raw = url.searchParams.get("settings") || "{}";
    settings = JSON.parse(decodeURIComponent(raw));
  } catch {
    res.writeHead(400, { "Content-Type": "application/json" });
    res.end(JSON.stringify({ ok: false, error: "bad settings JSON" }));
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

    const outBytes = await readFile(outFile);
    res.writeHead(200, {
      "Content-Type": "image/gif",
      "Content-Disposition": 'attachment; filename="gifscythe-opt.gif"',
      "Access-Control-Expose-Headers":
        "X-Gifscythe-Command, X-Gifscythe-In-Bytes, X-Gifscythe-Out-Bytes",
      "X-Gifscythe-Command": argv.map(shellQuote).join(" "),
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
server.listen(PORT, "0.0.0.0", () => {
  console.log(`Gifscythe web server on http://0.0.0.0:${PORT}`);
  console.log(`Engine: ${enginePath || "NOT FOUND (build with ./build.sh or set GS_ENGINE)"}`);
});
