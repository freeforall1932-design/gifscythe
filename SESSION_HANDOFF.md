# Session Handoff

**Session:** S26 · **Date:** 2026-09-17
**Branch:** arena/01a0afc4-gifscythe (platform-assigned arena branch)
**PR #32 merged as `e885d58`** (2026-09-17, branch `arena/01a0af41-gifscythe` — the S25 P1-44 implementation; reviewed from history in S26, the write-up is the S26 section below, which is what the P6 sync requires before this line moves). PR #31 merged as `f1c5bc8` (S24 docs consolidation). This records the merged baseline, not a claim about current CI health. This session's own PR number is *not* written in this header: a session cannot know it at write time, and guessing it is how stale claims get born — the ledger row below is appended when `gh pr create` (or the API) returns the number.
**Docs synced through:** PR #32 · branch `arena/01a0af41-gifscythe` · merged as `e885d58`
*(the newest merge these docs actually describe. `pr_preflight.sh --online` step **P6** compares this against the newest merged PR and fails when a merge landed with no doc sync. Move this line as part of the sync, never before the writing is done.)*
Based on `main` commit `e885d58` (the PR #32 merge) ·
**Product version:** 0.1.0 (owner `OD-11 = a` S19: stays 0.1.0 until the release criteria are met) ·
**Web plan template:** SKELETON
*(mirror of `web/WEB_PLAN_TEMPLATE.md`; the flip to `WORKING PLAN` happens **once**, when the owner's draft is refitted into that template's slots — move both lines in the same commit. Gate **G16** compares the two tokens **and** the template's §1–§10 content: leftover slot placeholders = `SKELETON`; filled content = flip both lines. The gate never auto-edits and never flips back. Inspect that content at every new-session start.)*

## Next session — fast hand-off (after S26)

- **Review before accepting:** `working_code/gifscythe/scripts/review_change.sh`
  (`--commit <sha>` / `--range A..B` / `--patch FILE` / `--pr N`). Never take a
  diff blindly: it flags check-logic edits (**R1**), matchers that match nothing
  so a gate passes vacuously (**R2** — the failure mode U-82 just proved is
  still live: G10 never matched this audit's own header shape), prose counts
  that disagree with a measurement taken now (**R3**), lost executable bits
  (**R4**), and lists the docs the change obliges you to update (**R5**).
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
- **Register:** 122 DONE · 8 PARTIAL · 38 OPEN · 0 UNTRIAGED · 168 total
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
| #31 | S24 | docs/s24-consolidation-intake (unbackticked — G8 reads the docs/ prefix as a path) | `f1c5bc8` (S26 filled this cell in — the merge landed 2026-09-17 and rule 2 says whoever merges edits it; nobody did) | Opened 2026-09-17. S24: the four 2026-09-16 external reviews incorporated into `COMPILED_AUDIT.md` v4 §20 (54 findings re-verified against `3c67e14` first: 20 new rows U-77..U-96 — 4 FIXED incl. U-82's stale-header repair, 16 OPEN scoped P1-44..P3-19; already-fixed/tracked/refuted dispositioned with evidence; originals deleted, git history is the backup) + the owner-ordered docs consolidation (49 → 25 md files: new AUDIT_HISTORY + PLANNING merges, legal/ci/web/csharp folds, brief rewrites of README/WORKLIST/handoff/vision, all references repointed, G8-green) + the proof-backed stale sweep (pending-workflow marker deleted with the copies re-synced to the maintainer-fixed live line; W-30/R-03/GS-208 closed; register re-emitted at 119/8/41/0 = 168). Gates: check_docs 24/0/2, sweep 5/0/0, python sweep tests 14/14, review_change R1-only (E7 path move, mutation-tested). CI on this PR is the compile+suite proof (no toolchain in the S24 sandbox) |
| #32 | S25 | `arena/01a0af41-gifscythe` | `e885d58` | P1-44 implementation (web numeric honesty): the `validate.mjs` finite-number gate, `numOrNull()` in `command.mjs` + `app.js`, incomplete resize/scale omitted from argv, `web/test/numeric-honesty.test.mjs` added and wired into both byte-identical CI copies. Merged 2026-09-17; **its linux run 35225055959 is RED** at the doc gate (G10: this file and `COMPILED_AUDIT.md` still named the pre-#31 base) — S26 repaired that and closed U-78/U-87 with the engine-backed proof S25's sandbox could not run |

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

## Verification status this session (S26)

Everything marked ✅ was **run in this sandbox**; ⏳ could not be. Quote the
**runtime** counter for test counts, never the `CHECK(` source site count. The
S24 table this replaces lives in `IMPROVEMENT_LOG.md`'s S24 entry (**Verified** /
**Not verifiable here**) — its ⏳ rows are the ones S26 turned into ✅.

| Check | Result |
|---|---|
| `./build.sh` (engine + CLI + unit tests) | ✅ green — gifsicle 1.96 built, **372 checks, 0 failures** |
| `scripts/test_engine.sh` | ✅ 5/5 |
| `scripts/smoke_cli.sh` | ✅ 54/54 |
| `scripts/verify_audit.sh` (full) | ✅ 30 passed / 1 failed / 5 skipped at session start — the 1 failure was **F1←G10**, the stale base line repaired in S26; skips are cmake (C6/C9), Qt6 (B) and the CI/clean-Windows rows (C1–C5, D3/D4) |
| `node web/test/command.test.mjs` (W1 parity vs the real CLI) | ✅ green, +2 explicit-zero fixtures |
| `node web/test/validate.test.mjs` (W2 parity vs the real CLI) | ✅ green, +7 wrong-type keys pinned against the parser and `--strict` rc=3 |
| `node web/test/transport.test.mjs` (W3, live server + real engine) | ✅ **79 cases** (was 72), +7 U-78/U-87 cases |
| `node web/test/numeric-honesty.test.mjs` | ✅ green (S25's suite, re-run) |
| W4/W5/W6 suites (body-limit, static-hygiene, request-guard + device-names + server-bounds) | ✅ green under `verify_audit.sh` |
| Mutation test of the new fixtures (review rule R2) | ✅ 4/4 mutations produce FAILs — none of the new cases can pass vacuously |
| `scripts/check_docs.sh` | ✅ see the S26 log entry for the final measurement (start of session: 23/1/1 with G10 red) |
| GitHub API state | ✅ `main` = `e885d58`; **linux run 35225055959 RED** at the doc gate, windows + csharp-spike green; one published release (`snapshot-2026-09-07`, Pre-release, still no licence note — U-95 evidence) |
| CI log text for the red run | ⏳ `results-receiver`/blob storage unreachable from this sandbox — the failing step name came from the job summary, the cause from an exact local reproduction |
| GUI offscreen harness, `windeployqt` packaging, clean-Windows smoke | ⏳ no cmake/Qt6/mingw/wine here |

**Counts are stated by kind on purpose.** `grep -c 'CHECK('` counts lines;
`grep -o 'CHECK(' | wc -l` counts occurrences; neither equals the runtime
count. Gate **G9** compares like with like.

## Network/toolchain reality of this sandbox (re-check every session)

* **S26 sandbox (current):** **g++ 12.2 + make**, node v22.22.3, python3 3.11,
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
   its generated counts line (currently: 122 DONE · 8 PARTIAL · 38 OPEN · 0
   UNTRIAGED · 168 total — but `STATUS.md` itself always wins; sweep rule S2
   compares any quoted tally against it).
1. **What remains before 1.0.0** — criterion unchanged (*no Critical/High
   findings open, package-negative tests green, clean-Windows smoke against the
   exact tagged SHA*): U-59/P0-7 (the last data-loss row) → release re-cut
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
