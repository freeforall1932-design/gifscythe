# Gifscythe — wasm track (experimental, unproven, NOT SHIPPABLE)

A client-side alternative to the self-hosted Node web build: the reference
gifsicle engine compiled to WebAssembly with Emscripten, driven
synchronously from the main thread, behind a one-screen page. **Status S19
(2026-09-14): scaffold only — no `.wasm` binary has been built anywhere
yet, and the track is not shippable until owner decision `OD-16` answers
the in-process licence question** (`docs/legal/README.md` §3).
Until both land, the Node server (`web/server.mjs`) stays the shipped web
path; nothing in `web/` outside this directory was touched for this track.

## Design (v1 scope, cut hard)

- Engine: `reference_code/gifsicle` via `emcc`, single-threaded (no
  pthreads — every pthread use in the built sources sits inside
  `#if ENABLE_THREADS`, which the wasm config leaves undefined, and the
  scale path forces one thread on the `#else` branch). No filesystem
  beyond Emscripten's virtual FS (MEMFS).
- Transport: file bytes in, `callMain` on the main thread, bytes out. No
  Worker yet.
- Shared semantics, verbatim: `wasm.js` imports `../command.mjs` and
  `../validate.mjs` unchanged — the live pane shows exactly what runs,
  and out-of-range settings refuse the run exactly like the desktop.
  (`buildArgs` always emits a bare `-j`; the single-threaded engine
  accepts and ignores it — verified in `src/xform.c` of the reference
  tree.)
- One screen (`index.html`): file input, before/after preview, 6 setting
  groups (optimize level, lossy, color count, resize, loop, delay),
  live one-way command pane, result download.
- Deferred to v2: batch/queue, the rest of the settings surface, a
  Worker, settings persistence, retiring the Node server. None of that
  is designed here.

## Build

Prerequisite: Emscripten (`emcc` on `PATH`).

```bash
web/wasm/build_wasm.sh            # -> dist/gifsicle.js + dist/gifsicle.wasm
node web/wasm/prove_wasm.mjs      # byte proof on logo.gif (below)
```

`build_wasm.sh` stages `config.wasm.h` as `config.h` in a temp include
dir (the reference tree stays untouched, same pattern as the native
`scripts/build_engine.sh`), builds the same source list as the native
engine, and stages the `COPYING.gifsicle` engine licence text into the
output dir — every build ships it. Notices: §Third-party notices below
(the standalone notices file was folded into this README in S24).

## Prove (bytes, not a green build)

`prove_wasm.mjs [input.gif]` (default: `reference_code/gifsicle/logo.gif`)
loads the same module factory the page uses, runs `-O3` through the
virtual FS, and prints input bytes, output bytes, the output GIF magic,
and `--info`. It exits non-zero unless the module produced a non-empty
GIF itself.

Native oracle for comparison (measured S19 with the repo-built 1.96 —
a reference number, not this script's verdict): `logo.gif` 8703 B goes
to 8637 B under `-O3` (GIF89a, 12 images, 60x132, loop forever), and to
4106 B under `-O3 --resize-fit 30x66` (30x66).

## Glue harness (the JS, not the wasm binary)

`node web/wasm/glue_harness.mjs` runs `wasm.js` in Node with a stub DOM
and a fake engine module that shells out to the real native gifsicle, so
it needs no emcc output: it proves the live pane renders engine-valid
argv, the virtual-FS write/`callMain`/read flow round-trips bytes, the
GIF magic check and savings/download rendering work, and out-of-range
settings refuse the run. Requires a built engine (`build.sh` first);
prints `GLUE-HARNESS: PASS` and exits 0 on success. It does not promote
the track: the wasm binary itself is still unbuilt and `OD-16` unanswered.

## Serve the page

Any static server rooted at `web/` (the page imports `../command.mjs`,
`../validate.mjs`, and `../style.css` relatively and loads the built
module from its own output dir). `file://` may refuse the module
imports — use a local static server.

**Not `web/server.mjs`.** Since U-67/NF-10 (S21) the Node server serves an
allow-list of exactly the four files the shipped UI loads — `index.html`,
`style.css`, `app.js`, `command.mjs` — because serving the whole `web/` tree
exposed `server.mjs` itself and the test suite. This track is experimental and
not shippable, so `wasm/` is deliberately not on that allow-list and requesting
`/wasm/index.html` from `web/server.mjs` returns **404 by design**. Use any
other static server rooted at `web/` (for example
`python3 -m http.server -d web`), which also supplies the `../validate.mjs`
this page imports and the shipped UI does not.

## Why no binary exists yet (executed S19)

Emscripten cannot be installed in this sandbox: `git clone` of the SDK
works, but `./emsdk install latest` fails downloading its toolchain
from `storage.googleapis.com` (TLS EOF — the host is unreachable from
here; `nodejs.org` is unreachable too). So `build_wasm.sh` is written
and shell-checked but has never run green, `prove_wasm.mjs` has never
run against a real module, and the page has never loaded an engine.
The first emcc-equipped run that prints the proof bytes above is what
promotes this track from scaffold to proven. The licence question
(`OD-16`) is independent of that proof and still gates shippable.

## Third-party notices (folded from THIRD_PARTY_NOTICES.md, S24)

**gifsicle (the engine).** The GIF engine this track compiles to WebAssembly,
Copyright (C) Eddie Kohler. Licence: GNU General Public License, Version 2
ONLY. Full text: staged into the build output directory as `COPYING.gifsicle`
by `build_wasm.sh` (every build ships it); the same text lives at the repo root.
Source: `reference_code/gifsicle/` in this repo, or upstream
https://github.com/kohler/gifsicle.

**Everything else in this track.** The page, the glue script, the build script,
and the proof script are first-party Gifscythe code under the Ms-PL (`LICENSE`,
`COPYING.ms-pl`). No other third-party code ships in this track. The
in-process licence question for this track is tracked in
`docs/legal/README.md` §3 and decided by `OD-16`
(`docs/planning/OWNER_DECISIONS.md`).
