# Worklist

**Version:** 0.1.0 · **Status register:** `STATUS.md` · **Audit detail:** `COMPILED_AUDIT.md`

> ## ⚑ Start with `STATUS.md`, not this board
> **`STATUS.md`** is the single status register: one row per tracked item in
> exactly one of four states — **DONE / PARTIAL / OPEN / UNTRIAGED** — with a
> generated header that says how much of everything the repo knows about is
> done. It answers *"what is left?"* without reading the 97-row audit register.
>
> This file is the **human task board**: what to pick up next, in order. It is
> not the status source of truth, and it must not contradict `STATUS.md`
> (`check_docs.sh` gate **G2** fails if a ticked box here maps to a non-DONE
> `U-nn` row there).
>
> Per-finding evidence stays in **`COMPILED_AUDIT.md`** §5 (the S8 remediation
> record is condensed in `docs/archive/AUDIT_HISTORY.md` entry 6). Session
> history is **`IMPROVEMENT_LOG.md`** — this board no longer duplicates it
> (S24 consolidation).

## The rules that keep the docs true (S9 — do not skip these)

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

- **N-31** (S33, found by the post-merge sync CI red): G11's non-doc-date
  measurement degenerates in a shallow clone (depth-1 tip appears to add
  every file → the log is compared against the checkout date → daily
  false red). **Fixed same session** — G11 now skips honestly in shallow
  clones (G10's precedent) and still enforces on full history; reproduced
  failing-first in a local depth-1 clone.

- **N-30** (S33 registered, **S34 closed — DONE**): Windows CI red while linux
  passed. S33 read it as toolchain provisioning ("code refuted as the cause",
  "pin a modern MinGW") — that was **wrong**. The first un-masked Windows run
  (S34, run 36966494878) showed g++ 13.1.0 from `tools_mingw1310` (the aqt
  chain's *first* choice), both compiles succeeding, and the unit exe itself
  failing: the integer probes in `SettingsIO.h` classified values with a
  `long` (4 bytes on Windows), so 2147483648..4294967295 were refused there
  while Linux accepted them (N-27 was incomplete on Windows). Fixed test-first
  (`to_llong` + unit block 37; RED 7 failures on a 32-bit-`long` build, GREEN
  on both widths) and proven by Windows CI (run 36967608254, every Windows
  step green). **Do not pin MinGW or touch the provision step on N-30's
  account.** The check that would have caught it is committed:
  `scripts/test_unit_32bit_long.sh` (unit suite on a 4-byte-`long` target via
  zig). **Wired into CI (S34, the owner's call: two extra minutes is acceptable when it
  helps verify the project):** the `portability` job runs it beside the other jobs, so it
  adds no wall-clock time.

- **N-26** (S31 registered, **S34 closed — DONE**): not a flake. All 8 red
  main runs since 2026-09-22 failed linux at the *same* step, "Documentation
  status gate" — G10/G11 verdicts that depended on the checkout depth and the
  UTC date (reproduced deterministically in CI-shaped depth-1 clones). Fixed
  by N-31 (S33) + the full-history `docs` job and the gate leaving the linux
  job (S34); run 36967608254 has every job green. Evidence in the STATUS row.

- **N-32** (S34, **PARTIAL**): the wasm track's documented proof bar (output
  byte-equal to the native oracle) could not be met by a build with a different libc, and
  `prove_wasm.mjs` never compared bytes. **Decided and implemented S34 (the owner delegated
  the call): the bar is a same-libc native oracle** — wasm32-wasi must be byte-equal to a
  musl-native build of the same sources. Enforced in CI (the `portability` job runs
  `scripts/libc_parity/libc_parity.py --bar`: 9/9; `--bar --against glibc`, the old bar,
  fails 6 of 9) and in `prove_wasm.mjs --oracle` (tests: `test_prove_wasm_oracle.py`,
  `test_libc_parity_bar.py`, both mutation-checked). Rejected: a stable-`qsort` + fixed
  `random()` shim in every build (the shipped Windows engine is built from the read-only
  upstream `win32cfg.h`, which our config headers do not reach) and pixel equality (4 of
  the 6 differences are different pixels). **Still missing:** an Emscripten build run
  through `prove_wasm.mjs --oracle` — emcc is unobtainable here, and OD-16 blocks
  shipping the track either way. Do not chase byte parity with the glibc oracle. Detail
  in the STATUS row.

- **N-33** (S34, found and fixed the same session): the README's honesty
  summary was stale in all three claims — rewritten from live state.

- **N-34** (S34, **DONE**): GitHub annotated every run with "The `ubuntu-latest`
  label will migrate to Ubuntu 26 beginning October 19, 2026" (a gradual 24.04 → 26.04
  move through 2026-11-19), and the `linux` job installs Qt through apt. **Pinned
  (the owner's call, S34):** `runs-on: ubuntu-24.04` in all four Linux jobs of both
  byte-identical workflow copies. Moving to 26.04 is now a deliberate edit of the four
  labels, trialled on a branch first.

- **N-35** (S34, found and fixed the same session): CI's `linux` job went red once on a
  docs-only commit (run 36985082450) at the web-suite step, annotation "exit code 1". Root
  cause: a race in the **test helper** — `server-bounds.test.mjs`'s `startServer` declared the
  server up on the first startup-banner line and U-66 read the log before the `Engine [...]`
  line arrived (1 failure in 15 locally). Fixed test-first (wait for the last banner line; a
  deterministic regression group, RED 5/5 on the old rule; 45/45 clean after) and the step now
  names a failing suite as an annotation, so the next red names itself. Detail in the STATUS row.

- **N-36** (S34, **DONE**): the owner's direct commit to `main` (`fe4f0a7`, "Deleted
  shot_actions_tab.png") removed every image file — the 3 docs screenshots and the 4 gifsicle
  test GIFs — and the GIFs are test fixtures (engine tests, smoke, oracle and so the pre-push
  hook, glue harness, libc probe, Windows E2E smoke, C# spike, Qt harness), so `main`'s CI went
  red. The owner skipped the question asked, so the option that respects the deletion was taken:
  **the repo stays binary-free** — the two upstream test images are base64 text under
  `working_code/gifscythe/tests/fixtures/` (sha256 in `SHA256SUMS`), decoded on demand into
  `build/fixtures/` by `scripts/fixtures.sh` / `fixtures.mjs`, byte-identical, every consumer and
  CI step repointed. **Do not re-add images or `reference_code/gifsicle/*.gif`**; add a text
  fixture instead. The Windows/C#/Qt steps are proven only by CI. Detail in the STATUS row.

- **CI run shape** (S34, the owner's calls — not findings, so no register row): a `gate`
  job skips a *push* run when an open, mergeable PR exists for the branch (the PR's own
  run tests that commit), plus workflow-level `concurrency` (a newer push to the same
  branch or PR cancels the older run of the same event; main is never cancelled).
  Trade-off: **to cite a specific commit's run as evidence, let it finish before pushing
  again** (seen live: the cancelled run took ~1.5 min to wind down, and the newer run
  waited `pending` meanwhile). Details: `docs/ci/README.md` §1; table: `tests/test_ci_gate.py`.

**Gate/register freeze (U-89 / P2-22, effective S33).** While the derived
release bar in `STATUS.md` (open P0/P1 fix-order ids) is above zero: add **no
new gates and no new registers** — extend `check_docs.sh`'s existing gates and
the existing register blocks instead; anything newly discovered goes in an
`UNTRIAGED`/pending row per rule 2 and is committed under **G18** discipline as
usual. The freeze lifts itself when the derived bar reads 0 (no hand-editing:
`check_docs.sh --emit` owns the number).

Every new finding gets a matching line here. Nothing is `UNTRIAGED` right now:
the S24 intake was triaged in the same session (each row carries its §6 id).

### S33 (2026-10-01) — register mechanics + doc-machine cost (the provable lane)

**U-88/P2-21 DONE** and **U-89/P2-22 PARTIAL** — the derived fix-order block
(P-id members from §6 incl. non-U ids; state any-OPEN→OPEN / any-PARTIAL→
PARTIAL / else DONE; MISSING members surfaced), the derived release-bar line
(open P0/P1 + offscreen-only DONE rows), `; harness:`/`; desktop:` proof
markers that survive word-boundary truncation, `verify_audit.sh --json` +
sha256 digest, and the gate/register freeze rule (above). Mutation-tested
(M1 state derivation + bar move, M2 counter move, M3 hand-edit → G0 red).
Register 149/9/30/0 → **150/10/29/0 = 189** (+N-30, found and registered
same session). Detail in `IMPROVEMENT_LOG.md`'s S33
entry. **Lane close-out (same session):** `src/core/GifsicleCommand.h` (the
named remaining core file) read line by line — **clean**; U-96/U-91 non-Qt
halves **assessed** with exact remainders recorded in their §5 rows
(Qt-probe-blocked / dotnet-blocked — not forced). N-18's tail is now just the
risk-scanned PENDING files; U-89's remainder (CI-artifact publication +
register quoting the digest; U-14 tension to surface, not paper over) is the
lane's one carried-open item.

### S32 (2026-09-29) — audit/review close-out lane

Path call of record: the owner's planning report item 1 was stale (already
done at S28/S30/S31) and its items 2–7 are owner/hardware/Qt-blocked, so this
session took the only lane where audit/review tasks were still OPEN and
provable in this sandbox. Full detail in `IMPROVEMENT_LOG.md`'s S32 entry.

- [x] **N-25** — rate-limit map unbounded growth. **DONE** — limiter extracted
      to `web/rate_limit.mjs`, whole-map prune per call, server-bounds
      10 → 12 groups, mutation-tested both directions.
- [x] **N-27** *(found this session, N-18 sweep of SettingsIO.h)* — the
      position-pair probe narrow-cast long→unsigned (GS-206 class alive in the
      bypassed helper): `position_x = 4294967296` probed valid, the real parser
      refused, and the pair went live half-formed (`-p 0,5`). **DONE** —
      width-strict probe + decoded value, 4 assertions RED→green.
- [x] **N-28** *(suspicion this session, REFUTED by execution — the id is kept
      so the numbering reads continuously)*: "to_double accepts nan/inf and
      gamma junk reaches the engine" — libstdc++ rejects nan/inf (warns "not a
      number"), and `validate()` already warns "gamma=nan: must be srgb, oklab
      or a number" with `--strict` refusing. No code change.
- [x] **N-29** *(found this session, same sweep)* — dither OFF + remembered
      method saved as `dither = <m>` (loads as dither=TRUE): a deliberately
      unchecked dither resurrected itself on reload. **DONE** — the off+method
      state now saves as `dither = false` + `dither_method = <m>`.
- [x] **Doc drift repaired** — the S31 tails that still called N-10..N-17
      open (and misled the owner's planning report) corrected in place; review
      documents folded (`COMPILED_AUDIT` §21, `docs/planning/PLANNING.md` §6,
      `docs/archive/` rows 8–9).
- [x] **N-26 (DONE S34)** — not a flake: the doc-gate step failed every red main
      run, deterministically (see the pending-lines section above).

### S31 (2026-09-28) — verification of the 2026-09-27 external audit

The external audit was re-checked line by line against a real checkout. Its
target tree and this HEAD differ only by the two review files, so every claim
was checkable; **20 of 22 checkable claims held**, one mechanism was wrong (and
worse than reported), and one supporting claim was refuted. Full write-up:
`docs/archive/AUDIT_VERIFICATION_2026-09-27.md` (moved from the root S32).

**Fixed this session (DONE rows, each with a mutation-tested proof):**

- [x] **N-19** — validator/builder coercion seam + the empty-gamma TypeError.
      `web/command.mjs` now reads every numeric setting through one `numArg`
      helper, so a JSON `"5"` builds the same argv as `5`. The audit left
      `web/server.mjs` unread and said that one file decided MEDIUM vs LOW: it
      does **not** normalise types, so the seam is reachable over HTTP and
      MEDIUM stands.
- [x] **N-20** — gate E3's "never" keyword escape hatch (any line containing
      the word never was exempt from the no-shell check).
- [x] **N-21** — duplicate gate ids `A1` **and `A5`** (the audit found A1 only).
- [x] **N-22** — two code comments that state the opposite of the code.
- [x] **N-23** — the handoff bullet that pointed U-59/P0-7 at the wrong PR.

**Corrected S32 (2026-09-29):** the block that used to sit here ("Open rows,
excluded from this session because no fix here could carry executed proof")
was a *pre-fix draft* that S31 never updated after PR #6 closed those rows —
and the engine was buildable after all. That stale list is what
`docs/planning/PLANNING.md` §6.1's original item 1 was written against. Live state is
`STATUS.md`; the closures, with proof, ticked here to match:

- [x] **N-10 (HIGH, data loss)** — DONE: CLI two-phase frame write, promote
      only after verify (no-output CWD case included), mutation-tested smoke
      case; GUI cancel honesty asserted by harness T21 (CI-compiled).
- [x] **N-11 (MEDIUM, data loss)** — DONE: a verified partial is KEPT on
      promote failure (its path is named) and on cancel-after-exit ("Run had
      already finished - result kept.").
- [x] **N-12 (HIGH, gate integrity)** — DONE: B1-B20 parses the harness's
      block lines and check count instead of trusting the final banner.
- [x] **N-13** — DONE: E7 renamed E7-doc (doc literal; harness T11 is the
      behavioural check it now names).
- [x] **N-14** — DONE: `DOC_GATE_CHECKS` is derived from the gate block and
      published on a `GATE_TOTALS` line.
- [x] **N-15** — DONE: the CLI snapshots the partial, exactly as the GUI does.
- [x] **N-16** — DONE: T4's bound matches the code's 5000 ms budget, and the
      message names the right function.
- [x] **N-17** — DONE: `info` parity fixture added, mutation-tested both ways.

Still open from the audit intake (state lives in `STATUS.md`):

- [ ] **N-18 (PARTIAL)** — 39 files the external audit never opened. The
      three named resume targets are now read (S32: `SettingsIO.h` found
      N-27/N-29; `SettingsPanel.cpp` data paths clean; `test_gifsicle_command.cpp`
      vacuity-scanned). The rest stay risk-scanned only — read on touch.
- [x] **N-25 (DONE S32)** — `requestWindow` bounded by live traffic now
      (see the S32 section above).
- [x] **N-26 (DONE S34)** — registered as "CI's linux job is flaky" (5 of the last 7
      main runs failed linux while windows passed); the jobs API names the step —
      the doc gate, every time — and the cause is G10/G11 under depth-1 checkouts.
      Fixed S33/S34 (N-31 + the full-history `docs` job); run 36967608254 is green.

- [x] **U-97** (found AND fixed in S27 — rule 2's strong form): the 2026-09-22
      zip re-creation of the GitHub repo lost the root license set
      (`COPYING.ms-pl` / `COPYING.lgplv3` / `COPYING.gplv3` / `COPYING.gifsicle`),
      so both packagers fail closed — that is where CI windows "Package
      portable" died on all three runs of the new main (executed local repro:
      `ERROR: COPYING.ms-pl missing or empty`, exit 1). Restored from canonical
      sources (URLs + sha256 digests in the S27 log entry); repro flipped to
      `Package created` exit 0; the repair PR's CI packaging steps are the
      platform proof.

- [x] **External review intake 2026-09-16 (four uploaded files) — incorporated
      and closed out in S24.** All 54 external findings dispositioned in
      `COMPILED_AUDIT.md` §20 (20 new register rows — 4 already fixed, 16
      OPEN and scoped; the rest already-tracked, already-fixed or refuted with
      evidence). The four root files were deleted after incorporation; full
      texts live in git history at `3c67e14`. Fixed at/before intake (no new
      work implied): **U-77**, **U-79**, **U-80** (PR #30) and **U-82** (S24).
      The new OPEN rows, one line each, in §6 order:
      - [x] **U-78 + U-87** → **P1-44** — **DONE (implemented S25 / PR #32,
            proved + closed S26).** Web numeric honesty: the validate.mjs
            finite-number gate (a wrong type is a named 422, never 200 ok:true
            with the setting silently dropped) + `numOrNull()` so an emptied
            field means OMIT, not `Number("")`→0. S26 added the three fixture
            batches the row asked for and mutation-tested them: wrong-type
            pinned against the REAL CLI for seven integer keys (its counterpart
            is the conf parser's `not an integer` + `--strict` rc=3), explicit
            zero pinned as parity, empty-vs-zero pinned over HTTP (transport
            72 → 79 cases).
      - [x] **U-95** → **P1-45** — the published Release predates the Ms-PL
            relicence: owner release-notes edit (mark superseded/pre-release,
            never delete), then the P0-4 re-cut + tag-triggered asset gate.
            **DONE (S33)** — moot by verification: the same releases API check
            that confirmed the defect (S24) now returns 0 releases on the live
            repo (created 2026-09-22; the re-creation dropped the old
            snapshot with it) — nothing to mark. The re-cut + deferred
            tag-triggered asset gate are recorded on P0-4.
      - [x] **U-81** → **P1-46** — explode frame verification ignores the
            stream-output/--info exemptions the ordinary verifier documents;
            `output = -` explode downgrades an honest run to rc=1.
            **DONE (S28)** — measured first: the intake's repro was wrong on both halves (`-e -o -` scatters `<input>.NNN` into the CWD, it does not stream; `--info` + explode is refused by the ENGINE itself). `verify_explode` now carries the `verify_file` exemptions and explode + `output = -` is refused rc=2 with nothing scattered (smoke 61/61).
      - [x] **U-94** → **P2-18** — **DONE (S29):** seeded offline oracle-fuzz gate against the REAL engine. 64 full cases / 24 quick-prefix cases compare JS argv to the C++ CLI, validator decisions to strict mode, and accepted/refused behavior to gifsicle; verified outputs + committed matrix in `working_code/gifscythe/tests/oracle_fuzz_matrix.json`. Wired as verify_audit W7 + Linux CI full run + pre-push quick run.
      - [x] **U-92** → **P2-19** — web upload admission: GIF magic on the
            decoded buffer + strict base64 (GS-205's web twin).
            **DONE (S28)** — strict base64 + GIF-magic admission on both endpoints before any engine discovery (transport 79 → 86 cases, plus 2 in body-limit).
      - [x] **U-93** → **P2-20** — web transport bounds: capped stderr echo,
            output envelope, favicon 404.
            **DONE (S28)** — `GS_MAX_STDERR` cap with a disclosed truncation marker, `/favicon.ico` 204 + data-URI icon, `/run` output envelope documented (server-bounds 10/10).
      - [x] **U-88** → **P2-21** — register mechanics: derived P-id state +
            harness:/desktop: proof markers + derived release-bar counters.
            **DONE (S33)** — derived part-3 P-id block + release-bar line in `STATUS.md`, word-boundary truncation that keeps markers, M1/M2/M3 mutation proofs.
      - [ ] **U-89** → **P2-22** — doc-machine cost: --json/digest instead of
            hand-typed counts, truncation revisit, gate freeze until P0/P1 empty.
            **PARTIAL (S33)** — `verify_audit.sh --json` + sha256 digest done, truncation revisited, freeze rule landed; remainder: CI-artifact + register quotes the digest (U-14 tension, owner-visible).
      - [x] **U-85** → **P3-13** — PORT validation (named error + exit 2, not
            a raw RangeError stack).
            **DONE (S28)** — PORT validated once: named reason + usage line + exit 2, no RangeError stack (4 server-bounds cases).
      - [x] **U-84** → **P3-14** — /optimize reads the body before discovering
            the engine (413 contract parity with /run).
            **DONE (S28)** — `/optimize` reads and admits the body before `findEngine()`, so an oversized upload to an engine-less server is 413 on both endpoints, not 503 on one.
      - [x] **U-83** → **P3-15** — pin single-input batch+output engine
            semantics with a smoke case (or refuse it until pinned).
            **DONE (S28)** — pinned with a smoke case: the `-o` target is written and the source stays byte-identical.
      - [x] **U-86** → **P3-16** — three comment-vs-behavior mismatches
            (collision message, expand_home bare-~, run() double-resolve).
            **DONE (S28)** — all three comment/message truths corrected, and `run()`'s double-resolve is now an explicit `settleOnce` guard; the two transport assertions that pinned the old wording were re-pinned, not deleted.
      - [ ] **U-91** → **P3-17** — one shared exit-code contract (CLI vs C#
            spike collide on 3) + the spike's ReadExactly/quoting port traps.
      - [ ] **U-96** → **P3-18** — stemOf dotfile/extensionless parity vs Qt
            completeBaseName (probe + one shared helper + fixtures).
      - [ ] **U-90** → **P3-19** — register the parked-plan scope (stills →
            animated, video endpoints) as gated rows + the PROJECT_VISION
            amendment proposal; no code until the owner adopts the words.
- [x] **CI/infra (S14 → resolved S24):** the documentation status gate is LIVE
      in CI (maintainer-applied; **S34: now its own `docs` job**, see
      `docs/ci/README.md` §1); the last declared drift was the
      proposed copy lagging the maintainer's live cygpath fix, so S24 re-synced
      `docs/ci/build.yml.proposed` to the live line and deleted the pending
      marker in one commit — E9/G7/S1 enforce byte-equality again, and
      W-30/R-03/GS-208 closed with it.
- [x] **Older session findings** (N-03 screenshots, N-04 MinGW paths, N-05
      multi-input explode, N-06 awk-locale emitter, N-07 count sweep, N-08 G11
      date rule, N-09 settings-panel sentinels) — all closed; detail in
      `STATUS.md` + `IMPROVEMENT_LOG.md`.

## Direction decisions (2026-09-09 → S20; detail in `docs/planning/PLANNING.md` §1)

- **Offline-only.** No cloud, no auto-update, no telemetry. **Amended S14
  (owner):** the `web/` build is a **supported, self-hosted product surface**
  (loopback default, `GS_WEB_HOST` for LAN). Plan + split rules:
  `web/WEB_PLAN_TEMPLATE.md`.
- **Language: stay C++17 + Qt6 Widgets through 1.0.0**; revisit only on a
  documented trigger (`STATUS.md` D-08) — then spike Rust + Tauri. **Reaffirmed
  S19 (owner):** exe stays C++17/Qt6; the C# shell is parked (`OD-C7 = park`,
  resume point `docs/planning/PLANNING.md` §2). S24 note: an external review
  argues the web-tech trigger already fired — that is an owner re-decision
  (recorded in `COMPILED_AUDIT.md` §20.4), not a session call.
- **Windows-only ship (S20, `OD-17 = a`):** Windows exe + web app ship; Linux
  is the CI/sandbox test battery and ships nothing.

## The road to 1.0.0 (ordered; states live in `STATUS.md`)

1. **U-59 / P0-7** — cancel/failure must never leave a truncated file over a
   pre-existing good output (temp+rename on verified success). The only
   registered data-loss row; fix before anything else. **S28: the CLI/core half
   landed test-first** — the engine writes `<target>.gs-partial` and the target
   is renamed onto only after verification, so a cancel, a signal or a refusal
   leaves the previous bytes intact (smoke 54 → 58/58, three cases run RED
   first; `test_output_verify.sh` 25 assertions). **Remaining: the Qt half** —
   **S30: GUI guard implemented, pending Qt-enabled PR CI.** Batch/Merge/Auto
   now write beside the destination to `<target>.gs-partial`, verify before
   promotion, and discard partials on failure/cancel; offscreen T4/T8/T9 cover
   success, partial-writing failure, and cancellation while preserving an
   existing output. **DONE S30:** Qt-enabled PR CI run `36227237540` passed
   Linux and Windows builds and both GUI offscreen suites. **Separate follow-up
   task for the next agent:** independently review the change and rerun the
   offscreen suite if that agent has CMake + Qt6; report findings without
   merging absent owner approval.
2. **U-09 / P0-4 + U-95 / P1-45** — re-cut release artifacts from one exact
   tagged SHA; the owner marks the pre-relicence `snapshot-2026-09-07` release
   superseded in its notes (never delete — rollback policy).
3. **W-18 clean-Windows smoke + W-19 desktop probes** — real machine, published
   artifact: `docs/ci/README.md` §2–§3. Nothing in a Linux sandbox can close
   these.
4. **GS-203 / P1-25 GUI half** — wire the shared output verifier into the Qt
   run lifecycle (execution detail: `docs/planning/PLANNING.md` §4).
5. **The Qt/platform rows** — U-12 (P1-24 async state machine), U-59's GUI
   half, DS-10, GS-205, and the three rows S35 moved to PARTIAL: U-58 (P1-38
   batch settings snapshot), U-70/U-72 (P1-42). **S35 landed the source fix for
   all three but could not prove any of them** (no cmake/Qt6 in the sandbox), so
   each row names the behavioural case that is still unwritten — write those
   cases on a Qt-enabled runner before flipping them to FIXED. **U-71 (P2-17) is
   CLOSED (S35)**: GN-14 was right that the mask rule is a pure function, so it
   needed no VM — `scripts/test_u71_exit_codes.sh` ran green at both `long`
   widths (host c++, and `x86-linux-musl` where `sizeof(long)==4`) plus native
   `g++` on the windows job. **U-57 (P1-37) is CLOSED (S35)** on
   `node web/test/u57-stale-output.test.mjs`; the wasm TRACK itself stays not
   shippable (OD-16 / N-32) — closing a correctness finding is not a promotion.
6. **The S24 web-intake batch** — P1-44 first (U-78/U-87, closed S26), then the
   P2/P3 rows above; all are provable with node alone except where noted.
   **S28 closed the node/CLI batch**: U-92/P2-19, U-93/P2-20, U-85/P3-13,
   U-84/P3-14, U-86/P3-16, U-81/P1-46 and U-83/P3-15. S29 closed U-94/P2-18
   with the seeded engine-oracle matrix. The remaining doc-machine rows are
   U-88/P2-21 + U-89/P2-22 (they edit gate logic, so they are R1 work and need
   mutation testing).
7. **Owner decisions** — `docs/planning/OWNER_DECISIONS.md`: remaining
   OD-03…OD-07, OD-10, OD-13…OD-16, OD-18 (OD-16 blocks `web/wasm/`;
   OD-18 blocks closing U-76). Version decision (0.2.0 vs 1.0.0) last, per
   `OD-11 = a`.
8. WebP/APNG stay blocked until all of the above ships (D-01..D-04).

## Deferred bucket list — after GIF `1.0.0`

Tracked as **D-01…D-08** in `STATUS.md`. All `OPEN`: known, scoped, and
deliberately not started until GIF 1.0.0 ships.

- Common RGBA animation frame model (timing, disposal, blend, alpha, canvas, loop).
- Animated WebP (libwebp AnimDecoder/AnimEncoder).
- APNG (libpng/zlib with APNG support).
- GIF ⇄ APNG ⇄ WebP convert, explode, merge, reorder, loop controls.
- Frame editor, text/watermark overlays, presets, richer previews.
- Optional: logging framework, i18n, dark mode, system tray.
- **Web client-side wasm** (`web/wasm/`, D-07): scaffolded S19, unbuilt, and
  NOT SHIPPABLE until `OD-16` (in-process licence — `docs/legal/README.md` §3)
  + a real emcc proof (the bar is a same-libc native oracle, decided S34 — N-32;
  no emcc build has been run through it). The Node server stays the shipped web path.
  History of the option analysis: `web/README.md` §History.
- **Language migration (only if a trigger fires):** Rust + Tauri spike —
  `docs/planning/PLANNING.md` §1 triggers.
- **Stills → animated / video endpoints** (owner request 2026-09-14, parked
  plan scope): gated by U-90/P3-19 — a PROJECT_VISION amendment + decoder +
  licence story must land first.

## Build commands

```bash
cd working_code/gifscythe
./build.sh                 # engine + CLI + unit tests (also bootstraps git hooks)
./scripts/test_engine.sh
./scripts/smoke_cli.sh
./scripts/test_package.sh  # packaging negative suite
./scripts/test_unit_32bit_long.sh   # unit suite on a 4-byte long (N-30 class); needs zig: pip install ziglang
python3 tests/test_pr_preflight_p6.py   # P6 (docs-synced-through) regression, isolated temp repo + fake gh
python3 tests/test_ci_gate.py           # the push-vs-PR dedupe gate's decision table (fake gh)
python3 tests/test_libc_parity_bar.py   # the wasm bar's verdict logic (no zig needed)
python3 tests/test_prove_wasm_oracle.py # prove_wasm.mjs --oracle with a fake module (needs node + a built engine)
python3 scripts/libc_parity/libc_parity.py --bar   # N-32 bar: wasm32-wasi == musl-native, byte for byte (zig + node + Pillow)
python3 scripts/libc_parity/libc_parity.py --check # N-32 finding: glibc vs musl vs wasm output parity
node scripts/lint_workflow.mjs          # actionlint (wasm build from npm) over .github/workflows
./scripts/fixtures.sh                # rebuild the upstream test images (text -> build/fixtures/); build.sh does it
./scripts/sim_postmerge.sh [--style squash] [--date "YYYY-MM-DD HH:MM"]   # will main's docs job pass after this merges?
./scripts/check_docs.sh    # documentation gate — must be green before any PR
./scripts/check_docs.sh --emit   # regenerate STATUS.md from the repo
./scripts/verify_audit.sh  # whole COMPILED_AUDIT §6 suite + the doc gate (F1/F2)
./scripts/package_portable.sh
./scripts/package_system.sh
./scripts/build_engine.sh --windows   # needs mingw-w64
./scripts/bootstrap_hooks.sh          # make .githooks/pre-push live in this clone
# GUI harness (needs Qt6): cmake -S . -B build-cmake && cmake --build build-cmake
#   && QT_QPA_PLATFORM=offscreen ./build-cmake/test_gui_offscreen

# Web app (self-hosted product surface). All nine suites run in the CI linux
# job and in verify_audit.sh W1-W6; quote the RUNTIME counter of the run you
# just executed, never a number from a doc:
node web/test/command.test.mjs        # JS ⇄ C++ argv parity (needs the built CLI)
node web/test/validate.test.mjs       # JS ⇄ C++ validation parity + U-78 wrong-type (needs the built CLI)
node web/test/numeric-honesty.test.mjs # U-78/U-87 empty-vs-zero + wrong type (P1-44; engine-free)
node web/test/transport.test.mjs      # live-server HTTP net (needs a discoverable engine)
node web/test/body-limit.test.mjs     # U-68 413 mapping (engine-stub, no real engine)
node web/test/static-hygiene.test.mjs # U-67 allow-list + HEAD contract
node web/test/request-guard.test.mjs  # U-54/U-69 run ownership (pure module)
node web/test/device-names.test.mjs   # U-56 shared reserved-name table
node web/test/server-bounds.test.mjs  # U-06 concurrency/rate/timeout bounds
node web/server.mjs 8000              # from the repo root; binds 127.0.0.1
```

## Do not

- Bump to 1.0.0 as a placeholder.
- Add WebP/APNG before GIF UI is stable.
- “Fix” `--loopcount=0`, `-O0`, crop plus-form, or gamma sentinel (verified
  correct — `COMPILED_AUDIT.md` §15.1 VP-1..VP-5).
- Link gifsicle into the GUI binary (keep subprocess for GPL v2-only vs Ms-PL).
- Edit `reference_code/` (read-only; the manifest is the one exception).
- Hand-edit the generated block in `STATUS.md` — run `check_docs.sh --emit`.
- Leave reusable tooling only in `/tmp` (wiped between turns; `build/`, `dist/`, `.venv` are not
  snapshotted either): commit it under `scripts/` or `tests/`, or write down the exact recipe.
  `/tmp` is for one-time scratch.
- Open a PR that only copies the previous PR's facts (a "post-merge sync"): a PR pre-syncs
  itself after `gh pr create` (see the handoff's maintenance rule).
- Push, open or merge a PR with a red `check_docs.sh`, or bypass the pre-push
  hook with `--no-verify`.
- Reintroduce the removed `scripts/build_gifsicle.sh` shim.
- Recreate scattered audit/review copies at the repo root — external reviews
  are incorporated into `COMPILED_AUDIT.md` (§20 is the pattern) and the
  originals live in git history (G17/S2 will fail stale copies anyway).
