# Pending workflow change — needs a token with the `workflows` scope

**Status:** ⏳ waiting on a maintainer (created 2026-09-10, session S8).

`.github/workflows/build.yml` and `docs/ci/build.yml.proposed` currently
**differ on purpose**. The CI bot token used for this branch has no `workflows`
permission, so GitHub rejects any push that touches `.github/workflows/`:

```
! [remote rejected] arena/01a08a10-gifscythe -> arena/01a08a10-gifscythe
  (refusing to allow a GitHub App to create or update workflow
   `.github/workflows/build.yml` without `workflows` permission)
```

The intended change lives in **`docs/ci/build.yml.proposed`**, which is why
`docs/ci/README.md` keeps that copy: *"so the recipe survives even when
workflow pushes are blocked"*.

`verify_audit.sh` gate **E9** normally fails on any drift between the two
files. It reports **SKIP** instead while this file exists — declared drift is
tolerated, undeclared drift still fails. **Delete this file in the same commit
that applies the change**, so E9 goes back to enforcing byte-equality.

## How to apply

```bash
cp docs/ci/build.yml.proposed .github/workflows/build.yml
git rm docs/ci/PENDING_WORKFLOW_CHANGE.md
git add .github/workflows/build.yml
git commit -m "ci: run web parity + packaging gates in the linux job"
git push        # needs a token/maintainer with the workflows scope
```

Then confirm both jobs go green and the new steps appear in the linux run.

## What it adds (linux job only)

Two blocks, both derived from audit findings — none of them duplicate a step
the job already runs:

1. **Web JS/C++ parity + transport tests** — after `CLI smoke tests`.
   ```yaml
   - name: Web JS/C++ parity + transport tests
     run: |
       set -euo pipefail
       node web/test/command.test.mjs
       node web/test/validate.test.mjs
       node web/test/transport.test.mjs
   ```
   The `web/` demo ships a **second copy** of the command builder and the
   validation rules. Nothing in the C++ suites touches them, so before this
   step CI ran **none** of the three tests and the two clients could drift
   silently — audit **U-03** proved that they do (threads "Auto" emitted no
   `-j` on one side only). They need nothing but the CLI built by the step
   above and the Node already on `ubuntu-latest`.

2. **Packaging negative tests + manifest assertion** — after `Package portable`.
   ```yaml
   - name: Packaging negative tests (incomplete packages must fail)
     working-directory: working_code/gifscythe
     run: ./scripts/test_package.sh

   - name: Assert package manifest
     working-directory: working_code/gifscythe
     run: |
       set -euo pipefail
       version=$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' VERSION.md \
         | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
       pkg="release/$version/Gifscythe"
       for f in gifsicle gifscythe-cli gifscythe LICENSE COPYING.gifsicle README.txt; do
         if [[ ! -s "$pkg/$f" ]]; then
           echo "FAIL: $pkg/$f is missing or empty" >&2
           ls -la "$pkg" >&2 || true
           exit 1
         fi
       done
       echo "package manifest OK"
       ls -la "$pkg"
   ```
   Audit **U-02/U-08/U-14**: the packager used to exit 0 for a folder with no
   application in it, and CI uploaded that folder without looking inside — so a
   green build proved nothing about the release contents.

## Scope decision recorded here on purpose

The whole `scripts/verify_audit.sh` was **not** added to CI, even though audit
**U-14** asked for the release gates to be enforced there. It would duplicate
four steps the linux job already runs (`build.sh --all`, `test_engine.sh`,
`smoke_cli.sh`, the offscreen harness), roughly double job time, and add
runner-specific failure modes. These two targeted gates cover the actual gap —
package contents, and the untested JS mirrors — while `verify_audit.sh` stays
the local one-command runner. Its headline number is reproducible now that
**U-38** is fixed (a missing toolchain is a SKIP, not a FAIL).
