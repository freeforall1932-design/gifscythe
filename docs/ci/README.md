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
- **The documentation status gate is live in CI as its own `docs` job** (step
  "Documentation status gate (STATUS.md register)": `scripts/check_docs.sh
  --no-gate-run`, run after `./build.sh` so G9b can re-measure the unit count
  and G15 sees the hook bootstrap). Applied by the maintainer (`190d030` era;
  confirmed S14), which is why register rows W-30/R-03/GS-208 closed in S24.
  **S34 moved it out of the linux job.** As step 7 there, one doc failure
  skipped the web suites, the oracle, the Qt GUI harness and the packaging
  steps (main run `36960880593`), so CI could not tell stale docs from broken
  code. The job checks out **full history** (`fetch-depth: 0`): a depth-1
  checkout made G11 compare the log with the checkout's own date and made G10
  skip on every branch and PR run (N-26 / N-31), so main went red the day
  after each log entry while the same tree was green on its PR. With history
  the gate measures what the local pre-push hook measures. `--no-gate-run`
  skips G6 (the other jobs already build and run the suites; re-running
  `verify_audit.sh` inside the gate would double the work).
- **Agent sessions can push `.github/workflows/` (verified S34).** The token
  blocker recorded in S9 was lifted in S18 (scope granted, verified by the
  pushed `build.yml` change in `161e862`; PR #28 pushed workflow edits too —
  old-remote shas, kept as the written record), and S34 re-verified it on the
  re-created remote with the Arena agent's own GitHub App token: the branch's
  first push carried two workflow commits (`9bc155f`, `7e0feca`), was
  accepted, and Actions ran them (run 36966494878). So do **not** stage a CI
  fix as a proposal waiting for "a workflows-scoped token": edit
  `.github/workflows/build.yml` and `docs/ci/build.yml.proposed` in the same
  commit and push. The doc copy stays — gates E9/G7/S1 enforce byte equality —
  as the recipe that survives for a token that *is* rejected: if GitHub ever
  rejects a workflow push, quote the rejection text, then take the
  pending-marker route (recreate `PENDING_WORKFLOW_CHANGE.md` in the same
  commit, delete it in the commit that applies the change).
- **What the workflow runs:** the `gate`, `docs`, `linux`, `portability`, `windows` and
  `csharp-spike` jobs — engine build,
  static-linked CLI/tests, GUI (CMake; Ninja+MinGW on Windows), native E2E
  smokes, the offscreen GUI harness, the Windows unit-test exe
  (`build/test_gifsicle_command.exe` — relevant to `U-71`/`U-94`-class rows:
  pure Win32 rule cases can be proven in CI without a VM), all **eight** Node
  web suites, packaging + manifest assertion (Windows ships; linux is the test
  battery since S20/OD-17), artifact upload (`gifscythe-windows`, 14-day
  retention; binaries are banked on Releases), the csharp-spike job (parked
  track, still CI-run), and the doc gate (its own `docs` job). The Windows GUI
  build and its offscreen harness (S34) run even when the CLI/unit-test step
  before them failed; they only need the Qt provisioning step to have worked.
  Two diagnostics-only Windows steps (S34, N-30) publish what a failed run's
  unreadable log would show: a toolchain-identity notice after provisioning,
  and — only when the CLI/unit-test step failed — a re-run of the same two
  compiles with a `-lstdc++fs` probe whose tail is published as an error
  annotation. Check-run annotations are the one channel of a failed run the
  agent sandboxes can read (`gh api repos/<repo>/check-runs/<job id>/annotations`);
  GitHub caps them at 10 errors and 10 warnings per step and 50 per job.
  The linux job's web-suite step does the same (S34, N-35): every suite runs even if an
  earlier one failed, and a failing suite is named by an annotation carrying its `FAIL` lines
  and the three lines after each. Its first red (run 36985082450) said only "exit code 1"
  and was a startup-banner race in a test helper, found by reproducing it locally.
  These steps are what named N-30 — a 32-bit-`long` bug in the settings
  parser, not the toolchain, so the `-lstdc++fs` probe turned out to be a
  spare. Both stay (inert, `continue-on-error`) for the next red Windows run.
- **Run shape (S34, the owner's calls).** The workflow still triggers on `push` (every
  branch) and `pull_request`, but a push to a branch with an open, *mergeable* PR no
  longer repeats the PR's run: the `gate` job (`scripts/ci_gate.sh`; decision table
  `tests/test_ci_gate.py`) answers `skip=true` and the other jobs are skipped, which
  is neutral on the PR page. It answers `skip=false` — the push runs — for `main`,
  tags, a branch with no PR yet, a PR with merge conflicts (GitHub starts no
  `pull_request` run for those, so the push is its only CI) and for every doubt (API
  error, `mergeable` still `null` after 8 polls). The jobs carry
  `if: !cancelled() && needs.gate.outputs.skip != 'true'`, so even a crashed gate runs
  everything. Workflow-level `concurrency` cancels the older run of the same event on
  the same branch or PR when a newer push arrives; a push to `main` is never
  cancelled (its group is the run id). Seen live (S34): the push run of this PR's branch
  ran only the gate (`skip=true`, run 36983282655) while the `pull_request` run ran all six
  jobs (run 36983286343); the PR page lists the skipped jobs as neutral. The gate job (about
  6 s) sits in front of every other job; the whole PR run took 4.3, 4.1 and 4.4 minutes in
  three runs against 4.1 before, i.e. inside the noise. `cancel-in-progress` was seen working (S34):
  two commits pushed about a minute apart cancelled the first commit's `pull_request` run
  (36984484547: every job `cancelled`); the cancel took about a minute and a half to take
  effect after the second push, and the newer run waited as `pending` until then. The first
  commit's *push* run had already finished (the gate skips it in seconds), so there was
  nothing to cancel there. **Trade-off:** this repo cites the run of a
  specific commit as evidence (N-11, N-16, N-30) — to keep a commit's run, let it
  finish before pushing again.
- **Runner images (N-34, S34).** `ubuntu-24.04` is pinned in all four Linux jobs
  instead of `ubuntu-latest`, which GitHub moves to Ubuntu 26.04 between 2026-10-19
  and 2026-11-19. To move on purpose, change the four labels in both workflow copies
  and trial the change on a branch first.
- **`portability` (S34, the owner's calls; N-30, N-32).** The checks that need zig but
  neither a Windows box nor emcc, in one job beside the others. Measured on Actions:
  2.0 minutes (run 36983286343: toolchain 8 s, the 32-bit unit suite 69 s, the wasm bar 38 s)
  and 2.4 and 2.5 on the next two (36983984989: 9 s, 87 s, 44 s; 36985082450) — against about 4 minutes for the windows -> csharp-spike chain, so it adds no wall-clock
  time (the slower sandbox needs about 3 minutes for the same two checks with a cold zig
  cache). The owner accepted roughly two minutes because it helps verify the project. The
  job holds the unit suite on a 32-bit-`long` target (`scripts/test_unit_32bit_long.sh`, the
  N-30 class), `scripts/libc_parity/libc_parity.py --bar` (the wasm32-wasi build byte-equal
  to a musl-native build - N-32's bar) and the two Python suites that pin the bar's logic and
  `prove_wasm.mjs --oracle`. zig and Pillow are pinned pip wheels in a venv. Any non-zero
  exit fails the job, including a runner's "SKIPPED" exit 3, and `GS_REQUIRE_PROOF=1` makes
  a Python suite that cannot find its prerequisites a failure instead of ten silent skips
  (the first run of that suite was green in 0 s with no way to tell from outside whether it
  had run): a skipped proof must never read as a passed one.
- **Before asking for a merge: `scripts/sim_postmerge.sh` (S34).** It builds the commit GitHub would
  create on top of the current `origin/main` — `--style merge|squash|rebase`, `--date` the UTC day
  of the merge — in a scratch full-history clone and runs the `docs` job's steps on it. A PR's own
  green run cannot show what main's push run will say, because G10 (the base lines against main's
  tip and its first parent) and G11 (the log date against the newest non-doc commit) depend on the
  shape and the day of the merge. It caught, in S34, a main that had moved under the pre-synced PR
  (G10 and G8 red before the merge, not after), and measured that a merge commit passes on any
  day, a squash fails G11 when merged on a later UTC day than the log entry's date, and a rebase
  conflicts for a branch that holds merge commits.
- **Editing the workflow offline (S34).** No actionlint binary is obtainable here (its
  release host is blocked), but `node working_code/gifscythe/scripts/lint_workflow.mjs`
  runs its WebAssembly build from npm (installed on first use into `$TMPDIR`), drops
  the one known false positive (that build's label list predates `ubuntu-24.04`) and
  has a `--selftest`. With `check-jsonschema --builtin-schema vendor.github-workflows
  .github/workflows/build.yml` it catches structural mistakes before a push-and-wait
  cycle.
- **Gate places:** the gates run in three places — `.githooks/pre-push`
  (bootstrap once per clone: `scripts/bootstrap_hooks.sh`; `build.sh` does it),
  the CI `docs` job, and `scripts/pr_preflight.sh` at PR create **and** merge.
  Step **P6** of the preflight lets a PR pre-sync itself (S34): the handoff's
  "Docs synced through" line may name the branch's own open PR, so a merge from the
  GitHub UI leaves nothing to edit afterwards.
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
