# Next-session prompt (copy-paste hand-off)

Paste the block below into a fresh session to pick up S14-continuation work
fast. Everything it references lives in this repo.

---

```
CONTINUATION — gifscythe (freeforall1932-design/gifscythe), S14 continuation.

1. RECOVERY (run these, in order, from the repo root):
   working_code/gifscythe/scripts/bootstrap_hooks.sh      # if G15 says core.hooksPath != .githooks
   working_code/gifscythe/scripts/check_docs.sh           # must be 22 passed, 0 failed, 3 skipped
   working_code/gifscythe/scripts/sweep_stale.sh          # must be 5 passed, 0 failed, 0 skipped
   working_code/gifscythe/scripts/pr_preflight.sh --online --body /tmp/pr_body.md
   #  before gh pr create, and again before merge.

2. START HERE: STATUS.md (the single status register). COMPILED_AUDIT.md §5 is
   the detail. Never hand-edit STATUS.md's generated block — run check_docs.sh --emit.

3. SKILLOPT ASK (owner): incorporate microsoft/SkillOpt INTO THIS REPO so any
   session's agent has it. See docs/planning/SKILLOPT_INTEGRATION_QUERY.md for
   the verified facts, the three non-negotiable conditions (quarantined /
   optional-and-inert / MIT notice preserved), and the four shapes
   (A submodule [recommended] / B vendored / C venv wrapper / D skip).
   Await OD-15 first.

4. DECISION BACKLOG: docs/planning/OWNER_DECISIONS.md (OD-01..OD-15). Answer in
   the form "OD-nn = a|b". OD-01/OD-02 first (release blockers), then the rest.

5. STANDING CONSTRAINTS: docs-only unless a decision asks otherwise; do not
   implement fixes for untriaged findings; never hand-edit STATUS.md (edit
   COMPILED_AUDIT.md §5 then check_docs.sh --emit); keep every gate green;
   never ask the owner for tokens (Arena handles GitHub auth).
```

---

**Notes for the author of this file:** this prompt is a convenience only — it
holds no state and may be regenerated. The authoritative hand-off is
`SESSION_HANDOFF.md`; the register is `STATUS.md`; the decisions are
`docs/planning/OWNER_DECISIONS.md`; the SkillOpt ask is
`docs/planning/SKILLOPT_INTEGRATION_QUERY.md`.
