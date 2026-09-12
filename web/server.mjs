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
import { existsSync } from "node:fs";
import { tmpdir } from "node:os";
import { join, extname, normalize, resolve, dirname, basename } from "node:path";
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
    return bytesRead === 6 && buf.toString("latin1", 0, 4) === "GIF8"
      && (buf[4] === 0x37 || buf[4] === 0x39) && buf[5] === 0x61; // '7'|'9', 'a'
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
      const st = await stat(join(dir, ent.name));
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
    try { st = await stat(join(dir, ent.name)); } catch { continue; }
    const prev = before.get(ent.name);
    if (prev && prev.size === st.size && prev.mtimeMs === st.mtimeMs) continue; // stale leftover
    if (await isGifMagic(join(dir, ent.name))) frames.push(ent.name);
    else suspicious.push(ent.name);
  }
  return { frames, suspicious };
}

async function handleRun(req, res) {
  let payload;
  try {
    const raw = await readBody(req, MAX_BODY);
    payload = JSON.parse(raw.toString("utf8"));
  } catch {
    sendJson(res, 400, { ok: false, error: "bad JSON request body" });
    return;
  }
  const settings = (payload && typeof payload.settings === "object" && payload.settings) || {};
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

  const engine = await findEngine();
  if (!engine) {
    sendJson(res, 503, {
      ok: false,
      error: "Gifscythe engine (gifsicle) not found. Run ./build.sh first or set GS_ENGINE.",
    });
    return;
  }

  const dir = await mkdtemp(join(tmpdir(), "gsweb-"));
  try {
    // Decode uploads to neutral on-disk names; the user-facing names stay in
    // the response (nothing from the request ever becomes a path here).
    const paths = [];
    let inBytes = 0;
    for (let i = 0; i < files.length; i += 1) {
      const buf = Buffer.from(files[i].data, "base64");
      if (!buf.length) {
        sendJson(res, 400, { ok: false, error: `file "${files[i].name}" has empty data` });
        return;
      }
      inBytes += buf.length;
      const p = join(dir, `in${i}.gif`);
      await writeFile(p, buf);
      paths.push(p);
    }
    const quote = (argv) => argv.map(shellQuote).join(" ");

    // ---- plan + refuse BEFORE anything runs (desktop batch parity, U-01) ----
    let targets = [];
    if (mode === "batch") {
      targets = files.map((f) => `${stemOf(f.name)}_opt.gif`);
      const names = files.map((f) => f.name);
      const seen = new Map();
      for (let i = 0; i < targets.length; i += 1) {
        if (seen.has(targets[i])) {
          sendJson(res, 422, {
            ok: false,
            error: `refusing to run: "${names[seen.get(targets[i])]}" and "${names[i]}" would both write ${targets[i]}, destroying one result`,
          });
          return;
        }
        seen.set(targets[i], i);
        if (names.includes(targets[i])) {
          sendJson(res, 422, {
            ok: false,
            error: `refusing to run: the planned output ${targets[i]} would overwrite the uploaded file of the same name`,
          });
          return;
        }
      }
    }

    const outputs = [];   // { name, path }
    const commands = [];  // quoted command lines, in run order
    let outBytes = 0;

    const runOne = async (s, outPath, label) => {
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
      // rc=0 does not prove output (audit U-24, desktop parity).
      let st = null;
      try { st = await stat(outPath); } catch { st = null; }
      if (!st || st.size === 0) {
        sendJson(res, 422, {
          ok: false, exitCode: 0, file: label, command: quote(argv),
          stderr: "the engine exited 0 but produced no output file",
        });
        return false;
      }
      outputs.push({ name: basename(outPath), path: outPath });
      outBytes += st.size;
      return true;
    };

    let done = true;
    if (mode === "batch") {
      for (let i = 0; i < files.length && done; i += 1) {
        const outPath = join(dir, targets[i]);
        // The desktop batch runs one Auto command PER FILE (never one -b run);
        // mirror that exactly so the commands list matches the desktop pane.
        done = await runOne(
          { ...settings, mode: "auto", inputs: [paths[i]], output: outPath },
          outPath, files[i].name);
      }
    } else if (mode === "merge") {
      const outPath = join(dir, "merged.gif");
      done = await runOne({ ...settings, mode, inputs: paths, output: outPath },
                          outPath, "merged.gif");
    } else if (mode === "explode") {
      const prefixName = `${stemOf(files[0].name)}_frame`;
      const outPath = join(dir, prefixName);
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
        const p = join(dir, name);
        const st = await stat(p);
        outputs.push({ name, path: p });
        outBytes += st.size;
      }
    } else { // auto
      const outPath = join(dir, `${stemOf(files[0].name)}_opt.gif`);
      done = await runOne({ ...settings, mode: "auto", inputs: paths, output: outPath },
                          outPath, files[0].name);
    }
    if (!done) return; // the honest 422 was already sent by runOne

    const out = [];
    for (const o of outputs) {
      const data = await readFile(o.path);
      out.push({ name: o.name, bytes: data.length, data: data.toString("base64") });
    }
    sendJson(res, 200, { ok: true, mode, outputs: out, commands, inBytes, outBytes });
  } catch (err) {
    sendJson(res, 500, { ok: false, error: String(err && err.message ? err.message : err) });
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

const enginePath = await findEngine();
server.listen(PORT, HOST, () => {
  console.log(`Gifscythe web server on http://${HOST}:${PORT}`);
  if (HOST === "127.0.0.1") {
    console.log("  (loopback only — set GS_WEB_HOST=0.0.0.0 to expose it on the network)");
  }
  console.log(`Engine: ${enginePath || "NOT FOUND (build with ./build.sh or set GS_ENGINE)"}`);
});
