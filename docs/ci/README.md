# CI — workflow status, clean-Windows smoke, desktop probes (consolidated S24)

**Consolidated 2026-09-17 (S24):** this file absorbed `CLEAN_WINDOWS_SMOKE.md`
(§2) and `DESKTOP_PROBES.md` (§3). `PENDING_WORKFLOW_CHANGE.md` was **deleted in
S24** — the drift it declared was resolved (see §1), so gates **E9/G7/S1** are
back to enforcing byte-equality with no standing exception.

## 1. Workflow status

- **Two copies, byte-identical (again, S24):** `.github/workflows/build.yml`
  (live) and `docs/ci/build.yml.proposed` (doc copy). The one-line cygpath
  drift the pending marker tracked is resolved: the maintainer had already
  fixed the LIVE line in `414f5fc` (`${RUNNER_TEMP//\//}`, green on every
  Windows run since), while the proposed copy kept the pre-fix `\\/` form;
  S24 synced the proposed copy to the live-proven line and deleted the marker
  in the same commit, per the marker's own delete-rule. Edit both files in the
  same commit from now on; `verify_audit.sh` **E9** / `check_docs.sh` **G7**
  FAIL any undeclared drift.
- **The documentation status gate is live in CI** (linux job step
  "Documentation status gate (STATUS.md register)": `scripts/check_docs.sh
  --no-gate-run`). Applied by the maintainer (`190d030` era; confirmed S14),
  which is why register rows W-30/R-03/GS-208 closed in S24. `--no-gate-run`
  skips G6 (the linux job already builds and runs the web suites; re-running
  `verify_audit.sh` inside the gate would double the job).
- **Workflows-scope pushes work:** the token blocker recorded in S9 was lifted
  in S18 (scope granted, verified by the pushed `build.yml` change in
  `161e862`; PR #28 pushed workflow edits too). A push touching
  `.github/workflows/` still needs a token with that permission — the doc copy
  exists so the recipe survives even when a given token lacks it.
- **What the workflow runs:** linux + windows jobs — engine build,
  static-linked CLI/tests, GUI (CMake; Ninja+MinGW on Windows), native E2E
  smokes, the offscreen GUI harness, the Windows unit-test exe
  (`build/test_gifsicle_command.exe` — relevant to `U-71`/`U-94`-class rows:
  pure Win32 rule cases can be proven in CI without a VM), all **eight** Node
  web suites, packaging + manifest assertion (Windows ships; linux is the test
  battery since S20/OD-17), artifact upload (`gifscythe-windows`, 14-day
  retention; binaries are banked on Releases), the csharp-spike job (parked
  track, still CI-run), and the doc gate.
- **Gate places:** the gates run in three places — `.githooks/pre-push`
  (bootstrap once per clone: `scripts/bootstrap_hooks.sh`; `build.sh` does it),
  the linux CI job, and `scripts/pr_preflight.sh` at PR create **and** merge.
  **`scripts/review_change.sh`** is the separate diff reviewer (R1 edited check
  logic, R2 matchers that match nothing — how G10 stayed dead for five PRs —
  R3 prose counts vs live measurement, R4 lost executable bits, R5 obliged doc
  updates). It is run by a human/session at review time, not in CI.
- **History kept for context:** the S8 change (web parity + package manifest)
  was applied by the maintainer in `190d030` without deleting the old marker —
  that mismatch was finding N-01, and the reason the marker's delete-rule and
  sweep rule S1 exist. The `scripts/build_gifsicle.sh` shim workaround was
  retired in S7; do not reintroduce it.

## 2. Clean-Windows smoke checklist — gates C4 / D3 / D4 (W-18)

**Purpose:** prove the portable Windows bundle runs on a machine with **no Qt,
no MinGW, no dev tools** — the last unverified promise of the portable vision.
Status: **OPEN** (never executed). **Asset:** the `gifscythe-windows` artifact
of a green CI run on `main` (record run id + built commit; retention 14 days).
Do NOT use GitHub Release `snapshot-2026-09-07`: it predates S7 while its notes
pin a newer SHA (U-09) and predates the S18 Ms-PL relicence (U-95). A re-cut
release (P0-4) becomes the preferred asset once it exists. Contents: `build/`
(static `gifscythe-cli.exe`, tests), `build-win/` (`gifscythe.exe` +
windeployqt runtime + `gifsicle.exe` beside it), `release/` (engine exes).

**Procedure.**
1. **Prepare:** clean Windows 10/11 VM/PC or fresh user profile — no Qt,
   MinGW/MSYS, Visual Studio, or prior Gifscythe runs. Note the OS build
   (`winver`) for the evidence.
2. **Fetch + verify:** download the artifact from the chosen green run; record
   run id, built commit, zip sha256. Unzip to `C:\gifscythe-test\` (no spaces;
   optional second pass with a spaced path re-probes argv quoting, A4/W).
3. **Engine identity (C5 recall):** `cd C:\gifscythe-test\build-win &&
   gifsicle.exe --version` → first line `LCDF Gifsicle 1.96 (Windows)`.
4. **CLI E2E (C3 recall):** copy a test `.gif` next to the exe; write
   `test.conf` (`mode = auto`, `optimize = 3`, absolute input/output under
   `C:\gifscythe-test\build-win\`). `gifscythe-cli.exe test.conf` prints the
   command line; `--run` exits 0 with `out.gif` written, opening, frame count
   matching (`gifsicle.exe --info out.gif`). Honesty probe:
   `--run --engine C:\nope\gifsicle.exe` → **non-zero** + `ERROR: engine not
   found`.
5. **GUI double-click (D3/D4):** no missing-DLL dialog; status bar shows the
   engine found **beside the exe**; add GIF → Optimize → completes, output
   written, preview animates; tabs responsive; command pane live.
6. **Record evidence:** OS build, run id + commit, zip sha256, screenshots or
   console transcripts for 3–5. Then run §3's probes on the same clean build,
   tick C4/D3/D4 in `COMPILED_AUDIT.md` §7 and add an `IMPROVEMENT_LOG.md`
   line. Only then is the portable-Windows promise evidenced.

**Failure triage.** Missing-DLL dialog naming `Qt6*.dll`/`libstdc++-6.dll` →
windeployqt gap or partial unzip (workflow deploy step, CMake MINGW static
flags) · `ERROR: engine not found` with engine present → probe order in
`src/core/EngineLocator.h` · GUI starts, run fails exit≠0 → command-pane text
vs the §7.E probes · spaced-path run splits args → quoting regression
(`ProcessRunner.h win_quote_arg`, unit test 19).

**Hard rule:** this checklist evidences; it does not waive. A failed step
leaves C4/D3/D4 open until re-run green.

## 3. Real-desktop GUI probes — W-19 (B5/B6/B14-adjacent)

**Purpose:** the three GUI behaviors the offscreen harness cannot execute —
they need a physical desktop with a real window manager. Status: **OPEN**
(never executed). Run on the same build as §2, after its GUI step passes.
**Why offscreen cannot:** the harness drives widgets in-process; it can emit
the drop *signal* and cancel/close around a run, but cannot receive an
OS-level Explorer drop, cannot have a *third party* kill the engine mid-run,
and never shows a real engine-missing dialog to a user.

**Probe 1 — kill the engine mid-run (external kill, not Cancel).** Harness
complement: T8/T9/T10 (failing engine honest, Cancel sets `Cancelled.`,
window-close kills the engine) — none kills externally with no `cancelling_`
flag set, which must take the failure branch of `onProcessFinished`. Steps:
queue a large GIF, start Optimize; while the progress bar is visible kill
`gifsicle.exe` from Task Manager (do NOT press Cancel or close the window).
Expect, with no hang and no success claim: status `Optimization failed
(exit …).`, a warning dialog carrying the engine's stderr (or exit-code text),
Run re-enabled, Cancel hidden, queue intact, a second Run works. Fail: any
completion claim, frozen window, or a run that cannot restart without relaunch.

**Probe 2 — physical drag-and-drop from Explorer.** Harness complement: T3
(the `filesDropped` signal appends, dedupes, rejects `.txt`) — T3 emits the
signal directly and cannot prove the OS delivers a real drop to
`src/qtui/DropListWidget.h`. Steps: drag two real `.gif` files → both appended
(queue 0→2); drag the same two + one new → only the new one appended; drag a
`.txt` → NOT appended. Known gap, not a probe failure: the `.txt` rejection is
silent (no feedback line) — tracked as GS-205/P1-27 (web twin: U-92), and this
probe does not waive it. Fail: a drop that appends nothing, a duplicate row, or
a `.txt` in the queue.

**Probe 3 — engine-missing GUI (launch, run, recovery).** Harness complement:
T1/T18 (status names the engine; Run re-enables through `ensureEngine()`) — no
harness case removes the engine binary. Steps: rename `gifsicle.exe` beside
`gifscythe.exe` → launch → status `Engine not found — build with
./scripts/build_engine.sh`, Run disabled; add a GIF and press Run anyway →
critical dialog `Could not find the GIF engine (gifsicle).`, no process, no
output; rename the engine back while open → next interaction re-probes, status
shows the path, Run re-enables. Fail: crash or silent no-op at step 2, an
output written with no engine, or a GUI needing relaunch to notice.

**Record evidence:** OS build, app commit, per-probe observed status + dialog
text (screenshot or transcription); then tick W-19 via the normal register flow
with an `IMPROVEMENT_LOG.md` line. A failed probe stays open — it does not
waive, and it cannot become a harness case (the harness cannot run it).
