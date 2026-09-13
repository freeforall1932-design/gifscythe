# SkillOpt, in-repo (pinned upstream + wrapper)

Owner decision **OD-15 = a** (recorded session S18, 2026-09-13): SkillOpt is carried inside
this checkout so that any session can read and run it, and it is wired into **nothing** that
ships. The full acceptance argument, the alternatives that were measured and rejected, and
the first target are in [`docs/planning/SKILLOPT_INTEGRATION_QUERY.md`](../planning/SKILLOPT_INTEGRATION_QUERY.md);
this README is the operational half — what is here, how to use it, what it must never become.

## What this directory is

| Path | Role |
|---|---|
| `tools/skillopt/upstream` | git submodule pinned to the commit in [`upstream.pin`](upstream.pin). ~6 MB materialized (439 files at this pin, 531 commits upstream, `v0.2.0-318-g79124b3`) |
| [`upstream.pin`](upstream.pin) | machine-readable provenance: commit, `describe`, version, license, PyPI wheel sha256, who pinned it |
| [`LICENSE.SkillOpt`](LICENSE.SkillOpt) | upstream's MIT notice, byte-identical copy (1078 bytes, sha256 `9515a04280cf50ce154eb3ab8fa375c7cc5e6b45ea5cb23b7ff3830737020c15`), so attribution survives an uninitialized submodule |
| [`skillopt.sh`](skillopt.sh) | the one entry point: `status` / `init [--offline]` / `wheelhouse` / `run -- <cmd>` / `selftest` |
| [`selfcheck.sh`](selfcheck.sh) | 15 checks over the quarantine, the pin, the licence and the hygiene rules below; it passes with the subtree absent by skipping the three checks that need it |
| `.venv`, `wheelhouse`, `state`, `runs`, `outputs`, `.skillopt` | generated, git-ignored, disposable |

## The three conditions it is held to, and how each one is enforced

1. **Quarantined.** No file under `working_code/`, `web/`, `.github/workflows/` or `docs/ci/`
   names SkillOpt, so it cannot reach a package: the installers stage an explicit allow-list
   (`working_code/gifscythe/scripts/package_common.sh`), which a `tools/` directory is not part of.
   `selfcheck.sh` sections 1–2 fail the moment that stops being true.
2. **Optional and inert.** Nothing in `build.sh`, `check_docs.sh`, `verify_audit.sh`,
   `sweep_stale.sh`, `pr_preflight.sh`, `review_change.sh`, `.githooks/pre-push` or either CI copy
   reads this directory, so a clone that never runs `git submodule update --init` behaves exactly
   as it did before. Measured 2026-09-13 in a fresh clone of this commit: with the submodule
   uninitialized the gates reproduce the pre-integration totals (docs 23 passed / 0 failed, plus a
   `G10` SKIP any local clone has; sweep 5/0/0; audit 27/0/6) and `selfcheck.sh` passes 10 checks
   with 3 skips; with the subtree and venv present they are the same again. `git ls-files '*.md'`
   sees 0 upstream markdown files either way — only this README is tracked. `selfcheck.sh` section 3
   is the tripwire. The corollary is a rule for whoever edits here: **do not add a gate for SkillOpt**, and
   do not cite a path inside `tools/skillopt/upstream/` from a tracked doc — that tree is empty in a
   fresh clone and `check_docs.sh` G8 would then fail. Cite upstream as
   `microsoft/SkillOpt/<path>` instead.
3. **Attribution preserved.** MIT, `Copyright (c) 2026 Microsoft Corporation`, at
   the `LICENSE` file at the root of `tools/skillopt/upstream` once materialized, and in
   `LICENSE.SkillOpt` always.
   `selfcheck.sh` section 5 compares the two.

Hygiene rule that follows from the dirty-tree gate (`check_docs.sh` G18, `pr_preflight.sh` P3):
every generated path here is git-ignored, and `.gitmodules` carries `ignore = dirty` for the
submodule — chosen over `ignore = all` after measuring both, because `all` also hides a HEAD
that has drifted off the recorded pin. An in-tree `pip install ./upstream` leaves
`skillopt.egg-info/` and `build/` inside the pinned tree (upstream's own `.gitignore` covers
them); `skillopt.sh init` therefore builds from a copy under `$TMPDIR` and leaves the subtree at
0 changed paths, which `selfcheck.sh` section 6 checks.

## Using it

```bash
tools/skillopt/skillopt.sh status          # read-only: pin vs work tree vs venv
tools/skillopt/skillopt.sh init            # ~15 s, needs network for the dependencies
tools/skillopt/skillopt.sh selftest        # upstream's suite here + ./selfcheck.sh
```

For a sandbox with no route to PyPI, fill the cache once and install from it offline — both
timings below are measurements from 2026-09-13 on the pinned commit, not estimates:

```bash
tools/skillopt/skillopt.sh wheelhouse      # ~10 s -> 33 wheels, 29 MB (git-ignored)
tools/skillopt/skillopt.sh init --offline  # ~10 s, no network at all
tools/skillopt/skillopt.sh run -- skillopt-sleep status   # 0 nights, no proposals, rc 0
```

A venv is 172 MB and is deliberately never persisted: it is one `init` away, so this directory
stays small in every snapshot and PR diff.

## Acceptance evidence for the pin

* `tools/skillopt/skillopt.sh selftest` at commit `79124b37e9a6371e13b753f8bcd7adb1e493ade1`:
  upstream's own suite ran 1496 passed, 12 skipped, 353 subtests passed in ~30 s with no
  credential and no network (32.48 s on the first run, 26.62 s after a sandbox restart
  re-materialized the subtree — both measured 2026-09-13 here; `pytest` is the one extra venv
  package it needs). The same suite at the release tag `v0.2.0` ran 164 passed, 6 skipped in 1.2 s.
* `skillopt-train --help` and `skillopt-sleep status` both rc 0 from the offline venv.
* PyPI publishes `skillopt-0.2.0-py3-none-any.whl`, sha256
  `818db802507c6f82553fd24c75aa70c953ab0a712647f60e68e4595052c4b150`; that wheel is the build of
  tag `v0.2.0` = `e4ea6a6771e797ef820cdd8bfea64c57e0481065` (2026-07-02), while the pin here is the
  main-branch tip `v0.2.0-318-g79124b3` — 318 commits of post-release fixes, which is why the pin
  file records both.

## Moving the pin

1. `git -C tools/skillopt/upstream fetch --depth 1 origin <new-sha>` then `git -C tools/skillopt/upstream checkout <new-sha>`.
   The explicit fetch is not ceremony: `git submodule update --init --depth 1` can only reach a
   pin that is the fetched tip, which is why a shallow clone of a tag needed this step when
   measured on 2026-09-13.
2. Update the `commit`, `commit_date`, `describe`, `version` and PyPI lines in
   [`upstream.pin`](upstream.pin) — one commit moves the gitlink and the pin file together, which
   `selfcheck.sh` section 4 enforces against a staged-vs-HEAD mismatch.
3. Re-copy `LICENSE.SkillOpt`, run `tools/skillopt/skillopt.sh init` and `selftest`, then
   `tools/skillopt/selfcheck.sh`.

## What is deliberately not here

* **No training run.** Upstream reads backend config from `OPTIMIZER_*` / `TARGET_*`
  environment variables (deployment, backend, Azure OpenAI endpoint), and this repo commits no
  credential; the sandbox S18 measured in has none. `run -- skillopt-sleep status` is the
  offline-safe probe.
* **No seed skill, no environment adapter.** The first target — a doc-sweep skill trained
  against `working_code/gifscythe/scripts/sweep_stale.sh` — is registered as its own worklist
  item (`SW-05` in `STATUS.md`, contract in §5 of the integration query) and is blocked on model
  credentials. A SkillOpt environment is a `SplitDataLoader` plus an `EnvAdapter` subclass plus a
  YAML config; upstream has no plugin discovery, its registry is a plain dict in
  `microsoft/SkillOpt/scripts/train.py` and `microsoft/SkillOpt/scripts/eval_only.py`, and S18
  measured that injecting a key into either dict from a launcher is enough to register a custom
  env without editing the pinned tree. Keeping that launcher here, in this directory, is the plan —
  patching upstream in place is not.
* **Nothing that ships.** The product's Qt/CMake/MSVC trees, package staging and release gates are
  untouched; this directory is a workbench, outside the product tree, that a session may read.
