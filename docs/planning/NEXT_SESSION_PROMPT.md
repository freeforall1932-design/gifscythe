# Next-session prompt (copy-paste hand-off)

Paste the block below into a fresh session to pick up the next post-S22
work fast. Everything it references lives in this repo.

---

```
CONTINUATION — gifscythe (freeforall1932-design/gifscythe), after S22 continuation.

1. RECOVERY (run these, in order, from the repo root):
   working_code/gifscythe/scripts/bootstrap_hooks.sh      # if G15 says core.hooksPath != .githooks
   working_code/gifscythe/scripts/check_docs.sh           # must be 0 failed. G18 FAIL = uncommitted work — commit NOW (cutoff loses it).
   #  G16 reports SKELETON vs filled. Inspect web/WEB_PLAN_TEMPLATE.md §1–§10:
   #  leftover <placeholders> = stay SKELETON; filled = flip both G16 lines to WORKING PLAN
   #  in the same commit (never auto-edit, never flip back).
   working_code/gifscythe/scripts/sweep_stale.sh          # must be 0 failed
   working_code/gifscythe/scripts/review_change.sh --pr N  # review the change; every flag has evidence
   working_code/gifscythe/scripts/pr_preflight.sh --online --body /tmp/pr_body.md
   #  before gh pr create, AND AGAIN BEFORE MERGE (not after). P3 dirty / P3b unpushed FAIL.

2. S17 closed N-07/P2-15 via sweep S2 standalone numeric UNTRIAGED counts.
   Regression: python3 working_code/gifscythe/tests/test_sweep_stale.py
   S4 is still a five-phrase list; unnumbered prose needs human review.
   S17 continuation closed GS-202/P0-6: upload names rejected if unsafe, all
   output targets contained, case/NFC collisions refused. Transport 42/42.
   DS-13/P1-32 also closed: /optimize checks response-buffer GIF magic; transport
   at that checkpoint 53/53. GS-207/P1-29 also closed: invalid non-empty engine
   overrides fail, source logged; CLI smoke 30/30, web transport now 63/63 on Linux.
   S18 (merged PR #23): Ms-PL relicense + C# plan + spike GREEN — then S19 PARKED
   the C# shell (OD-C7, exe stays C++17/Qt6), closed U-08 (Qt LGPL staged,
   packaging 36/36), answered OD-11 = a / OD-12 = a, and scaffolded web/wasm/
   (UNPROVEN: no emcc here; OD-16 licence question open, blocks shippable). S20 (OD-17): windows-only ship — linux zip + CI upload dropped, Windows job gained the packaging + manifest gates (run 34812043127 green). S21 closed U-68's 413 mapping (still PARTIAL on the cap-value question), and S22 finished the follow-up from the PR #26/#27 review: PR #27's web allow-list files were ported onto this branch, U-67 is now FIXED, and the two new regressions (`body-limit.test.mjs`, `static-hygiene.test.mjs`) are now in CI and `verify_audit.sh` (W4/W5). S22 continuation then closed U-53, U-60, U-61, U-62, U-64 and DS-09 with test-first native/web coverage.
   STARTING PRIORITIES NOW: U-54, U-55, U-56, U-57, U-63, U-65, U-66, U-69,
   GS-203/U-06, release blockers GS-204/GS-208/U-09/DS-06, and the Windows
   smoke + desktop probes.

   START HERE: STATUS.md (the single status register). COMPILED_AUDIT.md §5 is
   the detail. Never hand-edit STATUS.md's generated block — run check_docs.sh --emit.

   COMPILED_AUDIT.md is now v3 (2026-09-15 consolidation — the two scattered root
   intake copies were folded in and removed; the register is still 76, unchanged).
   New/recovered sections: §15 VP-1..VP-5 + false-positive guardrails (the "do NOT
   fix these correct behaviors" list §12 cites), §16 recovered Audit A/B positives +
   method, §17 intake E/F verdicts + delivery paths + 19 per-finding regression cases,
   §18 merge-completeness checklist, §19 NEXT-SESSION REVIEW ASK.
   DO §19 BEFORE TRUSTING ANY ✅ FIXED ROW: nothing in the compilation was executed
   in the consolidating sandbox (no Qt6/cmake/Windows/mingw/Wine/emcc). Re-run the
   §17.1 validation order, write the failing test FIRST for each OPEN row (cases are
   named in §17.2 / §7), and hunt for new pits — a fix that closes one row and
   reopens another is a FAILED fix. Watch the §19.3 pairings: U-01↔U-55/U-59,
   U-33↔U-53, U-46↔U-54, U-03↔DS-06/VP-1/VP-2, GS-203↔U-57. Windows-only rows
   (U-55/U-56/U-70/U-71) cannot be closed on Linux — mark PARTIAL with the exact
   remaining proof, never DONE.

   §19 mandate 5, learned in S21: MEASURE each sub-claim against the running code
   BEFORE fixing it. NF-10 filed three sub-claims and only one reproduced — "HEAD
   returns a body" is false (Node suppresses HEAD bodies) and the "raw prefix
   containment" traversal was latent, not reachable (new URL() normalises
   dot-segments first). §2F F-10 has the per-claim table; U-67 in §5 states only
   what measured true. Fixing all three would have shipped two confident fixes for
   non-bugs and called the row closed.

   Two node-only tests now exist that need NO engine (this sandbox has no compiler,
   so they are the only web coverage runnable without a build):
     node web/test/body-limit.test.mjs       # U-68, 8 cases
     node web/test/static-hygiene.test.mjs   # U-67, 43 cases
   Both are red→green proven. NEITHER IS WIRED INTO CI — build.yml lists the web
   tests explicitly and needs workflows scope (same block as
   docs/ci/PENDING_WORKFLOW_CHANGE.md, byte-identical twin copy), and
   verify_audit.sh needs a DOC_GATE_CHECKS bump for new W-gates (gate G6). Wire
   them deliberately; an unwired test is the U-18/GS-201 "green CI enforces
   nothing" failure mode. U-67/U-68 are PARTIAL pending that plus the engine-gated
   transport.test.mjs no-regression re-run. U-69 (stale After image) is browser-DOM
   behaviour — not exercisable headless here.

3. SKILLOPT ASK (owner): incorporate microsoft/SkillOpt INTO THIS REPO so any
   session's agent has it. See docs/planning/SKILLOPT_INTEGRATION_QUERY.md for
   the verified facts, the three non-negotiable conditions (quarantined /
   optional-and-inert / MIT notice preserved), and the four shapes
   (A submodule [recommended] / B vendored / C venv wrapper / D skip).
   Await OD-15 first. Do not vendor, submodule or pip-install before that answer.

4. DECISION BACKLOG: docs/planning/OWNER_DECISIONS.md (OD-01..OD-17). Answer in
   the form "OD-nn = <letter>" from that row's own options (OD-15 is a–d).
   OD-01 = a, OD-02 = a, OD-09 = b, OD-11 = a, OD-12 = a, OD-17 = a are already executed.
   Remaining: OD-03…OD-08, OD-10, OD-13…OD-16 (OD-16 blocks web/wasm shippable).

5. STANDING CONSTRAINTS: HARD RULE — commit every edit/write/delete into the
   repo before merge AND before the session can close (G18 / P3 / P3b). Do not
   wait to be reminded. Never merge a PR without an explicit yes; open one when
   the session goal includes it. Never hand-edit STATUS.md's generated block
   (edit COMPILED_AUDIT.md §5 then run check_docs.sh --emit); keep every gate
   green; never ask the owner for tokens (Arena handles GitHub auth).
```

---

**Notes for the author of this file:** this prompt is a convenience only — it
holds no state and may be regenerated. The authoritative hand-off is
`SESSION_HANDOFF.md`; the register is `STATUS.md`; the decisions are
`docs/planning/OWNER_DECISIONS.md`; the SkillOpt ask is
`docs/planning/SKILLOPT_INTEGRATION_QUERY.md`.
