# Pending workflow change — needs a token with the `workflows` scope

**Status:** ⏳ waiting on a maintainer (rewritten 2026-09-10, session S9).

`.github/workflows/build.yml` and `docs/ci/build.yml.proposed` currently
**differ on purpose**. The CI bot token used for these branches has no
`workflows` permission, so GitHub rejects any push that touches
`.github/workflows/`. Re-confirmed by an actual push attempt this session:

```
! [remote rejected] arena/01a08bb3-gifscythe -> arena/01a08bb3-gifscythe
  (refusing to allow a GitHub App to create or update workflow
   `.github/workflows/build.yml` without `workflows` permission)
```

The intended change lives in **`docs/ci/build.yml.proposed`**, which is why
`docs/ci/README.md` keeps that copy: *"so the recipe survives even when
workflow pushes are blocked"*.

`verify_audit.sh` gate **E9** and `check_docs.sh` gate **G7** normally fail on
any drift between the two files. Both report **SKIP** while this file exists —
declared drift is tolerated, undeclared drift still fails. **Delete this file
in the same commit that applies the change**, so E9/G7 go back to enforcing
byte-equality.

---

## ⚠️ First read this: the PREVIOUS pending change is already applied

The S8 change (web parity + transport tests, packaging negative tests, package
manifest assertion) was applied by the maintainer in
**`190d030` — "Enhance CI workflow with additional tests and assertions"**
(2026-09-10T12:54Z, parent `7187cbb` = the PR #11 merge).

That commit **did not delete this marker**, which is why S9 found the repo in a
contradictory state:

* the two workflow copies were **byte-identical**, so **E9 PASSED**;
* but this file still existed, and every current-state doc still quoted the
  *"pending"* numbers **23 PASS / 0 FAIL / 5 SKIP**;
* the real number, measured by running the gate, was **24 passed, 0 failed,
  4 skipped** — E9 had moved from SKIP to PASS and nothing had been updated.

That is recorded in `STATUS.md` as **N-01** and is now caught mechanically by
`check_docs.sh` gate **G6**, which runs `verify_audit.sh` and compares its
actual totals against every number the docs quote.

**Lesson encoded in the process:** deleting this marker is part of applying the
change, not a follow-up. `check_docs.sh` G6 now fails if the quoted gate count
and the measured one disagree, so a half-applied change cannot survive a
session boundary.

---

## What is pending NOW (session S9)

One step, added to the **linux job only**, immediately after `CLI smoke tests`:

```yaml
      # The docs are the context the next session starts from, so a stale
      # STATUS.md is corrupted input rather than a cosmetic problem. This gate
      # regenerates the status register from the repo and diffs it against the
      # committed one, so a hand-fudged roll-up cannot pass. --no-gate-run keeps
      # it from re-running verify_audit.sh, whose build/package/web steps this
      # job already runs (the same reason the whole of verify_audit.sh is
      # deliberately not a CI step).
      - name: Documentation status gate (STATUS.md register)
        working-directory: working_code/gifscythe
        run: ./scripts/check_docs.sh --no-gate-run
```

**Why it is there.** Nothing in CI looked at the docs, so a PR could merge with
a `STATUS.md` that no longer matched the repo. The gate regenerates the
register from `COMPILED_AUDIT.md` §5 and diffs it against the committed file,
then runs the vocabulary, contradiction, PARTIAL-must-explain, referenced-file,
`CHECK(`-count and branch/SHA-freshness checks.

**Why `--no-gate-run`.** Plain `check_docs.sh` runs `verify_audit.sh` to learn
its real gate totals (check G6). In CI that would re-run `build.sh`,
packaging and the three web suites — four steps this job already runs — and
roughly double job time. `--no-gate-run` makes G6 a SKIP; the pre-push hook and
any local plain run still execute it in full.

**Needs nothing extra on the runner:** bash, git and the checkout. No Qt6, no
cmake, no network.

## How to apply

```bash
cp docs/ci/build.yml.proposed .github/workflows/build.yml
git rm docs/ci/PENDING_WORKFLOW_CHANGE.md
git add .github/workflows/build.yml
git commit -m "ci: run the documentation status gate in the linux job"
git push        # needs a token/maintainer with the workflows scope
```

Then, **in the same session**:

1. Re-run `working_code/gifscythe/scripts/verify_audit.sh`. Deleting this file
   turns E9 back into a PASS, which **changes the totals**.
2. Re-run `working_code/gifscythe/scripts/check_docs.sh`. Gate **G6** will fail
   until every doc quotes the new number — that is the point.
3. Run `check_docs.sh --emit`, commit the regenerated `STATUS.md`, and confirm
   `check_docs.sh` is green.

## Scope decision recorded here on purpose

The whole `scripts/verify_audit.sh` is **not** added to CI, even though audit
**U-14** asked for the release gates to be enforced there. It would duplicate
four steps the linux job already runs (`build.sh --all`, `test_engine.sh`,
`smoke_cli.sh`, the offscreen harness), roughly double job time, and add
runner-specific failure modes. The targeted gates cover the actual gap —
package contents, the untested JS mirrors, and now the status register — while
`verify_audit.sh` stays the local one-command runner. Its headline number is
reproducible now that **U-38** is fixed (a missing toolchain is a SKIP, not a
FAIL).
