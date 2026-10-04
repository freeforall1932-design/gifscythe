// u57-stale-output.test.mjs — audit U-57 / fix-order P1-37.
//
// Runs with plain `node web/test/u57-stale-output.test.mjs` — zero deps, no
// emcc, no browser. A fake MEMFS + a fake module stand in for Emscripten;
// the behaviour under test is the guard the page now calls
// (web/wasm/fs_run_guard.mjs), exercised through the page's exact fixed
// sequence and — as the RED leg — through its ORIGINAL sequence.

import test from "node:test";
import assert from "node:assert/strict";
import { clearRunArtifacts, readVerifiedGif } from "../wasm/fs_run_guard.mjs";

// Minimal MEMFS stand-in: only the two methods the guard touches.
class FakeFS {
  constructor() { this.store = new Map(); }
  writeFile(path, bytes) { this.store.set(path, Uint8Array.from(bytes)); }
  unlink(path) {
    if (!this.store.has(path)) throw Object.assign(new Error("ENOENT: no such file"), { errno: 2 });
    this.store.delete(path);
  }
  readFile(path) {
    if (!this.store.has(path)) throw Object.assign(new Error("ENOENT: no such file"), { errno: 2 });
    return this.store.get(path);
  }
}

// A GIF89a header plus a few payload bytes; per-run variants let T5 tell
// runs apart byte-for-byte.
const gif89 = (tag = 0x00) =>
  new Uint8Array([0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x01, 0x00, 0x01, 0x00, tag, 0x3b]);
const NOT_A_GIF = new Uint8Array([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]);

// Fake engine module. behavior "write" produces a tagged GIF for the
// current input; behavior "no-write" exits 0 while writing nothing — the
// audit's failing shape (gifsicle exit 0, no output file).
function makeModule() {
  const M = { FS: new FakeFS(), behavior: "write", calls: 0 };
  M.callMain = () => {
    M.calls += 1;
    if (M.behavior === "write") M.FS.writeFile("/out.gif", gif89(M.calls));
    // "no-write": returns without touching the FS, exit code 0.
  };
  return M;
}

// The page's ORIGINAL run sequence (pre-fix): write input, call, read.
function runOnceLegacy(M, inputBytes) {
  M.FS.writeFile("/in.gif", inputBytes);
  M.callMain(["-O3", "-o", "/out.gif", "/in.gif"]);
  return readVerifiedGif(M.FS, "/out.gif");
}

// The page's FIXED run sequence: clear fixed paths BEFORE the call.
function runOnceFixed(M, inputBytes) {
  clearRunArtifacts(M.FS, ["/in.gif", "/out.gif"]);
  M.FS.writeFile("/in.gif", inputBytes);
  M.callMain(["-O3", "-o", "/out.gif", "/in.gif"]);
  return readVerifiedGif(M.FS, "/out.gif");
}

test("T1 RED leg: without the guard, a stale /out.gif is reported as THIS run's success", () => {
  // This replays the audit's claim end to end and asserts the bug is real:
  // if this leg ever fails, the finding (not the fix) needs re-checking.
  const M = makeModule();
  const r1 = runOnceLegacy(M, gif89(0xaa)); // first run succeeds honestly
  assert.equal(r1.ok, true);
  M.behavior = "no-write"; // second run: engine exits 0, writes nothing
  const r2 = runOnceLegacy(M, gif89(0xbb));
  assert.equal(r2.ok, true, "legacy sequence MUST (wrongly) succeed — that is the U-57 bug");
  assert.deepEqual([...r2.bytes], [...r1.bytes], "and it serves run 1's bytes as run 2's product");
});

test("T2: with the guard, the same exit-0/no-write run is refused as missing", () => {
  const M = makeModule();
  const r1 = runOnceFixed(M, gif89(0xaa));
  assert.equal(r1.ok, true);
  M.behavior = "no-write";
  const r2 = runOnceFixed(M, gif89(0xbb));
  assert.equal(r2.ok, false);
  assert.equal(r2.reason, "missing");
});

test("T3: clearRunArtifacts removes both fixed paths and is safe to re-run", () => {
  const M = makeModule();
  M.FS.writeFile("/in.gif", gif89(0x01));
  M.FS.writeFile("/out.gif", gif89(0x02));
  clearRunArtifacts(M.FS, ["/in.gif", "/out.gif"]);
  assert.equal(M.FS.store.has("/in.gif"), false);
  assert.equal(M.FS.store.has("/out.gif"), false);
  // Second clear on absent paths must not throw (ENOENT is the goal state).
  clearRunArtifacts(M.FS, ["/in.gif", "/out.gif"]);
  // A path never created at all is likewise fine.
  clearRunArtifacts(M.FS, ["/never-existed"]);
});

test("T4: readVerifiedGif admission table", () => {
  const FS = new FakeFS();
  assert.deepEqual(readVerifiedGif(FS, "/out.gif"), { ok: false, reason: "missing" });
  FS.writeFile("/out.gif", new Uint8Array(0));
  assert.deepEqual(readVerifiedGif(FS, "/out.gif"), { ok: false, reason: "empty" });
  FS.writeFile("/out.gif", NOT_A_GIF);
  assert.deepEqual(readVerifiedGif(FS, "/out.gif"), { ok: false, reason: "not-gif" });
  const g87 = new Uint8Array([0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x00, 0x3b]); // GIF87a
  FS.writeFile("/out.gif", g87);
  assert.equal(readVerifiedGif(FS, "/out.gif").ok, true);
  FS.writeFile("/out.gif", gif89(0x00));
  assert.equal(readVerifiedGif(FS, "/out.gif").ok, true);
});

test("T5: three consecutive runs each return their own product bytes", () => {
  const M = makeModule();
  const a = runOnceFixed(M, gif89(0x01));
  const b = runOnceFixed(M, gif89(0x02));
  const c = runOnceFixed(M, gif89(0x03));
  assert.equal(a.ok && b.ok && c.ok, true);
  const tags = [a.bytes[10], b.bytes[10], c.bytes[10]];
  assert.deepEqual(tags, [1, 2, 3], "each run serves the bytes IT wrote (callMain #1/#2/#3)");
  assert.notDeepEqual([...b.bytes], [...c.bytes]);
});

console.log("u57-stale-output: T1 RED leg + T2..T5 cases loaded (node:test)");
