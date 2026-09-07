# Gifscythe

**Version:** 0.1.0 (see `VERSION.md`)

Gifscythe is the *actual working product* — a portable, click-and-use desktop app
for **animated (moving) images only**: GIF now; APNG and WebP later. It is built
on top of the gifsicle engine with an XNConvert-feel GUI, while retaining the
full gifsicle terminal control underneath.

This folder is the **working source code**. Reference material lives in the repo
root `reference_code/` (read-only).

## Status (2026-09-07)
- **0.1.0** — engine + control layer + CLI + GUI MVP.
- P0 silent-failure fixes and P1 honesty work are **in tree** (argv exec, honest
  exits, Batch default, EngineLocator, SettingsIO save/validate, win32cfg, etc.).
- Re-verify via root `COMPILED_AUDIT.md` §6 before trusting checkmarks.
- Full UI/UX retrofit (tabs, preview, remaining controls) still pending before
  **1.0.0**. WebP/APNG deferred.

## Build and test
```bash
# Engine + CLI + unit tests (always works without Qt)
./build.sh

# Also build the Qt6 GUI (fails honestly if Qt6 is missing)
./build.sh --all

# Engine pipeline + CLI integration smoke tests
./scripts/test_engine.sh
./scripts/smoke_cli.sh

# Packaging
./scripts/package_portable.sh
./scripts/package_system.sh
```

Cross-compile the Windows engine (needs mingw-w64):
```bash
./scripts/build_gifsicle.sh --windows
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
    qtui/   MainWindow + DropListWidget (Qt6 GUI)
  scripts/  build_gifsicle.sh, test_engine.sh, smoke_cli.sh, package_*.sh
  release/  portable output per version
  tests/    unit tests
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

## Versioning
0.1.0 → 1.0.0–1.9.9 (finished GIF product) → 2.0.0–3.0.0 (WebP + APNG).  
**Do not call it 1.0.0 until the UI/UX task is done.**
