# Session Handoff

**Date:** 2026-09-06 · **To:** next session

## Just run this
```bash
cd working_code/gifscythe
./build.sh          # builds engine + CLI, runs tests. Add --all to also build GUI.
```
That's it. If `build.sh` passes, the project is healthy. Nothing else is required to get oriented.

## What's already built and verified
- **Engine:** `gifsicle` built natively, at `release/0.1.0/gifsicle`. Version 0.1.0.
- **Core (Qt-independent):** `src/core/` — `GifsicleSettings` (every UI control), `GifsicleCommand` (builds argv + live CLI string), `SettingsIO` (load/save settings).
- **CLI driver:** `gifscythe-cli` — prints the exact command (`gifscythe-cli examples/animation.conf`) or runs it (`--run`). This is the "show me the command" feature, proven.
- **Tests:** `scripts/test_engine.sh` + unit tests — all pass.

## Status: merged to main
The P0/P1 work (engine + control layer + CLI driver + Qt GUI scaffold + one-command
build) is **committed and merged**. Just run `./build.sh` to confirm it's healthy.

## The remaining work (in order)
1. **Build the Qt6 GUI** (`src/qtui/` — a minimal `MainWindow` already scaffolded, wired to the core layer). It **can't compile here** — Qt6 isn't installed, apt is offline, Qt mirrors fail SSL. Build it where Qt6 exists: `sudo apt install qt6-base-dev`, then `./build.sh --all`, or `qmake6 gifscythe.pro && make`.
2. **Windows** `gifsicle.exe`: `./scripts/build_gifsicle.sh --windows` (needs mingw-w64).
3. Then **P2/P3** (WebP + APNG + converter) — deferred until the GUI retrofit is done, and only after that bump to **1.0.0**.

## Gotchas (read once, then never again)
- Engine options with optional values need **attached** form: `--lossy=N`, `-O3`, `-j4`, `--loopcount=0`. The core layer already handles this.
- `reference_code/` is read-only; all work is in `working_code/gifscythe/`.
- Version scheme: 0.1.0 → 1.0.0–1.9.9 (finished GIF) → 2.0.0–3.0.0 (WebP+APNG). Don't jump to 1.0.0 early.
