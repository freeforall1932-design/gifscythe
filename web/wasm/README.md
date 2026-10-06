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
node web/wasm/prove_wasm.mjs      # proof on logo.gif; add --oracle PATH for the byte bar (below)
```

`build_wasm.sh` stages `config.wasm.h` as `config.h` in a temp include
dir (the reference tree stays untouched, same pattern as the native
`scripts/build_engine.sh`), builds the same source list as the native
engine, and stages the `COPYING.gifsicle` engine licence text into the
output dir — every build ships it. Notices: §Third-party notices below
(the standalone notices file was folded into this README in S24).

## Prove (bytes, not a green build)

`prove_wasm.mjs [input.gif]` (default: the upstream `logo.gif`, rebuilt from
`working_code/gifscythe/tests/fixtures/logo.gif.b64` — the repo holds no image files)
loads the same module factory the page uses, runs `-O3` through the
virtual FS, and prints input bytes, output bytes, the output GIF magic,
and `--info`. It exits non-zero unless the module produced a non-empty
GIF itself — and, since S34, says plainly that it compared nothing unless
`--oracle PATH` is given (below).

Native oracle for comparison (measured S19 with the repo-built 1.96 —
a reference number, not this script's verdict): `logo.gif` 8703 B goes
to 8637 B under `-O3` (GIF89a, 12 images, 60x132, loop forever), and to
4106 B under `-O3 --resize-fit 30x66` (30x66).

**The bar (S34, N-32 — decided).** A byte comparison with the *glibc* native oracle is not a
bar any wasm build can be held to: gifsicle sorts with libc `qsort` (the median-cut quantizer,
the optimizer) and seeds its dither with libc `random()`, and both behave differently on glibc
than on musl — the libc family of wasi-libc, of the zig/WASI build, and (by inference) of
Emscripten. The bar is therefore a **same-libc native oracle**: the wasm build must be
byte-equal to a *native* build of the same sources against musl.

- **Enforced in CI today, for a zig-built wasm32-wasi engine** (no emcc needed): the
  `portability` job runs `python3 working_code/gifscythe/scripts/libc_parity/libc_parity.py
  --bar` — 9 invocations, byte for byte, under Node's WASI. `--bar --against glibc` (the old
  bar) fails 6 of 9, which is the proof that the bar has teeth.
- **For the Emscripten build** (whoever has `emcc`):
  `python3 working_code/gifscythe/scripts/libc_parity/libc_parity.py --build-oracle /tmp/gs-oracle`
  then `node web/wasm/prove_wasm.mjs --oracle /tmp/gs-oracle/gifsicle-musl`. It runs the same
  `-O3` natively and exits non-zero unless the outputs are byte-equal, naming the first differing
  offset. **That run has never happened** — no Emscripten module exists, so N-32 is PARTIAL;
  the comparison logic itself is covered by `tests/test_prove_wasm_oracle.py` with a fake module.

The measurement behind the decision is reproducible: `libc_parity.py --check` builds the engine
with zig for glibc, musl and wasm32-wasi (the last under Node's WASI) and compares.

## Glue harness (the JS, not the wasm binary)

`node web/wasm/glue_harness.mjs` runs `wasm.js` in Node with a stub DOM
and a fake engine module that shells out to the real native gifsicle, so
it needs no emcc output: it proves the live pane renders engine-valid
argv, the virtual-FS write/`callMain`/read flow round-trips bytes, the
GIF magic check and savings/download rendering work, and out-of-range
settings refuse the run. Requires a built engine (`build.sh` first);
prints `GLUE-HARNESS: PASS` and exits 0 on success.

**Wired into CI 2026-10-07** (linux job, after `build.sh`) — it was the
only wasm-track layer with no CI step, and the glue is where U-57 lived.
It does not promote the track: no Emscripten module exists and `OD-16`
keeps `web/wasm/` experimental, so this proves the JS, not the binary.

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

## What is actually built, and what is not (re-derived 2026-10-07)

**"No `.wasm` has ever been built anywhere" is FALSE, and the distinction matters.** Every CI run
builds a real WebAssembly engine and runs it:

- the `portability` job (`libc_parity.py --bar`) compiles these same engine sources for
  **wasm32-wasi** with zig, **executes the `.wasm`** under Node's WASI, and asserts the output is
  byte-equal to a **musl-native** build of the same sources — 9 invocations, 9/9, green in run
  `37518884447`. `--bar --against glibc` fails 6 of 9, which is the proof that bar has teeth.

What has **never** existed is narrower and specific: the **Emscripten module** that
`build_wasm.sh` produces and `wasm.js` loads — `gifsicle.js` + `gifsicle.wasm` built by `emcc`
(the `-sMODULARIZE` factory `createGifsicle`, MEMFS, `EXIT_RUNTIME=0`). No `web/wasm/dist/`
has ever been produced, so `prove_wasm.mjs --oracle` has never run against a real module and
the page has never loaded an engine. That part of the old note below was right; the blanket
"no `.wasm` ever" claim was not.

**Re-measured 2026-10-07 in the agent sandbox** (the npm route proposed as a lighter path):

| route | result |
|---|---|
| `storage.googleapis.com` (emsdk's toolchain host) | **unreachable**: `curl -I` → HTTP 000; the connection never establishes |
| `emscripten.org` | unreachable (HTTP 000) |
| `github.com` (releases, clone) | reachable (HTTP 200) |
| npm package `emsdk` | exists — 0.4.0, published 2026-02-17, bins for `emcc`/`em++`/`emrun`/`emcmake`, `engines: node >=18`, `os: [darwin, linux]`, ~300 MB toolchain |
| `npm install emsdk` here | fails exactly as predicted: `Downloading from https://storage.googleapis.com/webassembly/emscripten-releases-builds/…` → *"Client network socket disconnected before secure TLS connection was established"* |
| `npm install emsdk@4.0.23` (the npm README's own Quick Start line) | **E404** — the registry has only `0.0.1` and `0.4.0`, and the package then treats its own version (`0.4.0`) as the Emscripten version |

So the "GCS is a different endpoint, so it may not be blocked" hypothesis is **refuted**: it is the
same host the old note blamed, still blocked from here. The selective-install suggestion
(`./emsdk install clang-<ver>-64bit emscripten-<ver>`) belongs to the old git-clone driver and
fetches the same `wasm-binaries.tar.xz` archives from that same bucket — a smaller argument
surface, not a different host. The remaining route that would work is **CI** (GitHub runners have
the network this sandbox lacks); see the candidate below.

**Is the oracle low-value because the C is portable?** Partly — and for a different reason than
the source-purity argument. Measured: `grep -rn '__EMSCRIPTEN__|__wasm__|EMSCRIPTEN'` over
`reference_code/gifsicle/src/*.c`, `src/*.h` and `config.wasm.h` returns **zero** hits, so there
are no Emscripten-specific *code* branches to review. But the N-32 bar already covers
wasm-vs-native byte behaviour, so the residual risk was never in the C source either. It is in
**two layers the zig build does not exercise**: Emscripten's own libc/ABI, and — the one that
actually bit — the **JS glue and module lifecycle**, where U-57's stale `/out.gif` lived
(a singleton `EXIT_RUNTIME=0` module whose virtual FS survived run to run). So skipping the
emcc build is defensible, but the honest reason is **OD-16**: the track cannot ship until
counsel's terms exist, so a second wasm toolchain buys nothing shippable today — not "the C is
pure, so it doesn't matter".

**What IS covered without emcc** (all four run in CI, none needs `emcc`):

| layer | proof | where |
|---|---|---|
| the run guard (fixed FS paths, GIF-magic admission) | `web/test/u57-stale-output.test.mjs` — 5/5, incl. an in-suite RED leg replaying the ORIGINAL bug | linux job |
| **the page glue end to end** (`wasm.js`: settings → argv, FS write/`callMain`/read, magic check, savings + download rendering, refusal path) | `node web/wasm/glue_harness.mjs` — stub DOM + a fake module shelling out to the **real** native engine; measured locally **PASS in 0.4 s** | linux job (added 2026-10-07) |
| the oracle's comparison logic | `tests/test_prove_wasm_oracle.py` — fake module, real native engine | portability job |
| wasm bytes vs same-libc native | `libc_parity.py --bar` — zig wasm32-wasi under Node's WASI | portability job |

**Candidate, not done (owner decision).** A CI job could close the last gap without any local
toolchain: on `ubuntu-24.04`, `git clone https://github.com/emscripten-core/emsdk` (github is
reachable), `./emsdk install <ver> && ./emsdk activate <ver>` with `actions/cache` on the SDK dir,
then `libc_parity.py --build-oracle /tmp/gs-oracle` and `prove_wasm.mjs --oracle
/tmp/gs-oracle/gifsicle-musl`. Cost: ~1 GB cold download, minutes; cache makes it cheap after
that. It is not implemented because nothing about it changes what may ship (`OD-16` keeps
`web/wasm/` experimental and unshipped), and the repo's rule is no unrequested work. If the
track is ever revived toward shipping, this is the first thing to add.
