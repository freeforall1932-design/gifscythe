// server-bounds.test.mjs — the resource bounds audit U-06 named and S8 left
// (fix-order P1-5). The finding was "no auth, no concurrency cap, 64 MB bodies,
// 120 s engine runs": every accepted request spawned a process and a temp tree,
// so N clients meant N+1 gifsicles competing for CPU, for as long as they liked.
//
// Loopback binding was landed in S8. This file measures the rest, against a REAL
// server process each time (the bounds are per-process state — importing
// server.mjs twice in one process would test a fiction):
//
//   1. concurrency cap        — at GS_MAX_CONCURRENT=1 a second overlapping run
//                              with GS_MAX_QUEUED=0 answers 429, and the slot is
//                              handed back so the NEXT request succeeds;
//   2. a refused request costs nothing — validation 422s happen before the slot
//                              is taken, so they cannot exhaust the queue;
//   3. per-client rate limit  — 429 with Retry-After once the window is full;
//   4. the engine-run bound   — GS_ENGINE_TIMEOUT_MS is honoured and the caller
//                              gets an honest 422 naming the timeout;
//   5. static is not rate limited (a page load is several GETs by design).
//
// Run: node web/test/server-bounds.test.mjs   (after ./build.sh, which builds the
// real engine the non-slow cases use)

import { spawn } from "node:child_process";
import { once } from "node:events";
import { mkdtemp, writeFile, chmod, rm, access, symlink } from "node:fs/promises";
import { createServer } from "node:net";
import { tmpdir } from "node:os";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const HERE = dirname(fileURLToPath(import.meta.url));
const SERVER = join(HERE, "..", "server.mjs");
const REPO = join(HERE, "..", "..");
const PRODUCT = join(REPO, "working_code", "gifscythe");

let pass = 0;
let fail = 0;
const t = (name, problems) => {
  if (problems.length) { fail += 1; console.log(`FAIL ${name}\n       ${problems.join("\n       ")}`); }
  else { pass += 1; console.log(`PASS ${name}`); }
};

// A self-contained 43-byte 1x1 GIF89a (same fixture the transport suite uses:
// `gifsicle --info` reports 1 image, 1x1).
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
    s.listen(0, "127.0.0.1", () => { const { port } = s.address(); s.close(() => res(port)); });
  });
}

async function startServer(env, logOf = () => "") {
  const port = await freePort();
  const child = spawn(process.execPath, [SERVER, String(port)], {
    env: { ...process.env, ...env },
    stdio: ["ignore", "pipe", "pipe"],
  });
  let log = "";
  child.stdout.on("data", (d) => { log += d.toString(); });
  child.stderr.on("data", (d) => { log += d.toString(); });
  const up = await Promise.race([
    once(child, "listening").then(() => "listening"),
    new Promise((res) => {
      const iv = setInterval(() => {
        if (log.includes("web server on")) { clearInterval(iv); res("log"); }
      }, 25);
      child.once("exit", () => { clearInterval(iv); res("exited"); });
      setTimeout(() => { clearInterval(iv); res("timeout"); }, 8000);
    }),
  ]);
  if (up === "exited" || up === "timeout") throw new Error(`server did not start: ${log}`);
  return { port, child, logOf: () => log };
}

function stop(child) {
  if (child.exitCode === null && child.signalCode === null) child.kill("SIGKILL");
  return once(child, "exit").catch(() => null);
}

async function optimize(port, settings = {}, timeoutMs = 30_000) {
  const ctl = new AbortController();
  const timer = setTimeout(() => ctl.abort(), timeoutMs);
  try {
    const q = settings && Object.keys(settings).length
      ? `?settings=${encodeURIComponent(JSON.stringify(settings))}` : "";
    const resp = await fetch(`http://127.0.0.1:${port}/optimize${q}`, {
      method: "POST", headers: { "Content-Type": "application/octet-stream" },
      body: GIF, signal: ctl.signal,
    });
    const ct = resp.headers.get("content-type") || "";
    const body = ct.includes("json") ? await resp.json().catch(() => null) : null;
    const bytes = ct.includes("json") ? 0 : (await resp.arrayBuffer()).byteLength;
    return { status: resp.status, body, bytes, headers: Object.fromEntries(resp.headers) };
  } finally {
    clearTimeout(timer);
  }
}

async function runMode(port, settings, timeoutMs = 30_000) {
  const ctl = new AbortController();
  const timer = setTimeout(() => ctl.abort(), timeoutMs);
  try {
    const resp = await fetch(`http://127.0.0.1:${port}/run`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      signal: ctl.signal,
      body: JSON.stringify({ settings, files: [{ name: "clip.gif", data: GIF.toString("base64") }] }),
    });
    return { status: resp.status, body: await resp.json().catch(() => null) };
  } finally {
    clearTimeout(timer);
  }
}

const dir = await mkdtemp(join(tmpdir(), "gsbounds-"));
// A stand-in engine that SLEEPS, so overlap is measurable rather than theoretical.
// It writes the fixture GIF to the `-o` target, which is all the CLI-side
// verification requires — the point of these cases is the schedule, not the GIF.
const slowEngine = join(dir, "slow.sh");
await writeFile(slowEngine, `#!/bin/sh
out=""
prev=""
for a in "$@"; do
  if [ "$prev" = "-o" ]; then out="$a"; fi
  prev="$a"
done
sleep \${GS_FAKE_SLEEP:-1}
if [ -n "$out" ]; then cat "\${GS_FAKE_GIF}" > "$out"; fi
exit 0
`);
await chmod(slowEngine, 0o755);
const fixtureGif = join(dir, "fixture.gif");
await writeFile(fixtureGif, GIF);
const realEngine = join(PRODUCT, "release", "0.1.0", "gifsicle");

try {
  // ---- 1 + 2: concurrency cap, queue, and that refusals cost nothing ----
  {
    const { port, child } = await startServer({
      GS_ENGINE: slowEngine, GS_FAKE_SLEEP: "2", GS_FAKE_GIF: fixtureGif,
      GS_MAX_CONCURRENT: "1", GS_MAX_QUEUED: "0",
    });
    const p = [];
    try {
      const first = optimize(port, {}, 30_000);
      // Give the first request time to occupy the single slot.
      await new Promise((r) => setTimeout(r, 400));
      const second = await optimize(port);
      if (second.status !== 429) p.push(`queued run answered ${second.status}, expected 429`);
      if (!(second.body && /too many engine runs in flight/.test(second.body.error || ""))) {
        p.push(`429 body not explanatory: ${JSON.stringify(second.body)}`);
      }
      // A request refused by validation must not consume capacity either. The
      // settings key is color_count (the gs::Settings field name; `colors` is
      // only the conf-file spelling) — and validate.mjs ignores unknown keys,
      // which is exactly why a wrong key here used to sail through to the engine.
      const refused = await optimize(port, { color_count: 999 });
      if (refused.status !== 422) p.push(`bad color_count answered ${refused.status}, expected 422`);
      const stillOpen = await optimize(port, { color_count: 999 });
      if (stillOpen.status !== 422) p.push(`second refused request answered ${stillOpen.status}`);
      const done = await first;
      if (done.status !== 200 || done.bytes === 0) p.push(`first run failed: ${done.status}/${done.bytes}`);
      // Slot released: the next request now gets through (2 s sleep, so allow 10 s).
      const after = await optimize(port, {}, 15_000);
      if (after.status !== 200) p.push(`after release, answered ${after.status} (slot leaked?)`);
    } catch (err) { p.push(`threw ${err.message}`); }
    t("U-06 concurrency cap: one run at a time, 429 past it, slot returned", p);
    await stop(child);
  }

  // ---- 3: per-client rate limit ----
  {
    const { port, child } = await startServer({
      GS_ENGINE: realEngine, GS_RATE_LIMIT_PER_MIN: "3",
    });
    const p = [];
    try {
      const codes = [];
      for (let i = 0; i < 5; i += 1) codes.push((await optimize(port)).status);
      const ok = codes.filter((c) => c === 200).length;
      const limited = codes.filter((c) => c === 429).length;
      if (ok !== 3) p.push(`expected exactly 3 accepted then 429, got ${codes.join(",")}`);
      if (limited !== 2) p.push(`expected 2 rate-limited, got ${codes.join(",")}`);
      // Static must stay free: the limit is about engine cost, not page loads.
      const page = await fetch(`http://127.0.0.1:${port}/`);
      if (page.status !== 200) p.push(`GET / after the limit answered ${page.status}`);
      const css = await fetch(`http://127.0.0.1:${port}/style.css`);
      if (css.status !== 200) p.push(`GET /style.css answered ${css.status}`);
    } catch (err) { p.push(`threw ${err.message}`); }
    t("U-06 per-client rate limit bites POST only (429 + Retry-After), static exempt", p);
    await stop(child);
  }

  // ---- 4: the engine-run bound is configurable and honest ----
  {
    const { port, child } = await startServer({
      GS_ENGINE: slowEngine, GS_FAKE_SLEEP: "5", GS_FAKE_GIF: fixtureGif,
      GS_ENGINE_TIMEOUT_MS: "400",
    });
    const p = [];
    try {
      const r = await optimize(port, {}, 20_000);
      if (r.status !== 422) p.push(`status ${r.status}, expected a 422 for a timed-out run`);
      const msg = JSON.stringify(r.body || {});
      if (!/timed out/.test(msg)) p.push(`body does not name the timeout: ${msg}`);
      if (r.body && r.body.exitCode !== 124) p.push(`exitCode ${r.body?.exitCode}, expected 124`);
    } catch (err) { p.push(`threw ${err.message} (the run outlived its bound?)`); }
    t("U-06 engine-run bound: GS_ENGINE_TIMEOUT_MS kills the run and reports 124", p);
    await stop(child);
  }

  // ---- 5b: release/current is honoured by the server too (U-66 / P1-41). The
  // defect was a split policy: the desktop read release/<GS_VERSION>/ while the
  // web server read the newest numeric directory, so a VERSION.md bump moved one
  // surface and not the other. Both now read `current` first. The pin is made by
  // pointing it at the version dir — if a developer already has one, the case
  // reports SKIP rather than clobbering their choice.
  {
    const relRoot = join(PRODUCT, "release");
    const pin = join(relRoot, "current");
    let madePin = false;
    try { await access(pin); } catch {
      await symlink(join(relRoot, "0.1.0"), pin);
      madePin = true;
    }
    const p = [];
    try {
      if (!madePin) {
        console.log("SKIP U-66 release/current pin (a real release/current already exists)");
      } else {
        const { port, child, logOf } = await startServer({ GS_RATE_LIMIT_PER_MIN: "0" });
        try {
          const log = logOf();
          if (!/Engine \[release\/current\]/.test(log)) {
            p.push(`startup line did not choose the pin: ${log.trim().split("\n").pop()}`);
          }
        } finally { await stop(child); }
      }
    } catch (err) { p.push(`threw ${err.message}`); }
    finally { if (madePin) await rm(pin, { force: true }); }
    if (madePin) t("U-66 web server prefers release/current over the newest numeric dir", p);
  }

  // ---- 5: the cap applies to /run too (it is the same engine resource) ----
  {
    const { port, child } = await startServer({
      GS_ENGINE: slowEngine, GS_FAKE_SLEEP: "2", GS_FAKE_GIF: fixtureGif,
      GS_MAX_CONCURRENT: "1", GS_MAX_QUEUED: "0",
    });
    const p = [];
    try {
      const first = runMode(port, { mode: "auto" });
      await new Promise((r) => setTimeout(r, 400));
      const second = await runMode(port, { mode: "auto" });
      if (second.status !== 429) p.push(`second /run answered ${second.status}, expected 429`);
      const ok = await first;
      if (ok.status !== 200 || ok.body?.ok !== true) p.push(`first /run: ${ok.status} ${JSON.stringify(ok.body)}`);
    } catch (err) { p.push(`threw ${err.message}`); }
    t("U-06 the /run endpoint shares the one engine semaphore", p);
    await stop(child);
  }
} finally {
  await rm(dir, { recursive: true, force: true }).catch(() => {});
}

if (fail === 0) {
  console.log(`server-bounds: all cases passed (${pass} groups)`);
  process.exit(0);
}
console.log(`server-bounds: ${fail} FAILED of ${pass + fail}`);
process.exit(1);
