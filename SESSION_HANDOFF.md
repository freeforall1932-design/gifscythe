# Session Handoff

**Date:** 2026-09-10 (session S9) · **Branch:** `arena/01a08bb3-gifscythe` →
based on `main` commit `190d030` ·
**Product version:** 0.1.0 (do not bump to 1.0.0 yet — owner decision pending)

## TL;DR for the next session

0. **START HERE — `STATUS.md`.** It is the single status register: one row per
   tracked item, four states (**DONE / PARTIAL / OPEN / UNTRIAGED**), and a
   generated header that answers *"of everything this repo knows about, how much
   is done?"* in one line. It is **generated** by
   `working_code/gifscythe/scripts/check_docs.sh --emit` — do not hand-edit the
   generated block.
   The detail behind every `U-nn` row is still `COMPILED_AUDIT.md` §5 (source
   audit, verification mark, `file:line`, before/after). `STATUS.md` is the
   roll-up; the audit doc is the detail. Neither replaces the other.

1. **What S9 built.** A status-tracking system, because this repo recorded
   status in five places and they drifted:
   - **`STATUS.md`** — the register (four states, namespaced IDs, generated
     header).
   - **`working_code/gifscythe/scripts/check_docs.sh`** — emits the register and
     enforces it. `--emit` regenerates; plain mode regenerates to a temp file and
     **diffs** it against the committed one, so a hand-fudged roll-up fails.
     Every expected value is derived from the repo; nothing is hardcoded.
   - **`.githooks/pre-push`** — blocks a push with a red doc gate. Bootstrapped
     by `scripts/bootstrap_hooks.sh`, which `build.sh` now calls (git does not
     copy `.githooks/` on clone).
   - **`verify_audit.sh` gates F1/F2** — the doc gate is part of the one-command
     suite, so it cannot be skipped by forgetting to run a second script.

2. **What S9 found and fixed** (all in `STATUS.md`, `IMPROVEMENT_LOG.md` has the
   evidence):
   - **N-01** — `docs/ci/PENDING_WORKFLOW_CHANGE.md` was still in place although
     the maintainer had already applied that change in **`190d030`**. The two
     workflow copies were byte-identical, so **E9 PASSED**, and the real gate
     number was **24 passed, 0 failed, 4 skipped** — while **eight places in the
     docs still quoted 23 PASS / 0 FAIL / 5 SKIP**. Exactly the drift this system
     exists to catch.
   - **N-02** — `web/README.md` still told users the demo "binds 0.0.0.0"; the
     U-06 fix made it `127.0.0.1` by default. Code fixed, doc not.
   - **N-03 — UNTRIAGED, and it stays that way until someone scopes it:** the
     three PNGs in `docs/screenshots/` claim to show the S7 UI, S8 changed
     `src/qtui/` after they were taken, nothing links to them, and this sandbox
     has no Qt6 to regenerate them.

3. **Gates, all run locally this session:** `./build.sh` **211 checks,
   0 failures** · `test_package.sh` **9/9** · `test_engine.sh` **5/5** ·
   `smoke_cli.sh` **7/7** · web **14/14 + 19/19 + 17/17** ·
   `check_docs.sh` green · `verify_audit.sh` **25 PASS / 0 FAIL / 5 SKIP,
   exit 0** (skips: cmake, Qt6, E9 declared-pending workflow, CI-gated,
   clean-Windows).

4. **Caveat the next session must not lose:** this sandbox has **no cmake and no
   Qt6**, so anything touching `src/qtui/` or `tests/test_gui_offscreen.cpp` is
   **CI-COMPILED ONLY** — written and reviewed here, never compiled here. The
   GUI harness count **243 is the last *measured* figure (S7 sandbox)** and has
   not been re-measured since; the file currently holds **226 `CHECK(` source
   sites**, which is *not* the runtime count (loops expand checks).

5. **What remains before 1.0.0** — the audit's criterion is *"no Critical/High
   findings open, package-negative tests green, clean-Windows smoke against the
   exact tagged SHA"*: re-cut release artifacts from the tagged SHA (**U-09**) →
   clean-Windows `windeployqt` smoke (C4/D3/D4,
   `docs/ci/CLEAN_WINDOWS_SMOKE.md`) → desktop probes B5/B6/B14 → owner
   decisions (two-way CLI pane, version). Cut releases per
   `docs/release/RELEASE_PROCEDURE.md`.

6. **Direction unchanged:** offline-only; C++17 + Qt6 Widgets through 1.0.0;
   `web/` is a demo/parity harness only (see
   `docs/planning/OFFLINE_BUILD_REVIEW.md`).

7. **Naming policy in force:** *Gifscythe* = product; *gifsicle* = upstream
   engine only. Engine script: `scripts/build_engine.sh` (the
   `build_gifsicle.sh` shim is gone — do not reintroduce it).

## Important constraints

**Read this section before editing anything.** These live here, in `WORKLIST.md`
and in `docs/release/RELEASE_PROCEDURE.md` on purpose — rules only stick if they
are in files a new session reads, not in a conversation.

### The status rules (NEW in S9 — these are the ones that make the docs stick)

1. **Every session ends by updating the docs** — `STATUS.md` (via
   `check_docs.sh --emit`), `SESSION_HANDOFF.md` (context for the next session),
   `WORKLIST.md` (what is left), `IMPROVEMENT_LOG.md` (what this session did) —
   **then runs `check_docs.sh` until green.**
2. **A new finding is recorded in the same session it is found:** `UNTRIAGED` in
   `STATUS.md` **and** a pending `- [ ]` line in `WORKLIST.md`. It may not wait
   for a later audit pass. Gate **G12** fails if an `UNTRIAGED` row outlives the
   session that found it.
3. **Before `gh pr create`, and again before `gh pr merge`:** run
   `check_docs.sh`, fix every failure, re-run until green. Do not create or merge
   with a failing doc check, and **do not ask whether to run it**.
4. **Enforced mechanically** by `.githooks/pre-push`. Hooks are not shared by
   git clones, so run `working_code/gifscythe/scripts/bootstrap_hooks.sh` once
   per clone (`build.sh` does it for you). Confirm it is live with
   `git config core.hooksPath` → must print `.githooks`. Gate **G15** fails if
   it is not set, so "the hook exists" is never confused with "the hook is live".
5. **Every IMPROVEMENT_LOG entry uses the template** (`Changed / Partial / Left /
   Verified / Not verifiable here / Docs touched`). The **`Not verifiable here`**
   line is **mandatory** and must never be omitted or softened — it is the only
   thing that stops a sandbox-specific green being read as a universal one.

### Product constraints (unchanged)

- `reference_code/` is read-only; product work lives in `working_code/gifscythe/`.
- Do not bump to `1.0.0` before the UI/UX gates + owner decision.
- Do not add WebP/APNG before the GIF UI is stable.
- Keep gifsicle as a **subprocess** (GPL v2-only engine vs GPLv3 UI).
- Do **not** "fix" `--loopcount=0`, `-O0`, crop `+` form, or gamma sentinel —
  verified correct.
- Live CLI pane stays honest **one-way** (until the owner decides otherwise).
- Windows exec stays `CreateProcessA` + `win_quote_arg` — never `_spawnvp`/shell.
- Windows CLI/test exes stay `-static`; engine line keeps `-include src/win32cfg.h`
  before `-I.` and never passes `-DVERSION`.
- Keep `.github/workflows/build.yml` and `docs/ci/build.yml.proposed`
  byte-identical (`verify_audit.sh` **E9** / `check_docs.sh` **G7**). One
  declared exception: while `docs/ci/PENDING_WORKFLOW_CHANGE.md` exists the
  copies differ on purpose, because the CI token has no `workflows` scope.
  **Deleting that marker is part of applying the change** — S9 found it left
  behind after the change had landed (N-01).
- Extend `tests/test_gui_offscreen.cpp` with every GUI feature (regression net).
- Offline-only — no server, no auto-update, no telemetry; `web/` is a demo.
- Language stays C++17/Qt6 through 1.0.0 (see offline review triggers).
- **REMOVED (S7):** the `scripts/build_gifsicle.sh` shim is gone. Do not
  reintroduce it.
- The settings file is core-SettingsIO format — keep GUI-only keys
  (`batch_dir`, `name_template`) tolerant both ways (unit test 20 guards this).
  New GUI state keys go in the same "GUI state" section, never `gs::Settings`.
- The default name template must keep rendering exactly `<name>_opt.gif`
  (audit E4) — harness T1/T16 pin it.
- The harness sets `GS_SETTINGS_PATH` at startup; keep every persistence test on
  its own temp path and restore the scratch value after.

## Verification status this session (S9)

Everything marked ✅ was **run in this sandbox**; ⏳ could not be (no cmake, no
Qt6). Quote the **runtime** counter for test counts, never the `CHECK(` source
site count.

| Check | Result |
|---|---|
| `./build.sh` (engine + CLI + unit tests) | ✅ **211 checks, 0 failures** (runtime counter) |
| `scripts/test_engine.sh` | ✅ 5/5 |
| `scripts/smoke_cli.sh` | ✅ 7/7 |
| `scripts/test_package.sh` (packaging negative suite) | ✅ 9/9 |
| `node web/test/command.test.mjs` | ✅ 14/14 |
| `node web/test/validate.test.mjs` | ✅ 19/19 |
| `node web/test/transport.test.mjs` (live server) | ✅ 17/17 |
| `scripts/check_docs.sh` (documentation gate) | ✅ green, exit 0 |
| `scripts/verify_audit.sh` | ✅ **25 PASS / 0 FAIL / 5 SKIP, exit 0** (skips = cmake, Qt6, E9 declared-pending workflow, CI-gated, clean-Windows) |
| GUI offscreen harness (`test_gui_offscreen`) | ⏳ **not runnable here** — no cmake/Qt6. **CI-COMPILED ONLY.** Last *measured* runtime count **243** (S7 sandbox); the file holds **226 `CHECK(` source sites** now (T17 added, T8 rewritten in S8) — a source count, not a runtime count |
| `diff .github/workflows/build.yml docs/ci/build.yml.proposed` | ⏳ **differ on purpose** — S9's doc-gate step cannot be pushed (no `workflows` scope); it lives in `docs/ci/build.yml.proposed` + `docs/ci/PENDING_WORKFLOW_CHANGE.md`. E9/G7 SKIP for declared drift, FAIL for undeclared |
| Windows CI / linux CI | ⏳ not re-run this session (no code under `src/` changed) |

**Counts are stated by kind on purpose.** `grep -c 'CHECK('` counts **lines**;
`grep -o 'CHECK(' \| wc -l` counts **occurrences** (unit suite: 193 lines vs
**196 occurrences** — three lines hold two); neither equals the **runtime**
count (unit **211**, harness last measured **243**), because loops expand
checks. `check_docs.sh` gate **G9** prints which kind it means and compares like
with like.

## Network/toolchain reality of this sandbox (re-check every session)

**This varies between sandboxes — always re-check before trusting an older
section of this file.**

* **S9 sandbox (current):** **no cmake, no Qt6.** Present: g++ 12.2.0, make,
  git, node v22.22.3, python3. So **core / CLI / scripts / web / docs gates are
  all verifiable here; `src/qtui/` and `tests/test_gui_offscreen.cpp` are
  CI-only.** Use `reference_code/gifsicle/logo.gif` as the test GIF.
* **S8 sandbox:** `apt-get update` failed with `Acquire (13: Permission denied)`
  as uid 1001; no cmake or Qt6 either.
* **S7 sandbox:** Debian apt worked, so `g++`, `cmake` and `qt6-base-dev`
  installed and the GUI + offscreen harness were compiled and run locally.
  `-j4` OOMs; use `-j2`.
* If apt is blocked again, the S5 fallback was `pip install cmake ninja PySide6`
  for the cmake steps; GUI verification then belongs to CI.
* **The clone is shallow (1 commit).** Anything that reads git history —
  `check_docs.sh` **G11** (log currency) and **G10** (branch/SHA freshness) —
  has only `190d030` to work with. Recorded as risk **R-02**.

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
| `docs/audit/REMEDIATION_2026-09-10.md` | S8 record — what was fixed and the executed proof. **Dated snapshot: its gate numbers are S8's, not current** |
| `docs/audit/CONSOLIDATED_AUDIT_2026-09-10.md` · `FIX_PICK_2026-09-10.md` · `POST_S7_AUDIT.md` | Dated audit snapshots — excluded from `check_docs.sh` by policy |
| `docs/release/RELEASE_PROCEDURE.md` | How to cut snapshots/releases, incl. the doc gate |
| `docs/planning/OFFLINE_BUILD_REVIEW.md` | Offline feasibility + language choice + phased plan |
| `docs/web/WEB_FEASIBILITY.md` | Web-run review (Option 3 demo exists; Option 4 = future) |
| `docs/ci/README.md` · `docs/ci/PENDING_WORKFLOW_CHANGE.md` · `docs/ci/CLEAN_WINDOWS_SMOKE.md` | CI workflow status, the blocked workflow change, and the C4/D3/D4 clean-Windows checklist |
| `docs/archive/` | The two dated review snapshots (moved there by S8, **U-44**). Keep their historical line refs |
| `web/` | Server-side web POC + 3 parity/transport tests (demo only, not the product path) |
| `working_code/gifscythe/VERSION.md` | Version source of truth → `src/core/version.h` |

## Prior-session history (S4/S4b/S5/S6/S7/S8)

Windows engine recipe fixed + Wine-proven; `CreateProcessA` quoting; static
linking; workflow hardening; XNConvert-style UI retrofit (tabs, ~30 controls,
async preview). CI on main green on both jobs (runs #23/#24); binaries banked on
Release `snapshot-2026-09-07`; PR #5 merged (`0ad1ff5`). S5: GUI honesty fixes
(cancel dialog, batch pane truth, validate surfacing) + naming alignment + web
POC; S6: offline-only direction + language decision; merged via PR #6
(`9643654`, plus maintainer follow-up `c5efe07`). S7: GUI settings persistence,
queue reorder, `{name}` naming templates, `docs/release/RELEASE_PROCEDURE.md`;
harness measured at 243 checks in that sandbox; merged as `8190c08`. S8: audit
remediation in two batches — **31 of the 52 registered findings closed with
executed proof** (both release blockers included), 3 partial (U-10/U-14/U-18),
15 open, U-27 closed earlier by S7, 2 register rows corrected (U-19/U-20);
evidence in `docs/audit/REMEDIATION_2026-09-10.md`; PR #11 merged (`7187cbb`),
then the maintainer applied the pending CI change in `190d030`.
