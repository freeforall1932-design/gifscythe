# CI workflow status

> **⏳ There is a pending workflow change right now** — see
> **`docs/ci/PENDING_WORKFLOW_CHANGE.md`**. The two copies differ *on purpose*:
> the CI bot token has no `workflows` scope, so GitHub rejects pushes to
> `.github/workflows/`. `verify_audit.sh` gate **E9** SKIPs while that file
> exists and FAILs on any undeclared drift.

## Current state (2026-09-10, session S7)

- `.github/workflows/build.yml` builds **linux + windows** jobs: engine,
  static-linked CLI/tests, GUI (CMake; Ninja+MinGW on Windows), native E2E
  smokes, the offscreen GUI harness (`test_gui_offscreen`, 243 checks as of
  S7), `windeployqt` staging, portable packaging, and artifact upload
  (`gifscythe-linux`, `gifscythe-windows`, 14-day retention — binaries are
  banked on Releases instead; see `docs/release/RELEASE_PROCEDURE.md`).
- **Both jobs green on main** since 2026-09-07 (runs #23/#24 after PR #5
  merge `0ad1ff5`); S5/S6 merged via PR #6 (`9643654`); maintainer follow-up
  `c5efe07` switched the Windows engine step to `scripts/build_engine.sh`.
  The S7 branch must re-confirm green on both OSes (its harness additions
  are linux-verified locally; Windows Qt verification only happens in CI).
- Historical: the old "App lacks `workflows` permission" workaround (the
  `scripts/build_gifsicle.sh` compatibility shim) was **retired in S7** —
  the workflow calls `build_engine.sh` directly and the shim was deleted.

## Keeping the two copies in sync

`build.yml.proposed` is kept **byte-identical** to
`.github/workflows/build.yml` so the recipe survives even when workflow
pushes are blocked, and as review documentation. Verify with:

```bash
diff .github/workflows/build.yml docs/ci/build.yml.proposed   # must be empty
```

Drift history: after `c5efe07` updated only the live workflow (line 98,
`build_gifsicle.sh` → `build_engine.sh`), the copies diverged; S7 re-synced
the proposed copy. When editing the workflow, **edit both files in the same
commit** (a maintainer or a token with the `workflows` scope must push the
`.github/` change).

**S8 made drift a gate** (`verify_audit.sh` **E9**), so this cannot silently
recur. E9 has exactly one tolerated exception: while
`docs/ci/PENDING_WORKFLOW_CHANGE.md` exists, the drift is *declared* and E9
reports SKIP instead of FAIL. Delete that file in the commit that applies the
change and E9 goes back to enforcing byte-equality.

## Apply manually (the pending S8 change, or any future drift)

```bash
cp docs/ci/build.yml.proposed .github/workflows/build.yml
git add .github/workflows/build.yml
git commit -m "ci: sync Gifscythe build workflow"
git push
```
