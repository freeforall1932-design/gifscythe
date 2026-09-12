# Owner decisions — the open questions register

**What this is.** The 15 questions the owner must answer to unblock S14
continuation work. Each row is a *question*, not a work item: options, the
recommended answer, and what the answer unblocks. **No work items live here**
— once an answer lands, the corresponding `STATUS.md` / `WORKLIST.md` / plan
entry is updated, not this file.

**How to answer.** Reply in the form `OD-nn = <letter>`, using one of the letters
in that row's own **Options** cell — most rows offer `a`/`b`, but `OD-15` runs
`a`–`d`, so a fixed `a|b` form cannot express a real answer to it. Multiple
answers are fine; `OD-14` has two sub-questions and needs both.

| ID | Question | Options | Recommended | Unblocks |
|----|----------|---------|-------------|----------|
| **OD-01** | Triage the 18 untriaged findings into `COMPILED_AUDIT.md` §6 fix-order ids | (a) all 18 now · (b) the 5 release-blockers first | **b** — schedule the Critical/High release-blockers (`GS-201`…`GS-204`) before the lower-tier intake | Scheduling the release-blocker remediation in §6 |
| **OD-02** | Minimal `GS-201` stop-loss: CLI `--run` refuses Batch with no `output`, exit 2 | (a) ship it now (~10 lines, Critical only) · (b) wait for the full Batch redesign | **a** — a ~10-line refusal closes the only Critical before any redesign | Closing `GS-201` (the Critical: Batch can rewrite source GIFs) |
| **OD-03** | Refit the owner's web draft into `web/WEB_PLAN_TEMPLATE.md` and flip SKELETON → WORKING PLAN | (a) refit now + flip · (b) hold the template | **a** — the template exists for exactly this | The web plan becomes live (gate **G16** flips once, both lines) |
| **OD-04** | Web deployment model | (a) self-hosted only · (b) hosted/cloud too | **a** — keeps the "no cloud, no telemetry" promise | The web surface's privacy story stays unchanged |
| **OD-05** | Web argv handling | (a) keep the web's own builder now · (b) spike routing web through `gifscythe-cli` after `GS-201` | **a** now, **b** later | Unblocking a `gifscythe-cli`-based web argv spike after the stop-loss lands |
| **OD-06** | Web release artifact | (a) in-repo until 1.0.0 · (b) a separate release artifact now | **a** — desktop stays the 1.0.0 artifact | Deciding whether the web build gets its own release path pre-1.0.0 |
| **OD-07** | How frozen is the desktop lane | (a) the plan's floor (build-green, tests-green, release-blockers) · (b) a deeper freeze | **a** — correctness-only, not a code freeze | The desktop lane's allowed-change envelope |
| **OD-08** | Apply `docs/ci/build.yml.proposed` and delete `docs/ci/PENDING_WORKFLOW_CHANGE.md` | (a) apply + delete the marker · (b) leave pending | **a** — needs a `workflows`-scoped token | Returning gates **E9**/**G7** to byte-equality enforcement |
| **OD-09** | First-party licence | (a) GPLv3 + Qt LGPL notices staged, legal review before 1.0.0 · (b) other | **a** | Closing the `U-08` licence-set remainder |
| **OD-10** | Release re-cut | (a) re-cut from one reviewed SHA after the blockers · (b) re-cut now | **a** | `U-09` (the banked snapshot predates S7) |
| **OD-11** | Bump to 1.0.0 | (a) not yet · (b) bump now | **a** — gates + decisions still open | Version stays 0.1.0 until the release criteria are met |
| **OD-12** | Two-way CLI settings in 1.0.0 | (a) out · (b) in | **a** — the live pane stays honest one-way | Keeping the one-way CLI pane unchanged through 1.0.0 |
| **OD-13** | Threads contract (`<0` / `0` / `>0`) | (a) yes, restore the sentinel · (b) leave as-is | **a** — `DS-06`'s tri-state fix | The `threads` sentinel semantics (`DS-06`/`DS-07`) |
| **OD-14** | Disposal 4..7 + settings whitespace | (a) fix disposal 4..7 · (b) document settings quoting | **a** for disposal, **b** for settings whitespace | `DS-10` (unreachable disposal values) and `DS-12` (whitespace round-trip) |
| **OD-15** | SkillOpt: how to incorporate | (a) pinned git submodule, quarantined · (b) vendored pinned copy · (c) venv/pip wrapper · (d) skip | **a** — see `docs/planning/SKILLOPT_INTEGRATION_QUERY.md` | The first in-repo SkillOpt experiment (the doc-sweep skill) |

## Notes

- **OD-01 and OD-02 first** — they gate the release-blocker remediation; the
  rest are direction choices the plan can proceed without.
- **OD-15** feeds directly into `docs/planning/SKILLOPT_INTEGRATION_QUERY.md`,
  which holds the three non-negotiable conditions and the four integration
  shapes. Answering `OD-15 = a` unblocks the submodule shape.
- Nothing here is a `STATUS.md` row; the register stays the single source of
  work-item state.
