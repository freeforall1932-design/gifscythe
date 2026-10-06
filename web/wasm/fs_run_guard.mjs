// fs_run_guard.mjs — audit U-57 / fix-order P1-37 (E:NA-05).
//
// The Emscripten module behind the wasm page is a SINGLETON: one instance,
// many runs, one virtual FS that survives from run to run. wasm.js used to
// write /in.gif, callMain(), and read /out.gif WITHOUT clearing first — so
// after one successful run, an exit-0/no-write run read the PREVIOUS run's
// bytes and reported success for the wrong input. (Same family the desktop
// closed with U-59 output snapshots and the CLI with the N-15 partial.)
//
// Pure ESM, no DOM, no Emscripten import — Node runs it directly, which is
// how web/test/u57-stale-output.test.mjs proves the guard against a fake
// MEMFS without needing emcc at all.

// Remove every path the run is about to reuse. "Already absent" is the goal
// state, never an error; whatever is readable afterwards is what THIS
// callMain re-created.
export function clearRunArtifacts(FS, paths) {
  for (const p of paths) {
    try {
      FS.unlink(p);
    } catch {
      // ENOENT (or any unlink failure) is safe to ignore: readVerifiedGif
      // re-verifies existence + content of the actual output after the run.
    }
  }
}

// The read-back admission rule — exists, non-empty, and starts with a GIF
// header (the same bar as the server's U-92 upload gate).
// Returns { ok: true, bytes } or { ok: false, reason: "missing" | "empty" | "not-gif" }.
export function readVerifiedGif(FS, outPath) {
  let bytes;
  try {
    bytes = FS.readFile(outPath);
  } catch {
    return { ok: false, reason: "missing" };
  }
  if (!bytes || bytes.length === 0) return { ok: false, reason: "empty" };
  const magic = String.fromCharCode(...bytes.slice(0, 6));
  if (magic !== "GIF87a" && magic !== "GIF89a") return { ok: false, reason: "not-gif" };
  return { ok: true, bytes };
}
