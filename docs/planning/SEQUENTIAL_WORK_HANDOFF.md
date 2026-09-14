# Remaining work after S17's high-confidence sequence

Owner authorization (2026-09-13): implement highest→high-confidence work, leave
medium/low-confidence portions for another agent, then create a PR. No merge,
release, version bump, workflow edit or additional owner decision is authorized.
`STATUS.md` remains authoritative; this file supplies execution details.

## What is closed

**DS-11 / P2-14 is DONE.** S5/G17 checks both contradiction directions, ignores
`Original report:` history, prefers explicit §5 references, and fails uncheckable
current claims. 20 regressions pass; baseline falsely passes three OPEN-vs-fixed
variants. Python failure is a gate failure; absent Python is an explicit SKIP.

## What the next suitably equipped agent must finish

| Finding | Already implemented/proved locally | Remaining work / required environment |
|---|---|---|
| **GS-210 / P2-13 — PARTIAL** | Both build scripts parse every argument before side effects; unknown rc=2, help rc=0. 12 isolated cases pass; original scripts fail all 12. Native build passes. | Qt6 + CMake/qmake: choose and test CMake-first/only dispatch, remove `.pro` version drift using the product source of truth, exercise missing/broken tools and clean output trees. Do not assume the current qmake-first path was changed. |
| **GS-204 / P1-26 — PARTIAL** | Both wrappers use `package_common.sh`, private staging, required non-empty manifests, explicit target extensions, fresh destinations and strict headless scope. 36 Linux checks cover failures, both types and real native engine/CLI artifacts in isolated trees (30 + 2 Ms-PL cases S18 + 4 Qt LGPL cases S19). Portable Windows GUI refuses absent, failing or incomplete deployment fixtures. | Real Qt Windows build, real `windeployqt`, dependency/architecture inspection, clean-machine GUI startup and encode; test system-dependent package with documented installed Qt. Linux GUI runtime bundling is not implemented. Filename extensions are target-selection policy, not executable-format/architecture validation. Keep the U-09 release/artifact provenance blocker (U-08 licensing closed S19). |
| **GS-203 / P1-25 — PARTIAL** | Core `OutputVerify.h` and JS `output-verify.mjs`: new or size/mtime-changed, non-empty regular file, exact GIF87a/89a prefix. CLI explicit ordinary file outputs use it; `/optimize` and `/run` ordinary modes verify and serve the same buffer. Smoke 40/40 (12 core assertions), transport 67/67 on Linux. | Qt-equipped agent integrates the core helper into `MainWindow` single/queue/Merge lifecycles; capture before launch, verify only after exit zero, retain correct busy/cancel/error state. Test no-write, stale valid file, text/PNG/short output, valid new/refreshed output, source preservation and cancellation. Run offscreen plus real desktop probes. Do not silently buffer binary stdout or break `--info`; those contracts were intentionally unchanged. |

## Contract limits, not hidden claims of completion

- GIF magic is **not full decoding**. Six signature bytes alone satisfy the
  signature predicate, matching existing Explode/DS-13 behavior. A full structural
  decoder/engine validation policy needs a separate considered change.
- Size/mtime snapshots conservatively reject identical rewrites within filesystem
  timestamp granularity. They do not establish cryptographic provenance or defend
  against an adversarial local process restoring metadata.
- CLI verification reports failed postconditions; it does not roll back files the
  engine wrote. GS-202 still assumes a trusted engine/private web request directory.
- GUI integration is **not done**. Existing Explode verification stays unchanged;
  web shares its magic predicate, not one cross-language compiled verifier.
- Windows deployment tests use fake `.exe`/DLL fixtures and fake deployers; no
  Windows binary was executed. Missing Qt/CMake prevented actual GUI builds here.
- Linux packaging tests must not overwrite an already-produced release GUI bundle:
  real engine/CLI binaries are copied into disposable fixture trees for headless
  tests. Keep this isolation (CI asserts the GUI package after running the suite).
- No changes to `reference_code/`, product 0.1.0, SKELETON template, pending
  workflow proposal, or OD-03…OD-15. Do not turn these into collateral tasks.

## Repeatable local checks

From the repository root:

```sh
python3 working_code/gifscythe/tests/test_sweep_stale.py
python3 working_code/gifscythe/tests/test_build_options.py
working_code/gifscythe/build.sh
working_code/gifscythe/scripts/smoke_cli.sh
working_code/gifscythe/scripts/test_package.sh
node web/test/command.test.mjs
node web/test/validate.test.mjs
node web/test/transport.test.mjs
working_code/gifscythe/scripts/verify_audit.sh
# Commit before the clean-tree documentation/PR gates; do not weaken G18/P3b.
working_code/gifscythe/scripts/check_docs.sh --no-gate-run
working_code/gifscythe/scripts/pr_preflight.sh --online
```

The full audit now includes the Python sweep/build-parser regression suites as
F3/F4. Existing CI smoke/transport/package steps exercise the expanded runtime
and packaging tests; this sequence does not alter workflow files. Re-check the
remote CI for the exact pushed SHA; local skips never count as platform proof.
