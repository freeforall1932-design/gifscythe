# Clean-Windows smoke checklist — gates C4 / D3 / D4

**Purpose:** prove the portable Windows bundle runs on a machine with **no Qt,
no MinGW, no dev tools** — the last unverified promise of the portable vision
(audit §6.D). Status: **OPEN** (nothing here has been executed yet).

**Asset:** `gifscythe-windows.zip`
- Preferred source: GitHub Release `snapshot-2026-09-07` (does not expire).
- Alternate: Actions run artifact (`gifscythe-windows`, retention 14 days).
- Contents: `build/` (static `gifscythe-cli.exe`, tests), `build-win/`
  (`gifscythe.exe` + windeployqt runtime + `gifsicle.exe` staged beside it),
  `release/` (engine exes).

## Procedure

1. **Prepare the machine.** Clean Windows 10/11 VM or PC (or fresh user
   profile): no Qt, no MinGW/MSYS, no Visual Studio, no prior Gifscythe runs.
   Note OS build (`winver`) for the evidence record.
2. **Fetch + verify.** Download `gifscythe-windows.zip`; record its sha256.
   Unzip to `C:\gifscythe-test\` (no spaces; optional second pass later with
   a spaced path to re-probe argv quoting, audit A4/W).
3. **Engine identity (C5 recall).** In `cmd.exe`:
   `cd C:\gifscythe-test\build-win && gifsicle.exe --version`
   → expect first line `LCDF Gifsicle 1.96 (Windows)`.
4. **CLI E2E (C3 recall).** Copy any test `.gif` next to the exe; write
   `test.conf`:
   ```
   mode = auto
   optimize = 3
   input = C:\gifscythe-test\build-win\in.gif
   output = C:\gifscythe-test\build-win\out.gif
   ```
   - `gifscythe-cli.exe test.conf` → prints the gifsicle command line.
   - `gifscythe-cli.exe test.conf --run` → exit 0; `out.gif` exists, opens,
     frame count matches the input (`gifsicle.exe --info out.gif`).
   - Honesty probe: `gifscythe-cli.exe test.conf --run --engine C:\nope\gifsicle.exe`
     → **non-zero exit** + `ERROR: engine not found`.
5. **GUI double-click (D3/D4).** In Explorer, double-click `gifscythe.exe`:
   - **No missing-DLL dialog** (windeployqt runtime complete).
   - Status bar shows the engine path found **beside the exe**.
   - Add the test GIF → Optimize GIF → completes; output written; preview
     pane animates before/after.
   - Tabs (Input/Actions/Output) responsive; command pane updates live.
6. **Record evidence.** Machine/OS build, zip sha256, screenshots or copied
   console output for steps 3–5. Then tick in `COMPILED_AUDIT.md` §6.D:
   C4, D3, D4 (and note D1/D2 re-confirm if desired), plus an
   `IMPROVEMENT_LOG.md` line. Only then is the portable-Windows promise
   fully evidenced.

## Failure triage

| Symptom | Likely cause | Where to look |
|---|---|---|
| Missing-DLL dialog names `Qt6*.dll`/`libstdc++-6.dll` | windeployqt gap or partial unzip | workflow windeployqt step; CMake MINGW static flags |
| `ERROR: engine not found` with engine present | EngineLocator probe order | `src/core/EngineLocator.h` |
| GUI starts, run fails exit≠0 | engine/argv issue | command pane text vs §6.E probes |
| Spaces-path run splits args | quoting regression | `ProcessRunner.h win_quote_arg` + unit test 19 |

**Hard rule:** this checklist evidences; it does not waive. If any step
fails, C4/D3/D4 stay **open** in the audit until re-run green.
