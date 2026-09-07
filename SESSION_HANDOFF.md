# Session Handoff

**Date:** 2026-09-07 · **Branch:** `arena/01a07959-gifscythe`  
**Product version:** 0.1.0 (unchanged — do not bump to 1.0.0 yet)

## TL;DR for the next session

1. Read **`COMPILED_AUDIT.md`** (master audit: S1+S2 forensic + S3 branch worklist).
2. Run **§6 verification checklists** there and tick with evidence — treat ✅ FIXED
   as *implemented, not blessed* until re-verified (watch for new pits).
3. Then pick remaining work from **§5** / `WORKLIST.md` (Windows CI green → GUI
   tabs/preview/controls → clean portable → only then 1.0.0).
4. **Do not** start WebP/APNG. **Do not** “fix” VP-1…VP-5 (loopcount=0, -O0,
   crop `+` form, gamma sentinel, AUTOMOC).

## Current state

P0 silent-failure fixes and P1 honesty work from the consolidated code reviews
are **implemented** in `working_code/gifscythe/`. A separate branch audit
(`codebase-review-and-optimization-3bbfe` / `WORKLIST_CODE_REVIEW.md`) was
adjudicated and merged into `COMPILED_AUDIT.md`.

| Layer | State |
|-------|--------|
| Engine (gifsicle subprocess) | Builds native; Windows path uses `win32cfg.h`; reports **1.96** |
| Core control layer | Header-only: Settings, Command, SettingsIO, Validate, EngineLocator, ProcessRunner, version.h |
| CLI (`gifscythe-cli`) | Argv exec (no shell), honest exit codes, CWD-independent locator |
| Unit + smoke tests | 74 CHECKs; `test_engine.sh` 5/5; `smoke_cli.sh` 7/7 (sandbox green) |
| Qt6 GUI MVP | Batch default, mode combo, async QProcess, cancel, drag-drop, queue mgmt |
| Packaging | Probes `.exe` + multi GUI paths; ships LICENSE + COPYING.gifsicle |
| CI | Current `build.yml` still the pre-fix workflow on `main` (App cannot push workflow changes). **Proposed rewrite:** `docs/ci/build.yml.proposed` — apply manually or with `workflows` permission; **Windows green not yet confirmed** |
| Version / scope | Still **0.1.0**; WebP/APNG deferred |

### Local evidence already collected (Linux sandbox, no Qt)

```text
./build.sh                  → engine + CLI + unit tests OK
./scripts/test_engine.sh    → 5/5
./scripts/smoke_cli.sh      → 7/7 (missing engine rc=1; spaces; /tmp CWD; warnings)
gifsicle --version          → LCDF Gifsicle 1.96
E2E --run                   → /tmp/gifscythe_demo.gif ~9458 bytes
package_portable.sh         → engine+CLI+licenses (no GUI in sandbox)
```

### Fixed this session (review IDs)

| Area | Closed |
|---|---|
| CLI argv exec + honest exit + `std::filesystem` | U-B1/BR-1, U-B6, U-B12 |
| Engine locator (exe-dir → env → release/<ver>) | U-B2, U-MISS-5 |
| CMake `INTERFACE` core + version.h from VERSION.md | U-B7, U-M1, U-B14 |
| Windows engine uses `win32cfg.h` (not Linux config.h) | U-B11 |
| GUI: Batch default, mode combo, output check, async QProcess, cancel | U-B3–B5, U-MISS-4/9 |
| Queue append/dedupe, remove/clear, drag-drop | U-M2, U-MISS-7/8 |
| Live pane sync + shell_quote | U-M3, U-MISS-3 |
| SettingsIO safe parse, save, warnings, round-trip | U-B10, U-MISS-1/2/16 |
| GifsicleCommand stores Settings by value | U-B13 |
| Packaging probes `.exe` + GUI paths; ships COPYING | U-M6, U-MISS-11/12 |
| build.sh honest GUI dispatch; no silenced errors | U-B9, U-M4/M5 |
| FEASIBILITY table: delay 1/100s, crop `+` form; one-way pane honesty | U-M10, VP-5, U-MISS-14 |
| Root `.gitignore` / `.gitattributes` / LICENSE | U-M9, U-MISS-11 |
| Unit tests expanded + `smoke_cli.sh` | U-MISS-6 |
| CI workflow rewritten (aqtinstall, artifacts, tests) | U-B8 |
| Merged S3 branch audit into `COMPILED_AUDIT.md` | process |

### Still open (before 1.0.0)

1. **Re-verify** all §6 boxes in `COMPILED_AUDIT.md` (esp. GUI §6.B, CI §6.C).
2. Confirm **Windows CI** green with downloadable artifact.
3. Full GUI verification on a real Windows desktop + clean portable folder.
4. XNConvert-style Input / Actions / Output tabs.
5. Before/after preview (debounced, async — do not re-block UI).
6. Expose remaining `GifsicleSettings` controls in the GUI.
7. Output-folder actions, size/count display, optional presets/templates.
8. Only then: bump `VERSION.md` → **1.0.0**.

## Verify locally

```bash
cd working_code/gifscythe
./build.sh                 # engine + CLI + unit tests
./scripts/test_engine.sh
./scripts/smoke_cli.sh
./build.sh --all           # GUI when Qt6 is installed (fails honestly if missing)
./scripts/package_portable.sh
```

Sandbox often has no Qt6; GUI compile is expected on Linux CI / a Qt machine.

## Document map

| Doc | Role |
|-----|------|
| `COMPILED_AUDIT.md` | **Start here** — master findings, fix status, §6 verify, remaining work |
| `WORKLIST.md` | Short task board (checkboxes) |
| `IMPROVEMENT_LOG.md` | Chronological decisions |
| `PROJECT_VISION.md` | Product mission + hard constraints |
| `FEASIBILITY_REVIEW.md` | Architecture + flag mapping (delay/crop corrected) |
| `gifscythe-comprehensive-review.md` / `gifscythe-final-code-review.md` | Prior full audits (historical; superseded as checklist by COMPILED_AUDIT) |
| `working_code/gifscythe/VERSION.md` | Version source of truth → `src/core/version.h` |
| `working_code/gifscythe/README.md` | App build/layout |

## Important constraints

- `reference_code/` is read-only reference material.
- Product work belongs in `working_code/gifscythe/`.
- Do not bump to `1.0.0` before the UI/UX retrofit is complete.
- Do not add WebP/APNG before the GIF UI is stable.
- Keep gifsicle as a **subprocess** (GPL v2-only engine vs GPLv3 UI).
- Do **not** “fix” `--loopcount=0`, `-O0`, crop `+` form, or gamma sentinel
  (verified correct against gifsicle 1.96 source/man page).
- Live CLI pane is **honest one-way** (widgets → quoted command). Two-way
  reverse parse is optional later — do not claim two-way in 1.0 without it.
