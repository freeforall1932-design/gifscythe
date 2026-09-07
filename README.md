# Gifscythe — work-in-progress (GIF / APNG / WebP animation tool)

**Current product version:** 0.1.0 (see `working_code/gifscythe/VERSION.md`)  
**Status:** Engine + control layer + CLI + GUI shippable as pre-release.
P0/P1 silent-failure and honesty fixes landed 2026-09-07 and were **verified
with evidence the same day** (audit §6: CLI/unit/engine/smoke green, offscreen
GUI harness green, Windows engine+CLI proven under Wine). The **XNConvert-style
UI retrofit also landed the same day (S4b)**: Input/Actions/Output tabs, ~30
engine-truth controls, debounced async before/after preview, batch output
folder — harness now at **143 checks**. **Windows CI green and merged
2026-09-07** (PR #5 → `0ad1ff5`; main runs #23/#24 green on both jobs; binaries
banked on Release `snapshot-2026-09-07`). Remaining: clean-Windows desktop
probes (C4/D3/D4, B5/B6/B14 — checklist in `docs/ci/CLEAN_WINDOWS_SMOKE.md`)
and the version decision (0.2.0 vs 1.0.0, owner's call). WebP/APNG deferred.

This repository deliberately separates **reference code** from **working code**
so the finished product is never confused with source-material we copied or
fetched.

## Quick start

```bash
cd working_code/gifscythe
./build.sh                  # engine + CLI + unit tests
./scripts/test_engine.sh
./scripts/smoke_cli.sh
./build/gifscythe-cli examples/animation.conf          # print command
./build/gifscythe-cli examples/animation.conf --run    # run engine
./build.sh --all            # also Qt6 GUI (fails honestly if Qt missing)
```

## Repo layout

```
gifscythe/                        (repo root)
  PROJECT_VISION.md               what the product is
  WORKLIST.md                     task board (P0–P3)
  SESSION_HANDOFF.md              notes for the next session
  IMPROVEMENT_LOG.md              decision/change log
  FEASIBILITY_REVIEW.md           architecture + gifsicle flag mapping
  COMPILED_AUDIT.md               master audit checklist (start here for reviews)
  LICENSE / COPYING.gifsicle      license notices (GPLv3 UI intent + GPLv2 engine)
  README.md                       this file

  reference_code/                 SOURCE MATERIAL — do not edit, do not ship
    gifsicle/                     canonical gifsicle 1.96 source
    gifsicle-nested-1.96/         older alternate variant (reference only)
    gifsicle-upstream/            shallow clone (auto-fetched, gitignored)
    caesium-source/               Caesium UI source GPLv3 (auto-fetched, gitignored)
    caesium-bin/                  Caesium Win bundle — portable Qt pattern only (gitignored)

  working_code/                   THE PRODUCT — edit & ship this
    gifscythe/                    the app (v0.1.0)
```

## What is reference vs. working
- **reference_code/** — unmodified source material we reference, adapt, or bundle
  into releases. **Never edit** these; treat them as read-only imports.
- **working_code/** — our actual product. All edits happen here.
- Large auto-fetched trees (`gifsicle-upstream`, `caesium-source`, `caesium-bin`)
  are gitignored; re-fetch or see `reference_code/REFERENCE_MANIFEST.md`.

## Docs for reviewers / next session
1. **`COMPILED_AUDIT.md`** — findings, what was fixed, §6 verify-before-trust.
2. **`SESSION_HANDOFF.md`** — current state + constraints.
3. **`WORKLIST.md`** — short checkbox board toward 1.0.0.

## Versions
The *product* version lives in `working_code/gifscythe/VERSION.md` and is synced
into `src/core/version.h` by `build.sh` / CMake.  
0.1.0 (now) → 1.0.0–1.9.9 (finished GIF product) → 2.0.0–3.0.0 (WebP + APNG).  
**Never call it 1.0.0 until the UI/UX task is done.**

## License note
gifsicle remains a **separate subprocess** (GPL v2-only). The UI/control layer
is intended GPLv3-compatible with the Caesium-derived UX base. See `LICENSE` and
`COPYING.gifsicle`.
