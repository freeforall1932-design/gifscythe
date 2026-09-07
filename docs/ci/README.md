# CI workflow status

## Current state (2026-09-07, session S4)

- The rewritten workflow **has been applied** to `.github/workflows/build.yml`
  (maintainer commit `821a310` — identical to `build.yml.proposed` at that
  point). The old "App lacks `workflows` permission" workaround note is
  historical.
- **GitHub Actions status:** `linux` job **green** (run #18: build + GUI +
  engine tests + smoke + package + artifact). `windows` job **red** at step
  "Build Windows engine (win32cfg.h)" — **root cause fixed locally**
  (gifsicle 1.96 sources `#include <config.h>` unconditionally; the compile
  line needed `-I.`; plus upstream-mingw flags and no `-DVERSION`). Fix
  verified by local mingw-w64 cross-build + Wine execution.
- **S4 hardening (staged on branch `verify/windows-ci-fixes`, push blocked by
  an invalid PAT):**
  - static-linked Windows CLI/test exes (no MinGW runtime DLL dependency),
  - `-G Ninja` for the GUI cmake step (the default Visual Studio generator
    cannot consume aqt's MinGW Qt),
  - native Windows engine+CLI E2E smoke step (C:\ paths, spaces,
    missing-engine exit-1 guard),
  - GUI offscreen harness steps (`test_gui_offscreen`) on linux + windows.

## Keeping the two copies in sync

`build.yml.proposed` is kept **byte-identical** to
`.github/workflows/build.yml` so the recipe survives even when workflow
pushes are blocked. After the S4 branch merges, the proposed copy may be
retired (delete it and this note) or kept as documentation — owner's call.

## Apply manually (only needed if the copies drift)

```bash
cp docs/ci/build.yml.proposed .github/workflows/build.yml
git add .github/workflows/build.yml
git commit -m "ci: sync Gifscythe build workflow"
git push
```
