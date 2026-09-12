# Web surface — plan TEMPLATE (v0, currently filled with the S14 draft)

**What this file is.** The reusable skeleton for a Gifscythe web-surface plan, parked in `web/`
so it is easy to find next to the code it describes. The slots below are filled with the
**current draft (v0, 2026-09-12, S14)** so you can see the expected shape and weight of each
section. Replace the content inside the slots with your own plan; keep the headings.

**Why a template.** So the owner can hand this file plus a rough draft to an agent and have the
draft *refitted* here — same meaning, same facts, this structure — instead of producing a second
document that drifts from `STATUS.md`, `COMPILED_AUDIT.md` and the vision.

---

## 0. Rules for whoever refits a draft into this file

1. **Keep every heading and their order.** Add sub-headings, never new top-level sections.
2. **Fill slots, do not extend them into new claims.** Every factual statement must be
   re-checkable in this repository at the commit you are working from.
3. **Never change meaning to make a point land.** If the draft contradicts a repo document, keep
   both statements and record the conflict under §9, marked `[CONFLICT]`. (Precedent: the audit's
   `"Original report:"` pattern.)
4. **Do not invent numbers.** Quote only counts you measured, and name where they came from.
5. **No status claims.** States live in `STATUS.md`; this file names ids (`U-nn`, `GS-nn`,
   `DS-nn`) and points at the register instead of restating state.
6. **Placeholders use angle brackets** (`<feature>`, `<owner decision>`), never backticks — the
   documentation gate checks backticked paths, and a fake path fails it.
7. **§5 is fixed boilerplate.** Its meaning is the anti-divergence contract; a refit may extend
   the examples but must not weaken the rules.
8. **After editing, run `working_code/gifscythe/scripts/check_docs.sh`** and keep it green.

---

## 1. Decision record

*Slot: one line per decision — date · decider · what was decided · what it supersedes. Decisions
only; rationale belongs in §3/§4.*

| Date | Decider | Decision | Supersedes |
|---|---|---|---|
| 2026-09-12 | owner | **The `web/` build is a supported product surface — an alternative to the `.exe` / portable build for users who prefer a browser UI.** It ships the same engine and the same command semantics, self-hosted (loopback by default; `GS_WEB_HOST` opts into a LAN bind). | The S5/S6 statements "`web/` build is a demo only" / "not the product path" (`PROJECT_VISION.md`, `docs/planning/OFFLINE_BUILD_REVIEW.md`, `docs/web/WEB_FEASIBILITY.md`) |
| 2026-09-12 | owner | **This file is a template**, not a second source of truth: owner drafts are refitted into it under §0's rules. | — |
| `<date>` | `<owner>` | `<open: hosted/cloud deployment? if yes, a privacy + data-handling decision is required, because the current promise is "no cloud, no telemetry">` | — |

**Working interpretation, to confirm:** "use web instead of exe/portable" = the user runs the
server locally on their own machine. That keeps the offline-only promise intact. A **hosted**
service would change the privacy story and needs its own decision row above.

---

## 2. Goal and scope

*Slot: what this plan accelerates, what it deliberately does not touch, and the success sentence.*

**Goal (current draft).** Let the web surface move at full speed — features, UI, and its own
tests — while the desktop (Qt GUI + CLI) and the packaging/release lanes are frozen to
correctness-only, **without** the two surfaces drifting apart into two products.

**In scope:** everything under `web/`; the shared semantic contracts it depends on (§5); the
tasks the web surface needs to become trustworthy as a product (§6, phases P0–P2).

**Out of scope here:** new desktop features; the language-migration question (still governed by
`docs/planning/OFFLINE_BUILD_REVIEW.md`); WebP/APNG (`PROJECT_VISION.md` keeps those behind 1.0.0);
the packaging/release work in phases P3–P4, which stays in `WORKLIST.md` / `RELEASE_PROCEDURE.md`.

**Success sentence (draft).** A user can pick either the desktop build or the self-hosted web
build, get the same results for the same settings, and neither surface can report success while
producing nothing (`GS-203` class) or write anywhere the user did not choose (`GS-201`/`GS-202`).

---

## 3. Current state (facts, at the commit this plan is written from)

*Slot: only facts that are true in the tree and re-checkable by file:line or command.*

- **Three copies of the same semantics exist today**, and they drift — this is not hypothetical,
  the CI workflow itself records that `U-03` proved the JS and C++ copies drift, which is why the
  parity suites exist.

  | Semantics | Desktop copy | Web copy | Net that catches drift |
  |---|---|---|---|
  | argv building | `working_code/gifscythe/src/core/GifsicleCommand.h` | `web/command.mjs` | `web/test/command.test.mjs` |
  | validation rules | `working_code/gifscythe/src/core/Validate.h` | `web/validate.mjs` | `web/test/validate.test.mjs` |
  | engine/process plumbing | `src/core/EngineLocator.h`, `ProcessRunner.h`, `ExplodeVerify.h` | `server.mjs` | `web/test/transport.test.mjs` + CLI smoke |
  | output planning | `src/core/OutputPlan.h` | server-side planning in `server.mjs` | partial (unit tests on the C++ side) |

- **The web surface already carries real logic**, not a toy: `web/server.mjs` 572 lines,
  `web/app.js` 288, `web/command.mjs` 238, `web/validate.mjs` 89, plus three suites
  (196 + 353 + 129 lines of tests).
- **Open intake items hit the web directly** (all `UNTRIAGED` in `STATUS.md`, evidence in
  `COMPILED_AUDIT.md` §13): `GS-202` (output paths derived from upload names), `GS-203`
  (success without verification), `DS-13` (`/optimize` serves non-GIF bytes as `image/gif`),
  plus the `U-06` remainder (concurrency cap / rate limit / engine-run bound).
- **The desktop side carries the release bar** until the owner says otherwise: `COMPILED_AUDIT.md`
  §9 defines 1.0.0 around the desktop artifact + clean-Windows evidence, and `U-09`'s re-cut is
  still open.

---

## 4. Lanes and boundaries

*Slot: who may change what, and the floor each frozen lane keeps.*

| Lane | Paths | May do | May not do | Gate |
|---|---|---|---|---|
| **W — web** | `web/` | features, UI, its own structure, new tests | change a shared contract alone (§5.1) | `web/test/*.mjs` + `check_docs.sh` |
| **D — desktop** | `working_code/gifscythe/src/` | bug fixes, contract fixes, tests | new features, refactors, "while we're here" cleanups | unit + smoke + GUI harness |
| **P — packaging / release** | `scripts/package_*.sh`, `build*.sh`, `.github/workflows/`, `docs/ci/` | release-blocking fixes | feature work | `test_package.sh`, workflow gates, CI |

**Floor for a frozen lane** (this is what "don't care as much" may safely mean):
1. CI stays green on both jobs.
2. No new untested write path and no new silent-failure class.
3. The other lane's suites stay enabled and passing.
4. No compatibility break in the settings format, conf files or CLI flags.
5. Release claims cover only what **both** surfaces actually have (§5.3).

---

## 5. Guardrails (fixed boilerplate — do not weaken in a refit)

**5.1 Contract-first rule.** Shared semantics = argv order/flags/defaults, validation rules, mode
meanings (Auto/Batch/Merge/Explode), naming and stem semantics, engine discovery order, and the
output-collision policy. A change to any of them lands in **both** copies in the same commit with a
parity assertion. If you cannot write the parity assertion, it was a contract change.

**5.2 Label web-only work.** A web-only behaviour is shown as such in the UI and is never described
as a product feature in `README.md` or release notes until the desktop copy exists.

**5.3 Keep the surface matrix.** One table (feature × surface × state) in `README.md`, updated in
the same commit as any surface change. It is what stops documentation from promising a feature the
other surface does not have.

**5.4 Divergence-debt register.** Every intentional asymmetry gets a row: what differs, why, and the
trigger that forces convergence. `STATUS.md`'s hand-maintained block is the home for it.

**5.5 No cross-lane fix sweeps.** A bug found in one surface is fixed there; a sweep that rewrites
both is a rewrite and needs its own decision.

**5.6 De-duplication option (strategic, not mandatory).** The server could drive `gifscythe-cli`
instead of re-implementing argv building in `web/command.mjs`. It removes the largest duplication,
but it requires the CLI binary next to the server and must wait on `GS-201` (the CLI Batch `-b`
path) before anything is routed through it.

---

## 6. Phases and sequencing

*Slot: ordered phases; each phase names the register ids it closes and the evidence it must
produce. Phase content is the owner's draft — the mapping to ids is the refitter's job.*

| Phase | Content (draft) | Register ids | Evidence expected |
|---|---|---|---|
| **P0 — stop-loss** | CLI `--run` refuses Batch with no `output` (exit 2); web `/run` rejects path separators in upload names and asserts every target stays under the request temp dir; strict `GS_ENGINE`; workflow-copy sync + marker deletion | `GS-201`, `GS-202`, `GS-207`, `GS-208` | a regression per item; the unsafe cases refused with named exit codes |
| **P1 — web trustworthy** | shared output verifier (non-empty + `GIF87a`/`GIF89a` + changed since snapshot) in CLI, GUI and web; `/optimize` magic check; `U-06` remainder (concurrency cap, rate limit, engine-run bound) | `GS-203`, `DS-13`, `U-06` remainder | transport + CLI tests, including a lying-engine case |
| **P2 — web features** | **the owner's draft fills this in** (the acceleration itself) | `<ids the draft names>` | per §5.1–5.4 |
| **P3 — desktop catch-up bundle** | threads tri-state + spinner + validation; input admission; disposal picker; numeric domains; CLI/GUI verifier | `DS-06`, `DS-07`, `DS-09`, `DS-10`, `GS-205`, `GS-206` (+ desktop half of `GS-203`) | unit + harness assertions named in each row |
| **P4 — release / packaging** | fail-closed stager + negative tests; native config generation; build-option strictness; legal set; release re-cut | `GS-204`, `GS-209`, `GS-210`, `U-08`, `U-09` | packaging negative suite + clean-machine run |

**Fuse rule (answers "will going web-first create more bugs?").** Deferring a lane's *feature* work
is safe. Deferring *contract* work is what buys rewrites once the other surface has already moved:
`DS-06` (threads sentinel), `GS-206` (numeric domains) and `GS-203` (success semantics) must not
wait if the web keeps emitting argv for the same settings.

---

## 7. Definition of done

*Slot: per phase, the observable that makes it finished — not a feeling.*

- A phase is done when every id it names has a `STATUS.md` row that can move state legitimately,
  and the proof column names a test, command or diff produced in that phase.
- The web surface is "product-grade" (draft bar): `GS-202`, `GS-203`, `DS-13` closed, the `U-06`
  remainder closed, and the surface matrix (§5.3) listing the web column honestly.
- The split itself is healthy when: CI green on both jobs, every intentional asymmetry in §5.4,
  and no contract change in the git log that touched only one copy.

---

## 8. Risks and mitigations

*Slot: risk → mitigation → the artefact that enforces the mitigation.*

| Risk | Mitigation | Enforced by |
|---|---|---|
| Two surfaces drift into two products | contract-first rule | parity suites + review of the commit, §5.1 |
| Web-only features leak into product claims | label + surface matrix | §5.2/§5.3 |
| Frozen desktop rots into a rewrite | the §4 floor + a scheduled catch-up phase (P3) | CI still building and testing it |
| "Speed" ships a dishonest web surface | P1 precedes P2 | transport tests, lying-engine fixtures |
| Release claims cover neither surface properly | P4 keeps the release bar where it is | `RELEASE_PROCEDURE.md` |

---

## 9. Open questions / decisions needed

*Slot: the questions the owner must answer; mark contradictions `[CONFLICT]`.*

1. **Hosted vs self-hosted** — confirm the working interpretation in §1. `[CONFLICT]` if a hosted
   service is wanted: it contradicts the current "no cloud, no telemetry" promise in
   `docs/planning/OFFLINE_BUILD_REVIEW.md`.
2. **Which features define "web speed"?** §6 P2 is empty until this is answered.
3. **Does the web keep its own argv builder, or route through `gifscythe-cli`?** (§5.6) — the
   largest divergence-risk decision in the split.
4. **Does the web surface get its own release artifact** (install + browser-support matrix), or
   does it keep shipping inside the repo until a later version?
5. **How much desktop work is "frozen"?** §4's floor keeps build-green, tests-green and
   release-blockers; anything less will surface at ship time.

---

## 10. Change log

*Slot: newest first — date · who · what changed in this file.*

- **2026-09-12 · S14 ·** file created as `web/WEB_PLAN_TEMPLATE.md` (moved from
  `docs/planning/WEB_FIRST_SPLIT_PLAN_2026-09-12.md`), filled with draft v0, and the §1 decision
  record added after the owner made the web-is-a-product decision. Registered in `WORKLIST.md`;
  the 18 intake ids it references are `UNTRIAGED` rows in `STATUS.md`.
- `<next>` · `<who>` · `<what>`
