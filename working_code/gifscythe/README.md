# Gifscythe

**Version:** 0.1.0 (see `VERSION.md`)

Gifscythe is the *actual working product* — a portable, click-and-use desktop app
for **animated (moving) images only**: GIF now; APNG and WebP later. It is built
on top of the gifsicle engine with an XNConvert-feel GUI, while retaining the
full gifsicle terminal control underneath.

This folder is the **working source code**. Reference material lives in the repo
root `reference_code/` (read-only).

## Status (2026-09-07, sessions S4 + S4b)
- **0.1.0** — engine + control layer + CLI + GUI MVP.
- `COMPILED_AUDIT.md` §6 was **executed with evidence**: §6.A all green,
  §6.B green via the 81-check offscreen GUI harness, §6.E all green;
  `verify_audit.sh` → 21 PASS / 0 FAIL / 2 SKIP (CI-gated items).
- **Windows path proven under Wine**: engine exe (`1.96 (Windows)`), CLI E2E
  with `C:\` paths + spaces, static-linked exes, honest exit codes. Two
  Windows-only bugs fixed (engine `-I.` recipe; `_spawnvp` space-splitting →
  `CreateProcessA` + `win_quote_arg`).
- Windows CI job rerun + clean-machine windeployqt smoke still pending push
  (see root `SESSION_HANDOFF.md` §0 — provided PAT is invalid).
- **UI/UX retrofit implemented + harness-verified (S4b)**: tabs, ~30 controls,
  async preview, batch folder. Remaining before **1.0.0**: Windows CI green,
  desktop probes, optional naming templates/reorder. WebP/APNG deferred.

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

# Offscreen GUI harness (81 checks; needs Qt6)
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build-cmake
QT_QPA_PLATFORM=offscreen ./build-cmake/test_gui_offscreen

# Packaging
./scripts/package_portable.sh
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

## GUI layout (S4b retrofit)
- **Input** tab — queue with drag-drop, per-file size, count/total label.
- **Actions** tab — every whole-GIF gifsicle control (value lists taken from
  the engine source: dither/resize/color methods, disposal, gamma).
- **Output** tab — Save-as, batch output folder, Open-folder, honest
  per-mode summary of what will be written.
- **Preview** pane — before/after movies of the selected file; the "after"
  is a debounced (1.2 s), fully async single-file re-encode; captions stay
  honest (single-file semantics, Explode refusal, failure reasons).
- Bottom bar — one-way live command pane, progress, run/cancel, status.
- Regression net: `tests/test_gui_offscreen.cpp` (143 checks, runs in CI).

## Versioning
0.1.0 → 1.0.0–1.9.9 (finished GIF product) → 2.0.0–3.0.0 (WebP + APNG).  
**Do not call it 1.0.0 until the UI/UX task is done.**
