# Worklist

**Version:** 0.1.0 · **Status register:** `STATUS.md` · **Audit detail:** `COMPILED_AUDIT.md`

> ## ⚑ Start with `STATUS.md`, not this board
> **`STATUS.md`** is the single status register: one row per tracked item in
> exactly one of four states — **DONE / PARTIAL / OPEN / UNTRIAGED** — with a
> generated header that says how much of everything the repo knows about is
> done. It answers *"what is left?"* without reading a 52-row register.
>
> This file is the **human task board**: what to pick up next, in order. It is
> not the status source of truth any more, and it must not contradict
> `STATUS.md` (`check_docs.sh` gate **G2** fails if a ticked box here maps to a
> non-DONE row there).
>
> Per-finding evidence stays in **`COMPILED_AUDIT.md`** §5 and, for the S8
> remediation, **`docs/audit/REMEDIATION_2026-09-10.md`**.

## The rules that keep the docs true (NEW in S9 — do not skip these)

These live here, in `SESSION_HANDOFF.md` and in
`docs/release/RELEASE_PROCEDURE.md` because rules only stick if a new session
reads them in a file, not in a conversation.

1. **Every session ends by updating the docs** — `STATUS.md`,
   `SESSION_HANDOFF.md`, `WORKLIST.md`, `IMPROVEMENT_LOG.md` — **then runs
   `check_docs.sh` until green.** Stale docs are corrupted input for the next
   session, not a cosmetic problem.
2. **A new finding is recorded in the same session it is found:** `UNTRIAGED` in
   `STATUS.md` **and** a pending `- [ ]` line in the "Found this session" list
   below. It may not wait for a later audit pass. Gate **G12** fails if an
   `UNTRIAGED` row outlives the session that found it.
3. **Before `gh pr create`, and again before `gh pr merge`:** run
   `working_code/gifscythe/scripts/check_docs.sh`, fix every failure, re-run
   until green. Do not create or merge with a failing doc check, and **do not
   ask whether to run it**.
4. **It is also enforced mechanically.** `.githooks/pre-push` blocks a red push.
   Git does not copy hooks on clone, so run
   `working_code/gifscythe/scripts/bootstrap_hooks.sh` once per clone —
   `build.sh` does it for you. Verify: `git config core.hooksPath` → `.githooks`.
5. **`IMPROVEMENT_LOG.md` entries use the template** (`Changed / Partial / Left /
   Verified / Not verifiable here / Docs touched`). The **`Not verifiable here`**
   line is mandatory and must never be omitted or softened.

## Found this session — pending lines (rule 2)

Every `UNTRIAGED` row in `STATUS.md` must have a matching line here.

- [ ] **External review intake (S14) — registered `UNTRIAGED`, triage waits on the owner.**
      18 findings (Max/GPT-class `GS-201…GS-210`, DeepSeek `DS-06…DS-13`) are compiled in
      `COMPILED_AUDIT.md` §13 + `docs/audit/EXTERNAL_REVIEW_INTAKE_2026-09-12.md`, and recorded
      row-by-row in `STATUS.md` under the reviewers' ids. Proposed sequencing lives in
      `web/WEB_PLAN_TEMPLATE.md` (template; the owner's draft is refitted into it).
      Triage = map accepted items into `COMPILED_AUDIT.md` §6 fix-order ids. One pending line per
      finding, as rule 2 requires:
      - [ ] **GS-201** (Critical) CLI Batch → engine `-b` can rewrite the source GIFs; cheap
            stop-loss = refuse `--run` with Batch and no `output` key.
      - [ ] **GS-202** (High) web `/run` builds output paths from upload names; `../` escapes the
            request temp dir.
      - [ ] **GS-203** (High) ordinary runs claim success on exit 0 with no output verification
            (CLI Explode-only, GUI/web existence+size).
      - [ ] **GS-204** (High) packaging fail-open outside the portable happy path; negative tests
            cover portable only.
      - [ ] **GS-205** (Med) non-GIF inputs still admitted via the picker and drop.
      - [ ] **GS-206** (Med) `long`→`int` narrowing; no validation for loopcount/threads/gamma/enums.
      - [ ] **GS-207** (Med) unusable `GS_ENGINE` silently falls back to another engine.
      - [ ] **GS-208** (High, PARTIAL-fixed) main release-red; docs corrected in S14, workflow-copy
            sync + marker deletion still open (needs a `workflows`-scoped token).
      - [ ] **GS-209** (Med) native linux/mac build uses a fixed glibc config.
      - [ ] **GS-210** (Low) build entry points ignore mistyped options; qmake tried before CMake.
      - [ ] **DS-06** (High) `threads <= 0` → bare `-j`; the `-1` sentinel now means 8 threads.
      - [ ] **DS-07** (Med) GUI threads spinner cannot express "no flag".
      - [ ] **DS-08** (Low) non-strict print mode exits 0 after warnings.
      - [ ] **DS-09** (Info) `threads < -1` accepted silently.
      - [ ] **DS-10** (Info) disposal 4..7 unreachable from the desktop picker.
      - [ ] **DS-11** (Med, PARTIAL-fixed) audit narrative reconciled in S14; the mechanical gate
            check that keeps it reconciled is still missing.
      - [ ] **DS-12** (Low) settings values lose leading/trailing whitespace on round trip.
      - [ ] **DS-13** (Med) web `/optimize` serves non-GIF output as `200 image/gif`.
- [ ] **CI/infra (S14):** apply `docs/ci/build.yml.proposed` and delete
      `docs/ci/PENDING_WORKFLOW_CHANGE.md` in one commit — both copies then enforce
      byte-equality again (E9/G7). Needs a token with the `workflows` scope.

- [x] **N-03** — `docs/screenshots/*.png` (3 shots) claim to show the S7 UI; S8
      changed `src/qtui/` afterwards, nothing links to them, and this sandbox has
      no Qt6 to regenerate them. Decide: re-shoot on a Qt machine and link them
      from `README.md`, or mark them historical. **Resolved in S10:** the S10
      sandbox HAD Qt6 (apt), so the shots were re-taken offscreen from the
      current UI and are now linked from the root README; see
      `docs/screenshots/README.md`.
- [x] **N-06** — check_docs.sh emitter truncation was awk-locale-dependent
      (gawk chars vs mawk bytes). **Found and fixed in S11** (`LC_ALL=C` +
      ASCII-only §5 notes; emit byte-identical under both awks).
- [x] **N-05** — multi-input Explode scattered frames silently (engine rc=0,
      all-but-last inputs → CWD). **Found, verified and refused in S11**
      (validate warning + CLI rc=2 + GUI refusal; tests at every layer).
- [x] **N-04** — MinGW libstdc++'s narrow `fs::path` conversions are not UTF-8
      (decode bytewise, encode UTF-8), silently mangling non-ASCII paths at
      every core boundary on Windows. **Found and resolved in S11** under Wine
      while executing U-07: `u8path_compat`/`path_u8string` (`WinUnicode.h`)
      now sit at every string↔path boundary; Wine E2E cases B/C/D prove it.

## Direction decisions (2026-09-09 — see `docs/planning/OFFLINE_BUILD_REVIEW.md`)

- **Offline-only.** No server, no auto-update, no telemetry. **Amended 2026-09-12 (S14,
  owner):** the `web/` build is now a **supported product surface** — a **self-hosted**
  alternative to the `.exe`/portable build (loopback by default, `GS_WEB_HOST` for LAN). No
  cloud/hosted service; the offline-only promise stands. Plan + split rules:
  `web/WEB_PLAN_TEMPLATE.md`.
- **Language: stay C++17 + Qt6 Widgets through 1.0.0** (already offline,
  portable, CI-verified). Revisit only if a documented trigger fires — then
  spike **Rust + Tauri**. Comparison matrix in the offline review doc.

## Current status

### Foundation (done)
- [x] P0 project setup and reference/working separation
- [x] GIF engine build and verification (`release/<ver>/gifsicle`, identity 1.96)
- [x] Qt-independent command/settings control layer
- [x] CLI driver + unit tests + integration smoke (`smoke_cli.sh`)
- [x] Qt6 GUI MVP scaffold (Batch default, mode combo, async run, queue, DnD)
- [x] Portable and system-dependent packaging scripts (`.exe` probe + licenses)
- [x] Linux GitHub Actions path with Qt6 + artifacts (recipe present)
- [x] Root LICENSE / COPYING.gifsicle / `.gitignore` / `.gitattributes`

### Review remediation (implemented 2026-09-07 — re-verify via §6)
- [x] **P0 silent-failure fixes** — CLI exit codes, argv exec, CMake INTERFACE,
      GUI batch default, output validation, SettingsIO safety, Windows `win32cfg.h`
- [x] **P1 honesty work** — single version source, EngineLocator, queue UX,
      live pane sync, `save_settings`/`validate`, packaging, `shell_quote`,
      Settings-by-value
- [x] Smoke suite + engine tests scripts; CI recipe proposed in `docs/ci/build.yml.proposed`
- [x] Apply `docs/ci/build.yml.proposed` → `.github/workflows/build.yml` — applied
      by maintainer in `821a310`; S4 hardened both copies (static-link CLI/tests,
      Ninja generator, native Windows E2E smoke, GUI offscreen steps).
- [x] Merged compiled audit (S1+S2+S3) → `COMPILED_AUDIT.md`

### Gates before more features
- [x] **Verify fixes** (`COMPILED_AUDIT.md` §6) — one-command rerun:
      `working_code/gifscythe/scripts/verify_audit.sh`, currently
      **28 PASS / 0 FAIL / 3 SKIP, exit 0** (skips: E9 declared-pending
      workflow, CI-gated, clean-Windows; measured in the S11 sandbox which has
      cmake+Qt6 — a toolchain-less sandbox skips C6/C7*/C9/B as well; S11
      added gate C9, the cmake source-tree-purity check for U-15)
- [x] **Documentation gate** (NEW S9) — `scripts/check_docs.sh` emits and
      enforces `STATUS.md`; wired into `verify_audit.sh` as **F1/F2**, into CI
      (pending — see `docs/ci/PENDING_WORKFLOW_CHANGE.md`) and into
      `.githooks/pre-push`
- [x] Windows engine + CLI **proven under Wine**: `gifsicle.exe` runs
      (`1.96 (Windows)`), CLI E2E with `C:\` paths + spaces + honest exit 1;
      found & fixed two Windows-only bugs (engine `-I.` recipe, `_spawnvp`
      space-splitting → CreateProcessA + quoting; static-linked exes)
- [x] **Valid push token** — token #3 worked (2026-09-07); PR #5 merged
      into `main` (merge `0ad1ff5`)
- [x] Windows GitHub Actions job **green** with downloadable artifact
      (main run #23: windows ✅ + linux ✅; `gifscythe-windows` 27.8 MB)
- [ ] Clean-machine portable smoke (esp. Windows + `windeployqt`) — from CI
      artifact after C2 (C4/D3/D4). **Blocked by U-09**
- [ ] One-time real-desktop GUI probes: B5 (kill engine mid-run), B6 physical
      drag-drop, B14 engine-missing GUI variant

### Audit remediation S8 (2026-09-10) — 31 of 52 register findings closed
> Evidence per finding: **`docs/audit/REMEDIATION_2026-09-10.md`**.
> Roll-up: **`STATUS.md`**. Tally: 31 fixed outright (21 batch 1 + 10 batch 2) ·
> 3 partial · 15 open · 1 fixed earlier by S7 · 2 register rows corrected = **52**.

- [x] Both release blockers: **U-01** (`src/core/OutputPlan.h`) · **U-02**
      (fail-closed packager + `scripts/test_package.sh`)
- [x] Engine-truth guards: **U-03** threads `-j` · **U-22** resize geometry ·
      **U-11** strict booleans · **U-33** half-specified `-p`
- [x] CLI honesty: **U-04** stdout purity · **U-05** PATH engine search ·
      **U-23** strict arg parser
- [x] Settings serializer: **U-51** `encode_line_value()` (9 sites + JS mirror)
- [x] Process layer: **U-32** `128+WTERMSIG` · **U-31** `-lstdc++fs` probe
- [x] Output names: **U-21** `src/core/OutputName.h` (`NameRules` Host/Win/Posix)
- [x] Web app: **U-24** honest rc=0 · **U-25/U-29**
      Scale default + Touch · **U-26** version sort · **U-30** validation layer ·
      **U-49/U-50** transport · **U-46/U-52** request ownership + URL revoke
- [ ] Web app bounds: **U-06** loopback default **landed** (S8); still open — concurrency cap,
      per-client rate limit, engine-run bound. U-06 was corrected DONE → PARTIAL in S14.
- [x] Process hygiene: **U-38** SKIP not FAIL · **U-39** workflow-drift guard
      **E9** · **U-44** `docs/archive/` · **U-43** · **U-48** empty comment
- [x] Three web suites now gate the JS copies — **W1** command (14) ·
      **W2** validation (19) · **W3** transport (17)
- [x] **CI confirms `src/qtui/` and harness T8/T17** — PR #11 run `34471563229`:
      **linux pass 1m14s, windows pass 2m56s**
- [x] **U-40** — CLI prints `validate()` warnings and runs anyway while the GUI
      refuses. **S10:** `--strict` flag added (any parse/validation warning →
      refusal, exit 3, nothing printed or run); `--help` documents the warning
      policy and the exit-code table; smoke suite 7 → 9 cases.
- [ ] Qt-only findings still need a Qt machine to *change*: **U-12 only**
      (scoped S11 as P1-24; implementation deliberately deferred — see
      `COMPILED_AUDIT.md` §6). S10 closed U-16/U-34/U-35/U-36/U-37/U-45/U-47;
      **S11 closed U-15 and U-17 locally** (Qt6 sandbox again, harness ran).
- [x] Windows-only: **U-07** ANSI process APIs — **closed in S11 under Wine**:
      the S11 sandbox installed mingw-w64 + Wine 8, so the fix (CreateProcessW +
      GetCommandLineW argv re-fetch + wide env + u8path boundaries) was
      cross-compiled AND executed here: é-path E2E rc=0 (the pre-fix build fails
      the same conf rc=1 with mojibake), CJK proven lossless to the child's
      UTF-16 command line, unit suite green under Wine (289 checks). Residual
      (documented, upstream): gifsicle's own CRT hands IT ACP-encoded argv, so
      characters outside the system ACP need Windows' UTF-8-ACP option to reach
      the engine's file APIs. A real-Windows desktop pass stays nice-to-have
      (W-18/W-19 territory).
- [ ] **U-09** re-cut the release from *this* SHA.

### Session S14 (2026-09-12) — external reviews compiled; stale status claims corrected (docs only)

- [x] **Intake compiled (no remediation).** `docs/audit/EXTERNAL_REVIEW_INTAKE_2026-09-12.md`
      plus `COMPILED_AUDIT.md` §13: 10 findings from the Max/GPT-class review, 8 from DeepSeek
      (renamed `DS-06…DS-13`; its own `N-06…N-13` collided with this repo's N-series), and an
      empty Gemini deployment. Each item carries evidence, impact, the reviewer's proposed fix
      and a re-check against `2d51347`.
- [x] **`COMPILED_AUDIT.md` header:** stale base `2176573` → `main` `2d51347` (**G10** was
      failing on it); verification sessions extended through S14; a current-state banner added
      (main is release-red) and §0 rule 6 for narrative-vs-register reading.
- [x] **Narrative reconciliation:** 35 `**Status:**` lines in §2/§3/§4 read OPEN for items §5
      marks fixed. Each now names its §5 row; the audit as filed follows after *"Original
      report:"*.
- [x] **Two register states corrected DONE → PARTIAL** — see the `STATUS.md` rows for the
      web-bounds item (loopback bind landed; concurrency cap, rate limit and engine-run bound
      still missing) and the licence-set item (silent-skip closed; full GPLv3 text and Qt LGPL
      notices still not staged).
      U-08 removed from §6 P0-3's Closes list; `STATUS.md` re-emitted.
- [x] **`docs/ci/PENDING_WORKFLOW_CHANGE.md`** now describes the drift that really remains (the
      Windows temp-path fallback line) instead of a step that is already live.
- [x] **CI re-verified:** run `34707532582` at commit `ddc4194` on this branch — linux +
      windows green, including the documentation status gate that failed in run `34705247115`.
- [x] **Merged and green on main:** PR #16 merged as `629135a`; `main` run `34709202307` green on
      linux + windows. The documentation gate that failed in run `34705247115` now passes on the
      merged tip.

### Session S14 follow-up — tasks placed in their documents

- [x] **Intake registered:** the 18 findings now have `STATUS.md` rows (`UNTRIAGED`, reviewers'
      ids) and the pending lines above, so nothing lives only in a chat message or a report.
- [x] **Plan template in place:** `web/WEB_PLAN_TEMPLATE.md` (moved into `web/` so it is findable
      next to the code) — the owner's draft is refitted into its slots under §0's rules. Its state
      line reads `SKELETON` and is mirrored in `SESSION_HANDOFF.md`; **gate G16** fails if the two
      disagree. The flip to `WORKING PLAN` happens **once**, in the refit commit (both lines).
- [x] **Direction decision (owner, S14):** the web build is a **supported product surface**, a
      self-hosted alternative to the `.exe`/portable build. `PROJECT_VISION.md`, the direction
      decisions above, `STATUS.md` (D-07) and the template's §1 record it.
- [x] **Release notes placed:** `docs/release/RELEASE_PROCEDURE.md` carries the open
      release-blocking pointers; `docs/ci/PENDING_WORKFLOW_CHANGE.md` carries the CI one.

### Session S9 (2026-09-10) — status-tracking system
- [x] **N-01** — the pending-workflow marker was left behind after the maintainer
      applied that change in `190d030`; every doc still quoted **23/0/5** while
      the real gate run was **24/0/4**. Marker rewritten to describe the *new*
      pending change; gate **G6** now measures the real number and compares.
- [x] **N-02** — `web/README.md` documented the pre-bind-fix (S8) bind address
      (`0.0.0.0`); now states `127.0.0.1` + `GS_WEB_HOST`.

### Session S10 (2026-09-11) — the sandbox finally had Qt6; closed 9 findings + N-03
- [x] **N-03** — screenshots re-shot offscreen (Qt 6.4.2) from the current UI and
      linked from the root README; `docs/screenshots/README.md` rewritten.
- [x] **U-45 (P0-1 last gap)** — output group (incl. both Browse buttons) locked
      during runs; chooser slots guard `busy_`; T18 proves a mid-run folder edit
      cannot redirect the planned outputs. **P0-1 is now fully closed.**
- [x] **U-16** — atomic settings persistence: core `save_settings_file` is
      tmp+fsync+rename; GUI save is `QSaveFile`; unit test 32 + T19.
- [x] **U-34 / U-47 (P1-10)** — `invalidatePreview()` on every schedule/clear/
      cancel/busy; stale+failed previews delete their file; every success
      sweeps `preview_*.gif` except the displayed one; T20.
- [x] **U-35** — `setBusy(false)` re-enables Run only through `ensureEngine()`.
- [x] **U-36** — `guiStateKey` (the third parser) deleted; `load_settings`
      collects unknown keys; GUI reads its keys from that map; test 31.
- [x] **U-37** — one-time status note when persistence is unavailable.
- [x] **U-40** — `--strict` (rc=3) + documented warning policy/exit codes.
- [x] **U-42** — web Scale X/Y inputs; asymmetric parity fixture + transport case.
- [x] **R-01** — harness measured locally at last: **306 checks, 0 failures**.

### Session S13 (2026-09-12) — product engine config moved out of the vendored tree
- [x] **S13 config relocation** — moved the product-owned native config to
      `working_code/gifscythe/build_support/gifsicle/config.native.h` and stages
      it as `config.h` in a temporary include directory. Native build, unit
      suite, engine tests, smoke 19/19, audit, and Linux/Windows CI passed.
      The related audit row remains PARTIAL only for CI hash-pinning
      (`workflows` scope).

### Session S12 (2026-09-12) — regression coverage expanded; post-merge doc gate repaired
- [x] **U-18 (P2-4)** — expanded the CLI smoke net from 14/14 to **19/19**:
      unknown/incomplete CLI options, byte-pure binary stdout, PATH-only engine
      discovery from an isolated executable directory, and output-equals-input
      refusal. Existing unit/package coverage pins thread flags, output planning,
      and incomplete-package failures. `COMPILED_AUDIT.md` U-18 is now DONE.
- [x] **G10 post-merge fix** — `check_docs.sh` accepts a merge tip or its first
      parent, including depth-1 checkouts where the parent object is absent;
      G6 skips honestly when gcc/g++/Node/CMake/Qt6 are unavailable rather than
      comparing a reduced local audit total with the full-toolchain headline.
      PR #15 (`91afbdd`) is green on Linux and Windows.

### Session S11 (2026-09-12) — mingw + Wine in the sandbox flipped U-07; 4 findings closed, provenance recorded
- [x] **U-15 (P2-2)** — CMake no longer writes into `src/`: single
      `configure_file` into the build tree, template moved to
      `build_support/version.h.in`, generated-first include order, `core/version.h`
      includes switched to path form. New gate **C9** builds a copy with the
      committed fallback DELETED; the audit's read-only-`src/` repro flips
      FAIL→PASS (executed both halves, uid 65534). `build.sh` stays the only
      writer of the committed fallback; the G8 `build_support` allowance was
      deleted as promised.
- [x] **U-17 (P1-19)** — explode runs now verify their frames: new
      `src/core/ExplodeVerify.h` (snapshot `<prefix>.*` before, require ≥1
      new/changed real GIF after; failure names prefix+dir) wired into the CLI
      `--run` path (rc=1) and the GUI (`No frames produced…` + dialog). Lying
      engine (`fake_engine_exit0`) refused at every layer: unit test 33, smoke
      9→12 cases, harness T7 extended, Wine rc=1. Success reports the verified
      frame count.
- [x] **U-07 (P1-4)** — Windows Unicode execution (see the ticked line above).
- [x] **U-41 (P2-11)** — web demo grew all four desktop modes: scope line
      added FIRST, then mode selector + multi-file queue + results list in the
      UI, and `POST /run` (JSON multi-file) on the server with desktop
      semantics — per-file Auto batch runs with planned targets + collision
      refusal, one `-m` merge run, `-e`/`-E` explode with the P1-19 frame
      verification. Suites extended: command 15→**17**, validate 19→**23**,
      transport 18→**30**.
- [ ] **U-10 (P2-3)** — provenance and product-config relocation are DONE in
      S11/S13; row stays PARTIAL until CI hash-pinning lands (`workflows`
      scope). `reference_code/gifsicle` is now upstream-only at `07f5c4c3`
      (master, 5 commits past `v1.96`); `config.native.h` lives under
      `working_code/gifscythe/build_support/gifsicle/` and is staged as
      `config.h` in a temporary include directory. Native build, CLI/unit
      build, engine pipeline, and smoke 19/19 pass after the move. The
      four upstream deltas remain verified as commits `9efcc14`/`ed5b018`;
      `gifsicle-nested-1.96` remains pristine `v1.96` (`a08e0f66`). Updated
      digests + reproduce recipe are in `reference_code/REFERENCE_MANIFEST.md`.
      CI hash-pinning stays proposal-only (`workflows` scope).
- [ ] **U-12** — scoped in §6 as **P1-24** with all five waits, current line
      numbers, and the fix sketch; implementation deliberately NOT attempted
      (the freeze needs a slow process start, which the offscreen harness
      cannot produce; cancel rewiring would risk T2/T9/T10 semantics with no
      executable proof of improvement).
- [x] **N-06** — the check_docs.sh emitter's 150-char proof-note truncation was
      awk-locale-dependent (gawk counts characters, mawk bytes): the committed
      STATUS.md passed G0 under gawk but failed under mawk. Found when the
      sandbox restarted mid-session and lost gawk. Fixed the same session:
      `LC_ALL=C` in the gate + S11 §5 notes reworded ASCII-only under the
      limit; `--emit` verified byte-identical under BOTH awks.
- [x] **N-05** — multi-input Explode silently scattered frames (engine rc=0;
      every input but the LAST exploded into the CWD as `<basename>.NNN`, only
      the last honored the `-o` prefix). Found while designing the U-41 web
      explode rule; verified against the bundled 1.96; refused the same session
      at every layer: `validate()` warning (C++ + byte-identical JS mirror),
      CLI `--run` rc=2, GUI dialog + REFUSED summary; unit block 35, smoke
      case 12 (2 lines), harness T7 subcase, Wine rc=2.
- [x] **N-04** — found under Wine: this MinGW libstdc++'s narrow `fs::path`
      conversions decode bytewise but encode UTF-8 (asymmetric mangling of
      every non-ASCII path). Closed in-session: `u8path_compat`/`path_u8string`
      at every core boundary (`WinUnicode.h`).
- [x] Base-commit drift fixed on arrival: `SESSION_HANDOFF.md` +
      `COMPILED_AUDIT.md` named `414f5fc`; origin/main is `2176573` (the PR #13
      merge) — G10 green again.

### GIF UI/UX → 1.0.0  (S4b retrofit 2026-09-07 + S7 polish 2026-09-10 + S10/S11 fixes — harness T1–T20 last *measured* at **324 runtime checks** in the S11 sandbox (Qt 6.4.2); before that 306 in S10 and 243 in S7; S8's T17/T8 additions were CI-green on PR #11)
- [x] Input / Actions / Output tab flow (XNConvert feel) — QTabWidget + Preview
      pane in splitter; bottom live pane/progress/status bar kept
- [x] Before/after preview (debounced 1200 ms, fully async, seq-guarded;
      honest captions; savings readout; refuses in Explode mode)
- [x] File size/count display (row sizes + total label), output-folder actions
      (batch folder + Open folder via QDesktopServices)
- [x] Free-form naming templates — **S7**: Output-tab `Name template`, default
      `{name}_opt.gif` renders exactly the historical auto-name (E4 unchanged);
      separators stripped, `.gif` appended, constant-template multi-file
      collision REFUSED with dialog (S3-25, harness T16)
- [x] Expose remaining `GifsicleSettings` controls (~30 widgets, engine-truth
      value lists; harness T11 asserts each control → exact flag)
- [x] Queue reorder (move up/down) — **S7**: list+model stay index-aligned,
      selection follows, merge order = queue order (S3-9, harness T15)
- [ ] Optional: two-way CLI pane (`gs::parse_args`) **or** keep one-way forever
      (UI label now says one-way explicitly) — owner decision
- [x] **Persist GUI settings between sessions** — **S7**: SettingsIO-backed
      load/save (ctor/closeEvent) at `AppConfigLocation/gifscythe.conf`,
      override `GS_SETTINGS_PATH`; GUI keys `batch_dir`/`name_template`;
      queue + Save-as deliberately NOT persisted; warnings surfaced
      (S6 gap / audit row 15, harness T14)
- [x] Document release procedure — **S7**: `docs/release/RELEASE_PROCEDURE.md`
- [ ] Bump `VERSION.md` → **1.0.0** only after Windows CI green + desktop
      probes + the optional items above are decided (owner may take 0.2.0
      first per the minor-bump rule)

## Next actions (ordered)

- [ ] **Owner draft:** refit the web plan into `web/WEB_PLAN_TEMPLATE.md` (slot-by-slot; §0
      rules). On that commit do the **one-time flip** — `Template state:` here and the mirror line
      in `SESSION_HANDOFF.md` go to `WORKING PLAN` (G16 checks it) — then triage the 18 intake rows
      into `COMPILED_AUDIT.md` §6 fix-order ids.

1. **U-09** — re-cut release artifacts from the tagged SHA (the banked zip
   predates S7; its notes pin `d3544b1`). Needs a tag + `gh release` (and a
   token with write scope — the S10 token was read-only).
2. **Apply the pending CI change** — needs a token with the `workflows` scope.
   Steps + the follow-up doc-number refresh are in
   `docs/ci/PENDING_WORKFLOW_CHANGE.md`.
3. Clean-VM `windeployqt` smoke from the `gifscythe-windows` artifact (C4/D3/D4)
   — checklist: `docs/ci/CLEAN_WINDOWS_SMOKE.md`. **Blocked by U-09.**
4. One-time real-desktop GUI probes: B5 (kill engine mid-run), B6 physical
   drag-drop, B14 engine-missing GUI variant. (S10 note: the offscreen harness
   now covers busy-locking and preview invalidation, but these three still need
   a physical desktop.)
5. Owner decisions: two-way CLI pane **or** keep one-way forever; version
   (0.2.0 for the S7 feature set per the minor-bump rule, vs straight 1.0.0
   once 3+4 are green). Release how-to: `docs/release/RELEASE_PROCEDURE.md`.
   Audit release criterion: *no Critical/High open, package-negative tests green,
   clean-Windows smoke against the exact tagged SHA.*
6. The 2 remaining `OPEN` audit findings — **U-09** (release re-cut: needs a
   tag + release infra + write-scoped token) and **U-12** (UI-thread waits,
   scoped as P1-24 in `COMPILED_AUDIT.md` §6 — needs a testable async-start
   strategy before implementation) — plus the PARTIAL remainders: U-10 (CI
   hash-pinning, `workflows` scope; product config relocation is DONE in S13),
   U-14 (verify_audit in CI, `workflows` scope). U-18/P2-4 regression coverage
   is closed in S12; see `STATUS.md` for the remaining partial/open items.
7. WebP/APNG stay blocked until all of the above ships.

## Deferred bucket list — after GIF `1.0.0`

Tracked as **D-01…D-08** in `STATUS.md`. All `OPEN`: known, scoped, not started,
and deliberately not started until GIF 1.0.0 ships.

- Common RGBA animation frame model (timing, disposal, blend, alpha, canvas, loop).
- Animated WebP (libwebp AnimDecoder/AnimEncoder).
- APNG (libpng/zlib with APNG support).
- GIF ⇄ APNG ⇄ WebP convert, explode, merge, reorder, loop controls.
- Frame editor, text/watermark overlays, presets, richer previews.
- Optional: logging framework, i18n, dark mode, system tray (see COMPILED_AUDIT S3 P2/P3).
- **Web (product alternative; server-side chosen S14):** the self-hosted browser UI already
  exists (`web/`); optional client-side `gifsicle.wasm` + web UI
  (`docs/web/WEB_FEASIBILITY.md` Option 4) remains unbuilt.
- **Language migration (only if a trigger fires):** Rust + Tauri spike —
  see `docs/planning/OFFLINE_BUILD_REVIEW.md` §4.

## Build commands

```bash
cd working_code/gifscythe
./build.sh                 # engine + CLI + unit tests (also bootstraps git hooks)
./scripts/test_engine.sh
./scripts/smoke_cli.sh
./scripts/test_package.sh  # packaging negative suite
./scripts/check_docs.sh    # documentation gate — must be green before any PR
./scripts/check_docs.sh --emit   # regenerate STATUS.md from the repo
./scripts/verify_audit.sh  # whole COMPILED_AUDIT §6 suite + the doc gate (F1/F2)
./scripts/package_portable.sh
./scripts/package_system.sh
./scripts/build_engine.sh --windows   # needs mingw-w64
./scripts/bootstrap_hooks.sh          # make .githooks/pre-push live in this clone
# GUI harness (needs Qt6): cmake -S . -B build-cmake && cmake --build build-cmake
#   && QT_QPA_PLATFORM=offscreen ./build-cmake/test_gui_offscreen

# Web app (self-hosted product alternative; the CLI parity suites below are its gate)
node web/server.mjs 8000           # from the repo root; binds 127.0.0.1
node web/test/command.test.mjs     # JS ⇄ C++ command parity (17 checks)
node web/test/validate.test.mjs    # JS ⇄ C++ validation parity (21 checks)
node web/test/transport.test.mjs   # live-server transport net (30 cases)
```

## Do not

- Bump to 1.0.0 as a placeholder.
- Add WebP/APNG before GIF UI is stable.
- “Fix” `--loopcount=0`, `-O0`, crop plus-form, or gamma sentinel (verified correct).
- Link gifsicle into the GUI binary (keep subprocess for GPL v2-only vs GPLv3).
- Edit `reference_code/` (read-only).
- Hand-edit the generated block in `STATUS.md` — run `check_docs.sh --emit`.
- Push, open or merge a PR with a red `check_docs.sh`, or bypass the pre-push
  hook with `--no-verify`.
- Reintroduce the removed `scripts/build_gifsicle.sh` shim.
