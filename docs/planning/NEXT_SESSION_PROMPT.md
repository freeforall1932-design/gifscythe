# Next-session prompt (copy-paste hand-off)

Paste the block below into a fresh session to pick up post-S18 work
fast. Everything it references lives in this repo.

---

```
CONTINUATION — gifscythe (freeforall1932-design/gifscythe), after S18.

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
   GS-203/U-06 and pending owner decisions remain unresolved.

   START HERE: STATUS.md (the single status register). COMPILED_AUDIT.md §5 is
   the detail. Never hand-edit STATUS.md's generated block — run check_docs.sh --emit.

3. SKILLOPT (landed S18, OD-15 = a): tools/skillopt/ carries microsoft/SkillOpt as a
   pinned submodule + wrapper. It is available to read and run, and it is wired into
   nothing that ships — never add a gate, build step or CI job that depends on it.
     tools/skillopt/skillopt.sh status          # pin vs work tree vs venv, read-only
     tools/skillopt/skillopt.sh init            # ~15 s; --offline installs from the wheelhouse
     tools/skillopt/selfcheck.sh                # quarantine, pin, MIT, hygiene; passes with nothing fetched
     tools/skillopt/skillopt.sh selftest        # upstream's own suite here (~30 s, no credentials)
   The subtree is optional: a clone that never runs 'git submodule update --init' is
   unaffected. Do not write inside tools/skillopt/upstream, and do not cite a path
   inside it from a tracked doc (G8 fails on a fresh clone — cite microsoft/SkillOpt/<path>).
   The doc-sweep experiment is SW-05: blocked on model credentials, contract in
   docs/planning/SKILLOPT_INTEGRATION_QUERY.md §5.

4. DECISION BACKLOG: docs/planning/OWNER_DECISIONS.md (OD-01..OD-15). Answer in
   the form "OD-nn = <letter>" from that row's own options (OD-15 is a–d).
   OD-01 = a (S15), OD-02 = a (S16) and OD-15 = a (S18) are executed.
   Remaining: OD-03…OD-14.

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
`docs/planning/OWNER_DECISIONS.md`; the SkillOpt record — the review that settled
the shape and the contract the remaining experiment needs — is
[`docs/planning/SKILLOPT_INTEGRATION_QUERY.md`](SKILLOPT_INTEGRATION_QUERY.md), and
the operational half is [`tools/skillopt/README.md`](../../tools/skillopt/README.md).
