# Web-first split — proposed plan (v0 draft, for owner review)

> **STATUS: PROPOSAL ONLY. Nothing here is agreed, and nothing here is implemented.**
> This is the skeleton requested for review; the owner's own draft is expected to supersede or
> merge into it. All numbers, file references and statuses below come from this repository at
> `2d51347` + the S14 documentation corrections (`ddc4194`).

**Written:** 2026-09-12 (S14) · **Related:** `COMPILED_AUDIT.md` §13 (18-item intake),
`docs/audit/EXTERNAL_REVIEW_INTAKE_2026-09-12.md`, `STATUS.md` rows `GS-*`/`DS-*`,
`docs/planning/OFFLINE_BUILD_REVIEW.md`, `PROJECT_VISION.md`

---

## 0. What the owner asked for, and the one thing that must be decided first

**Ask:** "speed up the web-based app section instead of the exe app and portable version — split
the work", with the stated worry that letting one side race ahead will breed divergence bugs and
force rewrites.

**The blocker:** `PROJECT_VISION.md` does not merely leave web open — it pins the opposite:

| Pinned statement | Location |
|---|---|
| "the language **stays C++17/Qt6 through 1.0.0**; the `web/` build is a demo only" | `PROJECT_VISION.md:16` |
| Web build = "**Demo only** (`web/`, server-side POC + JS⇄C++ command parity); **not the product path**" | `PROJECT_VISION.md` progress table |
| "1.0.0 = Tabs + major controls + preview + clean Windows portable + no open U-01…U-10" | `COMPILED_AUDIT.md` §9 |
| Direction "pinned to **offline-only**" (S6) | `PROJECT_VISION.md:15`, `docs/planning/OFFLINE_BUILD_REVIEW.md` |

So "focus on web" is not a scheduling tweak — it is a **product-direction change** that touches
the vision, the 1.0.0 definition, the privacy story (offline-only vs a hosted web app) and the
legal/packaging story. That decision is yours; this plan gives you three options and states the
cost of each instead of picking one silently.

---

## 1. Option set — pick one before any sequencing

| | **A. Sprint split only** | **B. Web becomes the product surface** | **C. Dual surface, web as the fast lane** |
|---|---|---|---|
| What it means | Web stays a demo; desktop/portable drop to correctness-only while the demo gets the speed | The shipped product *is* the web app; desktop/CLI/portable are frozen at 0.1.0 | Web may add features fast; desktop stays the release-gated surface; features graduate web → desktop |
| Vision docs | unchanged | `PROJECT_VISION.md` (language/demo/POC rows), `COMPILED_AUDIT.md` §9 1.0.0 row and the offline-only decision all need rewriting | `PROJECT_VISION.md` gains a surface matrix; 1.0.0 keeps its desktop criteria |
| Contradicts a pinned decision? | no | **yes — needs an explicit reversal, recorded** | partly — needs a recorded amendment |
| Offline-only promise | kept (web demo is loopback-only, `GS_WEB_HOST` opt-in) | **at risk** — a hosted web app means user GIFs leave the machine; would need a privacy decision, not just code | kept if web is run locally (today's model) |
| What the release ships | Windows portable + GUI | a server + browser client | both, with the feature matrix stating which surface has what |
| Main risk | the *product* stops advancing; 1.0.0 stays blocked on desktop items you are deliberately not doing | two abandoned surfaces that still must compile in CI; every "quick web fix" is now a product regression surface | the split itself — this is the option that needs the guardrails in §4 |
| Honest recommendation | only if the web is genuinely just a test harness (it is no longer: `U-41` gave it modes, and `web/` has 572 + 288 + 238 + 89 lines of real logic) | choose it if you actually intend to ship a browser app; then say so in `PROJECT_VISION.md` and re-cut the release definition | choose it if you want speed **without** killing the desktop product — the tests below already exist to make it safe |

**Default if you want my recommendation:** **C**, because the anti-divergence machinery this
repository already has (parity tests, a whole-product CI job, one status register) is exactly what
option C requires, and because C keeps the desktop/portable release promise alive without asking
you to maintain two products' worth of features.

---

## 2. The divergence risk, measured (your worry, in repo terms)

There are **three copies of the same semantics** today:

| Semantics | Desktop copy | Web copy | Net that catches drift |
|---|---|---|---|
| argv building (flags, order, defaults) | `working_code/gifscythe/src/core/GifsicleCommand.h` (250 lines) | `web/command.mjs` (238 lines) | `web/test/command.test.mjs` (196 lines) |
| validation rules / ranges | `working_code/gifscythe/src/core/Validate.h` (96 lines) | `web/validate.mjs` (89 lines) | `web/test/validate.test.mjs` (129 lines) |
| process/engine plumbing | `src/core/EngineLocator.h`, `ProcessRunner.h`, `ExplodeVerify.h` | `server.mjs` `findEngine()`, `run()`, `snapshotPrefix()` | `web/test/transport.test.mjs` (353 lines) + CLI smoke |
| output planning | `src/core/OutputPlan.h` | server-side planning in `server.mjs` | partial (desktop-planning unit tests; web refuses collisions itself) |

**Drift is not hypothetical: the workflow file itself records that `U-03` proved the JS and C++
copies drift, which is why those two parity suites exist at all.**

Why asymmetry breeds rewrites:

1. A feature added in one copy adds a *code path*, not a *contract*. The other copy silently keeps
   the old behaviour for the same input (`DS-06` is exactly this: one sentinel, two meanings).
2. When the second copy is finally updated, it is copied from the first, not re-derived — so the
   second implementation inherits bugs that were fixed in the first, and the fix list resets.
3. The honesty work (U-01/U-04/U-17/U-24…) was all "one layer says success, another does not".
   Asymmetry re-creates that class faster than any other change mode.

---

## 3. The split (what each lane may do)

**Lane W — web** (`web/`, and only `web/`)
- May: add features, restructure its own JS, change the UI, add endpoints, add tests.
- May not: change shared *semantics* (see §4.1) without the desktop copy in the same commit.
- Gate: `web/test/command.test.mjs`, `validate.test.mjs`, `transport.test.mjs` + `check_docs.sh`.

**Lane D — desktop** (`working_code/gifscythe/src/`, CLI + Qt GUI)
- Maintenance mode by default: bug fixes, contract fixes, tests. No new features, no refactors,
  no "while we're here" cleanups.
- Every change must name the finding it closes and carry a test (the register already demands
  proof in the row).

**Lane P — packaging / release / portability** (`scripts/package_*.sh`, build scripts, CI)
- Maintenance mode, but **not** optional for releases: the open items here are release blockers
  (`GS-204`, `GS-208`, `U-08`, `U-09`).

**The floor for the frozen lanes** (this is what "don't care as much" may safely mean):
1. The build stays green in CI for both jobs — always.
2. No new untested write path, no new silent-failure class.
3. Existing suites keep passing and keep being run (nobody turns off the other lane's gate).
4. Version/schema compatibility is not broken (settings format, conf files, CLI flags).
5. Any release claim covers only what **both** surfaces actually have (§4.3).

---

## 4. Guardrails — how to move web-fast without buying a rewrite

**4.1 Contract-first rule.** Shared semantics = argv order/flags/defaults, validation rules,
mode meanings (Auto/Batch/Merge/Explode), naming/`stemOf` semantics, engine discovery order,
output-collision policy. Any change to those lands in **both** copies in the same commit, with a
parity assertion added. Implementation-local JS changes need no C++ change; contract changes
never travel alone. *Test:* if you cannot write the parity assertion, it was a contract change.

**4.2 Feature-flag or label every web-only feature.** Web-only behaviour must be visibly
"web preview" in the UI and never described as a product feature in `README.md` or release notes
until the desktop copy exists.

**4.3 Keep a surface matrix.** One small table (feature × surface × state) in `README.md`, updated
in the same commit as any surface change. It is the thing that stops "the docs say it works" from
becoming a bug report.

**4.4 Divergence-debt register.** Every intentional asymmetry gets a row: what differs, why, and
the trigger that forces convergence (usually "before 1.0.0" or "before the desktop copy of X").
`STATUS.md`'s hand-maintained block is the natural home; it already holds session-scoped items.

**4.5 No cross-lane "fix sweeps".** A bug found in the web copy is fixed in the web copy; a
sweep that rewrites both is a rewrite and needs its own decision.

**4.6 Consider de-duplicating the riskiest copy (strategic option, not required now).** The web
server could drive `gifscythe-cli` as its engine instead of re-implementing argv building in
`web/command.mjs`. Pros: one semantics owner; the parity suites largely become CLI tests; drift
becomes impossible for argv/validation. Cons: needs the CLI binary next to the server, adds a
process hop, and it must **wait on `GS-201`** (the CLI's Batch `-b` path) before the web routes
anything through it. This is the only proposal here that *removes* a duplication instead of
managing it.

---

## 5. Proposed sequencing (mapped to the intake, so speed buys safety)

| Phase | Content | Why here | Findings |
|---|---|---|---|
| **P0 — stop-loss** | CLI `--run` refuses Batch with no `output` (exit 2); web `/run` rejects path separators in upload names + asserts targets stay under the temp dir; strict `GS_ENGINE`; docs/CI sync | all are small, and two of them are in the lane you want to accelerate; GS-201 is the only **Critical** | `GS-201`, `GS-202`, `GS-207`, `GS-208` (sync + marker) |
| **P1 — web trustworthy** | shared output verifier (size + magic + changed-since-snapshot) wired into CLI, GUI and web; `/optimize` magic check; web concurrency cap / rate limit (the `U-06` remainder) | makes the web lane safe to build on; without this, "web-first" ships a surface that can report success wrongly | `GS-203`, `DS-13`, U-06 remainder |
| **P2 — web features (the speed part)** | whatever your draft specifies; each item passes §4.1–4.4 | the actual acceleration | your draft |
| **P3 — desktop catch-up bundle** | threads tri-state + spinner + validation, input admission, disposal picker, numeric domains, CLI/GUI verifier, GUI waits | batch them into one deliberate milestone rather than dribbling | `DS-06`, `DS-07`, `DS-09`, `DS-10`, `GS-205`, `GS-206`, `GS-203` (desktop half), `U-12` |
| **P4 — release / packaging** | fail-closed stager + negative tests, native config generation, build-option strictness, legal set, release re-cut | release-gated, not speed-critical | `GS-204`, `GS-209`, `GS-210`, `U-08`, `U-09` |

**The fuse rule (your fear, answered):** deferring Lane D/P *feature* work is safe; deferring
*contract* work past the point where the web has already moved is what causes rewrites. Concretely:
`DS-06` (threads), `GS-206` (numeric domains) and `GS-203` (success semantics) should not wait for
"later" if the web lane keeps emitting argv for the same confs — that is how the two copies become
two products.

---

## 6. What "don't care as much about exe/portable" must not mean

| Tempting | What it costs later |
|---|---|
| Skipping packaging fixes because web is the focus | a release that cannot be verified on a clean machine (`GS-204`), and `U-09`'s release evidence stays unusable |
| Letting the CLI rot (it is the desktop path people script, and option 4.6 would route the web through it) | `GS-201` is *destructive* today; `DS-08` makes exit codes unreadable to scripts |
| One-off web fixes that bypass the parity suites | restarts the `U-03` drift class, which is the most expensive bug family in this repo's history |
| Version/label drift (`gifscythe.pro` pinned 0.1.0, two workflow copies) | release claims disagree with artifacts — already this session's `GS-208` |
| Web-only features described as product features | user-bug reports against a surface that the release does not include |

---

## 7. Open questions your draft should answer

1. **Which option (§1) — A, B or C?** If B, `PROJECT_VISION.md` + the 1.0.0 definition must be
   edited in the same session, and the offline-only decision revisited explicitly.
2. **What is the web app's deployment model?** Loopback self-host (today), LAN, or public hosting?
   Only the first keeps the offline-only promise; the other two need a privacy/data decision.
3. **Which features define "web speed"?** (Presets? Multi-file UI? Preview/compare? Batch queue?)
   The plan's schedule block (P2) is empty until that is known.
4. **Does the web keep its own argv builder, or route through the CLI (§4.6)?** This is the single
   largest divergence-risk decision in the split.
5. **What is the release vehicle?** Desktop portable stays the 1.0.0 artifact (current vision), or
   does the web become the artifact, with packaging moving to "server install + browser support
   matrix"?
6. **How much desktop work is "frozen"?** My floor (§3) keeps build-green, tests-green and
   release-blocker fixes; anything less will show up as `GS-204`-class surprises at ship time.

---

## 8. How this plan plugs into the repo's rules

- The 18 intake items are already `UNTRIAGED` rows in `STATUS.md` (reviewers' ids `GS-*`/`DS-*`)
  with one pending line each in `WORKLIST.md`; the detail is in `COMPILED_AUDIT.md` §13 and the
  intake document.
- **Triage** = map the accepted items into `COMPILED_AUDIT.md` §6 fix-order ids (the register's
  `U-nn` space), then `check_docs.sh --emit`. Nothing is in the fix order until that happens.
- If the answer to §7.1 is B or C, `PROJECT_VISION.md` changes in the **same** commit as the
  triage, so an odd doc set cannot survive a session boundary (gate `G2`/`G5`/`G12`).
- Keep this file living: once the owner's draft lands, merge it here, replace this banner with a
  decision record (date, option chosen, who decided), and keep §4 as the standing guardrails.
