# Session Handoff

**Session:** S30 · **Date:** 2026-09-26
**Branch:** arena/01a0dc78-gifscythe
**Repo re-created 2026-09-22:** the GitHub repo was rebuilt from a zip upload; the old remote's history (S1–S26, PRs #1–#33, shas like `5c93680`) does not exist in this clone — every such sha below is an old-remote record kept for the written history. The new remote: `04a1cd4` (initial) → `60d3df4` (zip upload) → `ce5fd51` (unpack to root) → PR #1 merge `824bf20`, whose tree is the S26 state minus the four root license files (loss = finding **U-97**, restored in S27 with executed proof). The ledger carries a separator: rows #1–#33 are the OLD repo's; the new repo's numbering restarts at #1. This session's own PR number is *not* written in this header: a session cannot know it at write time, and guessing it is how stale claims get born — the ledger row below is appended when `gh pr create` (or the API) returns the number. This records the merged baseline, not a claim about current CI health.
**Docs synced through:** PR #4 (re-created repo) · branch `arena/01a0dbc8-gifscythe` · merged as `7c035fd`
*(the newest merge these docs actually describe. `pr_preflight.sh --online` step **P6** compares this against the newest merged PR and fails when a merge landed with no doc sync. Move this line as part of the sync, never before the writing is done.)*
Based on `main` commit `7c035fd` (the re-created repo's PR #4 merge, the S28 doc sync; old-remote shas inside the historical sections are records this clone cannot resolve) ·
**Product version:** 0.1.0 (owner `OD-11 = a` S19: stays 0.1.0 until the release criteria are met) ·
**Web plan template:** SKELETON
*(mirror of `web/WEB_PLAN_TEMPLATE.md`; the flip to `WORKING PLAN` happens **once**, when the owner's draft is refitted into that template's slots — move both lines in the same commit. Gate **G16** compares the two tokens **and** the template's §1–§10 content: leftover slot placeholders = `SKELETON`; filled content = flip both lines. The gate never auto-edits and never flips back. Inspect that content at every new-session start.)*

## Next session — fast hand-off (after S30)

- **U-59 / P0-7 GUI implementation is on the open PR #5 branch.** Ordinary
  Batch/Merge/Auto runs use `<target>.gs-partial`, verify then promote, and
  clean up on failure/cancel. Offscreen cases cover success promotion,
  partial-writing failure preserving old bytes, and cancellation preserving
  old bytes. Qt-enabled PR run `36227237540` passed Linux and Windows builds
  and both GUI offscreen suites. This sandbox has no CMake/Qt6. **Separate
  follow-up:** if you have CMake + Qt6, perform an independent code review and
  rerun `test_gui_offscreen`; document findings. Do not merge without explicit
  owner approval.
- **Review before accepting:** `working_code/gifscythe/scripts/review_change.sh`
  (`--commit <sha>` / `--range A..B` / `--patch FILE` / `--pr N`). Never take a
  diff blindly: it flags check-logic edits (**R1**), matchers that match nothing
  so a gate passes vacuously (**R2** — the failure mode U-82 just proved is
  still live: G10 never matched this audit's own header shape), prose counts
  that disagree with a measurement taken now (**R3**), lost executable bits
  (**R4**), and lists the docs the change obliges you to update (**R5**).
- **S29 completed U-94/P2-18:** the seeded 64-case engine-oracle suite is at
  `working_code/gifscythe/scripts/oracle_fuzz.mjs`; its checked-in evidence is
  `working_code/gifscythe/tests/oracle_fuzz_matrix.json`. `--quick` (24 cases)
  is in pre-push; `--full` (64) is verify_audit W7 + Linux CI. See the S29 log
  entry for the exact assertions and measurements.
- **Copy-paste prompt:** `docs/planning/PLANNING.md` §5 — recovery steps, the
  SkillOpt ask, the decision backlog and the standing constraints in one block.
- **Owner decisions:** `docs/planning/OWNER_DECISIONS.md` — answered so far:
  `OD-01 = a` (S15), `OD-02 = a` (S16), `OD-09 = b` (S18), `OD-11 = a` +
  `OD-12 = a` (S19), `OD-17 = a` (S20); `OD-08` resolved in substance by S24
  (workflow copies re-synced, marker deleted). Remaining: `OD-03`…`OD-07`,
  `OD-10`, `OD-13`…`OD-16`, `OD-18`. `OD-16` blocks calling `web/wasm/`
  shippable; `OD-18` blocks closing `U-76`.
- **SkillOpt ask:** `docs/planning/PLANNING.md` §3 — verified facts, the three
  non-negotiable conditions, the four shapes, open questions Q1–Q4. Await
  `OD-15`. **Do not vendor, submodule or pip-install anything before that
  answer.**
- **Register:** 132 DONE · 8 PARTIAL · 29 OPEN · 0 UNTRIAGED · 169 total
  (`STATUS.md` is generated — quote its counts line, never a hand-typed copy,
  and re-run `check_docs.sh --emit` after any §5/hand-block edit).

## Context-budget protocol (S24 — enforce at every new session)

The first context pass is deliberately small. Read `SESSION_HANDOFF.md` first,
then read only these orientation files: `STATUS.md`, `WORKLIST.md`,
`docs/planning/PLANNING.md` §5, and `docs/planning/OWNER_DECISIONS.md`.
Record the files actually read in the session response. Do not bulk-read every
Markdown file.

The orientation files are the persistent context. Everything else is deferred
until the task requires it. Open task-specific sections only: `COMPILED_AUDIT.md`
for the relevant `U-*`/`P-*` item, `IMPROVEMENT_LOG.md` for a specific session
or a new log entry, `PROJECT_VISION.md` for scope/architecture, and release,
CI, legal, web, WASM, C#, screenshot, reference, or product README files only
for work in those areas. Historical/archive Markdown is deferred by default.

After the one-time orientation check, stop rereading orientation files and shift
the conversation to the requested task or review. Keep a compact working set:
current task, exact files/sections, constraints that apply, and verification
results. Summarize or drop superseded excerpts instead of carrying full files
forward. Never spend the remaining context window re-reading large documents;
re-open a deferred section only when new work makes it relevant.

## PR ledger (append-only — this is how you see a skipped or closed PR)

One row per PR, appended at `gh pr create` time and never rewritten. A gap in
the numbering is not an error: **#3 and #9 were closed without merging.** The
"handoff said" column is read from `SESSION_HANDOFF.md` at each merge commit, so
where it lags that is the header trailing reality by one merge — the failure
this ledger exists to make obvious.

| PR | Handoff said "session …" at that merge | Branch | Merged as | What it did |
|---|---|---|---|---|
| #1 | — | `arena/01a07410-gifsicle-1-96` | `9c75b13` | P0/P1: gifsicle engine + Gifscythe control layer + Qt GUI scaffold |
| #2 | — | `arena/01a0746d-gifscythe` | `b8bb58c` | Advance GIF GUI, packaging, tests, and CI preparation |
| #3 | — | `codebase-review-and-optimization-3bbfe` | never merged (closed) | — |
| #4 | — | `arena/01a07959-gifscythe` | `f065a85` | P0/P1 silent-failure fixes + compiled audit (stay 0.1.0) |
| #5 | S4 | `verify/windows-ci-fixes` | `0ad1ff5` | Windows CI fix + verification evidence + XNConvert-style UI retrofit |
| #6 | — | `arena/01a086c5-gifscythe` | `9643654` | S5+S6: GUI honesty fixes, naming, web POC, offline-only review |
| #7 | S7 | `arena/s7-settings-persistence` | `8190c08` | GUI settings persistence + queue reorder + naming templates + release doc |
| #8 | S7 | `arena/01a088f8-gifscythe` | `a73b881` | Post-merge review of PR #7 — 13 findings, 1 High |
| #9 | — | `codebase-review-and-fix-implementation-b8d7e` | never merged (closed) | — (branch kept: it is the Audit A/B extract backup, §18) |
| #10 | S7 | `arena/01a089ca-gifscythe` | `a55a68d` | GPT 6 Astra Medium audit compiled (8 findings) |
| #11 | S7 | `arena/01a08a10-gifscythe` | `7187cbb` | S8: close 31 of 52 audit findings with executed proof |
| #12 | S9 | `arena/01a08bb3-gifscythe` | `801960c` | Status-tracking system: STATUS.md + doc gate + pre-push hook |
| #13 | S10 | `arena/s10-gifscythe` | `2176573` | S10: close 9 findings + N-03 with executed proof |
| #14 | S11 | `arena/s11-gifscythe` | `53a6eda` | S11: U-15/U-17/U-07/U-41 closed (Wine E2E incl.); U-10 provenance; U-12 scoped |
| #15 | S13 | `arena/01a0950f-gifscythe` | `2d51347` | Fix documentation gate after merge commits |
| #16 | S14 | `arena/01a0968e-gifscythe` | `629135a` | External review intake + status-truth corrections + web-as-product + plan template |
| #17 | S14 | `arena/01a0968e-gifscythe` | `75b73a5` | Post-merge sync for #16 |
| #18 | S14 cont. | `arena/01a096ec-gifscythe` | `e32ed28` | Stale-claim sweep + PR preflight + owner-decision register |
| #19 | S14 cont. | `arena/01a096ec-gifscythe` | `43e3f96` | Re-sync handoff to #18 |
| #20 | S14 cont. | `arena/01a09712-gifscythe` | `2542f1b` | Post-merge sync for #19 + owner patch adjudicated |
| #21 | S16 | `arena/01a098ff-gifscythe` | `df1dfd5` | S15 OD-01 triage + S16 GS-201 stop-loss + G18/G16/P3b gates |
| #22 | S17 | `arena/01a09934-gifscythe` | `f760ebe` | S17 sequential work: N-07, GS-202, DS-13, GS-207, packaging/output-verifier hardening |
| #23 | S17 | `arena/01a09dae-gifscythe` | `8230247` | S18 Ms-PL relicense + C# plan Phase 0 + Phase-1 spike GREEN |
| #24 | S20 | `arena/01a09e2e-gifscythe` | `dcb9279` | S19 exe-direction/U-08/wasm-scaffold + S20 windows-only product |
| #25 | S21 | `arena/01a0a508-gifscythe` | `034ad65` | Audit attribution correction (intake files were swapped on upload); §14 discrepancy log |
| #26 | S21 | `audit/compiled-v3-consolidation` | `d1d7939` | COMPILED_AUDIT v3 (§15–§19 recovered) + U-68/NF-11 413 fix + PR #25 doc sync |
| #27 | S21 | `audit/u67-serve-static-allowlist` | `6cd7c7b` | U-67/NF-10 measured-then-fixed: static allow-list + HEAD contract + static-hygiene suite |
| #28 | S22 | `arena/01a0a5bc-gifscythe` | `794a996` | W4/W5 wired into CI + verify_audit; U-53/U-60/U-61/U-62/U-64 + DS-09 closed test-first |
| #29 | S23 | `arena/01a0a632-gifscythe` | `2c98284` | Tier-1 batch (P0-2, P1-5/13/28/34/36/40-loop/41/43, P2-16, P3-5/12) replayed onto #28; N-09/DS-07 found+fixed pre-merge; W6 + all eight web suites in CI |
| #30 | (no session logged — see S24 write-up) | `arena/01a0aa77-gifscythe` | `3c67e14` | Fixed U-77/U-79 (`d7f8ef9`: JSON null-body guards; /optimize explode refusal) and U-80 (`717c082`: glue harness version from VERSION.md) from the 2026-09-16 uploaded reviews; repaired doc-gate consistency — two red mains (runs 35063943000 on `ff35227` and 35066443529 on `4d49919`, both linux "Documentation status gate": G10 base-line lag + the uploads' stale register tallies failing S2) went green (run 35112077599); deleted the four review files, then the owner's restore commit `a4ba82c` put them back "without deleting or merging, marking verified fixed items in-place" — S24's §20 intake supersedes those in-file markings with register rows |
| #31 | S24 | docs/s24-consolidation-intake (unbackticked — G8 reads the docs/ prefix as a path) | `e1d61fb` (post-repair sha of f1c5bc8 — see the attribution-repair note in the S24 addendum of IMPROVEMENT_LOG.md; S26 filled this cell in — the merge landed 2026-09-17 and rule 2 says whoever merges edits it; nobody did) | Opened 2026-09-17. S24: the four 2026-09-16 external reviews incorporated into `COMPILED_AUDIT.md` v4 §20 (54 findings re-verified against `3c67e14` first: 20 new rows U-77..U-96 — 4 FIXED incl. U-82's stale-header repair, 16 OPEN scoped P1-44..P3-19; already-fixed/tracked/refuted dispositioned with evidence; originals deleted, git history is the backup) + the owner-ordered docs consolidation (49 → 25 md files: new AUDIT_HISTORY + PLANNING merges, legal/ci/web/csharp folds, brief rewrites of README/WORKLIST/handoff/vision, all references repointed, G8-green) + the proof-backed stale sweep (pending-workflow marker deleted with the copies re-synced to the maintainer-fixed live line; W-30/R-03/GS-208 closed; register re-emitted at 119/8/41/0 = 168). Gates: check_docs 24/0/2, sweep 5/0/0, python sweep tests 14/14, review_change R1-only (E7 path move, mutation-tested). CI on this PR is the compile+suite proof (no toolchain in the S24 sandbox) |
| #32 | S25 | `arena/01a0af41-gifscythe` | `825ff2c` (post-repair sha of e885d58; content identical) | P1-44 implementation (web numeric honesty): the `validate.mjs` finite-number gate, `numOrNull()` in `command.mjs` + `app.js`, incomplete resize/scale omitted from argv, `web/test/numeric-honesty.test.mjs` added and wired into both byte-identical CI copies. Merged 2026-09-17; **its linux run 35225055959 is RED** at the doc gate (G10: this file and `COMPILED_AUDIT.md` still named the pre-#31 base) — S26 repaired that and closed U-78/U-87 with the engine-backed proof S25's sandbox could not run |
| #33 | S26 | `arena/01a0afc4-gifscythe` | `5c93680` (filled by the attribution-repair session per rule 2: #33 merged 2026-09-17; sha is post-repair) | Opened 2026-09-17. S26: P1-44 proved and closed (**U-78/U-87 ✅ FIXED**) with the engine-backed proof S25's sandbox could not run — `build.sh` 372/0, smoke 54/54, `verify_audit.sh` 31/0/5, transport 72 → **79 cases** — plus the three missing fixture batches, each mutation-tested (R2); two register drifts synced (**DS-09** was closed in S22 but still said OPEN/S15; the ninth web suite was missing from `web/README.md`'s three-suite list with counts frozen at S17); the **G10 base line** repaired, which is why linux run 35225055959 on `e885d58` was red at the doc gate; gate **G6**'s SKIP message now names the tools actually missing (message-only, R1-probed in four states). No product behaviour changed. Register 119/8/41/0 → **122/8/38/0** |


**— the repo was re-created from a zip on 2026-09-22: rows #1–#33 above are the OLD remote's PRs (that history is gone from GitHub; the rows stay as the written record and their shas do not resolve in this clone). The NEW remote's ledger starts here; its PR numbers are independent of the rows above. —**

| PR (new repo) | Handoff said "session …" at that merge | Branch | Merged as | What it did |
|---|---|---|---|---|
| #1 | S26 (the uploaded tree's own header) | `arena/01a0c72c-gifscythe` | `824bf20` | The platform unpack: zip upload (`60d3df4`) unpacked to root + zip removed (`ce5fd51`); content = the old repo's S26 state minus the four root license files (the loss is registered as U-97 and restored in S27). `git diff ce5fd51..824bf20` is empty — the merge took the branch tree as-is |
| #2 | S27 | `s27-recreation-repair` | `e06b5db` | Opened 2026-09-23. S27: the re-creation repair — root license set restored (**U-97 ✅ FIXED**; executed packager fail-closed repro flipped exit 1 `ERROR: COPYING.ms-pl missing or empty` → exit 0 `Package created`, 11/11 required files non-empty; canonical texts, digests in the S27 log entry) + doc-gate re-sync (G10 enforced lines → `824bf20`; G11 entry dated 2026-09-23) + old/new-repo ledger separator + register re-emitted 122/8/38/0 → **123/8/38/0 = 169**. Gates: check_docs 24/0/2, sweep 5/0/0, python 20/20, review_change 4/0/1 ×2 (no R1), pr_preflight P1/P2/P3/P3b PASS (P4/P6 SKIP — no gh; API-equivalents via urllib). CI on this PR is the packaging + engine-suite proof (no toolchain in the S27 sandbox) |

| #3 | S28 | `arena/01a0dbc8-gifscythe` | `42306bb` | Opened 2026-09-26. S28: **U-59/P0-7 CLI/core half** (the only registered data-loss row) fixed test-first — the engine now writes `<target>.gs-partial` and the target is renamed onto only after verification, so a cancel, a signal or a refusal leaves the previous bytes intact (3 smoke cases run RED first, `cmp=DIFFERS`; smoke 54 → **58/58**, `test_output_verify.sh` 12 → **25 assertions**, unit 372/0, transport 79 cases, `verify_audit.sh` 30/1/5) + the Qt half deliberately left (no Qt6 here) so **U-59 is PARTIAL** + the `set -e`/`wait` harness trap fixed + the **P6 doc sync to PR #2** (the header had lagged one merge) + the S28 log entry that **clears G11**, the red main on `e06b5db` (run 35904935321) + the whole OPEN board triaged against this sandbox's measured toolchain. Gates: check_docs 24/0/1, sweep 5/0/0, python 20/20, review_change 4/0/1 (no R1), pr_preflight P1/P2/P3/P3b/P4/P6 PASS. CI on this PR is the Windows/MinGW compile proof. *(Batch 2 landed in the same PR before the merge — U-92/U-84/U-85/U-93/U-86/U-81/U-83 also closed — so the MERGED totals are smoke 61/61, transport 86 cases, body-limit 13, server-bounds 10 groups, register **130/9/30/0**; the numbers above are batch 1's, kept as written rather than rewritten. Merged 2026-09-26 as `42306bb`, linux + windows + csharp-spike green on run 36218477342.)* |

| #4 | S28 | `arena/01a0dbc8-gifscythe` | `7c035fd` | Opened 2026-09-26, immediately after merging #3. The post-merge sync the ledger's rules require: row #3's *Merged as* cell filled with `42306bb` (rule 2 — whoever merges edits it) with batch 2's real totals appended as a note rather than a rewrite, the **P6** line moved to PR #3, and the **G10 base-line lag the merge itself caused** repaired — merging moved main's tip, so `COMPILED_AUDIT.md`'s enforced **Base:** line naming `824bf20` stopped matching (it was only legal as `e06b5db`'s first parent) and main went red at the doc gate; both enforced lines now name `42306bb`, which stays legal across the next merge because a merge sha is also its first parent (the S26 mechanic). No product code. Gates: check_docs 24/0/1, sweep 5/0/0, pre-push green. Merged 2026-09-26 as `7c035fd` (merge commit). |
| #5 | S29/S30 | `arena/01a0dc78-gifscythe` | **open** | Opened 2026-09-26. U-94/P2-18: seeded offline 64-case engine-oracle harness (`--quick` 24-case prefix), committed matrix, verify_audit W7, Linux CI full run, and pre-push quick run. S30 adds U-59/P0-7 GUI partial-output guard plus offscreen success/failure/cancel preservation cases and a fake partial-writing failure engine. Qt-enabled run **36227237540** passed linux/windows/csharp-spike, including both GUI offscreen suites; run 36226011075 was the previous green baseline. Not merged; no merge without owner approval. |
**Maintenance rule (one row per PR, three touches):**
1. At `gh pr create`, append this session's row with the number GitHub returned
   and `**open**` in the *Merged as* cell. Never guess the number beforehand.
2. Whoever merges edits that one cell to the merge sha.
3. The next session moves the header's **`Docs synced through:`** line to the
   newest merged PR — but only *after* writing up what that PR changed. Step
   **P6** of `pr_preflight.sh --online` fails until the writing and the line
   agree, and names every PR that has not been reviewed yet.

**Why the PR number is the key and the branch only a cross-check:** two branches
here each carried two PRs — `arena/01a0968e-gifscythe` produced **#16 and #17**,
and `arena/01a096ec-gifscythe` produced **#18 and #19**. A check comparing
branch names alone would have passed straight through both skipped syncs.

## S30 — U-59/P0-7 GUI partial-output guard + offscreen regressions (2026-09-26)

**Changed:** On the active `arena/01a0dc78-gifscythe` branch / open PR #5,
ordinary GUI Batch, Merge, and Auto runs now direct gifsicle to a same-directory
`.gs-partial`. Before a run, any stale sidecar is discarded and the partial is
snapshotted; after an exit-zero run, the shared `OutputVerify.h` verifier checks
the partial and only a valid changed GIF is promoted over the destination.
Non-zero exit, failed start, failed verification/promotion, and Cancel discard
the partial; Explode remains unchanged because it writes numbered frames under
a prefix. Batch continuation performs the same guarded lifecycle for each
planned destination. Added a cross-platform fake engine that writes corrupt
partial bytes and exits 7. Offscreen T4/T8/T9 cover successful promotion,
failed partial writes preserving an existing target, and cancellation
preserving an existing target; each path asserts no sidecar remains.

**Follow-up task:** WORKLIST.md separately asks the next agent to independently
review the change and rerun the GUI/offscreen suite if that agent has CMake +
Qt6 capabilities. This is not a request to merge.

**Verified:** `git diff --check`; local fake-fixture compile; unit 372/0,
CLI smoke 61/61, output verifier 25/0; Qt-enabled PR run **36227237540** passed
Linux and Windows builds and both GUI offscreen suites. The prior PR #5 green
run predates these changes; run 36227237540 is the evidence for this patch.

**Not verifiable here:** a local Qt6/CMake build/run and the independent review
queued for the next agent; CI supplied the current cross-platform GUI evidence.

**Docs touched:** `COMPILED_AUDIT.md` (U-59/P0-7 evidence), `STATUS.md` (emit
from audit), `WORKLIST.md` (current U-59 state and separate next-agent review
task), `SESSION_HANDOFF.md` (S30 handoff and PR #5 scope),
`working_code/gifscythe/README.md` (fixture and test count),
`web/WEB_PLAN_TEMPLATE.md` (historical S14 state wording; G16 token retained),
`working_code/gifscythe/scripts/check_docs.sh` (G16 pipeline reliability), and
`IMPROVEMENT_LOG.md` (this entry).

## S29 — PR #4 post-merge doc sync; U-94/P2-18 seeded real-engine oracle gate (2026-09-26)

**PR #4 merged first, with explicit owner approval.** The documentation-only
sync merged as true merge commit `7c035fd`; main's repair run `36225207865` is
SUCCESS. It fixes the exact G10 main failure from PR #3 run `36218999564` by
moving both enforced base lines to PR #3's merge `42306bb`, fills PR #3's ledger
merge cell, appends its batch-2 measured totals without rewriting the original
row, and moves P6 to PR #3. The current header then syncs through PR #4.

**U-94 / P2-18 — seeded offline settings oracle, executed against the real
engine.** `working_code/gifscythe/scripts/oracle_fuzz.mjs` uses a fixed xorshift
seed (`0x47534631`), 24 curated boundary cases plus 40 seeded combinations.
For every case it compares the JS builder's command to the real C++ CLI's
conf-parsed command, checks JS validation against the CLI's `--strict` result,
runs argv directly against the bundled gifsicle 1.96, and checks GIF signature
and non-empty output. Product-accepted cases must produce a verified GIF;
engine refusals must have a product warning; cases the engine silently accepts
but the product warns about are explicitly included (e.g. `--scale 0x1` and
invalid gamma). It also runs the same accepted samples through CLI `--run` and
checks its output. The full measured matrix is committed at
`working_code/gifscythe/tests/oracle_fuzz_matrix.json`; `--quick` checks the
24-case prefix in pre-push, and `--full` checks all 64 rows in verify_audit W7
and Linux CI. There are no downloads, third-party dependencies, or random-seed
nondeterminism. PR #5 carries the change from this fixed session branch; its
initial push CI run `36225748067` passed on linux, windows, and csharp-spike.

**Gates wired.** `.githooks/pre-push` now requires the quick oracle after the
existing docs gate; `verify_audit.sh` W7 runs the full sample; Linux CI runs the
same full command. `docs/ci/build.yml.proposed` was re-copied byte-identically
after editing the live workflow. The review flagged these as R1/check-logic
changes; mutation probes confirmed a matrix mismatch and a deliberately broken
invariant make the oracle fail, and the W7 result marker is present on real
output (not vacuous).

**Verified here:** `./build.sh` — 372/0; `test_engine.sh` 5/5;
`smoke_cli.sh` 61/61; all nine web suites green; oracle `--quick` 24/24 and
`--full` 64/64; `check_docs.sh --no-gate-run` and `sweep_stale.sh` green after
this write-up; `verify_audit.sh` includes W7. The working tree is on the fixed
session branch, fast-forwarded to the PR #4 merge; no Qt6/CMake or Windows
MinGW/Wine in this sandbox.

**Not verifiable here:** the GUI/offscreen harness and Qt half of U-59 (no
Qt6/CMake); Windows-only execution/package behavior (no MinGW/Wine). The oracle
uses the Linux native engine and fixture.

**Docs touched:** `COMPILED_AUDIT.md` (U-94/P2-18 proof, S29 verification and
G10 base), `STATUS.md` (re-emitted after U-94 closure), `WORKLIST.md` (U-94
closed), `SESSION_HANDOFF.md` (PR #4 row/SHA, P6 header, S29 handoff),
`IMPROVEMENT_LOG.md` (this entry), `.github/workflows/build.yml` and its byte
copy `docs/ci/build.yml.proposed`; added the oracle runner and committed matrix.

## S28 — U-59/P0-7 CLI/core half fixed test-first, red main diagnosed as G11, whole OPEN board triaged against this sandbox (2026-09-26)

**PR #3 is this session's own PR — what it merged as `42306bb` is exactly the two batches
written up below, which is why the header's sync line can move to it now.

**PR #2 write-up.** The new remote's PR #2
(branch `s27-recreation-repair`, merged as `e06b5db`) was S27's re-creation
repair: the four root licence files restored (**U-97 ✅ FIXED**) and the two
enforced base lines re-synced. The header had lagged it by exactly one merge —
the ledger row still said **open** and the sync line still named PR #1, which is
the drift **P6** exists to fail. Both moved in this session.

**What S28 changed in product code (the first product change since S26):**

1. **U-59 / P0-7, the only registered data-loss row — CLI/core half, test-first.**
   The engine wrote direct to `-o <target>`, so a cancel or any failed run left a
   truncated file over the last good result. `OutputVerify.h` now carries the
   tmp+rename guard (`partial_output_path` / `redirect_output_operand` /
   `promote_partial` / `discard_partial`) and `main.cpp` runs every real-file
   output through `<target>.gs-partial`, verifies the PARTIAL and only then
   renames it onto the target; a cancel, a signal or a refusal discards the
   partial, so the previous bytes survive. Three new smoke cases were run RED
   against the unfixed build first (`cmp=DIFFERS` — the pre-existing output
   really was destroyed) and are green after; a fourth pins the self-heal
   contract. Smoke 54 → **58/58**, `test_output_verify.sh` 12 → **25 assertions**,
   unit 372/0 and transport 79 cases unchanged.
2. **Left deliberately: the Qt half.** `runCommand()` still passes the real
   target to the engine, so the GUI Cancel keeps the old behaviour. No Qt6/cmake
   in this sandbox (measured), so the edit would have been unverifiable here and
   could only have reddened CI. **U-59 is PARTIAL, not DONE**, and the row names
   the remainder — this is the same discipline the Windows-only rows follow.
3. **A harness trap worth recording:** `smoke_cli.sh` runs with `set -e` active
   (case 1 turns it on, nothing turns it off), so a `wait` on a SIGTERM'd job
   returned 143 and aborted the suite before `==> Done.` — exit 143 with no trap
   fired and no signal ever delivered to the script. The cancel case now runs in
   an explicit `set +e` window like every other negative case in the file.

**The red main, diagnosed rather than assumed.** Run `35904935321` on `e06b5db`:
linux `failure` at **Documentation status gate** (`check_docs.sh
--no-gate-run`, `build.yml:35-37`), **windows all 11 steps green** — including
Package portable (Windows) + manifest assert, which is the platform proof U-97
asked for — and csharp-spike green. Reproduced locally on the same tree: **G11**,
newest log entry 2026-09-23 vs the merge commit's own author date 2026-09-24
+0700 (the N-08 class again). The S28 entry clears it. Also measured: the new
remote has **zero releases and zero tags**, so U-95's "mark the published
Release superseded" has no artifact left to act on; the only downloadable build
is the CI artifact `gifscythe-windows` (52,955,462 B, expires 2026-10-07).

**The whole high-confidence lane closed in the same session (7 more rows).**
Measured, not argued: **U-92** (strict base64 + GIF-magic admission on both
endpoints, before engine discovery), **U-84** (both endpoints now share one
body-cap contract — 413 with no engine present, not 503 on one), **U-85** (PORT
validated once: named reason, usage line, exit 2), **U-93** (`GS_MAX_STDERR` cap
with a disclosed truncation marker, `/favicon.ico` 204 + data-URI icon, the
`/run` output envelope documented), **U-86** (three comment/message truths; the
two transport assertions that pinned the old wording were re-pinned, not
deleted), **U-81** and **U-83**. Two of those deserve the emphasis:

- **U-81's intake repro was wrong on BOTH halves, and measuring is what caught
  it.** `gifsicle -e -o - in.gif` writes ZERO bytes to stdout and drops
  `in.gif.000..011` into the CWD (rc=0) — N-05's scatter class, not the "honest
  stdout run" the row described; and `--info` + explode is refused by the ENGINE
  itself (rc=1, its own reason), so no false frame failure existed there either.
  The verifier now carries the `verify_file` exemptions, and explode + `output=-`
  is refused rc=2 with nothing scattered. §19's "re-probe every fixed row" rule,
  applied to an OPEN one, found a wrong scope.
- **U-83 was pinned rather than refused** because the measured behaviour is sound
  (the `-o` target is written, the source stays byte-identical).

Register after both batches: **130 DONE · 9 PARTIAL · 30 OPEN · 169**.

**Capability triage of the OPEN board (this sandbox: g++ 12.2, node v22, gh,
curl — no cmake/Qt6/mingw/wine/emcc/dotnet).** Provable here and therefore
schedulable without a new sandbox: the web lane (U-92, U-93, U-85, U-84, U-86,
U-96), the CLI/core lane (U-81, U-83, U-94) and the doc-machine rows (U-88,
U-89). CI-provable only (source edit here, proof on the runner): the Qt rows
(U-59's half, U-12, U-58, U-70/U-72, DS-10, GS-205, GS-203's GUI half) and the
Windows rows (U-55, U-71). Not doable in ANY sandbox: W-18 (clean Windows
machine), W-19 (physical desktop), U-09/U-95 (an owner release) and every OD-*
decision.

## S27 — the re-creation repair: license set restored (U-97), double-red main re-synced (2026-09-23)

**PR #1 write-up (the P6 sync this header records).** The new remote's PR #1
(branch `arena/01a0c72c-gifscythe`, merged as `824bf20`) was the platform's zip
unpack: the old repo's S26 tree restored into a fresh repo whose prior history
(PR #1–#33 there, base `5c93680`) is gone from GitHub. Its diff against its own
branch tip is empty (`git diff ce5fd51..824bf20`): it changed no content, but it
made the new main double-red, because two things did not survive the trip — the
docs' base shas became unresolvable in this clone (G10, the linux CI failure,
reproduced exactly here) and the four root license files both packagers
hard-require were dropped (the windows CI failure, reproduced exactly here).
The tree itself is the S26 one, verified by markers rather than trust:
`numOrNull()` in `command.mjs`/`app.js`, `g6_missing_tools()` in `check_docs.sh`,
the 27-row device table (device-names suite green) and the 79-case transport
write-up in this file.

**What S27 changed (no product behaviour was touched):**

1. **The root license set restored (U-97 — found AND fixed in-session, rule 2's
   strong form).** The windows CI failure was reproduced locally before fixing:
   the real `package_portable.sh` with fixture binaries on gitignored paths and
   the REAL repo root answers `ERROR: COPYING.ms-pl missing or empty (no usable
   same-target candidate)`, exit 1 — the packager's fail-closed licence check
   (U-02/U-08) is platform-independent, so no Windows runner was needed to prove
   the cause. Restored: `COPYING.gplv3` (GNU GPLv3, 35147 B), `COPYING.lgplv3`
   (standalone GNU LGPLv3, 7639 B — the 42 KB SPDX combined text was rejected),
   `COPYING.ms-pl` (canonical Ms-PL, 2663 B), `COPYING.gifsicle` (byte-copy of
   `reference_code/gifsicle/COPYING`, digests identical both sides); fetch URLs +
   sha256 digests are in the S27 log entry. Repro flipped: `Package created`,
   exit 0, all 11 required files verified non-empty by the packager's own check.
2. **The doc-gate re-sync (G10 + G11 — the linux CI failure).** Both enforced
   base lines (this file's and `COMPILED_AUDIT.md`'s) now name `824bf20` — the
   new main tip, and after this PR merges also its merge first parent, so the
   lines stay legal across the merge (the S26 mechanic). The S27
   `IMPROVEMENT_LOG.md` entry dated 2026-09-22 clears G11 (the docs lagged the
   code by five days — the unpack commits). U-97 was registered in §5 (the
   register is now 97 rows, the heading says so) and `STATUS.md` re-emitted:
   122/8/38/0 = 168 → **123/8/38/0 = 169**; every quoted tally in the
   current-state docs moved in the same pass (S2 rule).
3. **The re-creation recorded where the doc machine reads it.** The ledger
   separator (rows #1–#33 = the old repo, kept as the written record; the new
   table starts at the unpack row), the header's re-creation note, and the
   **Docs synced through** line naming the new remote's PR #1. The old/new PR
   numbers collide by construction — the separator is what keeps them readable,
   and P6 reads the newest merged PR from the CURRENT remote, where #1 is the
   unpack.

**Left deliberately:** every C++/Qt/Windows/wasm row (no compiler in this
sandbox — U-59/P0-7 remains the top product row and needs a toolchain sandbox);
`PLANNING.md` §5's copy-paste block (it claims to hold no state and was already
stale at S26 — refreshing it is P2-22/U-89 territory); every gate's logic (no
R1 edits this session); `reference_code/` (read-only — the COPYING copy went
OUT of it, nothing went in). The windows step diagnosis stays an inference from
an exact local reproduction (CI log blobs answer 401 here, as in S26) — this
repair's own CI run is the deciding evidence.

## S26 — P1-44 proof + closeout, two register drifts, and the red-main repair (2026-09-17)

**PR #32 write-up (the P6 sync this header records).** S25 implemented P1-44 and
merged it as `e885d58`, but logged it as *Partial* for a reason that turned out
to be sandbox-specific, not repo-specific: *"Not verifiable here: the compiler,
C++ CLI build, and engine-dependent parity/transport suites are unavailable in
this sandbox."* **S26's sandbox has the toolchain** (g++ 12.2, node v22 — see the
toolchain section below), so the missing proof was run rather than argued about:
`./build.sh` green (engine 1.96 + CLI + **372 unit checks, 0 failures**),
`smoke_cli.sh` **54/54**, `test_engine.sh` **5/5**, and all six web suites
including the engine-backed ones — `command`/`validate` parity against the real
CLI and `transport` end-to-end against a live server (**72 → 79 cases** after the
new fixtures). That is the proof P1-44's own row asked for, so **U-78 and U-87
are ✅ FIXED (S26)** and §6 P1-44 is DONE.

**What S26 added in code (fixtures only — no product behaviour changed):**
`validate.test.mjs` pins the wrong-TYPE class against the real CLI for seven
integer keys; `command.test.mjs` pins explicit-zero parity; `transport.test.mjs`
pins 422-not-200 for a wrong type and empty-vs-zero over HTTP. The wrong-type
class needed a *new* fixture shape rather than a parity row, and the reason is
measured, not assumed: a non-numeric conf value is caught by the C++ **parser**
(`WARNING: settings key 'colors' value 'abc': not an integer`, `parse=1`,
`--strict` rc=3), while the web has no parse layer at all — JSON hands
`validate()` the raw type, so its finite-number gate *is* the mirror. The two
wordings differ by design; the refusal must not. Likewise an emptied web field
has **no C++ counterpart**: an absent `resize_w` is re-defaulted to 0 and the CLI
prints `--resize-fit 0x200`, while the JS builder omits the flag — so that state
stays a JS contract (`numeric-honesty.test.mjs`) instead of being faked into a
parity row. **All four mutations fail the new cases** (drop the finite gate →
7+3 FAIL; `numOrNull("")`→0 → 1 FAIL; server-side `""`→0 → 3 FAIL;
explicit-0→unset → 1 FAIL), so none of them can pass vacuously (review rule R2).

**Two register drifts found and fixed — both were *docs* lying about code:**

1. **DS-09 was closed in S22 and still said OPEN/S15.** `COMPILED_AUDIT.md` §6
   P1-31 reads "DONE S22" and §16 records the proof, `Validate.h:44` and
   `web/validate.mjs:47` both carry the rule, and the `threads = -7` assertions
   pass in the 372 — but the hand-maintained `STATUS.md` row was never moved.
   Four sessions of contradiction, invisible to every gate: G0 only regenerates
   the §5-derived U-rows, and G17/S5 compares narrative against the *U-row*
   register, so a hand-block row that disagrees with §6 is nobody's job. Now
   DONE with re-measured proof.
2. **The G10 base line went stale when PR #32 merged, and that is why `main` is
   red** (linux run 35225055959, step *Documentation status gate*; windows and
   csharp-spike passed). G10 accepts main's tip or its merge first parent, so
   naming the PR #30 merge was legal right up until the next merge landed —
   exactly the U-82/H:F-06 failure mode, one merge later, in the very line S24
   rewrote to be G10-enforceable. Both enforced lines now name `e885d58`.

**The doc machine under-claimed the toolchain as well.** Gate **G6**'s SKIP line
named all five of its required tools unconditionally — `(gcc/g++/Node/CMake/Qt6
missing)` — so this sandbox, which has g++ 12.2 and node v22, was told it had
none. Same mistake S25 made in prose, emitted by a gate; that is why the S26
toolchain bullet above is written as a *measurement*. The five tests are now in
`g6_missing_tools()` and the message names what is really absent (`missing: cmake
Qt6` here). Message-only, and probed in four states (empty PATH → all five;
only node absent → `node`; here → `cmake Qt6`; all present → empty, so G6 still
runs its comparison). `review_change.sh` flags it **R1** as a check-logic file.

**Left untouched, deliberately:** every Qt/GUI row (no cmake, no Qt6 here),
every Windows-only row (no mingw, no wine), `web/wasm/` (no emcc, plus OD-16),
and the release rows (U-09/U-95 need an owner decision and Windows artifacts).
S26 changed no product behaviour, so none of those rows moved.

**Not verifiable here:** the GUI offscreen harness, `windeployqt` packaging, the
clean-Windows smoke, and CI itself — the log *text* of run 35225055959 could not
be downloaded (`results-receiver`/blob storage unreachable from this sandbox), so
"the red step is G10" is an inference from *which* step failed plus an exact local
reproduction of that one failure. The push of this branch is what confirms it.

## S24 — external-review intake + the owner-ordered docs consolidation (2026-09-17)

**PR #30 write-up (the P6 sync this header records).** PR #30 was a fix +
gate-repair session that logged no `IMPROVEMENT_LOG.md` entry of its own (G11
passed only because the S23 entry carries the same date). What it did, reviewed
from history in S24: three fixes from the freshly uploaded external reviews —
**U-77** (`/run` + `/optimize` answered JSON `null` bodies with a text/plain 500
TypeError leak; now object-shape-guarded to documented 400s), **U-79**
(`/optimize` accepted `mode:"explode"` and answered a misleading 422 blaming the
engine; now a 400 pointing at `POST /run`, with batch/merge single-file probed
benign), **U-80** (the wasm glue harness hardcoded `release/0.1.0/gifsicle`; now
parses VERSION.md at runtime) — each with transport-suite regressions, plus the
doc-gate repair that turned two red mains green (row #30 above). It also
deleted the four uploaded review files, and the owner's next commit restored
all four "without deleting or merging, marking verified fixed items in-place".

**What S24 did (this branch).** The owner's instruction: review every md file,
consolidate them into merged per-topic files, sweep what is provably stale, and
for the audit side — delete the already-done and incorporate the four restored
review files into the compiled audit, adding only problems that are NEW and not
already worked on. Executed:

1. **Intake (COMPILED_AUDIT.md → v4, §20).** All 54 findings across the four
   files re-verified against main `3c67e14` (they pinned `794a996`; PR #29/#30
   moved the tree after) and dispositioned: **20 new §5 rows U-77..U-96** (4 ✅
   FIXED — U-77/U-79/U-80 by PR #30, U-82 by S24 — and 16 ⬜ OPEN scoped as
   P1-44..P1-46 / P2-18..P2-22 / P3-13..P3-19); **9 already fixed or already
   tracked** (H:F-02/F-03/F-04 by S23, H:F-05 + G:GN-01 by PR #30, J's sixteen
   re-frames); **3 refuted/void with executed evidence** (G:GN-03 probed clean —
   the U-22↔U-62 pairing went into §19.3; G:GN-06 and J:F-17 contradict the
   README's own text); **3 adopted as edits** (§19.4 currency, P2-17's
   CI-testable note, the W-30/R-03/GS-208 closures GN-05's falsify-line
   predicted). Register: 76 → 96 U-rows, re-emitted after the closures (the
   tally S24 produced is quoted in its own `IMPROVEMENT_LOG.md` entry — sweep
   rule **S2** deliberately does not exempt a dated tally in a current-state
   doc, so the number lives in the append-only log and today's counts are read
   from `STATUS.md`). The four root files were then
   deleted (full text in git history at `3c67e14`; §20.5 is the completeness
   checklist — no finding dropped, nothing already worked re-added).
2. **Stale sweep with proof.** The pending-workflow marker (docs/ci/PENDING_WORKFLOW_CHANGE.md) deleted:
   the change it waited on was already live (the doc-gate step is in CI since
   the maintainer applied it; the last drift was the PROPOSED copy lagging the
   maintainer's live cygpath fix `414f5fc`), so S24 re-synced the doc copy to
   the live line — no workflows-scoped push needed — and E9/G7/S1 enforce
   byte-equality again. **W-30, R-03, GS-208 closed** (P2-7 resolved);
   U-14's blocker text corrected (scope is no longer the blocker; verify_audit
   stays out of CI by design). U-82 fixed: the audit header's six-merges-stale
   Branch line was rewritten into the G10-matching Base shape.
3. **Consolidation (49 → 25 md files; every merge recorded where it landed).**
   New merged files: `docs/archive/AUDIT_HISTORY.md` (absorbs the seven dated
   audit snapshots — two from docs/archive/, five from docs/audit/),
   `docs/planning/PLANNING.md` (absorbs OFFLINE_BUILD_REVIEW, CSHARP_SHELL_PLAN,
   SKILLOPT_INTEGRATION_QUERY, SEQUENTIAL_WORK_HANDOFF, NEXT_SESSION_PROMPT).
   Merged into existing homes: FEASIBILITY_REVIEW → `PROJECT_VISION.md`
   (architecture + the flag-map table, with VP-5's crop-form correction
   applied; `verify_audit.sh` E7 re-pointed); WEB_FEASIBILITY → `web/README.md`
   §History; WHY_MSPL + COPYING_RULES + WASM_LICENSE_QUESTION →
   `docs/legal/README.md`; CLEAN_WINDOWS_SMOKE + DESKTOP_PROBES →
   `docs/ci/README.md` §2–§3; THIRD_PARTY_NOTICES → `web/wasm/README.md`;
   csharp/spike/README → `csharp/README.md`. Rewritten brief: root `README.md`
   (session narrative removed — it duplicated the log), `PROJECT_VISION.md`,
   `WORKLIST.md` (session sections folded out; the rules and board stay), this
   file. Untouched by design: `STATUS.md` (generated), `COMPILED_AUDIT.md`'s
   machine-parsed shapes, `WEB_PLAN_TEMPLATE.md` (G16), `VERSION.md` (parsed by
   five scripts), `OWNER_DECISIONS.md` (active register, refs updated),
   `RELEASE_PROCEDURE.md` (G13; blockers table refreshed), the screenshots
   README, `REFERENCE_MANIFEST.md` (provenance record — one stale note
   corrected), and everything under `reference_code/` (read-only).
   Deleted-file names are cited WITHOUT backticks in current docs (the S21
   trick that keeps gate G8 honest) and every live reference was repointed.

**Executed here (this sandbox):** `check_docs.sh` → green before the first edit
(baseline 23/0/3) and re-run after every batch; `sweep_stale.sh` green;
`python3 working_code/gifscythe/tests/test_sweep_stale.py` (14 tests); node
probes for U-78 (NaN passes validate.mjs with zero issues), GN-03 (resize/scale
refused, crop allowed), GN-18 (stemOf edge names); GitHub API checks for the
open-PR list, main's run states and the U-95 release facts. **Not verifiable
here:** no compiler/Qt/wine/emcc/dotnet — every C++ claim in the new rows is
source-read (✅ SRC) and labelled so; the engine-dependent suites (command/
validate/transport, smoke, unit, harness) were NOT run; `verify_audit.sh` was
not run in full (capability skips). The CI run on this branch is the compile +
web-suite proof.

**Left / next.** The 16 new OPEN rows (P1-44 web numeric honesty first — it has
executed repro; then P0-7/U-59 which predates the intake and stays the top
product row); the owner edits: U-95's release-notes line and the OD backlog.
Deliberately NOT done in S24: no gate logic changes beyond E7's path move and
no code fixes for the new rows (the owner's order was consolidation + intake;
fixing them is the next session's P-lane work). The register-mechanics asks
(GN-04/GN-16/GN-17 → U-88/U-89) are registered, not implemented — implementing
them mid-consolidation would have been the doc-machine-churn the intake itself
warns about.

## Session history (condensed — full detail in `IMPROVEMENT_LOG.md`, per-session)

- **S4–S8 (2026-09-07→10):** Windows engine recipe + Wine proofs; XNConvert UI
  retrofit; offline-only direction; settings persistence/reorder/templates; the
  status-tracking machine (STATUS.md + gates + pre-push); S8 closed 31 of 52
  findings with executed proof. PRs #5–#12.
- **S9–S13 (2026-09-10→12):** doc-gate repairs (N-01/N-02), the S10 Qt sandbox
  (9 findings + screenshots), the S11 mingw+Wine sandbox (U-07/U-15/U-17/U-41,
  N-04/N-05/N-06), smoke expansion (U-18), engine-config relocation (U-10
  half). PRs #13–#15.
- **S14–S17 (2026-09-12→13):** the 18-finding external intake (§13) + triage
  (`OD-01 = a`); web-as-product decision + plan template; sweep_stale +
  pr_preflight + OWNER_DECISIONS; GS-201 stop-loss (P0-5); GS-202/DS-13/
  GS-207/DS-11/N-07 closed; GS-203/204/210 partial with handoffs. PRs #16–#22.
- **S18–S20 (2026-09-14):** Ms-PL relicense (`OD-09 = b`) + legal folder; C#
  plan + spike GREEN then PARKED (`OD-C7`); U-08 closed; wasm scaffold
  (unproven, OD-16); windows-only ship (`OD-17 = a`). PR #23–#24.
- **S21–S22 (2026-09-15→16):** COMPILED_AUDIT v3 (§15–§19); U-68 413 (PARTIAL
  on the cap value); U-67 allow-list; W4/W5 wiring; U-53/U-60/U-61/U-62/U-64 +
  DS-09 closed test-first. PRs #25–#28.
- **S23 (2026-09-16):** the Tier-1 batch — threads tri-state (P0-2), int-width
  parsing + domains (P1-28), loop-once (P1-40 half), settings round-trip
  (P1-13), CLI honesty (P1-43), engine discovery + release/current pin (P1-41),
  web bounds (P1-5), device aliases (P1-36), request ownership module (P1-34),
  W6 + eight web suites in CI; N-09/DS-07 found reviewing its own merged tree
  and fixed pre-PR. PR #29.
- **PR #30 (2026-09-16, no logged session):** U-77/U-79/U-80 + the doc-gate
  repair — written up in full in the S24 section above.

**Gotchas carried forward (learned the hard way; do not re-learn):** gifsicle
image options must come BEFORE the filename or they apply to nothing; `--info`
prints the file name, so a test grepping for "loop" must not name the fixture
`loop_once.gif`; a parity check comparing two empty lists proves nothing
(explicit `expect:` lists); `set_field()` re-trims, so a decode added only to
`load_settings()` is silently undone; the GUI harness must close windows
(`w->close()`), never `delete w`, or `closeEvent` persistence never runs; a
sandbox can restart mid-session — commit and push early; awk changes must pass
under mawk AND gawk (`LC_ALL=C` everywhere).

## Important constraints

**Read this section before editing anything.** These live here, in
`WORKLIST.md` and in `docs/release/RELEASE_PROCEDURE.md` on purpose — rules
only stick if they are in files a new session reads, not in a conversation.

### The status rules (S9 — these are the ones that make the docs stick)

1. **Every session ends by updating the docs** — `STATUS.md` (via
   `check_docs.sh --emit`), `SESSION_HANDOFF.md`, `WORKLIST.md`,
   `IMPROVEMENT_LOG.md` — **then runs `check_docs.sh` until green.**
2. **A new finding is recorded in the same session it is found:** `UNTRIAGED`
   in `STATUS.md` **and** a pending `- [ ]` line in `WORKLIST.md`. Gate **G12**
   fails if an `UNTRIAGED` row outlives the session that found it. (Triaging it
   into a §6 id the same session — as S24 did for the whole intake — is the
   stronger form.)
3. **Before `gh pr create`, and again before `gh pr merge`:** run
   `working_code/gifscythe/scripts/pr_preflight.sh --online`, fix every
   failure, re-run until green. Do not create or merge with a failing check,
   and **do not ask whether to run it**.
4. **Enforced mechanically** by `.githooks/pre-push` (bootstrap once per clone:
   `scripts/bootstrap_hooks.sh`; `build.sh` does it; **G15** fails a clone that
   never bootstrapped).
5. **Every IMPROVEMENT_LOG entry uses the template** (`Changed / Partial /
   Left / Verified / Not verifiable here / Docs touched`). The **`Not
   verifiable here`** line is mandatory.
6. **HARD RULE — commit every edit/write/delete before merge AND before the
   session can close.** Gate **G18** fails `check_docs.sh` on a dirty tree;
   **P3/P3b** fail create/merge on dirty or unpushed. Anything labelled
   "after merge" is done **before** merging. At every new-session start,
   inspect `web/WEB_PLAN_TEMPLATE.md` §1–§10 (G16, one-way flip).

### Product constraints (unchanged unless noted)

- `reference_code/` is read-only; product work lives in `working_code/gifscythe/`.
  The ONE exception: `reference_code/REFERENCE_MANIFEST.md` is the provenance
  record and gets updated with evidence (never the code trees).
- Do not bump to `1.0.0` before the UI/UX gates + owner decision. No
  WebP/APNG before the GIF UI is stable.
- Keep gifsicle as a **subprocess** (GPL v2-only engine vs Ms-PL UI). The
  experimental `web/wasm/` in-process track is the open `OD-16` question
  (`docs/legal/README.md` §3) and is NOT shippable until answered. Do not
  extend the in-process pattern anywhere else.
- Do **not** "fix" `--loopcount=0`, `-O0`, crop `+` form, or gamma sentinel —
  verified correct (§15.1 VP-1..VP-5).
- Live CLI pane stays honest **one-way** (`OD-12 = a`).
- Windows exec is `CreateProcessW` + `win_quote_arg` (UTF-8 argv → strict
  UTF-16 line) — never `_spawnvp`/shell; every std::string↔fs::path boundary
  goes through `u8path_compat`/`path_u8string` (`src/core/WinUnicode.h`).
  Windows CLI/test exes stay `-static`; the engine line keeps `-include
  src/win32cfg.h` before `-I.` and never passes `-DVERSION`.
- CMake never writes into `src/` (U-15, gate C9); `build.sh` is the only writer
  of the committed `src/core/version.h` fallback; include order stays
  generated-first.
- Explode: ONE file per run (N-05 — validate warns, CLI refuses rc=2, GUI
  refuses, web 400) and success means VERIFIED frames (`ExplodeVerify.h`
  snapshot-diff), never rc=0 alone. The `fake_engine_exit0` fixture must keep
  being built next to `test_gui_offscreen`.
- CLI `--run` refuses Batch with no `output` (GS-201/P0-5, exit 2) and Batch
  with >1 input + one output (U-74, exit 2). Print mode still emits `-b`.
- Threads tri-state (`<0` nothing / `0` bare `-j` / `>0` `-jN`) and loopcount
  sentinels (`-2` play once / `-1` unset / `0` forever / `N`) must survive
  every round trip — settings model, GUI panel (4th Looping item; spinner
  minimum "Unchanged"), serializer guards and harness cases pin them
  (DS-06/DS-07/N-09).
- Extend `tests/test_gui_offscreen.cpp` with every GUI feature; a persistence
  case copies the `spinEvents / w->close() / spinEvents / exists` idiom, not
  `delete`.
- Offline-only — no cloud, no auto-update, no telemetry. The `web/` app is
  self-hosted (loopback default) and supported since S14; its `/run` keeps the
  desktop honesty rules — do not fork the semantics.
- Language stays C++17/Qt6 through 1.0.0 (triggers in `docs/planning/PLANNING.md` §1).
- The `scripts/build_gifsicle.sh` shim is gone (S7). Do not reintroduce it.
- Settings file is core-SettingsIO format; GUI-only keys live in the
  unknown-key map (keep unit tests 20 + 31 green). Default name template
  renders exactly `<name>_opt.gif` (E4; harness T1/T16). The harness sets
  `GS_SETTINGS_PATH` at startup; persistence tests use their own temp path.
- Keep `.github/workflows/build.yml` and `docs/ci/build.yml.proposed`
  byte-identical (**E9**/**G7**/**S1** — since S24 with NO standing exception;
  if they must differ, recreate the pending marker in the same commit and
  delete it in the commit that applies the change).
- **NEW (S24):** external reviews are incorporated into `COMPILED_AUDIT.md`
  (the §20 pattern: verify against current main → disposition every finding →
  register only what is new → delete the original, git history is the backup).
  Do not leave review files scattered at the repo root — their stale tallies
  fail G17/S2 (that is exactly what reddened main at `4d49919`).
- **NEW (S24):** the dated doc snapshots are consolidated — do not recreate
  per-session copies of audit/legal/ci/planning docs; extend the merged file
  (`docs/archive/AUDIT_HISTORY.md` indexes what was folded where).

## Verification status this session (S28)

Everything marked ✅ was **run in this sandbox**; ⏳ could not be. The S27 table
this replaces lives in `IMPROVEMENT_LOG.md`'s S27 entry. Quote the **runtime**
counter for test counts, never the `CHECK(` source site count (**G9** compares
like with like).

| Check | Result |
|---|---|
| `./build.sh` | ✅ engine `LCDF Gifsicle 1.96` + CLI + **372 checks, 0 failures** |
| `scripts/smoke_cli.sh` | ✅ **61 passed, 0 failed** (was 54: +4 U-59, all RED before that fix, +2 U-81, +1 U-83) |
| `scripts/test_output_verify.sh` | ✅ **25 assertions, 0 failures** (was 12; the U-59 helper pins) |
| `scripts/test_engine.sh` · `scripts/test_package.sh` | ✅ 5/5 · **36/36** |
| All nine `web/test/*.test.mjs` | ✅ green — transport 79 → **86 cases**, body-limit 8 → **13**, server-bounds 5 → **10 groups**, command/validate parity against the real CLI unchanged, static-hygiene green after the favicon route |
| `scripts/verify_audit.sh` | ✅ **30 PASS / 1 FAIL / 5 SKIP** — the 1 FAIL is F1, its own re-report of the doc gate row below |
| `scripts/check_docs.sh` | ✅ 23 / 1 / 1 before the doc edits (the 1 = **G11**, the red main, reproduced exactly); G15 was the second failure until `build.sh` bootstrapped the hooks (the known fresh-clone R-04 state) |
| `gh run view 35904935321` (main at `e06b5db`) | ✅ linux `failure` at **Documentation status gate**; **windows all 11 steps green** (Package portable + manifest assert + upload); csharp-spike green. Artifact `gifscythe-windows` 52,955,462 B, expires 2026-10-07 |
| Release/tag state of the new remote | ✅ `gh release list` and `git ls-remote --tags origin` both EMPTY — zero releases, zero tags (recorded for U-09/U-95; no register change) |
| Qt/GUI harness (`test_gui_offscreen`) | ⏳ no cmake, no Qt6 here — **U-59's GUI half is therefore untouched and unproven either way** |
| Windows-only rows, wasm rows | ⏳ no mingw-w64, no wine, no emcc |
| CI log TEXT for the red step | ⏳ `gh run view --log-failed` dies at `results-receiver.actions.githubusercontent.com` (EOF), as in S26/S27 — "the red step is G11" is an exact local reproduction of that step's own command, not a log read |

## Network/toolchain reality of this sandbox (re-check every session)

* **Commit identity rule (S24 attribution repair, standing):** commits here are
  authored AND committed as the owner's real noreply identity —
  `300004558+freeforall1932-design@users.noreply.github.com` (numeric id taken
  from `GET /user`, never invented). GitHub resolves `users.noreply.github.com`
  addresses **by numeric id**, so a fabricated id attributes the commit to
  whichever unrelated account owns that id — that is exactly how two S24
  commits briefly showed under a stranger's account until the history repair.
  At every token hand-off: verify the token owner with `GET /user` before the
  first push; keep this clone's `user.name`/`user.email` set to the owner.

* **S28 sandbox (current):** node v22.22.3, **g++ 12.2 + make**,
  python3 3.11, git, **gh 2.23 authenticated**, curl, mawk (no gawk). **No cmake,
  no Qt6, no mingw-w64, no wine, no emcc, no dotnet** — the S26 shape. Network:
  github.com + api.github.com reachable through gh; Actions **log blobs are
  not** (EOF at `results-receiver`), so a red run can be *identified* but not
  *read*. Clone is SHALLOW (`depth 1`, risk **R-02**). Consequence: the whole
  engine/CLI/core lane and every web suite are fully provable here — which is
  why U-59's CLI half could be closed with executed proof — while every Qt,
  Windows-binary and wasm row stays source-read or CI-proved only.
* **S27 sandbox:** node v20.20.2, python3 3.11, git 2.39, bash 5.2,
  **mawk** (no gawk), **no compiler, no cmake/Qt6, no mingw/wine, no dotnet/emcc,
  no gh, no curl** — the S24/S25 shape. GitHub API works via python urllib + the
  session token; Actions log blobs are NOT reachable (HTTP 401 at the redirect
  target), so a red step can be *identified* but not *read*; gnu.org is
  unreachable but raw.githubusercontent.com is (the restored licence texts were
  fetched there). The sandbox restarted mid-session and came back WITHOUT `.git`
  (working tree intact, re-clone verified byte-identical) — the commit-early rule
  earned its keep. Consequence: the docs/gates lane, the engine-free web suites
  and fixture-based packager repros are fully provable here; everything
  compiler-bound is CI-proved or source-read only.
* **S26 sandbox:** **g++ 12.2 + make**, node v22.22.3, python3 3.11,
  git 2.39, **gh 2.23 authenticated**, curl. **No cmake, no Qt6, no mingw-w64,
  no wine, no emcc, no dotnet.** Network: github.com + api.github.com reachable,
  but Actions **log blobs are not** (`results-receiver`/`blob.core.windows.net`
  → connection failure), so a red run can be *identified* but not *read*.
  Consequence: the whole C++/CLI/core lane is fully provable here — engine,
  unit, smoke, packaging negatives, and the engine-backed web parity/transport
  suites — while every Qt, Windows-binary and wasm row is still source-read only.
  **This is the sandbox S25's "not verifiable here" line was written against a
  weaker version of:** before deferring a C++ item, re-measure the toolchain.
  Clone depth: this one started shallow (depth 1, risk **R-02**); `git fetch
  --unshallow` was run in S26 so G10/G11 see real history.
* **S24/S25 sandboxes:** node v20.20.2, python3 3.11, git — **no compiler,
  no cmake/Qt6, no mingw/wine, no dotnet/emcc, no gh, no curl** (the GitHub API
  works via python urllib + the session token). Network: github.com reachable.
  Consequence: docs/node-probe work is fully provable; anything C++ is
  source-read only and labelled ✅ SRC.
* **S21–S23 sandboxes:** node + python only (same shape as S24).
* **S19 sandbox:** gcc/g++ 12.2, node v22 — no cmake/Qt6; `storage.googleapis.com`
  unreachable (emsdk install fails — executed probe).
* **S11 sandbox (the full one):** uid 0 + apt: g++ 12.2, cmake 3.25.1, Qt 6.4.2,
  ninja, mingw-w64, Wine 8 (ACP immovable at 1252; stage Wine tests under /tmp;
  root ignores `chmod a-w` — use `setpriv --reuid 65534`), gawk + mawk.
* awk portability is a gate concern: CI runners use gawk, sandboxes have used
  mawk — dash first or last in every bracket class; verify under both.
* Clone depth: a shallow clone re-opens risk R-02 (G10/G11 need history;
  `git fetch --unshallow`).

## Document map (post-S24 layout)

| Doc | Role |
|-----|------|
| **`STATUS.md`** | **START HERE** — the single status register (generated; four states) |
| `COMPILED_AUDIT.md` | v4 master: the detail behind every `U-nn` row, §6 fix order, §13+§20 intakes, §15 guardrails, §19 review ask |
| `WORKLIST.md` | Human task board + the status rules + deferred bucket |
| `SESSION_HANDOFF.md` | This file — context for the next session |
| `IMPROVEMENT_LOG.md` | Chronological decisions, newest first, one template per entry |
| `PROJECT_VISION.md` | Mission + hard constraints + architecture/flag map (absorbed FEASIBILITY_REVIEW) |
| `README.md` | Public front page: pitch, quick start, layout, doc guide |
| `docs/planning/OWNER_DECISIONS.md` | The active owner-question register (OD-01..OD-18) |
| `docs/planning/PLANNING.md` | Direction review (§1), parked C# plan (§2), SkillOpt query (§3), sequential-work handoff (§4), next-session prompt (§5) |
| `docs/legal/README.md` | Licence single source of truth: Ms-PL rationale (§1), copying rules (§2), the wasm OD-16 question (§3–§4) |
| `docs/ci/README.md` | Workflow status (§1), clean-Windows smoke (§2), desktop probes (§3) |
| `docs/release/RELEASE_PROCEDURE.md` | How to cut snapshots/releases, incl. the doc gate + rollback policy |
| `docs/archive/AUDIT_HISTORY.md` | Condensed index of the seven dated audit snapshots (full texts in git history) |
| `docs/screenshots/README.md` | Re-shoot recipe + what each shot shows |
| `reference_code/REFERENCE_MANIFEST.md` | Provenance: upstream SHAs, diff verdicts, digests, reproduce recipe |
| `web/README.md` · `web/WEB_PLAN_TEMPLATE.md` | Web product surface (run/test/API + §History) · plan template (G16 state) |
| `web/wasm/README.md` | Experimental track — NOT SHIPPABLE (OD-16) + third-party notices |
| `csharp/README.md` | PARKED C# shell tree + spike record (exit-code collision noted, U-91) |
| `working_code/gifscythe/README.md` · `VERSION.md` | Product build/run docs · version source of truth |

## Prior-session orientation (the old TL;DR, condensed)

0. **START HERE — `STATUS.md`**; `COMPILED_AUDIT.md` §5 is the detail behind
   every `U-nn` row; neither replaces the other. The register line to quote is
   its generated counts line (currently: 132 DONE · 8 PARTIAL · 29 OPEN · 0
   UNTRIAGED · 169 total — but `STATUS.md` itself always wins; sweep rule S2
   compares any quoted tally against it).
1. **What remains before 1.0.0** — criterion unchanged (*no Critical/High
   findings open, package-negative tests green, clean-Windows smoke against the
   exact tagged SHA*): **U-59/P0-7's Qt half** (the CLI/core half landed S28;
   the GUI still writes straight onto the target) → release re-cut
   U-09/P0-4 + the U-95 release-notes edit → W-18 clean-Windows smoke →
   W-19 desktop probes → GS-203's GUI half (P1-25) → the Qt/platform rows →
   the S24 web-intake batch (**P1-44 closed S26** — next is P2-19/U-92, then
   the P2/P3 rows) → owner decisions (OD-16/OD-18 +
   the version call). PARTIALs: U-10 (CI hash-pinning), U-14 (verify_audit
   stays out of CI by design), U-68 (cap value), U-76 (OD-18 directory
   policy), GS-203/204/210 (named handoffs in `docs/planning/PLANNING.md` §4).
2. **Direction:** offline-only; C++17 + Qt6 through 1.0.0; the `web/` build is
   a supported self-hosted product surface (S14); windows-only ship (S20).
3. **Naming policy in force:** *Gifscythe* = product; *gifsicle* = upstream
   engine only.
4. **§19 review ask stands:** every ✅ FIXED row is a documentation claim until
   re-proven — failing test first, then green, then the command + exit code in
   the row; hunt the §19.3 pairings (including the S24 addition U-22↔U-62)
   after every fix.
