# Session Handoff

**Date:** 2026-09-12 (session S14) · **Branch:** `arena/01a0968e-gifscythe`
**Base:** `main` `2d51347` · **PR #15 is MERGED** (its head run `34704221643` was green on
Linux + Windows; the **post-merge `main` run `34705247115` is RED** — Linux documentation status
gate failed, Windows passed, every later Linux step skipped) ·
**Product version:** 0.1.0 (do not bump to 1.0.0 yet — owner decision pending)

## S14 — External reviews compiled for review; stale status claims corrected (docs only)

**No code was changed and no intake finding was remediated.** 18 findings from three external
reviews were compiled and parked untriaged in `COMPILED_AUDIT.md` §13 and
`docs/audit/EXTERNAL_REVIEW_INTAKE_2026-09-12.md`, by owner instruction (compile, do not fix yet).

- **Intake:** Max/GPT-class `GS-201…GS-210` (1 Critical / 4 High / 4 Medium / 1 Low: CLI batch
  in-place overwrite, web `/run` path-from-upload-name, missing output verification, packaging,
  input admission, numeric domains, `GS_ENGINE` fallback, red main, native config, build options);
  DeepSeek's `DS-06…DS-13` (renamed from its own `N-06…N-13`, which collided with this repo's
  existing N-series); the Gemini deployment served an empty page, so it contributed nothing.
- **Corrections made because verification proved the docs false:**
  `COMPILED_AUDIT.md` header named a stale base (`2176573`) — the gate **G10** failure; 35
  narrative §2/§3/§4 status lines still read OPEN for items §5 marks fixed — they now cite their
  §5 row, with the original wording kept after *"Original report:"*; **U-06** and **U-08**
  corrected **DONE → PARTIAL** (U-06: loopback bind landed, but the concurrency cap / rate limit /
  engine-run bound named in the finding are still missing; U-08: silent-skip closed, but the
  licence set itself is still incomplete — no full GPLv3 text, no Qt LGPL notices staged);
  `docs/ci/PENDING_WORKFLOW_CHANGE.md` rewritten to describe the drift that actually remains; the
  WORKLIST U-06 tick unticked.
- **CI re-verified after these corrections:** run `34707532582` at commit `ddc4194` on this
  branch — **linux + windows both green**, including the "Documentation status gate (STATUS.md
  register)" step that failed in run `34705247115`, and every Linux step that failure had skipped.
  The live `main` tip stays red until this branch lands.
- **Not done, deliberately:** the 18 intake findings are **not** triaged into §5/`STATUS.md`; and
  the workflow copy is not synced (needs a `workflows`-scoped token).

## S13 follow-up

- **U-10 config half completed:** the product-owned native engine configuration
  is now `working_code/gifscythe/build_support/gifsicle/config.native.h`.
  `scripts/build_engine.sh` stages it as `config.h` in a temporary include
  directory, so `reference_code/gifsicle/` is upstream-only and never written.
  Native build, unit suite, engine tests, and smoke suite pass after the move.
- U-10 remains PARTIAL only for CI hash-pinning, which still needs a token with
  `workflows` scope. U-12 remains OPEN; no untestable GUI refactor was claimed.

## S12 follow-up

- **U-18/P2-4 coverage expanded:** `smoke_cli.sh` is now **19/19**, covering
  unknown/incomplete options, byte-pure binary stdout, PATH-only engine
  discovery from an isolated executable directory, and output-equals-input
  refusal. Existing unit and package suites cover thread flags, output planning,
  and incomplete packages.
- **Post-merge documentation gate repaired:** G10 accepts the current main
  merge tip or its first parent and works in shallow checkouts; G6 skips
  honestly when a full local toolchain is unavailable. This fixed the Linux
  post-merge failure without changing the workflow file.
- The remaining partial/open items are unchanged: U-09, U-10's config/workflow
  remainder, U-12, U-14, clean Windows/desktop probes, workflow-scope work,
  and owner decisions. See `STATUS.md`.

## TL;DR for the next session

0. **START HERE — `STATUS.md`.** The single status register: one row per
   tracked item, four states (**DONE / PARTIAL / OPEN / UNTRIAGED**), generated
   header that answers *"how much is done?"* in one line. It is **generated**
   by `working_code/gifscythe/scripts/check_docs.sh --emit` — never hand-edit
   the generated block. `COMPILED_AUDIT.md` §5 is the detail behind every
   `U-nn` row; neither replaces the other. As of S13: **80 DONE · 3 PARTIAL ·
   17 OPEN · 0 UNTRIAGED · 100 total.**

1. **What S11 did.** The S11 sandbox had working apt (uid 0) and installed,
   beyond the S10 stack (g++ 12.2 / cmake 3.25.1 / Qt 6.4.2 / ninja / node
   v20), also **mingw-w64 (12-win32), Wine 8.0 and gawk**. That flipped U-07
   from "needs a real Windows run" to executable here. Closed **four findings
   with executed proof, recorded upstream provenance, and scoped a fifth**:
   - **U-15 (P2-2)** — CMake writes ONLY into the build tree now: template
     moved to `build_support/version.h.in`, generated dir first on every
     include path, `core/version.h` includes in path form. `build.sh` remains
     the sole writer of the committed `src/core/version.h` fallback (A5
     green). New gate **C9** (build a copy with the fallback DELETED); the
     audit's read-only-`src/` repro was executed before (FAIL:
     `version.h.tmp` write error) and after (PASS). The G8 `build_support`
     allowance was deleted as instructed by its own comment.
   - **U-17 (P1-19)** — explode verifies frames: `src/core/ExplodeVerify.h`
     (snapshot `<prefix>.*` before; require ≥1 new/changed GIF87a/GIF89a file
     after; failure names prefix+dir). Wired into CLI `--run` (rc=1) and the
     GUI (dialog + honest status; success reports the verified count). A
     lying engine (exits 0, writes nothing) is refused at EVERY layer: unit
     block 33, smoke 9→12, harness T7 (`fake_engine_exit0` fixture), Wine
     rc=1. Prefix rules read off upstream (`-e` `.NNN`, `-E` by name, no-`-o`
     = input basename in CWD).
   - **U-07 (P1-4)** — Windows Unicode: `CreateProcessW` + strict UTF-8→UTF-16
     conversion; new `src/core/WinUnicode.h` (MSVCRT command-line splitter —
     pure logic, unit block 34; `GetCommandLineW` argv re-fetch;
     `GetEnvironmentVariableW`; `u8path_compat`/`path_u8string`). Executed
     under Wine: pre-fix binary (built from HEAD) fails an `é`-path conf rc=1
     with mojibake; fixed binary rc=0 + output written; é conf-path via argv
     and é `GS_ENGINE` both work; CJK reaches the child's UTF-16 command line
     byte-exact (probe); unit exe **289 checks** green under Wine.
   - **U-41 (P2-11, scoped first)** — web demo: mode selector + multi-file
     queue + results list in the UI; `POST /run` JSON endpoint on the server
     with desktop semantics (per-file batch Auto runs + planned targets +
     collision refusal, one `-m` merge, explode with the P1-19 frame
     verification, U-24/U-30 honesty layers). Suites: command 15→**17**,
     validate 19→**21**, transport 18→**30**.
   - **U-10 (provenance half)** — fresh full clone of `kohler/gifsicle`
     diffed against both vendored trees: `reference_code/gifsicle` is
     byte-identical to upstream `07f5c4c3` except the handwritten `config.h`
     (the "functional patch" + "extra test" ARE upstream commits
     `9efcc14`/`ed5b018`, five past `v1.96`); `gifsicle-nested-1.96` is
     pristine `v1.96`. Digests + recipe in `REFERENCE_MANIFEST.md`. Row stays
     PARTIAL: CI hash-pinning (needs `workflows` scope) + the `config.h` move.
   - **U-12** — scoped as **P1-24** in §6 (all five waits, fresh line numbers,
     fix sketch) and deliberately NOT implemented: the freeze needs a slow
     process start, which the offscreen harness cannot produce, and the
     cancel rewrite would rewire T2/T9/T10 semantics with no executable proof
     of improvement. Scoped-OPEN beats an untestable refactor.
   - **N-06 (new, closed in-session)** — the check_docs.sh emitter's
     proof-note truncation was awk-locale-dependent (gawk counts characters,
     mawk bytes), so the committed STATUS.md passed G0 under gawk but failed
     under mawk. Found when the sandbox RESTARTED mid-session and lost the
     apt-installed gawk (the tree stayed byte-identical to the CI-green head).
     Fixed in the checker: `export LC_ALL=C` + the S11 §5 notes reworded
     ASCII-only under the 150 limit; `--emit` verified byte-identical under
     both awks. **Lesson: a sandbox can restart mid-session — commit and push
     early, and never trust locally-installed tools to persist.**
   - **N-05 (new, closed in-session)** — multi-input Explode silently
     scattered frames: `gifsicle -e a.gif b.gif -o p` exits 0, explodes every
     input but the LAST into the process CWD, and only the last input honors
     the prefix — while the GUI queued all inputs into one explode run.
     Refused at every layer the same session: `validate()` warning (C++ +
     byte-identical JS mirror), CLI `--run` rc=2, GUI dialog + REFUSED
     summary; unit 35, smoke case 12, harness T7 subcase, Wine rc=2.
   - **N-04 (new, closed in-session)** — MinGW libstdc++ narrow `fs::path`
     conversions decode bytewise but encode UTF-8 (verified by probe under
     Wine): every non-ASCII string↔path boundary was silently mangling. All
     core boundaries now route through the WinUnicode helpers.
   - **Drift fixed on arrival:** the two stale `414f5fc` mentions (G10) were
     re-synced to the real tip before anything else.

2. **S11 full-toolchain gate baseline (retained; S12 CLI/docs rerun is in
   the follow-up above):** `./build.sh` **296 checks, 0 failures** ·
   `test_engine.sh` **5/5** · `smoke_cli.sh` **14/14** ·
   `test_package.sh` **9/9** · web **17 + 23 + 30** · offscreen harness
   **324 checks, 0 failures** · `check_docs.sh` **21 passed, 0 failed,
   1 skipped** (G7 skip = declared-pending workflow) · `verify_audit.sh`
   **28 PASS / 0 FAIL / 3 SKIP, exit 0** (skips: E9 declared-pending workflow,
   CI-gated, clean-Windows). awk changes verified under **mawk AND gawk**.

3. **CI verdict — GREEN.** Final run `34685470992` on the head `635c986`:
   **linux success + windows success** (every intermediate head green too;
   only the first run failed, windows-only, on the T7 separator assertion —
   see the S11 log entry). The windows job was the first NATIVE compilation of the
   S11 Windows code (CreateProcessW/`_wopen`/splitter; the Wine proofs are
   executed but Wine ≠ Windows) and ran the extended harness green (317
   checks, incl. T7's `fake_engine_exit0` fixture). The FIRST run
   (`34671814580` on `f655987`) failed windows-only on a cosmetic T7
   assertion: `locate_engine()` returns native separators while
   `applicationDirPath()` uses forward slashes, so a full-path `contains()`
   could only pass on POSIX — fixed in `88e15ea` by comparing the basename
   (all U-17 behavioral checks had passed on Windows untouched). Re-check the
   tip before merging (rule 3 re-applies).

4. **What remains before 1.0.0** — criterion unchanged (*no Critical/High
   findings open, package-negative tests green, clean-Windows smoke against
   the exact tagged SHA*): release re-cut **U-09** → clean-Windows
   `windeployqt` smoke (C4/D3/D4, `docs/ci/CLEAN_WINDOWS_SMOKE.md`) → desktop
   probes B5/B6/B14 → owner decisions (two-way CLI pane, version 0.2.0 vs
   1.0.0). Audit findings still open: **U-12** (scoped P1-24) and **U-09**.
   PARTIALs: **U-10** (CI hash-pinning only) and **U-14** (verify_audit in CI
   — `workflows` scope). U-18/P2-4 regression coverage is DONE in S12.
   W-30 waits on the same `workflows`-scoped maintainer action.

5. **Direction unchanged:** offline-only; C++17 + Qt6 Widgets through 1.0.0;
   `web/` is a demo/parity harness only (see
   `docs/planning/OFFLINE_BUILD_REVIEW.md`).

6. **Naming policy in force:** *Gifscythe* = product; *gifsicle* = upstream
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

### Product constraints (unchanged unless noted)

- `reference_code/` is read-only; product work lives in `working_code/gifscythe/`.
  The ONE exception S8/S11 used: `reference_code/REFERENCE_MANIFEST.md` is the
  provenance record and gets updated with evidence (never the code trees).
- Do not bump to `1.0.0` before the UI/UX gates + owner decision.
- Do not add WebP/APNG before the GIF UI is stable.
- Keep gifsicle as a **subprocess** (GPL v2-only engine vs GPLv3 UI).
- Do **not** "fix" `--loopcount=0`, `-O0`, crop `+` form, or gamma sentinel —
  verified correct.
- Live CLI pane stays honest **one-way** (until the owner decides otherwise).
- **UPDATED (S11):** Windows exec is `CreateProcessW` + `win_quote_arg`
  (UTF-8 argv → strict UTF-16 line) — still never `_spawnvp`/shell. Every
  std::string↔fs::path boundary goes through `u8path_compat`/`path_u8string`
  (`src/core/WinUnicode.h`) — N-04 proved implicit narrow conversions mangle
  non-ASCII on MinGW. New Windows-side code MUST follow both rules.
- Windows CLI/test exes stay `-static`; engine line keeps `-include
  src/win32cfg.h` before `-I.` and never passes `-DVERSION`.
- Keep `.github/workflows/build.yml` and `docs/ci/build.yml.proposed`
  byte-identical (`verify_audit.sh` **E9** / `check_docs.sh` **G7**). One
  declared exception: while `docs/ci/PENDING_WORKFLOW_CHANGE.md` exists the
  copies differ on purpose, because the CI token has no `workflows` scope.
  **Deleting that marker is part of applying the change.**
- **NEW (S11):** CMake must never write into `src/` (U-15, gate **C9**). The
  committed `src/core/version.h` fallback is written ONLY by `build.sh`; the
  template lives at `build_support/version.h.in`; include order stays
  generated-first. Do not "simplify" these back.
- **NEW (S11):** Explode processes ONE file per run (N-05): `validate()`
  warns on multi-input explode (the JS mirror must stay byte-identical —
  parity-pinned), CLI `--run` refuses rc=2, the GUI refuses via the warning
  dialog, web `/run` answers 400. Do not re-allow multi-input explode.
- **NEW (S11):** Explode success means VERIFIED frames (`ExplodeVerify.h`
  snapshot-diff) — never rc=0 alone. The harness fixture `fake_engine_exit0`
  (CMake target) must keep being built next to `test_gui_offscreen`
  (verify_audit gate B builds both targets).
- Extend `tests/test_gui_offscreen.cpp` with every GUI feature (regression
  net). S10 added T18/T19/T20, S11 extended T7 — keep that habit.
- Offline-only — no server, no auto-update, no telemetry; `web/` is a demo.
  Its `/run` endpoint keeps the desktop honesty rules (planned targets,
  collision refusal, output verification) — do not fork the semantics.
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
  (`~/devtools/capture` in the S10 sandbox). S11 changed NO desktop-visible
  UI (statuses only), so the S10 shots remain accurate; re-shoot recipe:
  `docs/screenshots/README.md`.

## Verification status this session (S11)

Everything marked ✅ was **run in this sandbox**; ⏳ could not be. Quote the
**runtime** counter for test counts, never the `CHECK(` source site count.

| Check | Result |
|---|---|
| `./build.sh` (engine + CLI + unit tests) | ✅ **296 checks, 0 failures** (runtime counter; blocks 33/34/35 added in S11) |
| `scripts/test_engine.sh` | ✅ 5/5 |
| `scripts/smoke_cli.sh` | ✅ **14/14** (S11 added the explode-verification + N-05 refusal cases) |
| `scripts/test_package.sh` (packaging negative suite) | ✅ 9/9 |
| `node web/test/command.test.mjs` | ✅ **17** (S11 added the `-b`/`-e` fixtures) |
| `node web/test/validate.test.mjs` | ✅ **23** (S11 added info+explode, explode geometry, N-05 ×2) |
| `node web/test/transport.test.mjs` (live server) | ✅ **30** (S11 added the 12 `/run` cases) |
| GUI offscreen harness (`test_gui_offscreen`) | ✅ **324 checks, 0 failures** — COMPILED AND RUN HERE (T1–T20, T7 extended). New last-measured figure; 306 @ S10 |
| `scripts/check_docs.sh` (documentation gate) | ✅ **21 passed, 0 failed, 1 skipped, exit 0** (the skip is G7, the declared-pending workflow change) |
| `scripts/verify_audit.sh` | ✅ **28 PASS / 0 FAIL / 3 SKIP, exit 0** (skips = E9 declared-pending workflow, CI-gated, clean-Windows; S11 added gate C9) |
| MinGW cross-build (CLI, unit exe, fixture, engine exe) | ✅ zero warnings `-Wall -Wextra -static`; engine reports `LCDF Gifsicle 1.96 (Windows)` under Wine |
| Wine 8 E2E matrix (U-07/U-17) | ✅ old-build repro rc=1 mojibake; fixed build rc=0 on é paths (conf contents, conf path, `GS_ENGINE`); CJK byte-exact to the child UTF-16 line; unit exe 292 checks; explode 12 frames rc=0 / lying engine rc=1 / multi-input refused rc=2 |
| U-15 read-only + deleted-fallback repro | ✅ both halves executed (old FAIL / new PASS, uid 65534) |
| `diff .github/workflows/build.yml docs/ci/build.yml.proposed` | ⏳ **differ on purpose** — the doc-gate step cannot be pushed without `workflows` scope; declared in `docs/ci/PENDING_WORKFLOW_CHANGE.md`. E9/G7 SKIP for declared drift, FAIL for undeclared |
| GitHub Actions, S11 changes | ✅ final run `34685470992` on head `635c986`: **linux success + windows success** (first run `34671814580` on `f655987` failed windows-only on the T7 path-separator assertion — fixed in `88e15ea`; heads `88e15ea`/`74091b7`/`635c986` all green). Re-check the current tip before merging, not this row |

**Counts are stated by kind on purpose.** `grep -c 'CHECK('` counts **lines**;
`grep -o 'CHECK(' | wc -l` counts **occurrences** (S11: harness **250**, unit
**261** occurrences); neither equals the **runtime** count (unit **296**,
harness **324**), because loops expand checks. `check_docs.sh` gate **G9**
prints which kind it means and compares like with like.

## Network/toolchain reality of this sandbox (re-check every session)

**This varies between sandboxes — always re-check before trusting an older
section of this file.**

* **S11 sandbox (current):** **full toolchain + Windows cross-proof.** uid 0
  with working apt (aliyun mirror): g++ 12.2.0, cmake 3.25.1, Qt 6.4.2,
  ninja, **mingw-w64 (gcc 12-win32)**, **Wine 8.0** (runs; wine32/i386 absent —
  64-bit exes only), **gawk** (next to mawk — verify awk changes under BOTH),
  node v20.20.2, python3 3.11, DejaVu fonts, curl. Network open (github clone,
  apt). Token had read+write. **Wine quirks found the hard way:** (1) Wine 8's
  ACP is immovable at 1252 — registry (`iDefaultANSICP`, `Nls\CodePage`) and
  `LANG=ja_JP.UTF-8` all probed, `GetACP()` stays 1252, so CJK-through-engine
  E2E is out of reach here (Gifscythe's chain still proven byte-exact with a
  probe child); (2) `Z:` could not see `/home/user` (sandbox mount quirk —
  `GetFileAttributesW` err 2/3 while `/tmp` works): stage Wine tests under
  `/tmp`; (3) root ignores `chmod a-w` — read-only repros need
  `setpriv --reuid 65534`.
* **S11 sandbox quirk (carried from S10):** the shell/tooling layer rewrites
  the literal string `/home/user` to `$ARENA_WORKSPACE` inside *file contents*
  written through bash heredocs (shell commands still work because bash
  re-expands it, but CMake/scripts see an undefined variable). Write files
  that must contain workspace paths with the file tool, or pass such paths in
  via `-D`/argv/env at run time.
* **awk portability is a gate concern:** CI runners use gawk, sandboxes have
  used mawk. gawk rejects bracket classes like `[^U-0-9]` (invalid range)
  that mawk accepts — this broke CI-linux once (PR #13). Dash first or last
  in every bracket class; with gawk now installed locally, S11 verified both.
* **S10 sandbox:** full Qt toolchain via apt (harness 306); no mingw, no wine,
  no `gh`. GitHub Actions job logs ARE readable from a sandbox via
  `GET /repos/…/actions/jobs/<id>/logs` (302 to a reachable blob host).
* **S9 sandbox:** no cmake, no Qt6 (g++ 12.2.0, make, git, node v22.22.3,
  python3). Its numbers (25/0/5) were true there; G6 re-syncs them per sandbox.
* **S8 sandbox:** apt blocked (`Acquire (13: Permission denied)`, uid 1001).
* **S7 sandbox:** apt worked; harness measured 243; `-j4` OOMs, use `-j2`.
* If apt is blocked again, the S5 fallback was `pip install cmake ninja
  PySide6` for the cmake steps; GUI verification then belongs to CI.
* Clone depth: the S11 clone is **full** (no `.git/shallow`), so G10/G11 saw
  complete history. Shallow clones re-open risk **R-02**.

## Document map

| Doc | Role |
|-----|------|
| **`STATUS.md`** | **START HERE** — the single status register. Four states, one row per item, generated header. Roll-up only |
| `COMPILED_AUDIT.md` | The **detail** behind every `U-nn` row: findings, verification marks, §6 fix order (incl. P1-24 U-12 scope + P2-11 U-41), §7 checklist, §8 risks. The `U-nn` rows in `STATUS.md` are generated from its §5 |
| `WORKLIST.md` | Human task board + the status rules + the deferred bucket list |
| `SESSION_HANDOFF.md` | This file — context for the next session |
| `IMPROVEMENT_LOG.md` | Chronological decisions, newest first, one template per entry |
| `PROJECT_VISION.md` | Product mission + hard constraints |
| `FEASIBILITY_REVIEW.md` | Original architecture + gifsicle flag mapping |
| `reference_code/REFERENCE_MANIFEST.md` | **Provenance record (S11):** upstream clone SHAs, diff verdicts, tree digests, reproduce recipe |
| `docs/audit/REMEDIATION_2026-09-10.md` | S8 record — dated snapshot; its gate numbers are S8's |
| `docs/audit/CONSOLIDATED_AUDIT_2026-09-10.md` · `FIX_PICK_2026-09-10.md` · `POST_S7_AUDIT.md` | Dated audit snapshots — excluded from `check_docs.sh` by policy |
| `docs/release/RELEASE_PROCEDURE.md` | How to cut snapshots/releases, incl. the doc gate |
| `docs/planning/OFFLINE_BUILD_REVIEW.md` | Offline feasibility + language choice + phased plan |
| `docs/web/WEB_FEASIBILITY.md` | Web-run review (Option 3 demo exists; Option 4 = future) |
| `docs/ci/README.md` · `docs/ci/PENDING_WORKFLOW_CHANGE.md` · `docs/ci/CLEAN_WINDOWS_SMOKE.md` | CI workflow status, the blocked workflow change, and the C4/D3/D4 clean-Windows checklist |
| `docs/screenshots/README.md` | S10 re-shoot recipe + what each shot shows (S11 changed no desktop-visible UI, so they remain current); linked from the root README |
| `docs/archive/` | The two dated review snapshots (historical line refs kept) |
| `web/` | Server-side web POC: `/optimize` (legacy single-file) + `/run` (all four modes, S11) + 3 parity/transport suites (demo only, not the product path) |
| `working_code/gifscythe/VERSION.md` | Version source of truth → committed `src/core/version.h` fallback (build.sh) + build-tree copy (CMake, from `build_support/version.h.in`) |

## Prior-session history (S4/S4b/S5/S6/S7/S8/S9/S10)

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
maintainer updated the workflow again in `414f5fc` (2026-09-11). S10: the full
Qt stack in one sandbox again — nine findings + N-03 closed with executed
proof (U-16/U-34/U-35/U-36/U-37/U-40/U-42/U-45/U-47), harness re-measured at
306; PR #13 went through three CI lessons (Windows QMovie delete-locks,
fetch-depth-1 G10 SKIP, the gawk bracket-range bug) and merged green as
`2176573` — the commit S11 based on.
