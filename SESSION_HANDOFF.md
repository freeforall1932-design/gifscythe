# Session Handoff

**Date:** 2026-09-07 (session S4 — verification + Windows fixes) ·
**Branch:** `verify/windows-ci-fixes` (local; **push blocked — see §0**) ·
**Product version:** 0.1.0 (unchanged — do not bump to 1.0.0 yet)

## 0. Push/CI status — RESOLVED 2026-09-07 (token #3 worked)

Tokens #1 and #2 were rejected by GitHub (format-valid but expired/revoked).
**Token #3 (provided later the same day) worked**: branch
`verify/windows-ci-fixes` pushed, **PR #5 opened and MERGED** into `main`
(merge commit `0ad1ff5`), and **main run #23 is GREEN on both jobs** with
downloadable artifacts (`gifscythe-windows` 27.8 MB, `gifscythe-linux`
0.7 MB). Lesson kept: generate PATs fresh, copy immediately, grant
Contents/Workflows/Pull-requests R/W.

**Everything sessions S4+S4b produced is MERGED on `main`** (PR #5, merge
`0ad1ff5`): the Windows CI fixes, the GUI harness, the full UI retrofit,
and all doc updates. CI on main: linux ✅ + windows ✅ (run #23).

Next-session gates (audit §6): C4/D3/D4 clean-Windows windeployqt smoke from
the `gifscythe-windows` artifact; desktop probes B5 / B6-plumbing /
B14-engine-missing; then optional polish (naming templates, queue reorder,
release-procedure doc) and the version decision (0.2.0 vs 1.0.0).

## TL;DR for the next session

1. **Clean-Windows smoke from the CI artifact** (audit C4/D3/D4) — step-by-step
   checklist: `docs/ci/CLEAN_WINDOWS_SMOKE.md`. Asset banked on Release
   `snapshot-2026-09-07` (also Actions artifact, 14-day retention): run
   `gifscythe.exe` + `gifscythe-cli.exe` on a machine without Qt/MinGW — no
   missing-DLL dialog, engine found beside the exe.
2. From the artifact (or a real Windows box): clean-machine **windeployqt**
   smoke — GUI double-click finds `gifsicle.exe`, no missing-DLL dialog
   (C4/D3/D4).
3. One-time **real-desktop GUI probes**: B5 (kill engine binary mid-run),
   B6 physical drag-drop onto the queue, B14 engine-missing GUI variant —
   now with the NEW tabbed UI (also click through tabs/preview once).
4. The **P1 GUI retrofit is IMPLEMENTED + HARNESS-VERIFIED (S4b)**: Input/
   Actions/Output tabs, ~30 exposed controls (engine-truth value lists),
   debounced async before/after preview, batch output folder + open-folder,
   queue size/count display. Remaining polish before 1.0.0: naming
   templates (optional), queue reorder (optional), two-way-CLI decision,
   release-procedure doc. Extend `test_gui_offscreen.cpp` (now **143
   checks**) with every future feature.
5. **Do not** start WebP/APNG. **Do not** “fix” VP-1…VP-5 (loopcount=0, -O0,
   crop `+` form, gamma sentinel, AUTOMOC). **Do not** bump to 1.0.0 early
   (owner decides 0.2.0 vs 1.0.0 after CI green + desktop probes).

## What session S4 accomplished (2026-09-07)

The sandbox gained a full toolchain (apt online again): gcc 12.2, cmake 3.25,
**Qt 6.4.2**, **mingw-w64**, **Wine 8**. That unlocked everything the previous
session could not do:

### Verification — `COMPILED_AUDIT.md` §6 executed with evidence

- **§6.A all green** (A1–A12) + new **A13**: Windows unit-test exe under Wine
  → ALL TESTS PASSED. Unit suite + CLI also clean under **ASan+UBSan**.
- **§6.B green via new offscreen harness** `tests/test_gui_offscreen.cpp`
  (**81 checks, 0 failures**): batch N→N with frame counts, merge N→1
  (13=12+1), explode auto-prefix, merge-empty-output refusal, honest failure
  status, cancel mid-run (4800-frame GIF), close-while-running kill, dedupe,
  multi-select remove, live pane sync + quoting, Batch default, engine path
  in status. Remaining manual desktop probes: B5, B6 plumbing, B14 variant.
- **§6.C**: C1 confirmed (Actions linux green, run #18). C6/C7/C8 green —
  including a **new pit found & fixed**: stale GUI binaries made
  `build.sh --all` claim “GUI built” with Qt removed; build.sh now deletes
  stale GUI outputs before probing. C2 fix staged (below), awaits push.
- **§6.D**: D1/D2 green (portable folder now **includes the GUI**);
  **D5 fixed**: `reference_code/caesium-bin` (62 files, 74 MB) **untracked**
  via `git rm --cached` — gitignore/README claims are now true; manifest
  documents re-fetch from Caesium releases.
- **§6.E all green** (E1–E8), incl. explicit-output batch (E1, harness T4)
  and explode semantics (E2, T7).
- One-command rerun: **`scripts/verify_audit.sh`** → 21 PASS / 0 FAIL / 2 SKIP
  (skips = CI-gated + clean-Windows items).

### Windows fixes (the CI run #18 step-4 failure — root-caused & fixed)

1. **Engine recipe** (`scripts/build_gifsicle.sh --windows`): gifsicle 1.96
   sources do *unconditional* `#include <config.h>`; the old line lacked
   `-I.`, so the compile died (`config.h: No such file`). Fixed to mirror
   upstream `src/Makefile.mingw`: `-include src/win32cfg.h` first (defines
   `GIFSICLE_CONFIG_H`, LLP64 sizes), `-I.` (root config.h resolves, guard
   neutralizes it), `-DHAVE_CONFIG_H=1 -DHAVE_UINTPTR_T -DHAVE_INTTYPES_H`,
   and **no `-DVERSION`** (win32cfg.h owns it → engine reports
   `1.96 (Windows)`; the old `-DVERSION` only caused a redefinition warning).
2. **Proven under Wine**: `gifsicle.exe` runs (valid PE x86-64, LLP64
   static_asserts pass), optimizes GIFs correctly; CLI exe E2E with
   `C:\gs\...` conf paths → exit 0 + valid outputs; missing engine → exit 1.
3. **NEW BUG found via Wine E2E**: MinGW `_spawnvp` joins argv **without
   quoting** → any path with a space was split (`C:\...\my vacation\b.gif`
   became two args). `ProcessRunner.h` now builds a properly quoted command
   line (`win_quote_arg`, MSVCRT rules) and calls **CreateProcessA** — still
   never a shell. Unit test 19 covers the quoting edge cases; Wine E2E with
   spaces now passes.
4. **Static linking**: Windows CLI/test exes now link `-static` (KERNEL32 +
   msvcrt only). Dynamic exes die silently on machines without MinGW runtime
   DLLs — that would have broken the portable-artifact promise (C4/D4 class).
   CMakeLists applies this for MINGW builds; workflow updated to match.
5. **Workflow hardening** (`.github/workflows/build.yml` +
   `docs/ci/build.yml.proposed`, kept byte-identical): `-static` flags,
   **Ninja generator** for the GUI cmake step (windows-latest defaults to the
   Visual Studio generator, which cannot consume aqt's MinGW Qt — latent
   landmine), native **Windows engine+CLI E2E smoke step** (C:\ paths,
   spaces, missing-engine exit-1 guard), and **GUI offscreen harness steps**
   on both OSes.

### Docs

- `COMPILED_AUDIT.md` §6 rewritten as an **executed checklist with evidence
  tags** ([L]inux/[W]ine/[H]arness/[CI]); §3/§5/§7/§11 updated.
- `WORKLIST.md`, `docs/ci/README.md`, both READMEs updated.
- `IMPROVEMENT_LOG.md` has the full S4 entry, plus the S4b retrofit entry
  (tabs/controls/preview, harness 81→143, dangling-pointer fix).

## Current state

| Layer | State |
|-------|--------|
| Engine (gifsicle subprocess) | Linux ✅; **Windows recipe fixed + Wine-proven** ✅; CI rerun pending push |
| Core control layer | Header-only, unit-tested on both platforms (Wine), ASan/UBSan clean |
| CLI (`gifscythe-cli`) | ✅ Linux + Windows (CreateProcessA + quoting, static exe) |
| Tests | unit 19 blocks; engine 5/5; smoke 7/7; **GUI offscreen 81 checks**; verify_audit 21 PASS |
| Qt6 GUI | **Retrofit done (S4b):** Input/Actions/Output tabs + ~30 controls + async debounced preview + batch folder; compiles on Qt 6.4.2 via cmake AND qmake; harness 143 checks green |
| Packaging | Portable folder = engine+CLI+**GUI**+licenses (Linux); windeployqt smoke pending C2 artifact |
| CI | linux ✅ green (run #18); windows ❌ step 4 → **fix staged on `verify/windows-ci-fixes`** |
| Version / scope | Still **0.1.0**; WebP/APNG deferred |

## Verify locally (sandbox with Qt6 + mingw + wine)

```bash
cd working_code/gifscythe
./scripts/verify_audit.sh          # whole §6 suite, one command
./scripts/build_gifsicle.sh --windows   # cross-compile (mingw-w64)
QT_QPA_PLATFORM=offscreen ./build-cmake/test_gui_offscreen
# Wine E2E (optional): copy build/*.exe + release/0.1.0/gifsicle.exe into
# ~/.wine/drive_c/gs/, write a conf with C:\gs\... paths, run under wine64.
```

## Document map

| Doc | Role |
|-----|------|
| `COMPILED_AUDIT.md` | **Start here** — §6 now contains executed evidence; §5 = ordered remaining work |
| `WORKLIST.md` | Short task board (checkboxes) |
| `SESSION_HANDOFF.md` | This file |
| `IMPROVEMENT_LOG.md` | Chronological decisions (S4 entry on top) |
| `PROJECT_VISION.md` | Product mission + hard constraints |
| `FEASIBILITY_REVIEW.md` | Architecture + flag mapping (delay/crop corrected) |
| `docs/ci/README.md` | Workflow status + how the pieces fit |
| `working_code/gifscythe/VERSION.md` | Version source of truth → `src/core/version.h` |

## Important constraints (unchanged + new)

- `reference_code/` is read-only reference material (caesium-bin now untracked).
- Product work belongs in `working_code/gifscythe/`.
- Do not bump to `1.0.0` before the UI/UX retrofit is complete.
- Do not add WebP/APNG before the GIF UI is stable.
- Keep gifsicle as a **subprocess** (GPL v2-only engine vs GPLv3 UI).
- Do **not** “fix” `--loopcount=0`, `-O0`, crop `+` form, or gamma sentinel.
- Live CLI pane stays **honest one-way**.
- **NEW:** Windows exec must stay `CreateProcessA` + `win_quote_arg` —
  never revert to `_spawnvp` (space-splitting) or any shell.
- **NEW:** Windows CLI/test exes must stay `-static` (no MinGW DLL deps).
- **NEW:** Windows engine line must keep `-include src/win32cfg.h` **before**
  `-I.` and must not pass `-DVERSION` (see build_gifsicle.sh comments).
- **NEW:** Keep `.github/workflows/build.yml` and `docs/ci/build.yml.proposed`
  byte-identical until the proposed copy is retired.
- **NEW:** Extend `test_gui_offscreen.cpp` when adding GUI features — it is
  the regression net for the §6.B invariants (esp. no UI-thread blocking).
