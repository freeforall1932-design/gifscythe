# Gifscythe Versioning

**Product name:** Gifscythe  
**Current version:** 0.1.0

The version number tracks the *released product*, not the reference code. The
release is the portable bundle — if the released app needs more than one file to
run, it is still a single "release" and the version scheme below applies.

## Version scheme

| Version range | Meaning | Trigger |
|---|---|---|
| **0.x.x** (start 0.1.0) | Pre-release / internal. UI/UX retrofit in progress; not yet a finished product. | App is being built; features may be incomplete. |
| **1.0.0 → 1.9.9** | **Finished product** — the UI/UX retrofit task is complete and shipped. GIF fully supported. | Only after the UI/UX task is genuinely done. |
| **2.0.0 → 3.0.0** | Animated **WebP + APNG** support added (the "moving picture" multi-format line). | After v1.x is live; done as a major feature bump. |
| *(future, as needed)* | Extension of the above or new major lines. | Only if more things are needed along the way. |

## Rules
- Bump **patch** (0.x.y → 0.x.y+1) for bug fixes.
- Bump **minor** (0.x.y → 0.x+1.0) for new non-breaking features.
- Bump **major** (0.x → 1.0.0) only when the UI/UX retrofit task is complete and
  the product is a finished, shippable GIF app. Do **not** call it 1.0.0 early.
- **2.0.0** unlocks once GIF is stable and animated WebP + APNG are added.
- Stay in **0.x** until the UI/UX task is finished — never jump to 1.0.0 as a
  placeholder.

## Where the version lives
- **This file** is the human-readable source of truth (`Current version: X.Y.Z`).
- `build.sh` and CMake sync it into `src/core/version.h` (`GS_VERSION`).
- CLI banner, GUI window title, and `QApplication::applicationVersion` read
  `GS_VERSION` — do not hardcode `0.1.0` elsewhere.
- The **engine** binary keeps upstream identity **1.96** (`-DVERSION=1.96`);
  only the release directory name uses the product version
  (`release/0.1.0/gifsicle`).
- `release/` holds portable output per product version.
