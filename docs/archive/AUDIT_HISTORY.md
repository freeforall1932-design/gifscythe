# Audit history — condensed record of the dated review snapshots (S24 consolidation)

**Consolidated 2026-09-17 (S24) on owner instruction:** the seven dated audit/review
snapshots were folded into this one archive file and the originals deleted. Every
finding they raised lives on in `COMPILED_AUDIT.md` (§2–§5 registers, §13 intake,
§15 guardrails, §16–§18 recovered narrative + completeness checklists); the live
state is `STATUS.md`. **The full text of every original is preserved in git
history** — the last main commit carrying all seven is `3c67e14` (fetch any of
them with `git show 3c67e14:<path>`).

This file is a dated historical archive (like everything under `docs/archive/`):
its counts and gate numbers are past measurements, excluded from the doc gates by
policy. It exists so the provenance chain stays readable without keeping seven
overlapping snapshots alive.

| # | Original file (deleted S24) | Date · pinned tree | Role | Where its content lives now |
|---|---|---|---|---|
| 1 | docs/archive/gifscythe-comprehensive-review.md | 2026-09-06 · pre-`docs/` layout | The first consolidated review: merged the original review + external agent audit + 4 forensic review sites, re-verified against the gifsicle 1.96 man page and source; refuted the secondary audit's wrong claims | Fed the S1–S3 compilations → `COMPILED_AUDIT.md` §5 U-01…U-44 lineage; refuted claims → §15.2 false-positive table |
| 2 | docs/archive/gifscythe-final-code-review.md | 2026-09-06 · v0.1.0 | Code review + remediation plan; named the "silent false success" failure class; its §6 held the VP-1..VP-5 verified-correct guardrails | VP-1..VP-5 + false positives **recovered verbatim into §15** (v3, S21) — the master file is self-contained; findings → §5 |
| 3 | docs/audit/POST_S7_AUDIT.md ("Audit C") | 2026-09-10 · `8190c08` (PR #7) | Arena-agent post-merge audit, 13 findings (F-01…F-13); self-marked "superseded in part" (its threads-"benign" claim was withdrawn — it is bug U-03) | Findings → §5 (U-19…U-23, U-28, U-31…U-33, U-36…U-40, U-43…U-44 with C: Src cells); the withdrawal is recorded in file 4's register |
| 4 | docs/audit/CONSOLIDATED_AUDIT_2026-09-10.md | 2026-09-10 · `8190c08` | Merged audits A (GPT 5.6 sol xhigh, 20) + B (Seed 2.1 Pro, 16) + C (13) into the 44-finding register with per-claim verification notes; trust-ranked the sources | A's 20 → §3 (A-01…A-20); B's 16 → §4 (B-01…B-16); all → §5 U-01…U-44; A/B non-finding narrative → §16 |
| 5 | docs/audit/FIX_PICK_2026-09-10.md | 2026-09-10 · `a55a68d` | Self-marked SUPERSEDED: the reasoning that picked U-01 (+U-03) as the first fix, the U-19 correction, and the S8 sandbox-capability table | The pick was executed → file 6; U-19 correction → §5 row (☑ CORRECTED S8) |
| 6 | docs/audit/REMEDIATION_2026-09-10.md | 2026-09-10 (S8) | S8's remediation record: 31 of 52 findings closed with executed before/after evidence + the mutation-test record; dated gate numbers (23/0/5-era) kept as S8 measured them | Per-finding proofs condensed into the §5 rows' Proof cells (S8 markers); the mutation-test discipline became the standing §19 rule |
| 7 | docs/audit/EXTERNAL_REVIEW_INTAKE_2026-09-12.md | 2026-09-12 · re-checked at `2d51347` (S14 intake) | The 18-finding external intake in the reviewers' own words: Max/GPT-class A.1…A.10 (`GS-201…GS-210`) + DeepSeek B.1…B.8 (`DS-06…DS-13`, renamed from its N-series to avoid collision), each with evidence, proposed fix, verification limits, and the §8 status-truth corrections log | Compiled intake record → §13 (row table + evidence locations + re-checks); register rows → `STATUS.md` part 2 under the reviewers' ids; triaged into §6 in S15 (`OD-01 = a`) |

## Citation map for the intake labels

Register rows cite intake sections as "(intake A.n)" / "(intake B.n)" — those
labels refer to file 7's sections: **A.1…A.10 = GS-201…GS-210** (Max/GPT-class),
**B.1…B.8 = DS-06…DS-13** (DeepSeek). Full reviewer text: `git show
3c67e14:docs/audit/EXTERNAL_REVIEW_INTAKE_2026-09-12.md`.

## Why these were safe to consolidate (the proof, per the sweep rule)

1. **Finding-complete:** §18 of `COMPILED_AUDIT.md` is the v3 merge-completeness
   checklist — every finding of files 1–6 was verified present in §2–§5/§15/§16,
   ID by ID; §13 carries file 7's 18 rows. No finding depends on the originals.
2. **Self-declared superseded:** files 2 (via §15 recovery), 3, 5 and 6 carry
   their own supersession banners; files 1/4/7 are dated snapshots excluded from
   the doc gates by policy for the same reason.
3. **Nothing is lost:** git history at `3c67e14` holds every byte; this file
   indexes it.
4. **The 2026-09-16 external review files** (four root-level uploads) were
   incorporated the same way — see `COMPILED_AUDIT.md` §20 for that intake and
   its per-finding disposition; their originals are also in git history at
   `3c67e14`.
