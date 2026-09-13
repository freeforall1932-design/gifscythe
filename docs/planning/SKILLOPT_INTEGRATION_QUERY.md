# SkillOpt integration — the query, the answer, and the contract for the first experiment

**Status:** `OD-15` answered **a** by the owner on 2026-09-13 and **executed the same
session (S18)**: the integration lives in `tools/skillopt/` and `SW-04` is closed. What
remains is the experiment, registered as `SW-05` and blocked on model credentials.
This file keeps the request, the verified facts, the three non-negotiable conditions, the
four shapes with the measurements that settled them, and the env-adapter contract §5.
Read `tools/skillopt/README.md` for operations.

## 1. The request

Incorporate **microsoft/SkillOpt INTO this repo** — so that any session's agent
has it available inside the checkout — rather than installing it into whatever
model a given session happens to run. The goal is a reusable, in-repo mechanism
that lets a future session *train a natural-language skill* against this repo's
own ground truth, instead of hand-writing every rule.

## 2. Verified facts (do not re-derive)

| Fact | Value |
|------|-------|
| Upstream repo | `microsoft/SkillOpt` |
| Licence | MIT — `Copyright (c) 2026 Microsoft Corporation` |
| Language / packaging | Python — `pyproject.toml` + `requirements.txt` |
| Size | 531 commits |
| Project page | `microsoft.github.io/SkillOpt` |
| Paper | `arXiv:2605.23904` |
| What it does | A **text-space optimizer** that trains reusable natural-language skills for **frozen** agents via trajectory-driven edits, gated by a **validation loop** that only keeps an edit if it improves a scored metric; the result is deployed as `best_skill.md`. |
| Entry points | `microsoft/SkillOpt/scripts/train.py` and `microsoft/SkillOpt/scripts/eval_only.py`; the wheel also exposes console scripts `skillopt-train`, `skillopt-eval` and a `skillopt-sleep` CLI (`run`/`dry-run`/`status`/`adopt`/`harvest`/`schedule`/`unschedule`/`evalkit`) |
| Backends | *Corrected S18.* The shipped CLI `--backend` choices at the pin are `azure_openai`, `codex`, `codex_exec`, `claude`, `claude_chat`, `claude_code_exec`, `cursor`, `cursor_exec`, `copilot`, `copilot_chat`, `copilot_exec`, `qwen`, `qwen_chat`, `minimax`, `minimax_chat`. There is **no `openai_compatible` CLI choice** — that backend exists at config level only, so the old "OpenAI-compatible / vLLM" row over-stated the CLI surface |
| Runtime needs | API credentials + network for a train/eval run; **not** for install, import, `--help`, `skillopt-sleep status`, or upstream's own test suite. Backend config comes from `OPTIMIZER_*` / `TARGET_*` (plus `REFLACT_MODEL_BACKEND`, `QWEN_CHAT_MODEL`, `OFFICEQA_SEARCH_API_URL`) — so no credential is ever written into this repo |
| Pins (verified 2026-09-13) | tags are only `v0.1.0` and `v0.2.0`; `v0.2.0` = `e4ea6a6771e797ef820cdd8bfea64c57e0481065` (2026-07-02). The pin used here is the default-branch tip `79124b37e9a6371e13b753f8bcd7adb1e493ade1` (2026-09-06, `v0.2.0-318-g79124b3`), whose `pyproject` still says version 0.2.0 — 318 commits of post-release fixes ahead of the tag |
| PyPI | publishes `skillopt==0.2.0`, wheel `skillopt-0.2.0-py3-none-any.whl` sha256 `818db802507c6f82553fd24c75aa70c953ab0a712647f60e68e4595052c4b150`, requires-python >=3.10, "Development Status :: 3 - Alpha" |
| Cost of the wrapper | materialized subtree ~6 MB / 439 files (512 KB of git dir when shallow); venv **172 MB** after `pip install` (~15 s with network, ~10 s from a 29 MB / 33-wheel wheelhouse); the venv and wheelhouse are git-ignored, never committed |
| Offline acceptance | upstream's own suite is the acceptance test: **1496 passed, 12 skipped, 353 subtests** in ~30 s at `79124b37` and **164 passed, 6 skipped** in 1.2 s at `v0.2.0` — both with no credential and no network. `tools/skillopt/skillopt.sh selftest` runs it |
| Upstream shape (why the wrapper is thin) | an environment is a `SplitDataLoader` + rollout + an `EnvAdapter` subclass + a YAML config; there is **no plugin discovery** — the registry is a plain dict in each entry script |

*(Upstream paths are cited as `microsoft/SkillOpt/<path>` on purpose — the
documentation gate **G8** fails a backticked path whose first segment is a repo
directory that does not exist here, and this repo has no `microsoft/` tree. The
same trap applies to paths **inside** `tools/skillopt/upstream`: that tree is empty in
a fresh clone and in CI, so a citation of a file there fails **G8** for everyone who
comes after. Since S18 the repo has a `tools/` top-level directory, which puts every
backticked `tools/...` path in current-state docs under that check.)*

## 3. Verdict: possible, with three non-negotiable conditions

S18's review added one conclusion worth stating before the conditions: **SkillOpt is not
necessary for anything that ships.** No open release blocker depends on it — the remaining
blockers are `U-09` (release re-cut), `U-08` (licence set) and `GS-208`/`W-30` (needs a
`workflows`-scoped token); the Qt/CMake halves of `GS-203`, `GS-204`, `GS-205`, `GS-206`,
`GS-209` and `GS-210`; `DS-06`, `DS-07`, `DS-09` and `DS-12`, gated on `OD-13` and `OD-14`;
and the owner's web-plan draft. Its only claimed value is the trained doc-sweep skill, and the
mechanical detector it would compete with already ships mutation-tested. So it was accepted
as **optional infrastructure that nothing may depend on** — which is what condition 2 says —
and the experiment became its own register row rather than a reason to build anything.

SkillOpt can live in this repo **only if** all three hold, and each one now has a check
instead of an intention (`tools/skillopt/selfcheck.sh` sections are numbered to match):

1. **Quarantined from the product and the shipped packages.** It must not end up
   in `working_code/`'s build/packaging outputs — otherwise `GS-204` (packaging
   fail-open) and `U-08` (licence set) are violated by shipping a Python
   trainer + its MIT notice inside a Qt/C++ release bundle.
2. **Optional and inert.** No build step, no CI job, and **no gate** may depend
   on it. A missing/unset SkillOpt must never make `build.sh`, `verify_audit.sh`
   or `check_docs.sh` fail. S18 checked that twice: section 3 of `selfcheck.sh` greps the seven
   gate/build scripts and `.githooks/pre-push` for any reference (0 hits), and a **fresh clone
   that never initializes the submodule** — the state CI and every new session start in —
   reproduces the pre-integration baseline (measured 2026-09-13: docs 23 passed / 0 failed with
   one extra `G10` SKIP that any local clone has, sweep 5/0/0, audit 27/0/6), while the fully
   provisioned state measures the same totals again. `selfcheck.sh` itself passes 10 checks with
   3 skips when nothing is fetched. One claim deliberately *not* made: deleting `tools/` outright
   would move a gate, because current docs cite paths inside it and **G8** verifies citations —
   which is the §2 rule earning its keep, not a dependency on SkillOpt.
3. **MIT notice preserved** in whatever subtree holds it — and since a subtree can be
   uninitialized, the notice also lives beside it as `LICENSE.SkillOpt`, compared byte-wise by
   section 5 of `selfcheck.sh`.

## 4. Four integration shapes

| Shape | What it means | Notes |
|-------|---------------|-------|
| **A — pinned git submodule (chosen, `OD-15 = a`)** | `git submodule` at a pinned commit; quarantined outside `working_code/`; nothing vendored | **Measured gate-neutral**: `tools/` is 6.1 MB / 439 files, `git ls-files '*.md'` sees 0 upstream markdown files in a recursive and in a non-recursive clone alike, and **G8**/**G17**/**G18** behave identically with and without the subtree. One caveat that reading did not surface: content dirt inside the subtree makes the *parent* dirty and fails **G18**, so `.gitmodules` needs `ignore = dirty` — `ignore = all` was rejected after measuring it, because it also hides a HEAD that drifted off the recorded pin |
| **B — vendored pinned copy** | Copy the source at a pinned commit into a quarantined dir | **Rejected on measurement.** The 105 upstream `.md` files become current-state docs: replaying **G8**'s own extraction over them gives 9 backticked paths that do not exist across 12 files, plus 29 lines tripping sweep **S3** (upstream prose states volatile things). Either scanner would have to be loosened, which is the opposite of what condition 2 means |
| **C — venv/pip wrapper (nothing vendored)** | A wrapper script that creates a venv and `pip install`s at a pinned ref | **Adopted as A's execution layer, not as the answer.** A fresh `pip install skillopt==0.2.0` into a new venv takes ~15 s and 172 MB, both CLIs work, and `skillopt-sleep status` is offline-safe — but a wheel in a venv is invisible to a session reading the checkout, which is the whole request. Shape A ships no console scripts, so C is what makes the pin executable |
| **D — skip** | Do not incorporate; keep SkillOpt external | Excluded by the owner's request; kept here because the review says the *product* does not need it, only the workflow does |

## 5. First experiment candidate (recorded, not started)

The **doc-sweep skill**: a natural-language skill that detects the same
"true-when-written, false-when-read" staleness that
`working_code/gifscythe/scripts/sweep_stale.sh` detects mechanically. It is the
ideal first target because it has **ground-truth that is scorable** — the sweep's
own mutation tests (inject staleness, expect the rule to FAIL) can be turned
into a training/eval signal — and the skill's output can be checked against the
sweep on the same corpus. **Not started, and not part of the integration** — it is
`SW-05` in `STATUS.md`, blocked on model credentials (this repo commits none).

The contract a future session needs, measured against the pinned tree so it is not
guesswork:

- An environment is four pieces: a `SplitDataLoader` subclass (train/eval/split items), a
  rollout function, an `EnvAdapter` subclass implementing `build_train_env`,
  `build_eval_env`, `rollout` and `get_task_types`, and a YAML config naming `env.name`,
  `env.skill_init`, `env.split_mode: split_dir` and `env.split_dir`.
- `microsoft/SkillOpt/scripts/train.py` resolves envs through `get_adapter(cfg)`, which
  consults a **plain dict** (`_ENV_REGISTRY`) and passes only the `cfg` keys the adapter's
  `__init__` signature accepts. `microsoft/SkillOpt/scripts/eval_only.py` has its own copy
  of that dict.
- There is no entry-point/plugin discovery. S18 verified that pre-injecting a key into
  `scripts.train._ENV_REGISTRY` (and into the `eval_only` copy) survives `get_adapter(cfg)`
  — so a launcher in `tools/skillopt/` can register `gifscythe_doc_sweep` **without editing
  the pinned tree**, which is what condition 1 and the "never patch upstream in place" rule
  in `tools/skillopt/README.md` require.
- The corpus generator can be deterministic: emit a temp tree of mutated docs plus the
  expected `S<n>` verdict, then score the skill's findings against
  `working_code/gifscythe/scripts/sweep_stale.sh` on the same tree.

## 6. Constraints list (for whoever implements)

- **Never shipped** in a product/portable/system package.
- **Never required by a gate** — `check_docs.sh`, `verify_audit.sh`, `build.sh`
  and CI must be indifferent to its presence.
- **No credentials in the repo** — API keys live in the environment, never in a
  committed file or in `STATUS.md`/plan docs.
- **A README in the subtree** must say *where this came from and why it is here*
  (provenance + the `OD-15` decision).
- **Never write an upstream path as if it were ours** — cite `microsoft/SkillOpt/<path>`
  (see §2). A bare scripts/train.py (written without the upstream prefix) is read
  as *our* `scripts/` and fails gate **G8**.
- **Keep the MIT notice** intact in the subtree.

## 7. Task list — where each item stands

1. **Done S18 (2026-09-13).** `OD-15 = a` answered by the owner and recorded in
   `docs/planning/OWNER_DECISIONS.md`.
2. **Done S18 (2026-09-13).** `tools/skillopt/` holds the submodule, `upstream.pin`,
   `skillopt.sh`, `selfcheck.sh`, the subtree README and `LICENSE.SkillOpt`.
3. **Registered, not started.** The doc-sweep skill is `SW-05` in `STATUS.md` and a pending
   line in `WORKLIST.md` "Next actions (ordered)", with the §5 contract above as its spec.

## 8. Open questions — as answered by the review

- **Q1** — repo-wide tool vs a one-off branch? **Repo-wide, on `main`**: the point of
  `OD-15 = a` is that any session reading the checkout finds it. Inertness, not exclusion,
  is what keeps `main` safe (condition 2, checked by `selfcheck.sh`).
- **Q2** — which backend and credentials? **Unresolved, and it is the only blocker on
  `SW-05`.** Upstream reads `OPTIMIZER_*` / `TARGET_*`; the S18 sandbox had no model
  credential of any kind, so nothing beyond install/`--help`/`status`/self-test was
  executable. No key belongs in the repo, in `STATUS.md`, or in a plan doc.
- **Q3** — is the doc-sweep skill the right first target? **Yes, as the only one with
  scorable ground truth** (§5). It is also the honest one: the thing it would improve is a
  detector the repo already mutation-tests, so a loss is a real result, not a gap.
- **Q4** — is a submodule acceptable? **Yes — `OD-15 = a`.** B was measured as a doc-gate
  cost (§4), C became the execution layer, and a submodule is what keeps the pin and the
  provenance in one record.

## 9. Query log

| Next | Who | Decision |
|------|-----|----------|
| S17 | owner | asked whether SkillOpt should be incorporated into the repo for every session's use; `OD-15` opened with recommendation **a** |
| S18 (2026-09-13) | owner | **`OD-15 = a`** — incorporate it, in-repo, for sessions reading it, **and do not implement it into the app**. Feasibility review first, its verdict reported before the build |
| S18 (2026-09-13) | agent | review verdict: not necessary for anything that ships; accepted as optional, inert infrastructure. Shapes measured (§4): A + C, B rejected, D excluded. `tools/skillopt/` landed, `SW-04` closed, `SW-05` opened for the doc-sweep experiment |

*(One line per exchange; keep the `OD-nn = a|b` format where applicable.)*
