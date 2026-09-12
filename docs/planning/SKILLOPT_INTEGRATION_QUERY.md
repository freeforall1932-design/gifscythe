# SkillOpt integration — the query for the owner

**Status:** awaiting `OD-15` (see `docs/planning/OWNER_DECISIONS.md`). This file
records the request, the verified facts, the non-negotiable conditions, the four
integration shapes, and the open questions. It is a *query*, not a plan of
record: nothing is built until the owner answers.

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
| Entry points | `microsoft/SkillOpt/scripts/train.py` and `microsoft/SkillOpt/scripts/eval_only.py` |
| Backends | `claude_code_exec`, `codex_exec`, and OpenAI-compatible / vLLM |
| Runtime needs | API credentials + network |

*(Upstream paths are cited as `microsoft/SkillOpt/<path>` on purpose — the
documentation gate **G8** fails a backticked path whose first segment is a repo
directory that does not exist here, and this repo has no `microsoft/` tree.)*

## 3. Verdict: possible, with three non-negotiable conditions

SkillOpt can live in this repo **only if** all three hold:

1. **Quarantined from the product and the shipped packages.** It must not end up
   in `working_code/`'s build/packaging outputs — otherwise `GS-204` (packaging
   fail-open) and `U-08` (licence set) are violated by shipping a Python
   trainer + its MIT notice inside a Qt/C++ release bundle.
2. **Optional and inert.** No build step, no CI job, and **no gate** may depend
   on it. A missing/unset SkillOpt must never make `build.sh`, `verify_audit.sh`
   or `check_docs.sh` fail.
3. **MIT notice preserved** in whatever subtree holds it.

## 4. Four integration shapes

| Shape | What it means | Notes |
|-------|---------------|-------|
| **A — pinned git submodule (recommended)** | `git submodule` at a pinned commit; quarantined outside `working_code/`; nothing vendored | Pins provenance; keep the MIT notice in the subtree |
| **B — vendored pinned copy** | Copy the source at a pinned commit into a quarantined dir | Simpler than a submodule; must record the exact commit + upstream |
| **C — venv/pip wrapper (nothing vendored)** | A wrapper script that creates a venv and `pip install`s at a pinned ref | Nothing vendored; needs network + credentials at use time |
| **D — skip** | Do not incorporate; keep SkillOpt external | Acceptable if the owner prefers not to host it |

## 5. First experiment candidate (recorded, not started)

The **doc-sweep skill**: a natural-language skill that detects the same
"true-when-written, false-when-read" staleness that
`working_code/gifscythe/scripts/sweep_stale.sh` detects mechanically. It is the
ideal first target because it has **ground-truth that is scorable** — the sweep's
own mutation tests (inject staleness, expect the rule to FAIL) can be turned
into a training/eval signal — and the skill's output can be checked against the
sweep on the same corpus.

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

## 7. Ordered next-session task list

1. Get `OD-15` answered (shape A/B/C/D).
2. Add the submodule / vendored copy / wrapper at a pinned ref, quarantined,
   with its README + MIT notice.
3. Register the **first experiment** (the doc-sweep skill) as its own
   `WORKLIST.md` item — not as part of the SkillOpt task itself.

## 8. Open questions

- **Q1** — repo-wide tool vs a one-off branch? Does SkillOpt live on `main`
  (every session can reach it), or on a throwaway experiment branch?
- **Q2** — which backend and credentials? (`claude_code_exec` / `codex_exec` /
  OpenAI-compatible / vLLM — who provides the key, and where does it live?)
- **Q3** — is the doc-sweep skill the right first target, or is there a higher
  value one?
- **Q4** — is a submodule acceptable to the owner, or is a vendored copy (B) or
  wrapper (C) preferred?

## 9. Query log

| Next | Who | Decision |
|------|-----|----------|
| `<next>` | `<who>` | `<decision>` |

*(One line per exchange; keep the `OD-nn = a|b` format where applicable.)*
