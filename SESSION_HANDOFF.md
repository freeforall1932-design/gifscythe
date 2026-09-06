# Session Handoff

**Date:** 2026-09-06 · **Branch:** `arena/01a0746d-gifscythe`

## Current state

The GIF engine, CLI control layer, tests, Qt6 GUI MVP, packaging scripts, and
GitHub Actions workflow are present. Linux CI passes. Windows CI currently
fails and needs investigation on the next session; the failure log could not be
retrieved through the GitHub API in this session.

## Verify locally

```bash
cd working_code/gifscythe
./build.sh                 # engine + CLI + tests
./build.sh --all           # also builds GUI when Qt6 is installed
./scripts/package_portable.sh
./scripts/package_system.sh
```

The sandbox has no Qt6, so GUI compilation cannot be verified locally. The
Linux CI runner has Qt6 and successfully built the project.

## What changed this session

- Qt GUI now has an animation queue, optimization controls, lossy control,
  output selection, command preview, status feedback, and engine error handling.
- Added `package_portable.sh` and `package_system.sh`.
- Fixed clean engine builds by creating `release/<version>` automatically.
- Added SettingsIO parsing coverage to the Qt-independent tests.
- Added and merged the GitHub Actions workflow through the web UI.

## Next work, in order

1. Fix the Windows GitHub Actions job and build a Windows Qt GUI.
2. Verify the GUI and portable package on a clean Windows machine.
3. Add Input / Actions / Output tabs, drag-and-drop, preview, progress, and
   queue management.
4. Complete the GIF-only product and release `1.0.0`.
5. Only then implement the common animation frame model, WebP, APNG, and the
   Convert branch.

## Important constraints

- `reference_code/` is read-only reference material.
- Product work belongs in `working_code/gifscythe/`.
- Do not bump to `1.0.0` before the UI/UX retrofit is complete.
- Do not add WebP/APNG before the GIF UI is stable.
