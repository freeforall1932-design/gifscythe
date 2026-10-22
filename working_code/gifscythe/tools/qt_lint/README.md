# `qt_lint` — vendored, pinned, quarantined (OD-15 = b)

A static scan for Qt 6 / CMake anti-patterns, vendored from the owner's
`freeforall1932-design/dionysus` repository at the owner's direction
(`OD-15`, answered S41, 2026-10-22).

## Provenance

| Fact | Value |
|---|---|
| Upstream repository | `freeforall1932-design/dionysus` |
| Upstream path | `skills/qt6-cmake-cpp/scripts/qt_lint.py` |
| Upstream revision | `9b5e0e6a5a56ae407701650c728212dee7947f05` (2026-10-20) |
| `qt_lint.py` sha256 | `50194e4e75efca45eb6bdba699a567c380ceeda87806a28e739053ee6a8673c4` — **byte-identical to upstream at that revision** |
| `qt_lint.baseline.json` | **generated here**, not copied — see below |
| Vendored | S41, 2026-10-22 |

`dionysus` declares no `LICENSE` file. It is the same repository owner's own work,
vendored here by that owner's direction; this note records the fact so that a
future licence decision upstream can be reconciled rather than discovered.
Nothing else from `dionysus` is vendored: the skills prose, references and
templates stay outside this repo (see `docs/planning/SKILLS_INTAKE.md`).

## Quarantine rules

- **No network, no install, no auto-update.** Standard library only; the copy
  runs as-is with `python3`.
- **Pinned.** The file is in the `verify_reference_pins.sh` set list, so CI fails
  on any drift, deletion or added file in this directory — the same U-10 / A-09
  enforcement the vendored gifsicle trees get.
- **Updating is a deliberate, reviewable act:** copy the new upstream bytes,
  re-run `./scripts/verify_reference_pins.sh --emit`, say why in the commit
  message, and update the table above.
- **A finding never flips a project status on its own.** It is a candidate for a
  human to read, per the tool's own header and the universal guide.

## The baseline is this project's, not upstream's

Upstream's `gifscythe/qt_lint.baseline.json` was generated on `main@97b0758` and
carried 8 keys covering 9 findings, including two `QL006` keys for the hard-coded
`#202020` / `#909090` in `PreviewPanel.cpp`. **Those two sites were fixed in S41**
(palette roles, `N-52`), so the baseline here was regenerated against this tree:

- 7 findings, **6 keys**: 5 × `QL003` (the five U-12 waits, dispositioned
  `OD-19 = a` with T26 as the guard — do **not** re-open) and 2 × `QL002` info in
  the test harness (both receivers outlive the test).
- Six keys for seven findings: the two identical `waitForStarted(5000)` lines
  share one key, by design.

Regenerate with, from `working_code/gifscythe`:

```sh
python3 tools/qt_lint/qt_lint.py src tests CMakeLists.txt --write-baseline tools/qt_lint/qt_lint.baseline.json
```

Review the diff before committing: a baseline that grows silently is how a gate
becomes decoration.

## Running it

Always from `working_code/gifscythe` — baseline keys are relative to the current
directory, and a wrong cwd fails loudly instead of passing silently:

```sh
python3 tools/qt_lint/qt_lint.py --selftest                      # 24 checks, proves the copy works
python3 tools/qt_lint/qt_lint.py src tests CMakeLists.txt \
    --baseline tools/qt_lint/qt_lint.baseline.json               # exit 0 = nothing new
```

Without `--baseline` the same command exits 1 on the five baselined `QL003`
warnings — that is the tool working, not a regression.

## CI

`docs` job of `.github/workflows/build.yml` (and its byte-identical copy
`docs/ci/build.yml.proposed`): a selftest step, then the baselined scan. Any
**new** key fails the job. This is a new gate and therefore an explicit,
recorded owner exception to the U-89 freeze (`WORKLIST.md`), taken in S41.
