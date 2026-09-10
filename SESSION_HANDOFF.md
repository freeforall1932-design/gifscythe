# Session Handoff

**Date:** 2026-09-10 (session S7) · **Branch:** `arena/s7-settings-persistence` ·
**Product version:** 0.1.0 (do not bump to 1.0.0 yet — owner decision pending)

## TL;DR for the next session

0. **START HERE — `docs/audit/CONSOLIDATED_AUDIT_2026-09-10.md`.** Three
   independent reviews of `8190c08` (GPT 5.6 sol xhigh · Seed 2.1 Pro Preview ·
   this repo's own post-merge review) are merged there into one **44-finding
   register**, each row marked with how it was verified. Trust order set by the
   owner: **GPT > Seed > in-repo**, but where a claim was testable the test
   decided — and it found **two errors in this repo's own audit** (withdrawn in
   that file's §2). **Do not start feature work from this handoff; start from the
   audit's §5 fix order.** Two findings are release-blocking: **U-01** (batch
   auto-naming can overwrite another output *or the user's source file* —
   verified rc=0) and **U-02** (`package_portable.sh` exits 0 with no GUI in the
   folder). Everything else in this handoff is downstream of those.
1. **S7 landed the remaining implementable Phase-1 items** (all offline-review
   §5 Phase-1 work that does not need a clean Windows machine or a real
   desktop): **GUI settings persistence** (the S6 gap / audit row 15),
   **queue reorder** (S3-9 remainder), **free-form `{name}` naming templates**
   (S3-25), and the **release-procedure doc**
   (`docs/release/RELEASE_PROCEDURE.md`). Offscreen harness grew
   **150 → 243 checks** (T14/T15/T16) and is green locally on linux (Qt 6.4).
2. **Hygiene fixes:** the `scripts/build_gifsicle.sh` compatibility shim is
   **removed** (the maintainer updated `.github/workflows/build.yml` to
   `build_engine.sh` in `c5efe07`, which was the shim's only reason to exist),
   `docs/ci/build.yml.proposed` is **byte-identical** with the live workflow
   again, and a dead helper in `SettingsIO::save_settings` is gone. Unit test
   20 now pins the unknown-key tolerance the GUI state keys rely on.
3. **What remains before 1.0.0** — the audit's criterion is *"no Critical/High
   findings open, package-negative tests green, clean-Windows smoke against the
   exact tagged SHA"*:
   close **U-01/U-02/U-03** → re-cut release artifacts from the tagged SHA
   (**U-09**: the banked `gifscythe-windows.zip` predates S7 and its notes pin
   `d3544b1` while main is `8190c08`) → clean-Windows windeployqt smoke
   (C4/D3/D4, `docs/ci/CLEAN_WINDOWS_SMOKE.md`) → desktop probes B5/B6/B14 →
   owner decisions: two-way CLI pane **or** keep one-way forever, and the
   version (0.2.0 for the S7 feature set per the minor-bump rule, or straight
   1.0.0 once the gates are green). Cut releases per
   `docs/release/RELEASE_PROCEDURE.md`.
   *Stale step removed (finding U-27): "push this branch → CI green" is already
   done — runs `34425977060` (main) and `34427315414` (audit PR, linux 1m9s +
   windows 2m48s) are green on both jobs.*
4. **Direction unchanged:** offline-only; C++17 + Qt6 Widgets through 1.0.0;
   `web/` is a demo/parity harness only (see
   `docs/planning/OFFLINE_BUILD_REVIEW.md`).
5. **Naming policy in force:** *Gifscythe* = product; *gifsicle* = upstream
   engine only (bundled binary, `reference_code/gifsicle/`, version 1.96, its
   flags). Engine script: `scripts/build_engine.sh` (shim gone — if any doc
   still says `build_gifsicle.sh`, it is one of the two dated review
   snapshots, which keep historical refs by policy).

## What S7 did (2026-09-10, this branch)

- **Settings persistence** (`src/qtui/SettingsPanel.*`, `src/qtui/MainWindow.*`):
  - `SettingsPanel::readFrom(const gs::Settings&)` — exact inverse of
    `writeInto()`, signal-blocked (no `changed()` storm), syncs dependent
    enabled states; values the GUI cannot represent (e.g. `optimize = -1`,
    disposal 4..7) leave the control at its default.
  - `MainWindow::sessionFilePath()/loadSessionState()/saveSessionState()` —
    core **SettingsIO** `key = value` file (one serializer; CLI-compatible —
    NOT QSettings) at `AppConfigLocation/gifscythe.conf`
    (`%APPDATA%\Gifscythe\` on Windows); **`GS_SETTINGS_PATH`** env override
    (mirrors `GS_ENGINE`; the harness uses it for isolation).
  - Saved in `closeEvent`, loaded in the ctor (first launch = defaults, no
    file written until first close). GUI-only keys **`batch_dir`** and
    **`name_template`** ride in the same file; `SettingsIO` ignores unknown
    keys, so the CLI reads GUI-saved files without warnings (unit test 20 +
    a live CLI E2E both prove it).
  - **Deliberately NOT persisted:** the queue (files move between sessions)
    and the Save-as field (per-run choice — restoring it could silently
    overwrite a stale path). Both decisions are documented in
    `MainWindow.h` and asserted by harness T14.
  - Honesty rules kept: corrupt/partial file → valid keys apply, invalid keys
    warn **in the status bar** (never a pretend-first-launch); failed save →
    warning dialog on close (never silent).
- **Queue reorder** (Input tab `Move Up` / `Move Down`): list rows and
  `inputs_` stay index-aligned; selection follows the moved item; bounds are
  no-ops; disabled while busy; Merge order = queue order and the live pane
  shows it (harness T15, incl. a merge E2E after reordering).
- **Naming templates** (Output tab `Name template`): default
  **`{name}_opt.gif` renders exactly the historical auto-name — audit E4
  behavior is unchanged**; `{name}` = input base name; path separators are
  stripped (a template cannot escape the output folder — `../../evil` →
  `evil.gif`); `.gif` appended if missing; empty render falls back to the
  default. A constant template (no `{name}`) with >1 queued files would
  collide on one output: the summary label warns and `runCommand()`
  **refuses** with an explanatory dialog (harness T16).
- **Harness** (`tests/test_gui_offscreen.cpp`): run-scoped `GS_SETTINGS_PATH`
  isolation (no test touches the real user config dir); T1 extended; new
  T14/T15/T16. **243 checks, 0 failures** locally.
- **Docs:** this handoff, `WORKLIST.md`, `COMPILED_AUDIT.md` (row 15 → DONE;
  S3-9/S3-10 → DONE; §5 items 6/7/7b/10; §6.B counts; §11), root+product
  `README.md`, `PROJECT_VISION.md`, `IMPROVEMENT_LOG.md` (S7 entry), and the
  new `docs/release/RELEASE_PROCEDURE.md`.

## Verification status this session (all run locally, linux + Qt 6.4)

| Check | Result |
|---|---|
| `./build.sh` (engine + CLI + unit tests incl. new #20) | ✅ ALL TESTS PASSED |
| `scripts/test_engine.sh` | ✅ 5/5 |
| `scripts/smoke_cli.sh` | ✅ 7/7 |
| `scripts/verify_audit.sh` | ✅ **21 PASS / 0 FAIL / 2 SKIP** (skips = CI-gated + clean-Windows) |
| GUI offscreen harness (`test_gui_offscreen`) | ✅ **243 checks, 0 failures** (T1–T16) |
| `node web/test/command.test.mjs` (JS ⇄ C++ parity) | ✅ ALL PASSED |
| CLI reads a GUI-saved conf (GUI keys present) | ✅ exit 0, zero warnings |
| `diff .github/workflows/build.yml docs/ci/build.yml.proposed` | ✅ byte-identical |
| Windows CI (linux job too) | ⏳ **must confirm this branch** — the only Windows Qt verification |

## Network/toolchain reality of this sandbox (updated 2026-09-10)

Unlike the S5/S6 sandbox, **Debian apt worked here**: `g++`, `cmake`,
`qt6-base-dev` installed cleanly, so the **Qt GUI + offscreen harness were
compiled and run locally for the first time** (previous sessions had to defer
Qt verification to CI). `-j4` builds OOM this sandbox — use `-j2`. If a future
sandbox blocks apt again, fall back to the S5 recipe: `pip install cmake
ninja PySide6` for the cmake steps; GUI verification then belongs to CI.

## Document map

| Doc | Role |
|-----|------|
| **`docs/audit/CONSOLIDATED_AUDIT_2026-09-10.md`** | **START HERE** — 3 reviews of `8190c08` merged into 44 verified findings (U-01…U-44) + fix order. Trust order: GPT > Seed > in-repo |
| `docs/audit/POST_S7_AUDIT.md` | In-repo post-merge review (source C). Two of its claims are **corrected in place**; read the banner first |
| `docs/release/RELEASE_PROCEDURE.md` | **NEW (S7)** — how to cut snapshots/releases |
| `docs/planning/OFFLINE_BUILD_REVIEW.md` | Offline feasibility + language choice + phased plan (§6 persistence gap → closed by S7) |
| `docs/web/WEB_FEASIBILITY.md` | Web-run review (Option 3 demo exists; Option 4 = future) |
| `docs/ci/CLEAN_WINDOWS_SMOKE.md` | C4/D3/D4 clean-Windows checklist (still OPEN — needs a real clean VM; **blocked by U-09** until artifacts are re-cut) |
| `COMPILED_AUDIT.md` | Master checklist (§6 evidence). Its "21 PASS / 0 FAIL / 2 SKIP" is **not portable** — see finding **U-38** |
| `WORKLIST.md` | Short task board (checkboxes; S7 items ticked) |
| `SESSION_HANDOFF.md` | This file |
| `IMPROVEMENT_LOG.md` | Chronological decisions (S7 on top) |
| `PROJECT_VISION.md` | Product mission + hard constraints |
| `FEASIBILITY_REVIEW.md` | Original architecture + gifsicle flag mapping |
| `working_code/gifscythe/VERSION.md` | Version source of truth → `src/core/version.h` |
| `web/` | Server-side web POC + `command.mjs` parity test (demo) |

## Important constraints (unchanged + new)

- `reference_code/` is read-only; product work lives in `working_code/gifscythe/`.
- Do not bump to `1.0.0` before the UI/UX gates + owner decision.
- Do not add WebP/APNG before the GIF UI is stable.
- Keep gifsicle as a **subprocess** (GPL v2-only engine vs GPLv3 UI).
- Do **not** "fix" `--loopcount=0`, `-O0`, crop `+` form, or gamma sentinel.
- Live CLI pane stays honest **one-way** (until the owner decides otherwise).
- Windows exec stays `CreateProcessA` + `win_quote_arg` — never `_spawnvp`/shell.
- Windows CLI/test exes stay `-static`; engine line keeps `-include src/win32cfg.h`
  before `-I.` and never passes `-DVERSION`.
- Keep `.github/workflows/build.yml` and `docs/ci/build.yml.proposed` byte-identical.
- Extend `test_gui_offscreen.cpp` with every GUI feature (regression net).
- Naming policy (product=Gifscythe, engine=gifsicle); script is `scripts/build_engine.sh`.
- Offline-only — no server, no auto-update, no telemetry; `web/` is a demo.
- Language stays C++17/Qt6 through 1.0.0 (see offline review triggers).
- **REMOVED constraint (S7):** the `scripts/build_gifsicle.sh` shim is gone —
  the workflow now calls `build_engine.sh` directly (maintainer commit
  `c5efe07`). Do not reintroduce it.
- **NEW (S7):** the settings file is core-SettingsIO format — keep GUI-only
  keys (`batch_dir`, `name_template`) tolerant both ways (SettingsIO ignores
  unknown keys; unit test 20 guards this). New GUI state keys go in the same
  "GUI state" section, never into `gs::Settings`.
- **NEW (S7):** the default name template must keep rendering exactly
  `<name>_opt.gif` (audit E4) — harness T1/T16 pin it.
- **NEW (S7):** the harness sets `GS_SETTINGS_PATH` at startup; keep every
  persistence test on its own temp path and restore the scratch value after.

## Prior-session history (S4/S4b/S5/S6)

Windows engine recipe fixed + Wine-proven; `CreateProcessA` quoting; static
linking; workflow hardening; XNConvert-style UI retrofit (tabs, ~30 controls,
async preview). CI on main green on both jobs (runs #23/#24); binaries banked
on Release `snapshot-2026-09-07`; PR #5 merged (`0ad1ff5`). S5: GUI honesty
fixes (cancel dialog, batch pane truth, validate surfacing) + naming
alignment + web POC; S6: offline-only direction + language decision; merged
via PR #6 (`9643654`, plus maintainer follow-up `c5efe07`). The remaining
gates carried forward are C4/D3/D4 + desktop probes B5/B6/B14 (see
`WORKLIST.md`).
