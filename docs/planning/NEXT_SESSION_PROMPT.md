# Next-session prompt (copy-paste hand-off)

Paste the block below into a fresh session to pick up post-S19 work
fast. Everything it references lives in this repo.

---

```
CONTINUATION — gifscythe (freeforall1932-design/gifscythe), after S19.

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
   (UNPROVEN: no emcc here; OD-16 licence question open, blocks shippable).
   GS-203/U-06, release blockers GS-204/GS-208/U-09/DS-06, and the Windows
   smoke + desktop probes remain unresolved.

   START HERE: STATUS.md (the single status register). COMPILED_AUDIT.md §5 is
   the detail. Never hand-edit STATUS.md's generated block — run check_docs.sh --emit.

3. SKILLOPT ASK (owner): incorporate microsoft/SkillOpt INTO THIS REPO so any
   session's agent has it. See docs/planning/SKILLOPT_INTEGRATION_QUERY.md for
   the verified facts, the three non-negotiable conditions (quarantined /
   optional-and-inert / MIT notice preserved), and the four shapes
   (A submodule [recommended] / B vendored / C venv wrapper / D skip).
   Await OD-15 first. Do not vendor, submodule or pip-install before that answer.

4. DECISION BACKLOG: docs/planning/OWNER_DECISIONS.md (OD-01..OD-16). Answer in
   the form "OD-nn = <letter>" from that row's own options (OD-15 is a–d).
   OD-01 = a, OD-02 = a, OD-09 = b, OD-11 = a, OD-12 = a are already executed.
   Remaining: OD-03…OD-08, OD-10, OD-13…OD-16 (OD-16 blocks web/wasm shippable).

5. STANDING CONSTRAINTS: HARD RULE — commit every edit/write/delete into the
   repo before merge AND before the session can close (G18 / P3 / P3b). Do not
   wait to be reminded. Never open or merge a PR without an explicit yes.
   Docs-only unless a decision asks otherwise; never hand-edit STATUS.md (edit
   COMPILED_AUDIT.md §5 then check_docs.sh --emit); keep every gate green;
   never ask the owner for tokens (Arena handles GitHub auth).
```

---

**Notes for the author of this file:** this prompt is a convenience only — it
holds no state and may be regenerated. The authoritative hand-off is
`SESSION_HANDOFF.md`; the register is `STATUS.md`; the decisions are
`docs/planning/OWNER_DECISIONS.md`; the SkillOpt ask is
`docs/planning/SKILLOPT_INTEGRATION_QUERY.md`.
