# Session Handoff

**Date:** 2026-09-10 (session S7) · **Branch:** `arena/s7-settings-persistence` ·
**Product version:** 0.1.0 (do not bump to 1.0.0 yet — owner decision pending)

## TL;DR for the next session

0. **START HERE — `docs/audit/REMEDIATION_2026-09-10.md` (session S8).** The
   audit remediation has been executed in two batches: **batch 1 closed 21
   findings outright** (3 in part — U-10/U-14/U-18 — and 2 register rows
   corrected, U-19/U-20); **batch 2 closed 10 more** (U-21, U-30, U-31, U-32,
   U-44, U-46, U-49, U-50, U-51, U-52). Every one carries the command output
   that proves it, plus a mutation-test record showing the new tests fail when a
   guard is removed. **Counted from the register:** **31 fixed outright in S8**
   (21 + 10), 3 partial (U-10/U-14/U-18), **15 still open**, 1 fixed earlier by
   S7 (U-27) and 2 register rows corrected (U-19/U-20) — **52 total**.
   **Both release blockers are closed** — U-01 (batch
   auto-naming could overwrite another output *or the source file*) via the new
   Qt-independent `src/core/OutputPlan.h`, and U-02 (packager exiting 0 with no
   GUI) via a fail-closed `package_portable.sh` + `scripts/test_package.sh`.
   Everything below is now **downstream of S8**, not of the raw audit.
   **Gates (all run locally this session):** `./build.sh` **211 checks,
   0 failures** · `test_package.sh` **9/9** · `test_engine.sh` **5/5** ·
   `smoke_cli.sh` **7/7** · web **14/14 + 19/19 + 17/17** ·
   `verify_audit.sh` **24 PASS / 0 FAIL / 4 SKIP, exit 0**.
   **Caveat the next session must not lose:** this sandbox has no cmake and no
   Qt6, so `src/qtui/` and harness **T8/T17** were written and reviewed but
   **never compiled here** — CI is their first compiler. If CI fails, look
   there first.
0b. **The audit register itself is `docs/audit/CONSOLIDATED_AUDIT_2026-09-10.md`.**
   Three independent reviews of `8190c08` (GPT 5.6 sol xhigh · Seed 2.1 Pro
   Preview · this repo's own post-merge review) are merged there into one
   **44-finding register**, each row marked with how it was verified. Trust order
   set by the owner: **GPT > Seed > in-repo**, but where a claim was testable the
   test decided — it found two errors in this repo's own audit (withdrawn in that
   file's §2), and S8 found a third: **U-19 was not reproducible** and is now a
   wording nit. S8 added a fourth such correction: **U-48** (empty comments
   emitting a bare `--comment`) was hiding inside the U-13 row of
   `COMPILED_AUDIT.md` and is fixed too.
   **No release blocker is open any more.** What is left before 1.0.0 is the
   Windows-only work (U-07 ANSI process APIs, U-21 template sanitisation), the
   release re-cut (**U-09** — now from *this* SHA), the clean-Windows smoke, the
   desktop probes, and the owner decisions.
1. **S7 landed the remaining implementable Phase-1 items** (all offline-review
   §5 Phase-1 work that does not need a clean Windows machine or a real
   desktop): **GUI settings persistence** (the S6 gap / audit row 15),
   **queue reorder** (S3-9 remainder), **free-form `{name}` naming templates**
   (S3-25), and the **release-procedure doc**
   (`docs/release/RELEASE_PROCEDURE.md`). Offscreen harness grew
   **150 → 243 checks** (T14/T15/T16) and was green locally on linux (Qt 6.4)
   **in the S7 sandbox** — the S8 sandbox has no Qt6/cmake, so 243 is the last
   *measured* figure and has not been re-measured since.
2. **Hygiene fixes:** the `scripts/build_gifsicle.sh` compatibility shim is
   **removed** (the maintainer updated `.github/workflows/build.yml` to
   `build_engine.sh` in `c5efe07`, which was the shim's only reason to exist),
   `docs/ci/build.yml.proposed` is **byte-identical** with the live workflow
   again, and a dead helper in `SettingsIO::save_settings` is gone. Unit test
   20 now pins the unknown-key tolerance the GUI state keys rely on.
3. **What remains before 1.0.0** — the audit's criterion is *"no Critical/High
   findings open, package-negative tests green, clean-Windows smoke against the
   exact tagged SHA"*:
   **U-01/U-02/U-03 are closed (S8)**, so the remaining chain is: re-cut release
   artifacts from the tagged SHA
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

## What S8 did (2026-09-10, this branch)

Full per-finding evidence, before/after command output and the mutation record
live in **`docs/audit/REMEDIATION_2026-09-10.md`**. This is the short version.

- **New Qt-independent core headers** (all unit-tested here):
  - **`src/core/OutputPlan.h`** — `gs::plan_outputs(inputs, outputs)` computes
    every source/target pair *before* the first process starts and refuses a
    target that is any queued input, two inputs mapping to one target, or an
    empty target. Paths go through `weakly_canonical`, so `./a.gif`, `a.gif`
    and an absolute path to the same file compare equal. Closes **U-01**, the
    worst finding in the register: batch auto-naming could silently overwrite
    another output or the user's own source GIF and still exit 0.
  - **`src/core/OutputName.h`** — `sanitize_output_name()`,
    `is_windows_reserved_device_name()` and a `NameRules` enum
    (`Host`/`Windows`/`Posix`). `MainWindow::renderedOutputName()` delegates to
    it. **Parameterising the rule set is what made U-21 verifiable on Linux** —
    the Windows rules are unit-tested here instead of being an untested claim.
    Separators are stripped in both flavours on every platform (that preserves
    the GUI's existing POSIX behaviour); only the character filter, the
    trailing-dot trim and the reserved-name defusing are Windows-conditional.
- **Engine truth pinned by probing the bundled 1.96, not by reading docs:**
  `--resize-fit/--resize/--resize-touch 0x0` → rc=1, but `40x0` and `0x40` →
  rc=0; `--resize-width 0` → rc=1; `--scale 0x0` → rc=1 while **`--scale 0x1` →
  rc=0 and silently resizes nothing** (its output is byte-identical to
  `--scale 1x1`). `Validate.h` now covers all of it (**U-22**), and the web
  demo refuses the silent case rather than returning an unchanged GIF.
- **Threads "Auto" actually threads** (**U-03**): `threads == 0` now emits a
  bare `-j` (gifsicle's real auto = 8 threads, `gifsicle.c:1887-1893`);
  `threads == -1` emits no flag. The JS mirror moved in lockstep — changing one
  without the other breaks `web/test/command.test.mjs`, which is the point.
- **Packaging fails closed** (**U-02/U-08**): `package_portable.sh` requires the
  engine, CLI, GUI and the full license set, uses a fresh staging dir, and
  supports `--engine-cli-only` for headless CI. New
  **`scripts/test_package.sh`** (9 negative cases) asserts an *incomplete*
  package actually fails.
- **Settings serializer hardened** (**U-51**): `encode_line_value()` applied at
  all 9 string write sites, mirrored into `web/command.mjs`. Before the fix a
  comment containing a newline plus `mode = merge` was written verbatim and
  **hijacked the mode on reload** — reproduced, then guarded by unit test 30.
  *Known remaining limit:* leading/trailing whitespace in a value is still
  lost; fixing that needs a quoted format that would break every existing
  `.conf`, which is not a silent trade to make.
- **Process exit codes** (**U-32**): `run_argv` returns `128 + WTERMSIG` for a
  signalled child instead of `1`, so "cancelled" is no longer indistinguishable
  from "the program failed".
- **Web demo** (still a demo, not the product path): new **`web/validate.mjs`**
  mirrors `Validate.h` and answers **422 + `issues[]`** (**U-30**); double
  `decodeURIComponent` removed (**U-49**); `X-Gifscythe-Command`
  percent-encoded (**U-50**); `requestGen` counter and `beforeUrl` revoke in
  `app.js` (**U-46/U-52**); loopback-by-default binding (**U-06**).
- **Three web suites now gate the JS copies** — `command.test.mjs` (14),
  `validate.test.mjs` (19) and `transport.test.mjs` (17, which starts the real
  server and pushes `%`, `%20`, `%22`, `%2540`, plus signs, CJK, emoji and
  embedded newlines through the actual HTTP path). They are wired into
  `verify_audit.sh` as gates **W1/W2/W3** and into the CI linux job, which
  previously ran **none** of them.
- **Two things S8 broke and then fixed**, both recorded because they are the
  kind of thing that gets re-broken:
  - A comment added to `ProcessRunner.h` — "(no /bin/sh, no cmd.exe)." —
    tripped `verify_audit.sh` **E3**, turning a green run into *22 passed,
    1 failed*. E3 is line-based and the exemption words were on the previous
    line. E3 now exempts `//`/`*` comment lines, mutation-tested in four
    directions so it still catches a real `system()`, a real `/bin/sh` literal
    and a trailing `// spawns sh -c`.
  - The first `transport.test.mjs` harness **crashed** (`URIError: URI
    malformed`) instead of failing one case, so it reported "1 FAIL" where the
    real answer was 6. Its decoder now falls back to the raw header and each
    case is individually wrapped.
- **Deliberate scope decision (U-14).** The whole `verify_audit.sh` was *not*
  put into CI: it duplicates 4 steps already there, doubles job time and risks
  runner-specific breakage. Instead the linux job gained
  `./scripts/test_package.sh` plus a manifest assertion over
  `release/$version/Gifscythe/{gifsicle,gifscythe-cli,gifscythe,LICENSE,COPYING.gifsicle,README.txt}`.
  `verify_audit.sh` stays the local one-command runner, and its headline number
  is now reproducible (**U-38** fixed: a missing toolchain is a SKIP, not a
  FAIL).
- **Docs:** `docs/archive/` created and the two dated review snapshots moved
  there (**U-44**); register rows re-marked; README/WORKLIST/PROJECT_VISION/
  IMPROVEMENT_LOG/RELEASE_PROCEDURE/CI-docs brought back in line with the
  actual gate numbers.

## What S7 did (2026-09-10, this branch)

- **Settings persistence** (`src/qtui/SettingsPanel.*`, `src/qtui/MainWindow.*`):
  - `SettingsPanel::readFrom(const gs::Settings&)` — restores every control
    `writeInto()` reads, signal-blocked (no `changed()` storm), syncs dependent
    enabled states; values the GUI cannot represent (e.g. `optimize = -1`,
    disposal 4..7) leave the control at its default.
    **Not a byte-exact inverse of the serializer** (audit U-19, corrected
    2026-09-10): `save_settings()` writes the crop geometry, the position pair
    and the scale factors only while their parent toggle is on, so those values
    do not survive a restart with the toggle off. That is intentional — the
    fields are inert while the toggle is off — and unit test 26 pins that they
    DO survive exactly when the toggle is on.
  - `MainWindow::sessionFilePath()/loadSessionState()/saveSessionState()` —
    core **SettingsIO** `key = value` file (one serializer; CLI-compatible —
    NOT QSettings) at `AppConfigLocation/gifscythe.conf`
    (`%APPDATA%\Gifscythe\` on Windows); **`GS_SETTINGS_PATH`** env override
    (mirrors `GS_ENGINE`; the harness uses it for isolation).
  - Saved in `closeEvent`, loaded in the ctor (first launch = defaults, no
    file written until first close). GUI-only keys **`batch_dir`** and
    **`name_template`** ride in the same file; `SettingsIO` ignores unknown
    keys, so the CLI reads GUI-saved files with **no *load* warnings** (unit
    test 20 + a live CLI E2E both prove it). Corrected 2026-09-10 (audit U-20):
    a whole GUI-saved file still produces exactly one *validate* warning,
    `input=: at least one input file is required`, because the queue is
    deliberately not persisted. "No warnings at all" was an overstatement.
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
  T14/T15/T16. **243 checks, 0 failures** locally *(S7 sandbox)*. S8 added
  **T17** and rewrote **T8**; the file now holds **226 `CHECK(` sites**, but
  that source count is not a runtime count (loops expand some of them) and it
  has **not** been run here — CI only.
- **Docs:** this handoff, `WORKLIST.md`, `COMPILED_AUDIT.md` (row 15 → DONE;
  S3-9/S3-10 → DONE; §5 items 6/7/7b/10; §6.B counts; §11), root+product
  `README.md`, `PROJECT_VISION.md`, `IMPROVEMENT_LOG.md` (S7 entry), and the
  new `docs/release/RELEASE_PROCEDURE.md`.

## Verification status this session

Everything marked ✅ below was **run in this sandbox**; ⏳ items could not be
(no cmake, no Qt6 — see "Network/toolchain reality" below). S7's sandbox had Qt
6.4 and did compile the GUI; **this one does not**, so the GUI numbers in older
sections of this file are historical, not current.

| Check | Result |
|---|---|
| `./build.sh` (engine + CLI + unit tests, incl. #20–#30) | ✅ **211 checks, 0 failures** |
| `scripts/test_engine.sh` | ✅ 5/5 |
| `scripts/smoke_cli.sh` | ✅ 7/7 |
| `scripts/test_package.sh` (packaging negative suite) | ✅ 9/9 |
| `scripts/verify_audit.sh` | ✅ **24 PASS / 0 FAIL / 4 SKIP, exit 0** (skips = cmake, Qt6, CI-gated, clean-Windows) |
| GUI offscreen harness (`test_gui_offscreen`) | ⏳ **T1–T17, CI-only** — no cmake/Qt6 in this sandbox |
| `node web/test/command.test.mjs` (JS ⇄ C++ command parity) | ✅ **14/14 PASS** |
| `node web/test/validate.test.mjs` (JS ⇄ C++ validation parity) | ✅ **19/19 PASS** |
| `node web/test/transport.test.mjs` (live-server end-to-end) | ✅ **17/17 PASS** |
| CLI reads a GUI-saved conf (GUI keys present) | ✅ exit 0, zero warnings |
| `diff .github/workflows/build.yml docs/ci/build.yml.proposed` | ✅ byte-identical (enforced by `verify_audit.sh` E9) |
| Windows CI (linux job too) | ⏳ **must confirm this branch** — the only Windows Qt verification |

**S8 batch 2 (2026-09-10)** closed U-21, U-30, U-31, U-32, U-44, U-46, U-49,
U-50, U-51, U-52 on top of batch 1's 21. Every one is backed by an executed
check in `docs/audit/REMEDIATION_2026-09-10.md` §2b. Two files are new:
`src/core/OutputName.h` and `web/validate.mjs` (+ its test).

## Network/toolchain reality of this sandbox (updated 2026-09-10)

**This varies between sandboxes — always re-check before trusting an older
section of this file.**

* **S7 sandbox:** Debian apt worked, so `g++`, `cmake` and `qt6-base-dev`
  installed and the **Qt GUI + offscreen harness were compiled and run locally
  for the first time**. `-j4` OOMs; use `-j2`.
* **S8 sandbox (current):** `apt-get update` fails with
  `Acquire (13: Permission denied)` as uid 1001, and there is no cmake or Qt6.
  Present: g++ 12.2.0, make, git, node v22.22.3, python3, ImageMagick `convert`,
  curl. So **core / CLI / scripts / web are all verifiable here; `src/qtui/` and
  `tests/test_gui_offscreen.cpp` are CI-only.** PIL is not importable and there
  is no ffmpeg — use `reference_code/gifsicle/logo.gif` (60x132, 8703 B) as the
  test GIF.
* If apt is blocked again, the S5 fallback was `pip install cmake ninja PySide6`
  for the cmake steps; GUI verification then belongs to CI.

## Document map

| Doc | Role |
|-----|------|
| **`docs/audit/CONSOLIDATED_AUDIT_2026-09-10.md`** | **START HERE** — 3 reviews of `8190c08` merged into 44 verified findings (U-01…U-44) + fix order. Trust order: GPT > Seed > in-repo |
| `docs/audit/POST_S7_AUDIT.md` | In-repo post-merge review (source C). Two of its claims are **corrected in place**; read the banner first |
| `docs/release/RELEASE_PROCEDURE.md` | **NEW (S7)** — how to cut snapshots/releases |
| `docs/planning/OFFLINE_BUILD_REVIEW.md` | Offline feasibility + language choice + phased plan (§6 persistence gap → closed by S7) |
| `docs/web/WEB_FEASIBILITY.md` | Web-run review (Option 3 demo exists; Option 4 = future) |
| `docs/ci/CLEAN_WINDOWS_SMOKE.md` | C4/D3/D4 clean-Windows checklist (still OPEN — needs a real clean VM; **blocked by U-09** until artifacts are re-cut) |
| **`docs/audit/REMEDIATION_2026-09-10.md`** | **S8 record — what was fixed and the executed proof** (§0 gates + mutations, §1 critical, §2/§2b per finding, §3 CI-only, §4 register corrections, §5 still open) |
| `docs/audit/FIX_PICK_2026-09-10.md` | S8 pick rationale (how U-01 was chosen). **Superseded** by the remediation doc |
| `COMPILED_AUDIT.md` | Master checklist (§6 evidence) + the U-01…U-52 **register**, re-marked per S8. Its older "21 PASS / 0 FAIL / 2 SKIP" text is the pre-U-38 number and is **not portable** — U-38 is fixed |
| `WORKLIST.md` | Short task board (checkboxes; S7 + S8 items ticked) |
| `SESSION_HANDOFF.md` | This file |
| `IMPROVEMENT_LOG.md` | Chronological decisions, newest first (**S8 batch 2 → batch 1 → S7 → …**) |
| `PROJECT_VISION.md` | Product mission + hard constraints |
| `FEASIBILITY_REVIEW.md` | Original architecture + gifsicle flag mapping |
| `working_code/gifscythe/VERSION.md` | Version source of truth → `src/core/version.h` |
| `docs/archive/` | The two dated review snapshots (moved there by S8, **U-44**). Keep their historical line refs |
| `web/` | Server-side web POC + 3 parity/transport tests (demo only, not the product path) |

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
