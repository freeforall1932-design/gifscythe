# Skills intake — the dionysus Qt lint and its evidence

**Recorded:** S41, 2026-10-21 · **Status of the intake:** evidence kept, tool **not** installed
**Source:** `freeforall1932-design/dionysus` @ `9b5e0e6` (2026-10-20), folder `gifscythe/`
**Archive:** the owner's `gifscythe.zip` upload (18,491 bytes, sha256
`94e642e1b87123d6b4fb88557e7c8d529fad8bcd13db58c9709401b7a5cfbf1e`) was **removed from this
repo** in this change — it held six files, all byte-identical to that dionysus folder, so it was a
duplicate of material that already lives in the source repository. This file is the digest that
replaces it: nothing in the archive is lost by the deletion, and the one part worth keeping as data
(the lint baseline) is reproduced verbatim in §5.

> **Gate:** `OD-15` in `OWNER_DECISIONS.md` is still open. `PLANNING.md` §3 says *"Do not vendor,
> submodule or pip-install before that answer."* Nothing here installs anything. Running the lint
> as an external check is allowed; adding it to this repo or to CI is not, until `OD-15` is
> answered — and CI wiring would additionally sit under the U-89 "no new gates" freeze.

## 1. What was verified by running it

Run from `working_code/gifscythe`, with `LINT=<dionysus>/skills/qt6-cmake-cpp/scripts/qt_lint.py`:

| # | Command | Result |
|---|---|---|
| 1 | `python3 $LINT --selftest` | **PASS**, 0 failures |
| 2 | `python3 $LINT src tests CMakeLists.txt` | **9 findings** (5 warning, 4 info) · 5 at or above `--fail-on=warning` · **exit 1** |
| 3 | `python3 $LINT src tests CMakeLists.txt --baseline <dionysus>/gifscythe/qt_lint.baseline.json` | 9 findings · **0 not in baseline** · **exit 0** |
| 4 | same, `--json` | `{"total":9,"new":0,"baselined":9,"by_severity":{"info":4,"warning":5},"by_rule":{"QL002":2,"QL003":5,"QL006":2}}` |
| 5 | `python3 skills/validate_skills.py --root . --layout portable --quiet` (in dionysus) | **74 passed, 0 failed** |

Measured at branch tip `57d659c`, i.e. **after** the AUD-01/AUD-02 Qt changes: those changes add
**zero** findings. That is an independent check on Qt code this sandbox cannot compile.

The mutation check was done on a scratch copy, so the repo was never dirty: injecting
`process_->waitForFinished(9000);` produced **10 findings, 1 not in baseline, exit 1**. The baseline
is a gate, not decoration.

### The nine findings, and what each one is

| Rule | Where | Reading |
|---|---|---|
| QL003 ×5 | `src/qtui/MainWindow.cpp` — the five GUI-thread waits | **Exactly** the U-12 sites, already dispositioned (`OD-19 = a`, T26 is the standing guard). No sixth site. Independent confirmation the rule measures the right thing. |
| QL006 ×2 | `src/qtui/PreviewPanel.cpp:42,53` — `#202020`, `#909090` | **Real and still open.** Hard-coded hex in a palette-driven app; the concrete prerequisite for `D-06` and for any `OD-21` redesign. `SettingsPanel.cpp:51` already uses `palette(mid)` correctly. |
| QL002 ×2 | `tests/test_gui_offscreen.cpp` | Info-only in test files; both receivers outlive the test. Nothing to do. |

### The baseline does not rot on refactors

Our edits moved the QL003 findings from lines 217/1158/1234/1263/1530 to 1257/1294/1587-ish and the
baseline **still matched** — keys are `rule|path|normalised line text`, not line numbers. The tool
keeps working across ordinary churn.

## 2. Worth keeping

1. **The lint as a pre-PR external check.** It costs seconds, needs only Python 3.10+ and the
   standard library, and it independently re-derives our own registered debt. Suggested use: run
   commands 1 and 3 above before opening a PR; treat any *new* key as a question, not a rule.
2. **The two QL006 sites.** Carry them into the UI work when `OD-21` unblocks GUI edits — they are
   the cheapest item in the whole intake and a prerequisite for dark mode (`D-06`).
3. **`references/subprocess-engine.md` §6 and `references/ui-redesign.md`.** The engine contract in
   §6 (verify-then-claim, snapshot before launch, explode by directory diff, cancel keeps the
   previous good output, per-batch settings snapshot) is an independent statement of the same rules
   AUD-01/02/04 enforce — useful as a review checklist for future engine-facing changes. The
   redesign reference (state inventory, objectName contract, harness-migration plan) is the closest
   thing this repo has to a written procedure for the `OD-21` work.

## 3. Stale — do not act on these without checking `STATUS.md`

The drop-in's `NEXT_SESSION.md` state table is dated to the S40 run and has drifted:

| Drop-in says | Register says |
|---|---|
| "179 DONE / 7 PARTIAL / 14 OPEN" | **183 / 7 / 14 / 0 = 204** |
| `GS-210` CMake-first dispatch — OPEN/PARTIAL | **DONE** |
| `F-02` data loss on Cancel — **OPEN** | **DONE** (`U-59`), and hardened further by AUD-01/AUD-02 |
| (no audit rows) | `N-46`–`N-50` post-date it |

`F-02` is the dangerous row: followed literally, the next session would re-open work that is
closed. The drop-in says so itself — *"the project's own `STATUS.md` wins the moment it disagrees."*

Correct in the drop-in and still correct here: `U-12` (DONE by measurement), `OD-19` (answered `a`),
`OD-15` / `OD-21` / `OD-22` (all still open), and every command it names
(`check_docs.sh`, `sweep_stale.sh`, `bootstrap_hooks.sh`, `build.sh --gui`).

## 4. One skill rule this repo has outgrown — send upstream

`skills/qt6-cmake-cpp/references/subprocess-engine.md` §6 rule 4 ends:
*"…and for modes that already wrote partial frames, say so explicitly."* Since AUD-02 (`N-48`)
Explode writes to a claimed staging prefix and promotes, so a cancel leaves no partial frames and
the honest status is the flat `Cancelled.` The rule needs the conditional ("if the mode writes in
place") or a later reader will restore the old warning.

Ready-to-paste row for dionysus `SKILL_DEFECT_LOG.md`:

| # | rule / section | what happened | evidence | fix | promote? | status |
|---|---|---|---|---|---|---|
| 14 | `qt6-cmake-cpp/references/subprocess-engine.md` §6 rule 4 | The "say so explicitly for modes that already wrote partial frames" clause assumes in-place output. gifscythe's Explode now writes to a claimed `*.gs-partial[.N]` prefix and promotes on success, so a cancel writes and leaves nothing and the required status is the flat `Cancelled.` | gifscythe `STATUS.md` N-48; harness T21 asserts the flat status and that the frames survive | make the clause conditional on in-place modes; add a line that a staging-guarded mode reports plain "Cancelled" | y — "a cancel message must describe the write strategy, not the mode name" | open |

This row cannot be committed from this session: it belongs to `dionysus`, and this session's write
scope is the gifscythe branch.

## 5. The baseline, preserved as data

If dionysus moves or disappears, regenerate with
`python3 $LINT src tests CMakeLists.txt --write-baseline <file>` and diff — it must match this:

```json
{
  "keys": [
    "QL002|tests/test_gui_offscreen.cpp|QObject::connect(&ticker, &QTimer::timeout, [&ticks]() { ++ticks; });",
    "QL002|tests/test_gui_offscreen.cpp|QObject::connect(g_hb.timer, &QTimer::timeout, [] {",
    "QL003|src/qtui/MainWindow.cpp|if (!process_->waitForStarted(5000)) {",
    "QL003|src/qtui/MainWindow.cpp|previewProcess_->waitForFinished(1000);",
    "QL003|src/qtui/MainWindow.cpp|process_->waitForFinished(2000);",
    "QL003|src/qtui/MainWindow.cpp|process_->waitForFinished(3000);",
    "QL006|src/qtui/PreviewPanel.cpp|frame->setStyleSheet(QStringLiteral(\"QFrame { background: #202020; }\"));",
    "QL006|src/qtui/PreviewPanel.cpp|label->setStyleSheet(QStringLiteral(\"background:#202020;color:#909090;\"));"
  ]
}
```

Eight keys cover nine findings: the two identical `waitForStarted(5000)` lines share one key. The
source file also carries `generated_by`, `generated`, `subject` and `note` metadata; only `keys`
participates in matching.

## 6. Why nothing was installed

- `OD-15` is open and `PLANNING.md` §3 forbids vendoring, submodule or pip-install before it is
  answered. The options are in `OWNER_DECISIONS.md`; the recommendation on file is `a` — a pinned,
  quarantined submodule — but a recommendation is not an answer.
- CI wiring would be a new gate, frozen under U-89 until the derived P0/P1 bar reads 0.
- A root `gifscythe/` folder next to `working_code/gifscythe/` is a naming trap.
- It is mechanically harmless: copied in and run, the only gate that fired was G18 (uncommitted
  work). `check_docs.sh` 23/1, `sweep_stale.sh` 5/0. The blocker is policy, not compatibility.

## 7. Not verifiable here

- The six Qt templates in dionysus are **still uncompiled** — this sandbox has `g++` but no `cmake`
  and no Qt6. The drop-in's own defect log row 8 stays open for that reason.
- The lint's precision on a second Qt code base is unknown; only this repo has been measured.
- Desktop appearance, clean-machine packaging, and any claim that these skills improve agent output
  remain unmeasured.
