# Gifscythe

**Version:** 0.1.0 (see `VERSION.md`)

Gifscythe is the *actual working product* — a portable, click-and-use desktop app
for **animated (moving) images only**: GIF now; APNG and WebP later. It is built
on top of the gifsicle engine with an XNConvert-feel GUI, while retaining the
full gifsicle terminal control underneath.

This folder is the **working source code**. Reference material lives in the repo
root `reference_code/` (read-only).

Installation and current-branch Qt build instructions are in the repo root:
[`INSTALL.md`](../../INSTALL.md). The desktop verification checklist is
[`TESTING.md`](../../TESTING.md).

## Status (2026-09-10, session S9)
- **Status register:** `../../STATUS.md` — the roll-up of everything this repo
  tracks, in four states. This file is the product-level summary; `STATUS.md` is
  the register and `COMPILED_AUDIT.md` is the per-finding detail.
- **0.1.0** — engine + control layer + CLI + full GUI (S4b retrofit + S7 polish).
- `COMPILED_AUDIT.md` §6 was **executed with evidence** (S4) and **rerun green
  on 2026-09-10 (S7)**: §6.A all green, §6.B green via the offscreen GUI
  harness (**324 checks, T1–T20**, measured in the S11 sandbox; 306 in S10),
  §6.E all green; `verify_audit.sh` → **30 PASS / 0 FAIL / 3 SKIP, exit 0**
  after S22 added the missing web regressions (**W4/W5**). E9 skipped in that run
  because the CI workflow change then awaited a `workflows`-scoped token — a
  blocker that is gone: since S24 E9 enforces byte-equality of the two workflow
  copies with no standing exception, and an agent session can push
  `.github/workflows/` (S18; re-verified S34). F1/F2 are the S9
  documentation gate; C9 is the S11 source-tree-purity gate.
- **Windows path proven under Wine + CI**: engine exe (`1.96 (Windows)`), CLI
  E2E with `C:\` paths + spaces, static-linked exes, honest exit codes; main
  green on both jobs (runs #23/#24), binaries banked on Release
  `snapshot-2026-09-07`.
- **S7 (2026-09-10): GUI settings persistence** (SettingsIO-backed
  `gifscythe.conf`, `GS_SETTINGS_PATH` override), **queue reorder** (Move
  Up/Down), **naming templates** (default `{name}_opt.gif` = the historical
  auto-name; collision runs refused), release-procedure doc
  (`docs/release/RELEASE_PROCEDURE.md`), `build_gifsicle.sh` shim removed
  (workflow now calls `build_engine.sh`).
- Remaining before **1.0.0**: clean-Windows windeployqt smoke (C4/D3/D4 —
  `docs/ci/README.md` §2), desktop probes (B5/B6/B14 — §3), owner
  decisions (two-way CLI pane, version). WebP/APNG deferred.

## Build and test
```bash
# Engine + CLI + unit tests (always works without Qt)
./build.sh

# Also build the Qt6 GUI (fails honestly if Qt6 is missing)
./build.sh --all

# Engine pipeline + CLI integration smoke tests
./scripts/test_engine.sh
./scripts/smoke_cli.sh

# Whole COMPILED_AUDIT §6 regression suite in one command
./scripts/verify_audit.sh

# Packaging NEGATIVE tests — an incomplete package must fail (audit U-02/U-14)
./scripts/test_package.sh

# Offscreen GUI harness (T1–T17; needs Qt6). The check count is printed by the
# harness itself; it is not restated here so it cannot go stale.
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build-cmake
QT_QPA_PLATFORM=offscreen ./build-cmake/test_gui_offscreen

# Packaging
./scripts/package_portable.sh                    # GUI required (fails closed)
./scripts/package_portable.sh --engine-cli-only   # headless package, on purpose
./scripts/package_system.sh                      # GUI required; system Qt runtime
./scripts/package_system.sh --engine-cli-only
# Add --windows when packaging .exe artifacts from a non-Windows host.
```

Both packagers clear their destination, stage privately, and publish only after all
required manifest entries are non-empty. `--engine-cli-only` always excludes GUI,
even if one is built. Portable Windows GUI packaging requires `windeployqt` and
key Qt DLL/plugin entries; real clean-Windows testing is still a release gate.
Linux GUI bundles currently require system Qt6, not a self-contained Qt runtime.

CLI `--run` with an explicit ordinary GIF output verifies a new or size/mtime-
changed non-empty regular file with a GIF87a/GIF89a signature; failures return 1
and name the output. This is not full decoding or transactional rollback.
Byte-identical rewrites within timestamp granularity conservatively fail.
Streaming stdout (`output = -`), frame-selector inputs like `#0`, `--info`, and
Explode keep their existing contracts.


Cross-compile the Windows engine (needs mingw-w64):
```bash
./scripts/build_engine.sh --windows
```

## Layout
```
working_code/gifscythe/
  src/
    core/   Qt-independent engine control layer (header-only):
              GifsicleSettings.h  Settings model
              GifsicleCommand.h   argv builder + shell-quoted toString()
              SettingsIO.h        load/save key=value + warnings
              EngineLocator.h     find gifsicle regardless of CWD
              ProcessRunner.h     argv exec, no shell (CreateProcessW on Windows)
              Validate.h          out-of-range / conflict warnings
              ExplodeVerify.h     explode frame verification (U-17)
              WinUnicode.h        Windows UTF-8 argv/env/path shims (U-07)
              version.h           from VERSION.md (GS_VERSION); committed
                                  fallback synced by build.sh — CMake
                                  generates its own copy in the build tree
                                  from build_support/version.h.in (U-15)
    cli/    main.cpp → gifscythe-cli
    qtui/   MainWindow (tabs + bottom bar) + SettingsPanel (Actions)
            + PreviewPanel (before/after) + DropListWidget (Qt6 GUI)
  scripts/  build_engine.sh, test_engine.sh, smoke_cli.sh, verify_audit.sh,
            test_unit_32bit_long.sh (unit suite on a 4-byte long, via zig),
            libc_parity/ (N-32: --bar = the wasm proof bar, --check = the probe),
            ci_gate.sh (push-vs-PR dedupe decision), lint_workflow.mjs (actionlint),
            package_*.sh
  release/  portable output per version
  tests/    unit tests + test_gui_offscreen.cpp (Qt6 offscreen harness)
  examples/ animation.conf
  VERSION.md
```

## Engine control layer
- A UI widget sets a field on `gs::Settings`.
- `gs::GifsicleCommand(s)` converts it into argv (for exec) and a shell-quoted
  CLI string (for the live pane). Execution never goes through a shell.
- `gifscythe-cli` loads a settings file, prints the command, and `--run`s it.

```bash
./build/gifscythe-cli examples/animation.conf
./build/gifscythe-cli examples/animation.conf --run
GS_ENGINE=/path/to/gifsicle ./build/gifscythe-cli examples/animation.conf --run
```

**Engine selection (GS-207, S17).** CLI `--engine PATH` has priority over
`GS_ENGINE`; otherwise a non-empty `GS_ENGINE` is an exact path relative to CWD,
not a PATH search. An invalid environment override exits 1 in print and run mode
with a named error, never silently selecting a bundled/PATH engine. Empty/unset
preserves automatic discovery. `--run` logs the selected source to stderr, leaving
binary stdout untouched. Explicit `--engine` in print mode still allows a
prospective path that does not yet exist; `--run` requires an executable file.
Windows file format/architecture is checked by process launch; launch failures
never cause a fallback. The shared GUI locator returns no engine for an invalid
environment override; the GUI itself was not re-tested in the S17 non-Qt sandbox.

Default GUI mode is **Batch** (one optimized file per input). **Merge** is an
explicit choice (concatenates animations).

## What the engine can do that this control layer does not model (U-75)

The layer is a *subset* of gifsicle on purpose: every control has to be
reachable from a conf file, the CLI and the GUI with identical argv. The list
below is what a user has to drop to the engine for, and why.

| Engine option | Modelled? | Why not |
|---|---|---|
| `--name TEXT` | no | It names the frame of the *next* input, so it only does something alongside per-frame positionals. `gifsicle.c:785-804` shows the interaction (`-E` vs `-e` also differs **only** when the input already carries name extensions). A single `name` field would be a footgun, so `-E` is offered and `--name` is not — the web checkbox says so in its tooltip, and the GUI checkbox is the same one control. |
| `--use-colormap`, `--transform-colormap`, `--change-color`, `--logical-screen`, `--background <index>` semantics, `--transparent-tolerant`, `--extension`, `--app-extension`, `--delete-extension` | no | colormap/extension surgery has no conf-file vocabulary yet; needs its own settings block rather than an extra string field. |
| `--crop L,T+WxH` with **negative** W/H (extend past the edge) | half | `0` (extend to edge) is modelled — measured accepted by 1.96 — but `Settings::crop_w/h` are `unsigned`, so `-2x-2` is unreachable. Widening the model to `int` touches every surface plus the Qt panel; tracked separately, and hand-written confs get a `negative value rejected` warning rather than a surprise. |
| `--optimize=…` per-frame (`-x`), `--each-extension` | no | per-frame state needs a frame list in the model, i.e. the same missing vocabulary as `--name`. |
| `--no-loopcount` | yes, as `loopcount = -2` | see `GS_LOOPCOUNT_ONCE`; play-once is the ABSENT extension, so it needed a sentinel, not a count. |

Rules of thumb this table follows: the layer never re-interprets what the engine
means (see `examples/animation.conf` for the mapping), and where a value would
be silently dropped it is refused with a warning instead — see `Validate.h`.

## Conf-file details worth knowing (S23)

- **Values are trimmed, so padding is quoted for you.** A `comment` or GUI state
  value with leading/trailing whitespace is written as a quoted string
  (`comment = "  (draft)  "`) and read back exactly; plain values are still
  written plain, so existing confs are untouched (audit DS-12). One documented
  consequence: a hand-written `comment = "hi"` now reads as `hi`.
- **Integers are parsed at their destination width.** A value too large for `int`
  (or for the `unsigned` geometry fields) warns and is left alone instead of
  wrapping — `loopcount = 4294967296` used to become `0` = "forever" (GS-206).
- **Sentinels, not guesses:** `threads = -1` means *say nothing to the engine*
  (single-threaded default) and `threads = 0` means *bare `-j`* (auto, 8);
  `loopcount = -2` is play once (`--no-loopcount`), `-1` unchanged, `0` forever,
  `1..65535` a count. `loopcount = 65536` warns: the engine wraps it to "forever"
  and exits 0.
- **Paths resolve against the conf's directory.** A relative path that only exists
  in the CWD still resolves — but it is named on stderr, and `--strict` refuses it,
  because the same conf must not mean two things depending on where you ran it
  (U-73).
- **`mode = explode` with no `output`** writes `<stem>_frame.NNN` in the CWD and
  says so (U-76). The desktop and web write beside the input; that difference is
  owner decision `OD-18`.
- **Engine discovery** asks the OS where the running binary lives, so a symlinked
  install still finds `gifsicle` next to the real executable (U-65), and prefers
  `release/current` over `release/<version>` in lockstep with the web server (U-66).

## GUI layout (S4b retrofit + S7 polish)
- **Input** tab — queue with drag-drop, per-file size, count/total label,
  **Move Up/Move Down reorder** (merge order = queue order).
- **Actions** tab — every whole-GIF gifsicle control (value lists taken from
  the engine source: dither/resize/color methods, disposal, gamma), with
  plain-language descriptions beside the unfamiliar controls.
- **Output** tab — Save-as, batch output folder, **name template** (default
  `{name}_opt.gif`; `{name}` = input base name; collision runs refused),
  verified files-on-disk list, Open-file/Open-folder actions, and an honest
  per-mode summary. The list is the real destination; the preview is not a
  substitute for writing a file.
- **Guide** tab — an in-app glossary explaining optimization, lossy
  compression, palettes, dithering, resizing, timing, looping, disposal,
  threads, modes and output verification.
- **Preview** pane — separate Before/After canvases (status text never paints
  over a GIF), Play/Stop controls, optional autoplay, and a debounced (1.2 s)
  async settings preview. Automatic re-encoding is paused for GIFs over 32 MB;
  **Preview changes** is available when the user explicitly wants one. After a
  successful run shows the verified file written to disk, avoiding a second
  full encode of a large GIF.
- Bottom bar — one-way live command pane, bounded activity log, determinate
  per-file Batch progress or an honest indeterminate bar for one large engine
  run, run/cancel, and status.
- Windows engine subprocesses are started with `CREATE_NO_WINDOW`/hidden
  startup flags, so gifsicle never opens a second console window beside the
  app. The same headless flag is used by the CLI process runner.
- **Session persistence (S7)** — Actions state + batch folder + name
  template are saved on close to `gifscythe.conf` in the standard app-config
  location (`%APPDATA%\Gifscythe\` on Windows); override the path with
  `GS_SETTINGS_PATH`. The queue and Save-as field are deliberately *not*
  restored. Corrupt files apply their valid keys and warn in the status bar.
- Regression net: `tests/test_gui_offscreen.cpp` — T1–T24 plus the S23 desktop
  round-trip block, 292 `CHECK(` sites in source (which is more runtime
  assertions than sites, because several groups loop). T21 (S31, finding N-10)
  asserts that cancelling an Explode run reports the frame set as possibly
  incomplete; T22–T24 (S36) are the behavioural cases for the three S35 patch
  rows — the U-72 cancel latch (idle cancel arms nothing, a cancelled run's
  completion consumes the latch, the NEXT genuine failure still surfaces), the
  U-70 non-ASCII `GS_ENGINE` path (preview must reach the engine), and the U-58
  mid-batch settings mutation (job 2 runs the batch-start snapshot, proven from
  the fixture's argv log), and T26 (S37) the event-loop heartbeat: a 10 ms timer
  watches for UI-thread stalls across the five blocking waits U-12 names, using
  `tests/fake_engine_orphan_pipe.cpp` — a fixture whose forked child keeps the
  inherited pipes open after the parent is killed, i.e. the worst case a
  kill-first wait can face. On current code the gaps are 0-11 ms against
  1000-5000 ms deadlines (measured, with the target provably alive), so the row's
  freeze is not reachable through these sites; the leg bounds are half a
  deadline, so any future change that starts consuming them fails the harness.
  Last measured at
  **436 runtime checks** in the S37 sandbox (Qt 6.8.3 built from source) —
  figure rises with every added case, so read it as a measurement, not a target;
  324 in the S11 sandbox (Qt 6.4.2) before T21 existed, 306 in S10. Runs in CI
  and in any Qt6-equipped sandbox — and `scripts/build_qt6_local.sh` lets a
  sandbox with no Qt6 and no apt/root build one from source and run the
  harness there too (that is how the S36 figures above were measured).
- Cross-platform engine-probe fixtures: `tests/fake_engine_exit0.cpp` (CMake
  target `fake_engine_exit0`) — exits 0 without writing for T7;
  `tests/fake_engine_partial_failure.cpp` (CMake target
  `fake_engine_partial_failure`) — writes corrupt `-o` bytes then exits 7 for
  the U-59 failure-preservation regression; and
  `tests/fake_engine_argv_sleep.cpp` (CMake target `fake_engine_argv_sleep`) —
  slow and argv-logging for T22/T24: `GS_FAKE_ARGV_LOG` appends one TAB-joined
  argv line per run, `GS_FAKE_SLEEP_MS` (default 800) is the window in which
  Cancel can race a really-running process or a control can be mutated, and it
  copies its input to `-o` so the U-59 verification accepts it like real output.

## Versioning
0.1.0 → 1.0.0–1.9.9 (finished GIF product) → 2.0.0–3.0.0 (WebP + APNG).  
**Do not call it 1.0.0 until the UI/UX task is done.**
