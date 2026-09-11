# Session Handoff

**Date:** 2026-09-11 (session S10) · **Branch:** local `main` (not pushed —
the S10 token is read-only) → based on `main` commit `414f5fc` ·
**Product version:** 0.1.0 (do not bump to 1.0.0 yet — owner decision pending)

## TL;DR for the next session

0. **START HERE — `STATUS.md`.** The single status register: one row per
   tracked item, four states (**DONE / PARTIAL / OPEN / UNTRIAGED**), generated
   header that answers *"how much is done?"* in one line. It is **generated**
   by `working_code/gifscythe/scripts/check_docs.sh --emit` — never hand-edit
   the generated block. `COMPILED_AUDIT.md` §5 is the detail behind every
   `U-nn` row; neither replaces the other. As of S10: **72 DONE · 4 PARTIAL ·
   21 OPEN · 0 UNTRIAGED · 97 total.**

1. **What S10 did.** The S10 sandbox had a working apt, so for the first time
   since S7 the whole stack — core, CLI, GUI, offscreen harness, web — was
   compiled AND run in one sandbox. That let S10 close **nine audit findings
   and the untriaged N-03, each with executed proof**:
   - **U-45** — the last **P0-1** gap: both Browse buttons are now members
     locked by `setBusy()`, chooser slots guard `busy_`; T18 proves a mid-run
     folder edit cannot redirect planned outputs. **P0-1 fully closed.**
   - **U-16** — atomic settings persistence: core `save_settings_file` is
     tmp+fsync+rename; GUI save is `QSaveFile`. Unit test 32 + T19.
   - **U-34/U-47 (P1-10)** — `invalidatePreview()` on every schedule/clear/
     cancel/busy; stale+failed previews delete their file; successes sweep
     `preview_*.gif` except the displayed one. T20 (in-flight 360-frame
     preview through clear-queue and Explode switch: zero leaks, no ghost).
   - **U-35** — `setBusy(false)` re-enables Run only via `ensureEngine()`.
   - **U-36** — `guiStateKey` (the third conf parser) deleted; `load_settings`
     collects unknown keys; GUI reads its keys from that map. Test 31.
   - **U-37** — one-time status note when persistence is unavailable.
   - **U-40** — `--strict` (rc=3 on any parse/validation warning) + documented
     warning policy/exit codes in `--help`; smoke 7 → 9 cases.
   - **U-42** — web Scale X % / Scale Y % inputs; asymmetric parity fixture +
     live transport case.
   - **N-03** — screenshots re-shot offscreen (Qt 6.4.2) from the current UI
     and linked from the root README; `docs/screenshots/README.md` rewritten.
   - **R-01** closed by measurement: harness **306 checks, 0 failures** (T1–T20)
     — the new last-measured figure (243 @ S7).

2. **S10 also fixed pre-existing doc drift on fresh `main`:** G6 (docs quoted
   the S9 sandbox's 25/0/5 while a Qt-equipped sandbox measures 27/0/3), G10
   (docs named base `190d030`; origin/main is `414f5fc` after the maintainer's
   PR #12 merge + workflow update), G11 (log was behind the 2026-09-11
   workflow commit). **Lesson kept:** gate numbers are sandbox-relative; a
   session in a different sandbox must re-sync them (G6/G9 fail until it does).

3. **Gates, all run locally this session:** `./build.sh` **238 checks,
   0 failures** · `test_engine.sh` **5/5** · `smoke_cli.sh` **9/9** ·
   `test_package.sh` **9/9** · web **15/15 + 19/19 + 18/18** · offscreen
   harness **306 checks, 0 failures** · `check_docs.sh` **21 passed, 0 failed,
   1 skipped** (G7 skip = declared-pending workflow) · `verify_audit.sh`
   **27 PASS / 0 FAIL / 3 SKIP, exit 0** (skips: E9 declared-pending workflow,
   CI-gated, clean-Windows).

4. **NOT PUSHED.** The S10 token was read-only: everything lives in the local
   clone. **Next push must run `check_docs.sh` first (rule 3)** and then watch
   the CI matrix — GitHub Actions has not compiled any S10 code yet. The
   Windows half of the atomic save (`_commit`/`_fileno`) gets its first
   compilation there, as does everything else.

5. **What remains before 1.0.0** — criterion unchanged (*no Critical/High
   findings open, package-negative tests green, clean-Windows smoke against
   the exact tagged SHA*): release re-cut **U-09** → clean-Windows
   `windeployqt` smoke (C4/D3/D4, `docs/ci/CLEAN_WINDOWS_SMOKE.md`) → desktop
   probes B5/B6/B14 → owner decisions (two-way CLI pane, version 0.2.0 vs
   1.0.0). Open findings left: **U-07** (Windows Unicode APIs), **U-12**
   (UI-thread waits — still unscoped in §6), **U-15** (CMake writes into
   `src/`), **U-17** (explode frame verification), **U-41** (web batch/merge/
   explode — unscoped). PARTIALs: U-10/U-14/U-18.

6. **Direction unchanged:** offline-only; C++17 + Qt6 Widgets through 1.0.0;
   `web/` is a demo/parity harness only (see
   `docs/planning/OFFLINE_BUILD_REVIEW.md`).

7. **Naming policy in force:** *Gifscythe* = product; *gifsicle* = upstream
   engine only. Engine script: `scripts/build_engine.sh` (the
   `build_gifsicle.sh` shim is gone — do not reintroduce it).

## Important constraints

**Read this section before editing anything.** These live here, in
`WORKLIST.md` and in `docs/release/RELEASE_PROCEDURE.md` on purpose — rules
only stick if they are in files a new session reads, not in a conversation.

### The status rules (S9 — these are the ones that make the docs stick)

1. **Every session ends by updating the docs** — `STATUS.md` (via
   `check_docs.sh --emit`), `SESSION_HANDOFF.md` (context for the next
   session), `WORKLIST.md` (what is left), `IMPROVEMENT_LOG.md` (what this
   session did) — **then runs `check_docs.sh` until green.**
2. **A new finding is recorded in the same session it is found:** `UNTRIAGED`
   in `STATUS.md` **and** a pending `- [ ]` line in `WORKLIST.md`. It may not
   wait for a later audit pass. Gate **G12** fails if an `UNTRIAGED` row
   outlives the session that found it.
3. **Before `gh pr create`, and again before `gh pr merge`:** run
   `check_docs.sh`, fix every failure, re-run until green. Do not create or
   merge with a failing doc check, and **do not ask whether to run it**.
4. **Enforced mechanically** by `.githooks/pre-push`. Hooks are not shared by
   git clones, so run `working_code/gifscythe/scripts/bootstrap_hooks.sh` once
   per clone (`build.sh` does it for you). Confirm it is live with
   `git config core.hooksPath` → must print `.githooks`. Gate **G15** fails if
   it is not set, so "the hook exists" is never confused with "the hook is
   live".
5. **Every IMPROVEMENT_LOG entry uses the template** (`Changed / Partial /
   Left / Verified / Not verifiable here / Docs touched`). The **`Not
   verifiable here`** line is **mandatory** and must never be omitted or
   softened — it is the only thing that stops a sandbox-specific green being
   read as a universal one.

### Product constraints (unchanged)

- `reference_code/` is read-only; product work lives in `working_code/gifscythe/`.
- Do not bump to `1.0.0` before the UI/UX gates + owner decision.
- Do not add WebP/APNG before the GIF UI is stable.
- Keep gifsicle as a **subprocess** (GPL v2-only engine vs GPLv3 UI).
- Do **not** "fix" `--loopcount=0`, `-O0`, crop `+` form, or gamma sentinel —
  verified correct.
- Live CLI pane stays honest **one-way** (until the owner decides otherwise).
- Windows exec stays `CreateProcessA` + `win_quote_arg` — never `_spawnvp`/shell.
- Windows CLI/test exes stay `-static`; engine line keeps `-include
  src/win32cfg.h` before `-I.` and never passes `-DVERSION`.
- Keep `.github/workflows/build.yml` and `docs/ci/build.yml.proposed`
  byte-identical (`verify_audit.sh` **E9** / `check_docs.sh` **G7**). One
  declared exception: while `docs/ci/PENDING_WORKFLOW_CHANGE.md` exists the
  copies differ on purpose, because the CI token has no `workflows` scope.
  **Deleting that marker is part of applying the change.**
- Extend `tests/test_gui_offscreen.cpp` with every GUI feature (regression
  net). S10 added T18/T19/T20 — keep that habit.
- Offline-only — no server, no auto-update, no telemetry; `web/` is a demo.
- Language stays C++17/Qt6 through 1.0.0 (see offline review triggers).
- **REMOVED (S7):** the `scripts/build_gifsicle.sh` shim is gone. Do not
  reintroduce it.
- The settings file is core-SettingsIO format — GUI-only keys live in the
  unknown-key map `load_settings` collects (S10: this replaced `guiStateKey`);
  keep unit test 20 (unknown-key tolerance) and test 31 (the map) green.
- The default name template must keep rendering exactly `<name>_opt.gif`
  (audit E4) — harness T1/T16 pin it.
- The harness sets `GS_SETTINGS_PATH` at startup; keep every persistence test
  on its own temp path and restore the scratch value after (T14/T19 do).
- The screenshot capture driver is deliberately OUTSIDE the repo
  (`~/devtools/capture` in the S10 sandbox). Re-shoot recipe:
  `docs/screenshots/README.md`.

## Verification status this session (S10)

Everything marked ✅ was **run in this sandbox**; ⏳ could not be. Quote the
**runtime** counter for test counts, never the `CHECK(` source site count.

| Check | Result |
|---|---|
| `./build.sh` (engine + CLI + unit tests) | ✅ **238 checks, 0 failures** (runtime counter) |
| `scripts/test_engine.sh` | ✅ 5/5 |
| `scripts/smoke_cli.sh` | ✅ **9/9** (S10 added the two `--strict` cases) |
| `scripts/test_package.sh` (packaging negative suite) | ✅ 9/9 |
| `node web/test/command.test.mjs` | ✅ **15/15** (S10 added the asymmetric-scale fixture) |
| `node web/test/validate.test.mjs` | ✅ 19/19 |
| `node web/test/transport.test.mjs` (live server) | ✅ **18/18** (S10 added the U-42 case) |
| GUI offscreen harness (`test_gui_offscreen`) | ✅ **306 checks, 0 failures** — COMPILED AND RUN HERE (T1–T20). New last-measured figure; 243 @ S7 was the previous one |
| `scripts/check_docs.sh` (documentation gate) | ✅ **21 passed, 0 failed, 1 skipped, exit 0** (the skip is G7, the declared-pending workflow change) |
| `scripts/verify_audit.sh` | ✅ **27 PASS / 0 FAIL / 3 SKIP, exit 0** (skips = E9 declared-pending workflow, CI-gated, clean-Windows) |
| `diff .github/workflows/build.yml docs/ci/build.yml.proposed` | ⏳ **differ on purpose** — the doc-gate step cannot be pushed without `workflows` scope; declared in `docs/ci/PENDING_WORKFLOW_CHANGE.md`. E9/G7 SKIP for declared drift, FAIL for undeclared |
| GitHub Actions, S10 changes | ⏳ **not run** — read-only token, nothing pushed. CI verification pending on the next push (rule 3 first) |

**Counts are stated by kind on purpose.** `grep -c 'CHECK('` counts **lines**;
`grep -o 'CHECK(' | wc -l` counts **occurrences** (S10: harness **240**, unit
**223** occurrences); neither equals the **runtime** count (unit **238**,
harness **306**), because loops expand checks. `check_docs.sh` gate **G9**
prints which kind it means and compares like with like.

## Network/toolchain reality of this sandbox (re-check every session)

**This varies between sandboxes — always re-check before trusting an older
section of this file.**

* **S10 sandbox (current):** **full toolchain.** uid 0 with working apt:
  `apt-get install g++ make cmake qt6-base-dev ninja-build` succeeded (g++
  12.2.0, cmake 3.25.1, Qt 6.4.2, ninja); node v20.20.2; python3 3.11;
  DejaVu fonts present (offscreen screenshots render text correctly). So
  **every local gate including the GUI harness was runnable here.** No mingw,
  no wine, no `gh`, and the push token was **read-only**.
* **S10 sandbox quirk worth knowing:** the shell/tooling layer rewrites the
  literal string `/home/user` to `$ARENA_WORKSPACE` inside *file contents*
  written through bash heredocs (shell commands still work because bash
  re-expands it, but CMake/scripts see an undefined variable). Write files
  that must contain workspace paths with the file tool, or pass such paths in
  via `-D`/argv/env at run time. Cost S10 ~20 minutes of confusion.
* **S9 sandbox:** no cmake, no Qt6 (g++ 12.2.0, make, git, node v22.22.3,
  python3). Its numbers (25/0/5) were true there; G6 re-syncs them per sandbox.
* **S8 sandbox:** apt blocked (`Acquire (13: Permission denied)`, uid 1001).
* **S7 sandbox:** apt worked; harness measured 243; `-j4` OOMs, use `-j2`.
* If apt is blocked again, the S5 fallback was `pip install cmake ninja
  PySide6` for the cmake steps; GUI verification then belongs to CI.
* Clone depth: the S10 clone is **full** (no `.git/shallow`), so G10/G11 saw
  complete history and G10 caught the S9→main drift. Shallow clones re-open
  risk **R-02**.

## Document map

| Doc | Role |
|-----|------|
| **`STATUS.md`** | **START HERE** — the single status register. Four states, one row per item, generated header. Roll-up only |
| `COMPILED_AUDIT.md` | The **detail** behind every `U-nn` row: findings, verification marks, §6 fix order, §7 checklist. The `U-nn` rows in `STATUS.md` are generated from its §5 |
| `WORKLIST.md` | Human task board + the status rules + the deferred bucket list |
| `SESSION_HANDOFF.md` | This file — context for the next session |
| `IMPROVEMENT_LOG.md` | Chronological decisions, newest first, one template per entry |
| `PROJECT_VISION.md` | Product mission + hard constraints |
| `FEASIBILITY_REVIEW.md` | Original architecture + gifsicle flag mapping |
| `docs/audit/REMEDIATION_2026-09-10.md` | S8 record — dated snapshot; its gate numbers are S8's |
| `docs/audit/CONSOLIDATED_AUDIT_2026-09-10.md` · `FIX_PICK_2026-09-10.md` · `POST_S7_AUDIT.md` | Dated audit snapshots — excluded from `check_docs.sh` by policy |
| `docs/release/RELEASE_PROCEDURE.md` | How to cut snapshots/releases, incl. the doc gate |
| `docs/planning/OFFLINE_BUILD_REVIEW.md` | Offline feasibility + language choice + phased plan |
| `docs/web/WEB_FEASIBILITY.md` | Web-run review (Option 3 demo exists; Option 4 = future) |
| `docs/ci/README.md` · `docs/ci/PENDING_WORKFLOW_CHANGE.md` · `docs/ci/CLEAN_WINDOWS_SMOKE.md` | CI workflow status, the blocked workflow change, and the C4/D3/D4 clean-Windows checklist |
| `docs/screenshots/README.md` | S10 re-shoot recipe + what each shot shows; the shots are linked from the root README |
| `docs/archive/` | The two dated review snapshots (historical line refs kept) |
| `web/` | Server-side web POC + 3 parity/transport tests (demo only, not the product path) |
| `working_code/gifscythe/VERSION.md` | Version source of truth → `src/core/version.h` |

## Prior-session history (S4/S4b/S5/S6/S7/S8/S9)

Windows engine recipe fixed + Wine-proven; `CreateProcessA` quoting; static
linking; workflow hardening; XNConvert-style UI retrofit (tabs, ~30 controls,
async preview). CI on main green on both jobs (runs #23/#24); binaries banked
on Release `snapshot-2026-09-07`; PR #5 merged (`0ad1ff5`). S5: GUI honesty
fixes (cancel dialog, batch pane truth, validate surfacing) + naming alignment
+ web POC; S6: offline-only direction + language decision; merged via PR #6
(`9643654`, plus maintainer follow-up `c5efe07`). S7: GUI settings persistence,
queue reorder, `{name}` naming templates, `docs/release/RELEASE_PROCEDURE.md`;
harness measured at 243 checks in that sandbox; merged as `8190c08`. S8: audit
remediation in two batches — **31 of the 52 registered findings closed with
executed proof** (both release blockers included), 3 partial (U-10/U-14/U-18),
15 open, U-27 closed earlier by S7, 2 register rows corrected (U-19/U-20);
evidence in `docs/audit/REMEDIATION_2026-09-10.md`; PR #11 merged (`7187cbb`),
then the maintainer applied the pending CI change in `190d030`. S9: the
status-tracking system (`STATUS.md`, `check_docs.sh`, F1/F2 gates, pre-push
hook) + N-01/N-02 drift fixes; merged via PR #12 (`801960c`), then the
maintainer updated the workflow again in `414f5fc` (2026-09-11) — which is the
commit S10 based on, and which made G10/G11 red until S10 re-synced the docs.
