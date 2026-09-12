# Web build feasibility — "run Gifscythe on the web"

**Date:** 2026-09-09 · **Status:** reviewed + minimal working POC (`web/`) · **Product:** 0.1.0

> **Superseding note (2026-09-12, S14, owner).** This review's conclusion that `web/` is *not* the
> product path no longer stands: the web build is now a **supported product surface**, a
> **self-hosted** alternative to the `.exe`/portable build (offline-only / no-cloud promise
> unchanged). The option analysis below is kept as the dated record; **Option 3** (server-side
> engine, i.e. what `web/` implements) is the chosen path, and Option 4 (`gifsicle.wasm`) remains
> optional. Plan, split rules and phases: `web/WEB_PLAN_TEMPLATE.md`.

`FEASIBILITY_REVIEW.md` listed a web-tech UI (Tauri/Electron) as an *alternative*
to the Qt6 Widgets GUI, but the web path was never built or reviewed. This
document does that review and ships a working proof-of-concept.

## TL;DR

- The Qt desktop app stays the **1.0.0 product** (per `WORKLIST.md`). The web
  build is additive, not a replacement.
- **Best long-term web target:** compile the gifsicle engine to **WebAssembly**
  and drive it from a small web UI (fully portable, offline, no server).
- **Near-term demo (already built):** `web/` — a zero-dependency Node server
  that runs the engine as a subprocess + a web UI that reuses the *exact same*
  command builder as the desktop app (cross-tested against the C++ CLI).

## The four options

| # | Option | Verdict | Why |
|---|--------|---------|-----|
| 1 | **Qt for WebAssembly** (run the existing Widgets UI in-browser) | ❌ Not recommended | The GUI shells out to `gifsicle.exe` via `QProcess`, and `QProcess`/subprocess spawning does not exist under wasm. The whole engine layer (and `ProcessRunner`) would need a rewrite to in-process calls, which also changes the GPLv2 subprocess boundary. Also loses native file dialogs/DnD semantics. |
| 2 | **Tauri** (web UI + Rust shell) | ⚠️ Viable, but not "web" | Keeps the subprocess boundary (Rust spawns `gifsicle`) and gives a smaller bundle than Qt, but it is still a *desktop* app with a web-tech UI — it does not run in a browser. |
| 3 | **Web UI + server-side engine** (this POC) | ✅ Works today | A browser UI sends the GIF to a small server that runs the bundled engine as a subprocess. Zero new toolchains, same command layer as desktop. Limitation: needs a server, so not portable/offline. |
| 4 | **Client-side WASM engine** (compile gifsicle → wasm) | ✅ Best end-state | Fully portable and offline in the browser, matching the project's "portable, click-and-run" spirit. gifsicle is single-process C (no `fork`), so it ports cleanly. Note: in-process use ends the subprocess separation, so distribution must ship gifsicle's GPLv2 source/license (already done: `COPYING.gifsicle`). |

## Recommendation

1. Ship the Qt desktop app to **1.0.0** first (unchanged plan).
2. Treat **Option 4 (gifsicle.wasm)** as the real web deliverable for a later
   release: emscripten build of the engine + a static web UI (the `command.mjs`
   builder in `web/` is already format-identical to the desktop layer and
   ready to reuse client-side).
3. Keep **Option 3 (`web/`)** as the live demo / CI smoke harness in the
   meantime. It exercises the identical command layer and honest exit codes.

## What `web/` contains (POC, Option 3)

```
web/
  server.mjs            zero-dep Node HTTP server; POST /optimize runs the engine
  command.mjs           JS mirror of src/core/GifsicleCommand.h (buildArgs/shellQuote/toString)
  index.html / app.js / style.css   browser UI: upload → optimize → before/after + download
  validate.mjs          JS mirror of src/core/Validate.h (audit U-30)
  test/command.test.mjs  parity: JS builder    vs the real gifscythe-cli output
  test/validate.test.mjs parity: JS validation vs the real core/Validate.h
  test/transport.test.mjs live-server net for the HTTP transport (U-49/U-50)
```

- **Parity guarantee (command):** `test/command.test.mjs` serializes 16 settings
  fixtures (incl. the `-b`/`-e` shapes added for U-41) to conf files, runs the
  real C++ `gifscythe-cli` in print mode, and asserts the JS `toString()` is
  byte-identical. **17 green** (16 fixtures + a direct argv sanity check).
- **Parity guarantee (validation, added for audit U-30):**
  `test/validate.test.mjs` asserts `validate.mjs` returns exactly the same
  `(field, value, reason)` triples as `src/core/Validate.h` — same set, same
  order, same wording. **23 green.** Out-of-range settings now answer
  **HTTP 422** with an `issues[]` list instead of reaching the engine; e.g.
  `--scale 0x1` exits 0 and silently resizes nothing, so it is caught before
  the run rather than returned as a "successful" unchanged GIF.
- **Same execution model as desktop:** the server builds argv and spawns the
  engine directly (`child_process.spawn`, argv array) — never a shell.
- **Honest failures:** engine exit ≠ 0 returns HTTP 422 with `{ok:false,
  exitCode, stderr, command}`; invalid uploads are rejected, never silently
  "optimized".

## Sandbox constraints observed (2026-09-09)

Building Option 4 (wasm) or verifying the Qt GUI here was blocked by the
environment: only `pypi.org`, `registry.npmjs.org`, and `github.com` are
reachable. Debian apt, the Qt CDN (`download.qt.io`), `repo.anaconda.com`, and
emscripten's toolchain hosts (`storage.googleapis.com`, `nodejs.org`) are all
unreachable. Consequently:

- No `cmake`/Qt6 system packages, and PySide6's PyPI wheels ship only the
  Python-binding headers (no C++ Qt headers, no `moc`) → the Qt GUI could not
  be compiled/linked here; it remains CI-verified.
- No emscripten → `gifsicle.wasm` (Option 4) is documented but not built yet.

## Run it

```bash
cd working_code/gifscythe && ./build.sh          # engine + CLI (once)
node web/server.mjs 8000                          # from the repo root
# open http://localhost:8000  (or the live preview)
node web/test/command.test.mjs                    # JS ⇄ C++ command parity
node web/test/validate.test.mjs                   # JS ⇄ C++ validation parity
node web/test/transport.test.mjs                  # live-server transport net
```
