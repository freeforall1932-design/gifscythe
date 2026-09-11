# Gifscythe

**Version:** 0.1.0 (see `VERSION.md`)

Gifscythe is the *actual working product* — a portable, click-and-use desktop app
for **animated (moving) images only**: GIF now; APNG and WebP later. It is built
on top of the gifsicle engine with an XNConvert-feel GUI, while retaining the
full gifsicle terminal control underneath.

This folder is the **working source code**. Reference material lives in the repo
root `reference_code/` (read-only).

## Status (2026-09-10, session S9)
- **Status register:** `../../STATUS.md` — the roll-up of everything this repo
  tracks, in four states. This file is the product-level summary; `STATUS.md` is
  the register and `COMPILED_AUDIT.md` is the per-finding detail.
- **0.1.0** — engine + control layer + CLI + full GUI (S4b retrofit + S7 polish).
- `COMPILED_AUDIT.md` §6 was **executed with evidence** (S4) and **rerun green
  on 2026-09-10 (S7)**: §6.A all green, §6.B green via the offscreen GUI
  harness (**306 checks, T1–T20**, measured in the S10 sandbox; 243 in S7), §6.E all green;
  `verify_audit.sh` → **27 PASS / 0 FAIL / 3 SKIP, exit 0** (E9 SKIPs while the
  CI workflow change awaits a `workflows`-scoped token; F1/F2 are the S9
  documentation gate).
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
  `docs/ci/CLEAN_WINDOWS_SMOKE.md`), desktop probes (B5/B6/B14), owner
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
./scripts/package_system.sh
```

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
              ProcessRunner.h     argv exec, no shell
              Validate.h          out-of-range / conflict warnings
              version.h           from VERSION.md (GS_VERSION)
    cli/    main.cpp → gifscythe-cli
    qtui/   MainWindow (tabs + bottom bar) + SettingsPanel (Actions)
            + PreviewPanel (before/after) + DropListWidget (Qt6 GUI)
  scripts/  build_engine.sh, test_engine.sh, smoke_cli.sh, verify_audit.sh,
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

Default GUI mode is **Batch** (one optimized file per input). **Merge** is an
explicit choice (concatenates animations).

## GUI layout (S4b retrofit + S7 polish)
- **Input** tab — queue with drag-drop, per-file size, count/total label,
  **Move Up/Move Down reorder** (merge order = queue order).
- **Actions** tab — every whole-GIF gifsicle control (value lists taken from
  the engine source: dither/resize/color methods, disposal, gamma).
- **Output** tab — Save-as, batch output folder, **name template** (default
  `{name}_opt.gif`; `{name}` = input base name; collision runs refused),
  Open-folder, honest per-mode summary of what will be written.
- **Preview** pane — before/after movies of the selected file; the "after"
  is a debounced (1.2 s), fully async single-file re-encode; captions stay
  honest (single-file semantics, Explode refusal, failure reasons).
- Bottom bar — one-way live command pane, progress, run/cancel, status.
- **Session persistence (S7)** — Actions state + batch folder + name
  template are saved on close to `gifscythe.conf` in the standard app-config
  location (`%APPDATA%\Gifscythe\` on Windows); override the path with
  `GS_SETTINGS_PATH`. The queue and Save-as field are deliberately *not*
  restored. Corrupt files apply their valid keys and warn in the status bar.
- Regression net: `tests/test_gui_offscreen.cpp` — T1–T20, 240 `CHECK(` sites in
  source; last measured at **306 runtime checks** in the S10 sandbox (Qt 6.4.2);
  243 in the S7 sandbox before that. Runs
  in CI; this sandbox has no Qt6/cmake.

## Versioning
0.1.0 → 1.0.0–1.9.9 (finished GIF product) → 2.0.0–3.0.0 (WebP + APNG).  
**Do not call it 1.0.0 until the UI/UX task is done.**
