# CI workflow status

> **⏳ There is a pending workflow change right now** — see
> **`docs/ci/PENDING_WORKFLOW_CHANGE.md`**. The two copies differ *on purpose*:
> the CI bot token has no `workflows` scope, so GitHub rejects pushes to
> `.github/workflows/` (re-confirmed by a real push attempt in S9).
> `verify_audit.sh` gate **E9** and `check_docs.sh` gate **G7** SKIP while that
> file exists and FAIL on any undeclared drift.
>
> **The *previous* pending change (S8: web parity + package manifest assertion)
> is already applied** — maintainer commit `190d030`. S9 found the marker still
> in place with every doc quoting the pre-application gate numbers; that is
> `STATUS.md` **N-01**.

## Current state (2026-09-10, session S9)

- `.github/workflows/build.yml` builds **linux + windows** jobs: engine,
  static-linked CLI/tests, GUI (CMake; Ninja+MinGW on Windows), native E2E
  smokes, the offscreen GUI harness (`test_gui_offscreen`, **324 checks as last
  *measured* in the S11 sandbox** — Qt 6.4.2; 306 in S10, 243 in S7),
  `windeployqt` staging, the three web parity/transport suites and
  the package-manifest assertion (both added by `190d030`), portable packaging,
  and artifact upload (`gifscythe-linux`, `gifscythe-windows`, 14-day
  retention — binaries are banked on Releases instead; see
  `docs/release/RELEASE_PROCEDURE.md`).
- **Documentation status gate — APPLIED (confirmed S14).** The linux job runs
  `scripts/check_docs.sh --no-gate-run`; the S9 change is in the live
  `.github/workflows/build.yml`, not only in `docs/ci/build.yml.proposed`. S14 introduced gate
  **G16** (web plan template state) into that checker. The two workflow copies still differ by one
  line (the Windows E2E temp-path fallback), so
  `docs/ci/PENDING_WORKFLOW_CHANGE.md` now describes *that* drift instead — see it for the
  remaining apply-and-delete step.
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

**S8 made drift a gate** (`verify_audit.sh` **E9**), and **S9 mirrored it in
`check_docs.sh` as gate G7**, so this cannot silently recur. Both have exactly
one tolerated exception: while
`docs/ci/PENDING_WORKFLOW_CHANGE.md` exists, the drift is *declared* and E9
reports SKIP instead of FAIL. Delete that file in the commit that applies the
change and E9 goes back to enforcing byte-equality.

## Apply manually (the pending S9 change, or any future drift)

```bash
cp docs/ci/build.yml.proposed .github/workflows/build.yml
git rm docs/ci/PENDING_WORKFLOW_CHANGE.md      # deleting the marker IS part of applying
git add .github/workflows/build.yml
git commit -m "ci: sync Gifscythe build workflow"
git push                                       # needs the workflows scope
```

Then re-run `scripts/verify_audit.sh` and `scripts/check_docs.sh`: removing the
marker turns E9 back into a PASS, which changes the gate totals, and **G6 fails
until every doc quotes the new number**. Run `check_docs.sh --emit` and commit
the regenerated `STATUS.md`. S9 found this exact half-applied state (marker
present, change applied, docs quoting the old numbers) — see `STATUS.md` N-01.
