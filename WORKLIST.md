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
   `working_code/gifscythe/scripts/pr_preflight.sh --online` (it runs
   `check_docs.sh` and `sweep_stale.sh`, and prints the repo/run/PR state),
   fix every failure, re-run until green. Do not create or merge with a
   failing check, and **do not ask whether to run it**. Anything labelled
   "after merge" is done **before** merging.
4. **It is also enforced mechanically.** `.githooks/pre-push` blocks a red push.
   Git does not copy hooks on clone, so run
   `working_code/gifscythe/scripts/bootstrap_hooks.sh` once per clone —
   `build.sh` does it for you. Verify: `git config core.hooksPath` → `.githooks`.
5. **`IMPROVEMENT_LOG.md` entries use the template** (`Changed / Partial / Left /
   Verified / Not verifiable here / Docs touched`). The **`Not verifiable here`**
   line is mandatory and must never be omitted or softened.
6. **HARD RULE — commit every edit/write/delete before merge AND before the
   session can close.** Uncommitted work is lost when the sandbox is cut off
   (it happened twice). Do not wait to be reminded. Gate **G18** fails
   `check_docs.sh` on a dirty tree; `pr_preflight.sh` **P3** fails create/merge
   on dirty and **P3b** fails if HEAD is ahead of origin. Push after you
   commit. At every new-session start, inspect `web/WEB_PLAN_TEMPLATE.md`
   §1–§10: leftover `<placeholders>` = stay **SKELETON**; filled content =
   flip both **G16** lines to **WORKING PLAN** in the same commit (one-way).
   The gate never auto-edits.

## Found this session — pending lines (rule 2)

Every `UNTRIAGED` row in `STATUS.md` must have a matching line here.

- [x] **External review intake (S14) — triaged in S15 (`OD-01 = a`).**
      18 findings (Max/GPT-class `GS-201…GS-210`, DeepSeek `DS-06…DS-13`) are compiled in
      `COMPILED_AUDIT.md` §13 + `docs/audit/EXTERNAL_REVIEW_INTAKE_2026-09-12.md`, and recorded
      row-by-row in `STATUS.md` under the reviewers' ids. **Triage is done**: the owner chose
      (a) all 18, and every row is now mapped to a `COMPILED_AUDIT.md` §6 fix-order id — 14 new
      ids and 4 folded into actions that already covered them. Unchecked lines below are the remaining remediation work, one line per finding, each now naming its §6 id.
      - [x] **GS-201** → **P0-5** — (Critical) CLI Batch → engine `-b` can rewrite the source GIFs; cheap
            stop-loss = refuse `--run` with Batch and no `output` key. **Closed S16:** CLI `--run`
            exits 2 with a named reason before the engine starts; smoke 21/21; source GIF untouched.
      - [x] **GS-202** → **P0-6** — **closed S17**: unsafe upload names refused with 400;
            every resolved target/prefix contained before engine launch; case/NFC collisions
            refused with 422. Transport 42/42; original server fails 7 security groups;
            outside-request sentinels unchanged with the fix.
      - [ ] **GS-203** → **P1-25** — **PARTIAL S17:** core/CLI explicit-file and web
            postconditions implemented (smoke 40/40, transport 67/67 on Linux); Qt lifecycle
            integration remains. See `docs/planning/SEQUENTIAL_WORK_HANDOFF.md`.
      - [ ] **GS-204** → **P1-26** — **PARTIAL S17:** both packagers share verified fresh staging,
            explicit targets/headless scope; 36 Linux checks pass (30 + 2 Ms-PL cases S18 + 4 Qt LGPL
            cases S19). Real Qt/Windows deployment, architecture and clean-machine verification remain;
            the release re-cut blocker is not waived.
      - [ ] **GS-205** → **P1-27** — (Med) non-GIF inputs still admitted via the picker and drop.
      - [x] **GS-206** → **P1-28** — **closed S23:** `std::from_chars` into the destination
            width replaces every `long`→`int` narrowing, and the domains landed in C++ + the
            JS mirror (loopcount 0..65535 after measuring that the engine wraps 65536 to
            "forever" at rc=0, color_method/resize_method against the engine's own lists,
            gamma shape). `dither_method` is deliberately NOT enum-checked; unit 21c pins that.
      - [x] **GS-207** → **P1-29** — **closed S17:** invalid non-empty `GS_ENGINE`
            refuses fallback (CLI print/run exit 1; web 503); source logged; empty/unset
            preserves discovery and `--engine` has precedence. Smoke 30/30, web 63/63 on Linux.
      - [ ] **GS-208** → **P2-7** — (High, PARTIAL-fixed) main release-red; docs corrected in S14, workflow-copy
            sync + marker deletion still open (needs a `workflows`-scoped token).
      - [ ] **GS-209** → **P2-12** — (Med) native linux/mac build uses a fixed glibc config.
      - [ ] **GS-210** → **P2-13** — **PARTIAL S17:** strict arguments fixed (12 cases pass).
            qmake-first dispatch and hardcoded .pro VERSION remain for a Qt-equipped agent.
      - [x] **DS-06** → **P0-2** — **closed S23:** tri-state — `<0` emits nothing, `0` bare
            `-j`, `>0` `-jN`, in `GifsicleCommand.h` + `web/command.mjs`, with the sentinels
            named (`GS_THREADS_UNSET/AUTO`) and unit + parity + smoke 12g proof. PR #28's
            `threads < -1` message documents exactly these semantics; this row is what makes
            it true.
      - [x] **DS-07** → **P1-30** — (Med) GUI threads spinner cannot express "no flag".
            **DONE S23** in the same edit as N-09: range `GS_THREADS_UNSET..64`, the minimum reads
            "Unchanged (engine default)", and the offscreen harness pins that a `-1` conf survives
            save/close/reopen as absence. Agrees with DS-06 as P1-30 required; CI-compiled only.
      - [x] **DS-08** → **P3-5** — **closed S23:** the advisory contract is documented in
            `--help` and a continued warned run ends with one greppable
            `WARNING-SUMMARY: parse=N validation=M …` line (after the strict refusal, so it can
            never describe a run that did not happen). Smoke 12m pins presence AND absence.
      - [x] **DS-09** → **P1-31** — **closed S22:** `threads < -1` now warns in
            native + web validation; unit, web parity, and CLI smoke pin `threads = -7`.
            S23 superseded the builder half of that pin: `-7` warns and emits **no** thread
            flag (it no longer aliases to "auto"), which is what P0-2 promised.
      - [ ] **DS-10** → **P3-11** — (Info) disposal 4..7 unreachable from the desktop picker.
      - [x] **DS-11** → **P2-14** — **closed S17:** S5/G17 checks OPEN vs closed and
            closed vs nonclosed current status; ignores historical tails. 20 regression tests pass.
      - [x] **DS-12** → **P1-13** — **closed S23:** quoting only where the value would be
            lossy (padding or a leading quote), `"`/`\` escaped, decoded in exactly one place
            (`set_field`) so it cannot apply twice. save(load(x)) is byte-identical; the one
            documented consequence — a hand-written `comment = "hi"` reads as `hi` — is pinned
            by an assertion rather than hidden. `examples/animation.conf` needs no change.
      - [x] **DS-13** → **P1-32** — **closed S17:** `/optimize` checks the response buffer
            for GIF87a/GIF89a before success; invalid signatures get JSON 422. Transport 53/53;
            the pre-fix server fails all 6 invalid-signature cases. Not full GIF decoding.
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
  **Reaffirmed S19 (2026-09-14, owner):** the exe stays C++17/Qt6, no
  rewrite; the C# shell is parked until 1.0.0 ships (`OD-C7 = park`).

## Current status

### Foundation (done)
- [x] P0 project setup and reference/working separation
- [x] GIF engine build and verification (`release/<ver>/gifsicle`, identity 1.96)
- [x] Qt-independent command/settings control layer
- [x] CLI driver + unit tests + integration smoke (`smoke_cli.sh`)
- [x] Qt6 GUI MVP scaffold (Batch default, mode combo, async run, queue, DnD)
- [x] Portable and system-dependent packaging scripts (`.exe` probe + licenses)
- [x] Linux GitHub Actions path with Qt6 (test battery; ships nothing since S20/OD-17)
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
      **30 PASS / 0 FAIL / 3 SKIP, exit 0** (S22 added W4/W5 for the missing
      web regressions; skips: E9 declared-pending workflow, CI-gated,
      clean-Windows; measured in the last full-toolchain checkpoint plus the
      S22 Node-web additions — a toolchain-less sandbox skips C6/C7*/C9/B as
      well; S11 added gate C9, the cmake source-tree-purity check for U-15).
      **S23 added W6** for its own three web suites, which is why the S23 sandbox
      reports **30 PASS / 0 FAIL / 6 SKIP** (that is 31 at the next full-toolchain
      run: this box loses C6/C7*/C9/B, W6 adds one)
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
- [x] Web app bounds: **U-06** — loopback default (S8) plus the three bounds S14 left open:
      one engine semaphore for both endpoints (`GS_MAX_CONCURRENT`/`GS_MAX_QUEUED`, 429 past
      it, taken AFTER validation so a refusal costs nothing), a per-client POST window
      (`GS_RATE_LIMIT_PER_MIN`, static exempt), and `GS_ENGINE_TIMEOUT_MS` replacing the hard
      120 s. Closed S23 by `web/test/server-bounds.test.mjs` (5 groups, real server per case).
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

### Session S16 (2026-09-13) — GS-201 / P0-5 stop-loss
- [x] **GS-201 (P0-5)** — CLI `--run` refuses Batch with no `output` (rc=2, named
      reason) before the engine starts. Engine `-b` rewrite confirmed (8703→8637 B);
      CLI left the source `cmp`-identical. Smoke **21/21**. Print still emits `-b`.
- [x] **N-07 triaged to P2-15** (OPEN at S16; closed S17) — S4 is a 5-phrase list, not a general
      reversal detector. Sweep not changed this session.
- [x] **SW-03** — uncommitted-work hard rule (**G18** dirty-tree FAIL in
      `check_docs.sh`; **P3b** unpushed FAIL in `pr_preflight.sh`) plus **G16**
      content-vs-token (never auto-edit). Template stays **SKELETON**. Rule 6
      written in the three files a new session reads. No PR until yes.

### Session S18 (2026-09-14) — C# shell plan, Phase 0 decided (docs only)
- [x] **Plan:** `docs/planning/CSHARP_SHELL_PLAN.md` is a `WORKING PLAN` — a WPF shell driving the unchanged gifsicle subprocess, parity-tested against the C++ CLI.
- [x] **Decisions:** OD-C1 = a (fork reference-only, MS-PL stays out of the tree), OD-C2 = c (phased: sidecar through Phase 2, commit at Phase 3), OD-C3 = a (no recorder; XNConvert-style converter+compressor, APNG/WebP promoted to planned, stills (photo collections) and video strictly as conversion endpoints, ezgif-class editing later), OD-C4 = a (WPF), OD-C5 = a (archive Qt GUI at cutover; C++ CLI stays as parity oracle).
- [x] **License (OD-09 = b, OD-C6):** first-party code relicensed to Ms-PL (`LICENSE` rewritten, `COPYING.ms-pl` added, packagers + CI require it); Caesium base dropped (never incorporated); OD-C1 reference-only superseded.
- [x] **Legal consolidation:** licence rationale single-sourced into `docs/legal/` (`README.md` index + maintenance contract, `WHY_MSPL.md`, `COPYING_RULES.md`); live docs repointed, history left verbatim.
- [x] **Next:** Phase 1 spike — GREEN 2026-09-14 (run `34804350470`, all 9 spike steps + publish + published-run success, zero skips). Verdict: Phase 2 GO; CJK criterion corrected to honest-fail per the engine-ACP residual.

### Session S19 (2026-09-14) — exe stays C++/Qt6 (park C#, close U-08) + wasm MVP scaffold (unproven)
- [x] **Direction (owner):** exe stays C++17/Qt6, no rewrite; C# shell parked (`OD-C7 = park`) until 1.0.0 ships — spike inert, plan `PARKED`, offline-review §4 reinstated.
- [x] **Licences:** Qt LGPL remainder closed — `COPYING.lgplv3` + `COPYING.gplv3` staged by both packagers, generated `QT_NOTICE.txt` in GUI packages, 36 packaging checks, CI manifest asserts the set (`U-08` DONE, `W-08` re-proved).
- [x] **Owner answers:** `OD-11 = a` (stay 0.1.0) and `OD-12 = a` (two-way CLI out) executed; `OD-16` added for the wasm in-process licence question (open, blocks shippable).
- [x] **Desktop evidence docs:** `docs/ci/CLEAN_WINDOWS_SMOKE.md` re-pointed at the CI artifact (the banked snapshot cannot validate the current tree — see the smoke doc); `docs/ci/DESKTOP_PROBES.md` written for the three W-19 probes. Both still await a real Windows run.
- [x] **Wasm MVP scaffold:** `web/wasm/` (emcc build script, one-screen UI reusing `web/command.mjs` + `web/validate.mjs` verbatim, byte-proof script, staged `COPYING.gifsicle`) — glue proven against the real engine, but no `.wasm` built (no emcc here) and `OD-16` open, so the Node server stays the shipped web path.
- [x] **Sync:** PR ledger + docs caught up through PR #23 (`8230247`); handoff base re-synced.

### Session S14 continuation (2026-09-12) — stale-claim sweep, PR preflight, owner-decision register (docs only)

- [x] **Stale-claim sweep:** `scripts/sweep_stale.sh` (rules **S1–S5**, each mutation-tested) +
      `check_docs.sh` gate **G17** — the sweep scans current-state docs for claims checkable only
      against reality and names `file:line` + the fix. Its first real catches: two undated
      "CI green" cells in `docs/planning/OFFLINE_BUILD_REVIEW.md` (now dated S6) and the U-06/U-08
      narrative "FIXED (S8)" lines (now ◐ PARTIAL, matching §5).
- [x] **PR/merge companion:** `scripts/pr_preflight.sh` (`--online`/`--body`) — P1 `check_docs.sh`,
      P2 `sweep_stale.sh`, P3 clean-tree, P4 repo/run/PR state, P5 PR body skeleton.
- [x] **Owner-decision register:** `docs/planning/OWNER_DECISIONS.md` — `OD-01`…`OD-15`, options +
      recommendation + what each unblocks.
- [x] **SkillOpt integration query:** `docs/planning/SKILLOPT_INTEGRATION_QUERY.md` — verified
      facts, the three non-negotiable conditions, four shapes, first-experiment candidate, open
      questions; plus `docs/planning/NEXT_SESSION_PROMPT.md` for the copy-paste hand-off.

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

### Session S23 (2026-09-16) — the Tier-1 batch: 10 findings closed, 1 opened for the owner

Chosen by *sandbox capability*, not by severity: every row here is provable with
g++ + node on Linux, because this box has no cmake, Qt6, mingw, Wine, emscripten
or package network. The list was built by reading `STATUS.md`, then measuring the
sandbox (engine + CLI + unit + smoke + the five web suites then present, green,
before any edit — eight after S23 added three)

- [x] **P0-2** threads tri-state; **P1-28** int-width parsing + validation domains;
      **P1-40 (loop half)** `loopcount = -2` → `--no-loopcount` (C++, JS mirror, the
      web Loop control). The sentinel values are named in `GifsicleSettings.h`.
- [x] **P1-13** settings values round-trip exactly (quoting only where lossy).
- [x] **P1-43** `resolve_path` CWD fallback announced and `--strict`-refused;
      Batch with N inputs → 1 output refused after *measuring* that the engine writes
      only the last input; explode prefix name unified.
- [x] **P1-41 (two of three halves)** symlink-safe `exe_path_of` (OS query, then
      PATH) and the `release/current` engine pin honoured by BOTH surfaces.
- [x] **P1-5** web concurrency / per-client rate / engine-timeout bounds.
- [x] **P1-36** superscript COM¹²³ aliases, tested from ONE table shared by the
      C++ unit suite and `web/test/device-names.test.mjs`.
- [x] **P3-5** advisory warning contract + greppable summary; **P3-12** the
      "what the engine can do that this layer does not model" table.
- [x] **P1-34 / U-54** and **U-69** — request ownership lifted out of `app.js` into
      `web/request-guard.mjs` so it can be tested at all (13 assertions), and
      `static-hygiene` now proves every `app.js` import is routable (a module added
      to the UI without a route 404s the page — this was one edit away).
- [x] **Review of my own merged tree before opening (N-09, fixed here):** `SettingsPanel.cpp`
      had no representation for two states this batch introduced, so *opening and closing a window
      rewrote them* — `loopcount = -2` displayed as "Keep original" and saved back as `-1`, and a
      `threads = -1` conf fell through `if (s.threads >= 0)` to the spinner's 0. Harmless before
      P0-2 (both meant a bare `-j`); a real silent rewrite after it, which is why it is recorded
      rather than quietly amended. Fixed with P1-30's own prescription plus a 4th `Looping` item,
      **appended** because the harness addresses items 1 and 2 by index. The web side needed no
      change, so this half could not be credited to #28. Both halves are CI-compiled proof only.
- [ ] **U-76 remainder → `OD-18`**: which directory an un-prefixed CLI explode writes
      to. The scoped wording was tried and wrote frames into `reference_code/`.
- [x] **PR #28 reconciliation** — **closed S23,** executed after #28 merged as
      `794a996`: both `smoke_cli.sh` case sets are kept (12b–12f + 12g–12o) and every
      count was re-measured on the merged tree (unit 372, smoke 54) rather than added;
      #28's unit block 21b was **re-pinned rather than dropped** — its
      `threads=-7 → bare -j` assertion describes the pre-P0-2 builder, so it now
      checks all three states and keeps its warning coverage; ONE copy of the
      `main.cpp` special-token helpers (#28's names) and of the `Validate.h`
      `threads < -1` rule survives; and §5's narrowed `U-67` **Finding** text is
      restored while #28's `FIXED (S22)` status cell stands.
- [ ] Not started here, deliberately: **P1-35** (U-55) — Windows case-folding cannot
      be verified without a Windows host, and a C++-side guess would refuse legal
      names; every Qt row; `U-68`'s cap value (documented, left as-is on purpose).

## Next actions (ordered)

- [x] **C# shell Phase 1 spike** (per `docs/planning/CSHARP_SHELL_PLAN.md` §4) — GREEN 2026-09-14 (run `34804350470`): console → repo-built gifsicle → GIF verify → honest 0/2/3/4/5 → single-file publish runs stock. é+space passes natively (product previously only Wine-proven); CJK fails honestly, re-proving the engine-ACP residual. **PARKED S19 (2026-09-14, owner direction — `OD-C7 = park`):** the exe stays C++17/Qt6, no rewrite; the spike stays CI-run and inert until 1.0.0 ships on the current stack. The S18 "Phase 2 GO" verdict is suspended, not deleted.
- [ ] **Owner draft:** refit the web plan into `web/WEB_PLAN_TEMPLATE.md` (slot-by-slot; §0
      rules). On that commit do the **one-time flip** — `Template state:` here and the mirror line
      in `SESSION_HANDOFF.md` go to `WORKING PLAN` (G16 checks it). The 18 intake rows were triaged
      in S15; **GS-201 closed S16, GS-202 closed S17**.
- [ ] **Review, don't accept:** run `scripts/review_change.sh --pr <n>` (or `--commit`/`--patch`)
      on any change before merging it. R1 flags edited check logic, R2 flags a matcher that
      matches nothing (how G10 stayed dead for five PRs), R3 flags prose counts that disagree
      with a live measurement, R4 flags lost `+x`, R5 lists the docs the change obliges you to
      update. Then write the outcome into the docs R5 named.
- [ ] **Owner answers** — `docs/planning/OWNER_DECISIONS.md` `OD-01`…`OD-16` (reply `OD-nn = <letter>`,
      from that row's own options; `OD-15` runs `a`–`d`; `OD-16` added S19 for the wasm licence question).
      **`OD-01 = a`, `OD-02 = a` (2026-09-12), `OD-09 = b`, `OD-11 = a` and `OD-12 = a` (2026-09-14) are
      answered; the other 11 are direction choices the plan can proceed without.
- [x] **N-07 / P2-15 — closed S17 (2026-09-13):** S2 now checks standalone
      numeric `UNTRIAGED` counts against the generated register, including Markdown
      emphasis/backticks and line wraps. Fourteen isolated regression tests pass;
      two stale-count probes demonstrably fail against the pre-fix sweep. S4 remains
      the five-phrase retired-web-scope check, not a general prose reversal detector.
      Unnumbered prose still needs review; this closes the scoped count-check action.
- [x] **Execute `OD-01 = a` — done S15 (2026-09-13):** all 18 intake findings now carry a
      `COMPILED_AUDIT.md` §6 fix-order id (14 new, 4 folded into existing actions) and are `OPEN`
      in `STATUS.md`. Gate **G12** is unblocked: with no `UNTRIAGED` row left, the `## S15` entry
      in `IMPROVEMENT_LOG.md` now passes it. Triage scoped the work; it fixed none of it — the
      remediation lines above stay open.
- [x] **Execute `OD-02 = a` — done S16 (2026-09-13):** CLI `--run` refuses Batch with no `output`
      (exit 2, named reason) before the engine starts; smoke 21/21 (source GIF `cmp`-identical;
      print still emits `-b`). The only code change the current decisions authorized.
- [ ] **SkillOpt** — after `OD-15`, add microsoft/SkillOpt per
      `docs/planning/SKILLOPT_INTEGRATION_QUERY.md` (shape A pinned submodule, quarantined; the
      three non-negotiable conditions apply), then register the doc-sweep skill experiment as its
      own item.
- [x] **Push + PR** — pushed and merged as PR #18 (`e32ed28`; `main` run `34713398377` green on
      linux + windows), via `scripts/pr_preflight.sh --online --body /tmp/pr_body.md`.

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
5. Owner decisions — answered S19 (2026-09-14): two-way CLI pane is **out**
   (`OD-12 = a`, one-way stays through 1.0.0); version stays 0.1.0 for now
   (`OD-11 = a`, not yet). 0.2.0 remains allowed first per the minor-bump
   rule. Release how-to: `docs/release/RELEASE_PROCEDURE.md`.
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
  (`docs/web/WEB_FEASIBILITY.md` Option 4) is scaffolded in `web/wasm/` (S19)
  but unbuilt and NOT SHIPPABLE until `OD-16` is answered — the Node server
  stays the shipped web path until then.
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
node web/test/request-guard.test.mjs   # U-54/U-69 run-ownership semantics (pure module)
node web/test/device-names.test.mjs    # U-56 vs tests/windows_reserved_names.txt (shared table)
node web/test/server-bounds.test.mjs   # U-06 bounds (starts its own server per scenario)
node web/test/command.test.mjs     # JS ⇄ C++ command parity (17 checks)
node web/test/validate.test.mjs    # JS ⇄ C++ validation parity (21 checks)
node web/test/transport.test.mjs   # live-server transport net (63 check groups on Linux, S17)
```

## Do not

- Bump to 1.0.0 as a placeholder.
- Add WebP/APNG before GIF UI is stable.
- “Fix” `--loopcount=0`, `-O0`, crop plus-form, or gamma sentinel (verified correct).
- Link gifsicle into the GUI binary (keep subprocess for GPL v2-only vs Ms-PL).
- Edit `reference_code/` (read-only).
- Hand-edit the generated block in `STATUS.md` — run `check_docs.sh --emit`.
- Push, open or merge a PR with a red `check_docs.sh`, or bypass the pre-push
  hook with `--no-verify`.
- Reintroduce the removed `scripts/build_gifsicle.sh` shim.
