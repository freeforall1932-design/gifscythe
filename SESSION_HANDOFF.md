# Session Handoff

**Date:** 2026-09-09 (sessions S5 + S6) · **Branch:** `arena/01a086c5-gifscythe` ·
**Product version:** 0.1.0 (do not bump to 1.0.0 yet)

## TL;DR for the next session

1. **Direction is now explicit: offline-only.** No server, no auto-update, no
   telemetry. The `web/` server build is a **demo / command-parity harness
   only**, not the product path. Decision + rationale:
   `docs/planning/OFFLINE_BUILD_REVIEW.md`.
2. **Language decision: stay on C++17 + Qt6 Widgets through 1.0.0** (only stack
   that is already offline, portable, and CI-verified). Revisit only if a
   trigger fires (bundle < ~15 MB, or UI-must-be-web-tech): then spike
   **Rust + Tauri** (bundling a WebView2 fixed runtime). Full comparison matrix
   in the offline review doc.
3. **S5 landed (this branch, commit `64a713b`):** three GUI honesty fixes
   (cancel no longer pops a spurious error dialog; batch live pane now shows
   the real per-file commands, never a single `-b`; `gs::validate()` warnings
   are surfaced instead of discarded) + regression checks in
   `tests/test_gui_offscreen.cpp` (T2/T9/T13). **The Qt GUI could NOT be
   compiled in-sandbox** (see §Network); the offscreen harness must run on CI.
4. **Naming policy in force:** *Gifscythe* = product; *gifsicle* = upstream
   engine only (bundled binary, `reference_code/gifsicle/`, version 1.96,
   its flags). `scripts/build_gifsicle.sh` renamed → `scripts/build_engine.sh`
   (all live references updated; the two dated review snapshots keep their
   historical line refs). A **`scripts/build_gifsicle.sh` shim remains** only
   because the GitHub App cannot edit `.github/workflows/build.yml` (no
   `workflows` permission) — see §Constraints; remove it once a maintainer
   updates the workflow.
5. **Next actions (ordered):** push this branch → let CI (linux+windows
   offscreen harness) confirm the S5 GUI changes → clean-Windows windeployqt
   smoke (C4/D3/D4) → desktop probes B5/B6/B14 → add **GUI settings
   persistence** (new Phase-1 item; `SettingsIO` already exists, wire it into
   the GUI) → optional polish (naming templates, queue reorder,
   release-procedure doc) → owner's 0.2.0-vs-1.0.0 decision.

## What S5 did (2026-09-09, commit `64a713b`)

- **GUI fixes** (`src/qtui/MainWindow.{h,cpp}`, harness): see TL;DR item 3.
- **Naming alignment**: see TL;DR item 4.
- **Web build reviewed + POC built**: `docs/web/WEB_FEASIBILITY.md` (4-option
  review; best long-term web target = client-side `gifsicle.wasm`) and `web/`
  (zero-dep Node server + browser UI + `command.mjs`, a JS mirror of
  `GifsicleCommand.h`, parity-tested 13/13 against the real C++ CLI).
- **IMPROVEMENT_LOG.md**: new S5 entry.

## What S6 did (2026-09-09, this session)

- **Offline-only feasibility + language review + plan**:
  `docs/planning/OFFLINE_BUILD_REVIEW.md` (verdict: feasible/already true;
  keep C++/Qt; phased plan; self-reviewed). Includes a **gap found**: GUI
  settings are not persisted between sessions — added as a Phase-1 item.
- Updated this handoff, `WORKLIST.md`, `PROJECT_VISION.md`, root `README.md`
  for the offline direction + language decision.

## Verification status this session

| Check | Result |
|---|---|
| `./build.sh` (engine + CLI + unit tests) | ✅ green |
| unit suite (`test_gifsicle_command`) | ✅ ALL TESTS PASSED |
| `scripts/test_engine.sh` | ✅ 5/5 |
| `scripts/smoke_cli.sh` | ✅ 7/7 |
| `scripts/verify_audit.sh` (cmake via pip venv) | ✅ **20 PASS / 0 FAIL / 3 SKIP** (skips = CI-gated + clean-Windows + Qt) |
| `node web/test/command.test.mjs` (JS ⇄ C++ parity) | ✅ 13/13 |
| web server E2E (upload→optimize→download, honest 422) | ✅ verified locally |
| Qt6 GUI compile + offscreen harness | ⚠️ **NOT runnable in-sandbox** (see §Network) — CI must confirm |

## Network/toolchain reality of this sandbox (important for future sessions)

Only `pypi.org`, `registry.npmjs.org`, and `github.com` are reachable. Debian
apt, the Qt CDN (`download.qt.io`), `repo.anaconda.com`, and emscripten hosts
(`storage.googleapis.com`, `nodejs.org`) are all blocked. Consequences:

- No `cmake`/Qt6 system packages; PySide6 wheels ship only Python-binding
  headers (no C++ Qt headers, no `moc`) → the Qt GUI cannot be compiled here.
  Workaround used: `pip install cmake ninja PySide6` in a venv for the audit's
  cmake steps; GUI compilation itself must happen on CI.
- No emscripten → `gifsicle.wasm` (the long-term web target) is documented,
  not built.

## Document map

| Doc | Role |
|-----|------|
| `docs/planning/OFFLINE_BUILD_REVIEW.md` | **NEW — offline feasibility + language choice + plan** |
| `docs/web/WEB_FEASIBILITY.md` | Web-run review (Option 3 demo exists; Option 4 = future) |
| `COMPILED_AUDIT.md` | Master checklist (§6 evidence; 20/0/3 local rerun) |
| `WORKLIST.md` | Short task board (checkboxes) |
| `SESSION_HANDOFF.md` | This file |
| `IMPROVEMENT_LOG.md` | Chronological decisions (S5 on top) |
| `PROJECT_VISION.md` | Product mission + hard constraints |
| `FEASIBILITY_REVIEW.md` | Original architecture + gifsicle flag mapping |
| `working_code/gifscythe/VERSION.md` | Version source of truth → `src/core/version.h` |
| `web/` | Server-side web POC + `command.mjs` parity test (demo) |

## Important constraints (unchanged + new)

- `reference_code/` is read-only; product work lives in `working_code/gifscythe/`.
- Do not bump to `1.0.0` before the UI/UX task is done + CI green + owner decision.
- Do not add WebP/APNG before the GIF UI is stable.
- Keep gifsicle as a **subprocess** (GPL v2-only engine vs GPLv3 UI).
- Do **not** "fix" `--loopcount=0`, `-O0`, crop `+` form, or gamma sentinel.
- Live CLI pane stays honest **one-way**.
- Windows exec stays `CreateProcessA` + `win_quote_arg` — never `_spawnvp`/shell.
- Windows CLI/test exes stay `-static`; engine line keeps `-include src/win32cfg.h`
  before `-I.` and never passes `-DVERSION`.
- Keep `.github/workflows/build.yml` and `docs/ci/build.yml.proposed` byte-identical.
- **The GitHub App cannot edit workflow files** (no `workflows` permission).
  Until a maintainer updates `build.yml` to `scripts/build_engine.sh`, keep the
  `scripts/build_gifsicle.sh` compatibility shim (forwards to build_engine.sh).
- Extend `test_gui_offscreen.cpp` with every GUI feature (regression net).
- **NEW:** naming policy (product=Gifscythe, engine=gifsicle); script is
  `scripts/build_engine.sh`.
- **NEW:** offline-only — no server, no auto-update, no telemetry; `web/` is a demo.
- **NEW:** language stays C++17/Qt6 through 1.0.0 (see offline review triggers).

## Prior-session history (S4/S4b)

Windows engine recipe fixed + Wine-proven; `CreateProcessA` quoting; static
linking; workflow hardening (Ninja, native Windows smoke, GUI offscreen steps
on both OSes); XNConvert-style UI retrofit (tabs, ~30 controls, async preview,
143-check harness). CI on main was green on both jobs (runs #23/#24) and the
`gifscythe-windows` artifact is banked on Release `snapshot-2026-09-07`. The
remaining gates carried forward are C4/D3/D4 + desktop probes (now in
`WORKLIST.md` and the offline review Phase 1).
