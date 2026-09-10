# Improvement Log

Chronological log of decisions and changes. **Newest at the top.**

---

## 2026-09-10 (S8, batch 2) — ten more findings closed with executed proof

Same branch, same version. Evidence per finding is in
**`docs/audit/REMEDIATION_2026-09-10.md` §2b**.

### New files

* **`src/core/OutputName.h`** — `sanitize_output_name()`,
  `is_windows_reserved_device_name()`, and a `NameRules` enum
  (`Host`/`Windows`/`Posix`). `MainWindow::renderedOutputName()` now delegates
  here. Parameterising the rule set is what made **U-21** verifiable on Linux:
  the *Windows* rules are unit-tested here (30 assertions) instead of being an
  untested claim. Separators are stripped in both flavours on every platform,
  which preserves the GUI's existing POSIX behaviour.
* **`web/validate.mjs`** + **`web/test/validate.test.mjs`** — **U-30**. The web
  demo had no validation layer, so identical settings produced a clear message
  on desktop and a raw engine error in the browser. The new test runs the *real*
  `gifscythe-cli` and requires the `(field, value, reason)` triples to match the
  JS mirror exactly: 19/19.

### Fixes

| ID | What changed | Proof |
|---|---|---|
| **U-32** | `run_argv` returns `128+WTERMSIG` instead of `1` for a signalled child | test 28: SIGTERM→143, SIGKILL→137, `exit 3`→3, missing binary→127 |
| **U-51** | `encode_line_value()` at all 9 string write sites + JS mirror | reproduced a comment hijacking `mode`; test 30 guards the round-trip |
| **U-49** | dropped the double `decodeURIComponent` in `server.mjs` | `{"comments":["100%"]}`: HTTP 400 → **200 / 8679 B** |
| **U-50** | `X-Gifscythe-Command` percent-encoded, decoded in `app.js` | `{"comments":["作品"]}`: HTTP 500 → **200 / 8681 B** |
| **U-46** | `requestGen` counter in `app.js`, checked after fetch, after blob read, in `catch` and `finally` | a stale response can no longer populate a newer preview |
| **U-52** | `beforeUrl` tracked and revoked on replacement | `node --check` + review (browser-only) |
| **U-31** | `build.sh` link-probes `-lstdc++fs` and cleans up | probe ran, flag correctly empty on g++ 12, no artefact left |
| **U-44** | dated review snapshots `git mv`'d to `docs/archive/` | 3 prose references updated; no path links existed |

The strongest before/after is **U-30** on `--scale 0x1`: the engine exits **0**
and the output is **byte-identical to `--scale 1x1`** (`cmp` clean) — the user
asked for a resize and got nothing, with no message. The server used to answer
HTTP 200; it now answers 422 naming the field.

### Two things this batch broke and then fixed

1. **My own comment tripped `verify_audit.sh` E3.** The line "(no /bin/sh, no
   cmd.exe)." in `ProcessRunner.h` matched the no-shell-execution grep, and the
   exemption words were on the *previous* line. A green run became
   **22 passed, 1 failed**. Fixed by putting the exemption on the same line and
   by exempting `//` / `*` comment lines in E3 — then mutation-tested in four
   directions so the guard still catches a real `system()`, a real `/bin/sh`
   literal, and a trailing `// spawns sh -c`.
2. **`web/test/validate.test.mjs` initially reported the C++ side as silent.**
   `execFileSync` returns only **stdout**, so on a rc=0 run the piped stderr was
   thrown away. Switched to `spawnSync`. Writing the parity test also surfaced
   that `delay < 0` and `lossy < 0` never survive a conf round-trip — *both*
   writers skip them because `-1` means "unset" — while the C++ *reader* does
   accept them. Those two fixtures now feed the C++ side raw conf text, and the
   property is documented rather than hidden.

### Tests

| Check | Result |
|---|---|
| `./build.sh` | **211 checks, 0 failures** (tests 28–30 added) |
| `scripts/verify_audit.sh` | **23 passed, 0 failed, 5 skipped, exit 0** (new gates W1/W2/W3; E9 SKIPs — see below) |
| `node web/test/command.test.mjs` | **14/14 PASS** |
| `node web/test/validate.test.mjs` (new) | **19/19 PASS** |
| `node web/test/transport.test.mjs` (new) | **17/17 PASS** against a live server |
| Transport mutation testing | double decode → 4 FAIL · raw header → 6 FAIL · no `validate()` → 3 FAIL · restored → 0 |
| `scripts/test_package.sh` / `test_engine.sh` / `smoke_cli.sh` | 9/9 · 5/5 · 7/7 |
| E3 mutation testing | 4 cases, all correct |

### CI

The linux job now runs all three web suites. It previously ran **none** of them, so
the JS copies of the command builder and the validation rules had no automated
guard at all. `.github/workflows/build.yml` and `docs/ci/build.yml.proposed`
were edited together. **They are NOT identical on this branch, on purpose:**
the push was rejected with *"refusing to allow a GitHub App to create or update
workflow `.github/workflows/build.yml` without `workflows` permission"*, so the
live workflow was reverted to base and the change lives in
`docs/ci/build.yml.proposed` + **`docs/ci/PENDING_WORKFLOW_CHANGE.md`** —
exactly the situation `docs/ci/README.md` says that copy exists for. E9 was
extended with one tolerated state: declared drift (that marker file present) is
a SKIP, undeclared drift still FAILs. Mutation-tested in all four states.

### Documentation consistency sweep

Current-state docs had drifted during S8, so each was re-read against the
register instead of being left to disagree with it — root `README.md`,
`SESSION_HANDOFF.md` (a full **"What S8 did"** section added, and its
*"all run locally, linux + Qt 6.4"* header corrected, since this sandbox has
neither), `WORKLIST.md` (new S8 board), `PROJECT_VISION.md`,
`docs/release/RELEASE_PROCEDURE.md`, `docs/ci/README.md`, both product READMEs,
`web/README.md` and `docs/web/WEB_FEASIBILITY.md`. The full table is
`docs/audit/REMEDIATION_2026-09-10.md` §3b.

**A confusion this closes:** the GUI harness holds **226 `CHECK(` sites in
source** but its last **runtime** count was **243**. Not a conflict — `CHECK`
increments at runtime and several sites sit in loops. The unit suite is the
same shape: 196 source sites, **211** runtime checks. Quote the runtime number
and say where it was measured. Dated audit snapshots under `docs/archive/` and
the per-session entries below keep their historical numbers by policy.

### Still unverified here

Every `src/qtui/` edit — now including the `OutputName.h` delegation in
`renderedOutputName` — and `tests/test_gui_offscreen.cpp` T8/T17. This sandbox
has no cmake and no Qt6, so those compile in CI only. The Windows behaviour of
`NameRules::Host` is likewise unproven; only the rule set is.

---

## 2026-09-10 (S8, batch 1) — Audit remediation: 21 findings closed with executed proof

Working on branch `arena/01a08a10-gifscythe` (off `main` @ `a55a68d`).
Version stays **0.1.0**. Full per-finding evidence, including the before/after
command output and the mutation-test record, is in
**`docs/audit/REMEDIATION_2026-09-10.md`**; the pick rationale that preceded it is
`docs/audit/FIX_PICK_2026-09-10.md`.

### The headline fix: batch output planning (U-01)

New Qt-independent **`src/core/OutputPlan.h`**. `gs::plan_outputs(inputs, outputs)`
refuses a target that is any queued input, two inputs mapping to one target, or an
empty target, and *reports* (without refusing) targets already on disk. Paths are
compared through `weakly_canonical`, so `./a.gif`, `a.gif` and an absolute path to
the same file cannot slip past. Both drivers now plan **before the first process
starts**: the CLI refuses with rc=2, and the GUI plans the whole queue in
`runCommand()` and states the verdict in the summary label before Run is clicked.

Verified against the real engine: `input = solo.gif` / `output = solo.gif` used to
replace the source in place with rc=0; it now exits 2 and the file's md5 is
unchanged.

**Deliberately not done:** temp-sibling + rename. It would only protect a previous
output from a crashed engine, and it would put a staging path into the live command
pane, breaking the contract harness T1/T16 assert.

### Everything else closed

`-j` for threads "Auto" (U-03, mirrored in `web/command.mjs` — the parity harness
failed 4 fixtures the instant the C++ side changed) · CLI stdout purity (U-04) ·
real PATH engine search (U-05) · web binds loopback (U-06) · packager fails closed
+ license set asserted (U-02/U-08) · `scripts/test_package.sh` negative suite +
CI manifest assertion (U-14) · `parse_bool` warns (U-11) · drop filter `&&` (U-13) ·
resize/scale geometry validated, rules probed off the engine (U-22) · strict CLI
arg parser (U-23) · web 422 for "rc=0 but no output" (U-24) · web Scale default 100
and Touch option (U-25/U-29) · numeric version sort (U-26) · missing `return` (U-28) ·
`-p` needs both halves (U-33) · `verify_audit.sh` C6 SKIPs without cmake (U-38) ·
workflow-drift guard E9 (U-39) · "Batch (1 file)" (U-43) · empty comments skipped
in C++ **and** JS (U-48).

### Three register rows corrected

* **U-19 was not a data bug.** A round trip with the parent toggles *on* returns
  `crop_w 200 → 200`, `position 12,7 → 12,7`, `scale 0.5 → 0.5`, 0 load warnings.
  The original repro started from `crop=false`, where dropping the children is
  correct. Downgraded to a wording nit; unit test 26 pins the real behaviour.
* **U-20** — a GUI-saved conf does produce one *validate* warning (no `input`
  key); the "no warnings at all" phrasing is removed from the handoff and this log.
* **U-10** — `REFERENCE_MANIFEST.md` no longer calls `gifsicle/` "identical to
  upstream master"; it lists the observed deltas and marks provenance open.

### Tests

| Check | Result |
|---|---|
| `./build.sh` | **166 checks, 0 failures** (tests 21–27 added) |
| `scripts/test_package.sh` (new) | **9 passed, 0 failed** |
| `scripts/verify_audit.sh` | **21 passed, 0 failed, 4 skipped** (was 19/1/3, exit 1) |
| `scripts/test_engine.sh` / `smoke_cli.sh` | 5/5 · 7/7 |
| `node web/test/command.test.mjs` | all PASS (14 fixtures) |
| Mutation testing | 7 core guards each broken on purpose → 3–9 FAIL each; restored → 0 |
| GUI harness **T17** + T8 rewrite | written, **CI-compiled only** — this sandbox has no cmake/Qt6 |

### Constraint kept

`.github/workflows/build.yml` and `docs/ci/build.yml.proposed` are byte-identical,
and the new `verify_audit.sh` **E9** check now fails if they drift.

---

## 2026-09-10 (S7) — Settings persistence, queue reorder, naming templates, release doc

Working on branch `arena/s7-settings-persistence` (off `main` @ `c5efe07`).
Version stays **0.1.0** — the minor-bump/1.0.0 decision is the owner's.

### Implemented (the sandbox-codeable Phase-1 remainder)

1. **GUI settings persistence** — closes the S6 gap
   (`docs/planning/OFFLINE_BUILD_REVIEW.md` §6), audit §2.3 row 15 ("Config
   persistence", OPEN since S3), and the WORKLIST item. Design decisions:
   - **Serializer: core `SettingsIO`, not `QSettings`.** One format for CLI
     conf files and GUI sessions means the two can read each other's files;
     the flat `key = value` text stays diffable and Qt-independent.
   - **Location:** `QStandardPaths::AppConfigLocation` + `/gifscythe.conf`
     (`%APPDATA%\Gifscythe\gifscythe.conf` on Windows); **`GS_SETTINGS_PATH`**
     env override mirrors the established `GS_ENGINE` pattern (portable use,
     test isolation).
   - **`SettingsPanel::readFrom()`** restores every control `writeInto()` reads
     (not a byte-exact inverse of the serializer — crop geometry, the position
     pair and the scale factors are only written while their parent toggle is
     on; see the U-19 correction in
     `docs/audit/REMEDIATION_2026-09-10.md` §4):
     every control restored signal-blocked (no `changed()` storm → no
     preview/pane churn), dependent enabled states synced explicitly, values
     the GUI cannot represent (`optimize = -1`, disposal 4..7, unknown
     methods) leave the control at its default rather than forcing a wrong
     "off".
   - **GUI-only keys** `batch_dir` + `name_template` are appended after the
     core dump in a commented "GUI state" section. `SettingsIO` ignores
     unknown keys on load, so the CLI consumes GUI-saved files silently —
     pinned by new **unit test 20** and a live CLI E2E run.
   - **Not persisted (deliberate):** the queue (files move between sessions;
     stale rows teach users to distrust the queue) and the Save-as field (a
     per-run choice — silently restoring it could overwrite a stale path).
   - **Honesty:** first launch (no file) → defaults, no error, nothing
     written until first close; existing-but-unreadable file → status says
     so; parse warnings → surfaced in the status bar (S5 rule: never
     discard); failed save on close → warning dialog (never silent).
   - Saved in `closeEvent` (standard desktop behavior; crash-loss window
     accepted — settings are preferences, not work product).
2. **Queue reorder (S3-9 remainder)** — `Move Up` / `Move Down` on the Input
   tab. List rows and `inputs_` stay index-aligned (take/insert mirrored on
   both), selection follows the item, bounds are honest no-ops, buttons
   disabled while busy. Merge consumes the queue in order, and the live pane
   + merge E2E prove it (harness T15).
3. **Free-form naming templates (S3-25)** — Output-tab `Name template`
   field, default `{name}_opt.gif`. The default renders **exactly** the
   historical auto-name, so audit E4 ("auto `<name>_opt.gif` next to each
   input") is byte-identical for untouched installs; harness T1/T2/T4/T13
   keep pinning it. Safety/honesty rules: `{name}` = input base name
   (case-insensitive replace); **path separators stripped** (a template
   cannot escape the chosen output folder — `../../evil` → `evil.gif`);
   `.gif` appended when missing (the engine only writes GIF); empty render
   falls back to the default; **constant template (no `{name}`) + >1 queued
   files = collision → summary warns and the run is REFUSED** with an
   explanatory dialog instead of silently overwriting N-1 results.
   The template is persisted (`name_template` GUI key) and restored.
4. **Release procedure documented** — `docs/release/RELEASE_PROCEDURE.md`
   (snapshot vs version rules, pre-flight suite + expected counts, version
   bump mechanics, CI evidence, packaging contents incl. licenses, GitHub
   Release/banking conventions with sha256, post-publish clean-Windows +
   persistence spot checks, rollback = append-only).

### Hygiene fixes found while reviewing

- **`docs/ci/build.yml.proposed` had drifted** from the live workflow (line
  98 still said `build_gifsicle.sh` after the maintainer updated
  `.github/workflows/build.yml` in `c5efe07`) — the "byte-identical"
  constraint was violated by the copy, not the original. Synced; `diff` is
  now clean.
- **`scripts/build_gifsicle.sh` shim removed.** Its only purpose was the
  GitHub App's missing `workflows` permission; the maintainer's `c5efe07`
  removed that reason (S5 log: "remove it once that happens"). No live
  reference remains (the two dated review snapshots keep historical refs by
  policy).
- **Dead code:** the unused bool-string lambda in `SettingsIO::save_settings`
  (silenced with `(void)b`) is gone.
- **Stale user-facing strings:** two "<name>_opt.gif" texts now describe the
  template default (dialog + comment) — behavior unchanged.
- **Stale doc counts:** root/product READMEs said "143 checks" (S4b) while
  the harness reported 150 since S5; both now state the S7 truth (243) with
  history. The product README's S4-era status block (incl. the long-resolved
  "PAT is invalid" note) is rewritten.

### Tests

- Harness `test_gui_offscreen.cpp`: **150 → 243 checks**. New **T14**
  (persistence round-trip incl. file-content assertions, dependent-state
  restore, not-persisted assertions, corrupt-file honesty), **T15** (reorder
  sync/selection/bounds/merge-order + E2E), **T16** (template default =
  E4 name, custom template pane/summary/E2E, separator escape, collision
  refusal dialog + engine-never-started). `main()` now points
  `GS_SETTINGS_PATH` at a run-scoped scratch file (no test touches the real
  user config dir; run-order deterministic). T1 gained existence/default
  checks for the three new widgets.
- Unit suite: test 20 (unknown-key tolerance, no warnings).
- Full local rerun (linux, Qt 6.4, offscreen): build.sh green · engine 5/5 ·
  smoke 7/7 · **verify_audit 21 PASS / 0 FAIL / 2 SKIP** · harness 243/0 ·
  web parity all passed · CLI-on-GUI-conf exit 0.
- Windows side must be confirmed by CI on this branch (only Windows Qt
  verification available).

---

## 2026-09-09 (S5) — GUI honesty fixes, naming alignment, web build review + POC

Working on branch `arena/01a086c5-gifscythe`. Version stays **0.1.0**.

### GUI review fixes (3 bugs found in `src/qtui/MainWindow.*`)

1. **Cancel popped a spurious error dialog.** `cancelRun()` → `kill()` +
   `waitForFinished()` made gifsicle exit non-zero/crash, which synchronously
   fired `onProcessFinished` → the failure branch → a modal "optimization
   failed" dialog *before* the status flipped to "Cancelled.". The old B2
   harness check only asserted the final status (its DialogKiller silently
   closed the dialog), so it passed. Added a `cancelling_` flag so
   `onProcessFinished` skips the alarm path during a cancel; harness T9 now
   asserts **no dialog** appears in the cancel window.
2. **Batch live pane showed a command the app never runs.** The pane printed a
   single `gifsicle -b <all inputs> -o <first>_opt.gif` line while `runCommand()`
   actually launches **N separate Auto-mode invocations** (one per file, never
   `-b`). `refreshCommand()` now renders the real per-file commands (E1
   explicit Save-as honored for a single file; capped at 20 lines for huge
   queues). Harness T2/T13 now assert the pane contains the derived
   `<name>_opt.gif` names and no ` -b `.
3. **`gs::validate()` results were thrown away in the GUI.** `runCommand()`
   erased only the `input` warning and ignored the rest (comment claimed it
   was about batch output — no such warning exists). The one GUI-triggerable
   warning (crop 0×0) was silently dropped. It now surfaces remaining warnings
   in a dialog before running.

Regression checks added to `tests/test_gui_offscreen.cpp` (T2, T9, T13).
Core/CLI unaffected.

### Naming alignment — product is Gifscythe; engine stays gifsicle

Policy: **Gifscythe** = the product; **gifsicle** = the upstream engine only
(bundled `gifsicle[.exe]` binary, `reference_code/gifsicle/`, engine version
1.96, its flags/options — these MUST keep the name; audit A10 and the
"engine identity 1.96" rule depend on it).

Applied:
- Renamed `scripts/build_gifsicle.sh` → **`scripts/build_engine.sh`** (git mv)
  and updated every live reference (build.sh, CMakeLists.txt, verify_audit.sh,
  test_engine.sh, READMEs, WORKLIST, SESSION_HANDOFF, COMPILED_AUDIT,
  IMPROVEMENT_LOG, gifscythe.pro). A thin `scripts/build_gifsicle.sh`
  **compatibility shim** remains because the GitHub App cannot edit
  `.github/workflows/build.yml` (no `workflows` permission); it forwards to
  `build_engine.sh` and should be removed once a maintainer updates the
  workflow. The two dated review snapshots (now under `docs/archive/`) intentionally keep
  their historical line
  refs.
- User-facing strings now say the brand or "the GIF engine": CLI help, GUI
  bottom-bar label, mode combo ("Auto (engine decides)"), tooltips, error
  statuses/dialogs ("Could not start the GIF engine", "The GIF engine
  returned an error (exit N)", "Failed to start the GIF engine."), the batch
  pane annotation. Engine/source attributions (e.g. "gifsicle 1.96 source",
  "engine (gifsicle)") were kept where they name the engine precisely.

### Web build — reviewed + working POC (not built/reviewed before)

`FEASIBILITY_REVIEW.md` only mentioned web-tech UI as an alternative; nothing
was built. Delivered:
- **`docs/web/WEB_FEASIBILITY.md`** — full review of 4 options
  (Qt-for-WASM ❌ QProcess/subprocess can't exist in wasm; Tauri ⚠️ still a
  desktop app; server-side engine ✅ works today; **client-side
  `gifsicle.wasm` ✅ best end-state** for portable offline web).
- **`web/`** — zero-dependency Node server (`server.mjs`, spawn argv-array,
  never a shell) + browser UI (upload → optimize → before/after + download)
  + **`command.mjs`**, a line-by-line JS mirror of `GifsicleCommand.h` that is
  the *single* builder for the browser live-pane and the server.
- **Parity proof:** `web/test/command.test.mjs` serializes 12 settings
  fixtures to confs, runs the real C++ `gifscythe-cli`, and asserts the JS
  `toString()` is byte-identical — **13/13 PASS**. Server verified E2E: valid
  12-frame GIF out (8703→6856 B with `--lossy=40 --resize-fit 100x100 -O3`),
  honest 422 + stderr for non-GIF input.

### Verification this session

- `./build.sh`, unit suite, `test_engine.sh` 5/5, `smoke_cli.sh` 7/7, audit
  static probes (E3/E4/E8/A5): **all green** after the edits.
- **Qt GUI could NOT be compiled here:** sandbox network allows only
  pypi/npm/github; apt, Qt CDN and emscripten hosts are unreachable, and
  PySide6 wheels ship no C++ Qt headers/`moc`. GUI fixes are therefore
  logic-reviewed + harness-checked by inspection only; the offscreen harness
  must run on CI (it already runs on both OSes in `build.yml`).

### Still open (unchanged)

C4/D3/D4 clean-Windows smoke, desktop probes B5/B6/B14, optional polish
(naming templates, queue reorder), 0.2.0-vs-1.0.0 owner decision. WebP/APNG
stay blocked. Option-4 wasm build is a later release, not 1.0.0 scope.

---

## 2026-09-07 (S4c-close) — recovery pushed, PR #5 merged, CI green on main

- Token #3 (fresh fine-grained PAT) worked where tokens #1–#2 were rejected
  (expired/revoked at source). The 10 S4/S4b commits pushed to
  `verify/windows-ci-fixes` **SHA-identical** to the local originals
  (independently audited post-push: zero mangling, tree byte-identical to
  PR merge result).
- Run #20's windows hang (offscreen step, zero output ~28 min) diagnosed via
  post-cancel log pull; `160fea3` added the 8-min watchdog + stage markers,
  `timeout-minutes: 12` on both GUI steps, `qoffscreen.dll` staging beside
  the harness exe (presumed root cause), `isValidColorName` deprecation fix.
  Runs #21/#22 green; hang not recurred.
- **PR #5 merged** into `main` as true merge `0ad1ff5` (parents `821a310` +
  `160fea3`). Main green on both jobs twice: run #23 (`0ad1ff5`) and run #24
  (`d3544b1`, id 34092786153), incl. both GUI offscreen steps.
- Artifacts `gifscythe-windows` (~27.8 MB) + `gifscythe-linux` (~0.7 MB) up;
  default expiry 2026-12-06 → retention capped at 14 days in workflow and
  binaries **banked on GitHub Release `snapshot-2026-09-07`** (prerelease;
  nothing binary enters the git tree — caesium-bin precedent 95d62eb).
- Workspace archive banked: `gifscythe-workspace-2026-09-07.zip` (36.0 MB,
  sha256 `67e55363f79ddd2dc8236a5bed07d2032dec5fd360b4d2fb1eef5669a1d87b2a`),
  also attached to the Release.
- Merged/stale remote branches deleted afterwards (`verify/windows-ci-fixes`,
  `arena/01a07959-gifscythe`, `arena/01a0746d-gifscythe`,
  `arena/01a07410-gifsicle-1-96`); PR refs preserve history.
- **Still open (unchanged):** C4/D3/D4 clean-Windows smoke
  (`docs/ci/CLEAN_WINDOWS_SMOKE.md`), desktop probes B5/B6/B14, optional
  polish (naming templates, queue reorder, release-procedure doc), owner's
  0.2.0-vs-1.0.0 decision. Version stays 0.1.0.

---

## 2026-09-07 (S4b) — XNConvert-style UI retrofit: tabs, full controls, async preview

Owner approved starting the P1 GUI retrofit while the push token is dead.
Implemented + verified the same day; version stays **0.1.0** (bump is an
owner decision after Windows CI green).

**New structure** (`src/qtui/`):
- `SettingsPanel.{h,cpp}` — the Actions tab: ~30 controls covering the whole
  `GifsicleSettings` surface (mode, optimize, lossy, colors, dither,
  color-method, careful, resize kind/W×H/scale %/method, rotate, flips,
  interlace, position, crop + crop-transparency, delay, loop, disposal,
  unoptimize, threads, gamma, background/transparent with color pickers,
  metadata removals, comments, explode-by-name). Value lists are
  **engine-truth**, extracted from gifsicle 1.96 source: dither names from
  `set_dither_type()` (floyd-steinberg/atkinson/o3x3…ro64/diag45/halftone/
  sqhalftone), resize methods from `RESIZE_METHOD_TYPE` (point/mix/box/
  catrom/lanczos2/lanczos3/mitchell), disposal from `DISPOSAL_TYPE`
  (none/asis/background/previous), color methods from `COLORMAP_ALG_TYPE`
  (diversity/blend-diversity/median-cut), gamma from `GAMMA_OPT`
  (srgb/oklab/numeric). Every widget has a stable objectName.
- `PreviewPanel.{h,cpp}` — Before (QMovie of selected original) / After
  (QMovie of preview output) + size-savings readout + honest captions.
- `MainWindow.{h,cpp}` — rewritten layout: Input/Actions/Output tabs +
  preview in a splitter; bottom bar unchanged (live one-way pane, progress,
  run/cancel, status). Output tab: Save-as, batch output folder (mkpath'd
  before running), Open-folder via QDesktopServices, per-mode summary.
  Queue rows show per-file size; footer shows count+total. **Run semantics
  deliberately unchanged** (batch N→N `_opt.gif`, E1 explicit save-as,
  merge refuses empty output, explode auto-prefix, honest failures).
- Preview pipeline: 1200 ms debounce; **fresh QProcess per run + captured
  sequence number** so killed/stale runs can never be misread; killed on
  main-run start, cancel, close, and destructor; temp dir per PID, cleaned
  on destruction; previous preview file deleted after each success.

**Bug found & fixed during harness bring-up:** `previewProcess_` member
dangled after the completion lambda's `deleteLater()` → segfault in
`~MainWindow`/`killPreview` (gdb backtrace). Member is now nulled in both
completion paths.

**Harness grew 81 → 143 checks** (T1–T13): tabs exist and are named; all
control→flag mappings incl. regression guards for VP-1 (`--loopcount=0`),
VP-2 (`-O0`), VP-3 (no `--gamma` unless chosen), VP-5 (crop `1,2+30x40`
plus-form), E7 (delay label says **1/100 s**, never ms); preview pipeline
(savings appear, regenerates on change, honest Explode refusal); batch
output folder honored end-to-end; all original B-series semantics re-proven
against the rewritten MainWindow.

**verify_audit.sh** stays green: 21 PASS / 0 FAIL / 2 SKIP (E4 probe moved
to SettingsPanel.cpp). qmake **and** cmake build paths both compile the new
files (`gifscythe.pro` updated). Engine 5/5, smoke 7/7, unit ALL PASSED.

**Second PAT also rejected** (format-valid, 93 chars — GitHub says Bad
credentials/Invalid token). Push still blocked; everything accumulates on
local branch `verify/windows-ci-fixes`.

---

## 2026-09-07 (S4) — §6 verification executed + Windows CI root-caused & fixed

**Environment upgrade:** apt reachable again → gcc 12.2, cmake 3.25,
**Qt 6.4.2**, **mingw-w64 12**, **Wine 8** installed in sandbox. Everything
the S3 session marked "needs a Qt machine / real Windows" became testable.

**Push blocker:** the provided fine-grained PAT is invalid (API 401 "Bad
credentials"; push "Invalid username or token"; public-repo reads work
anonymously, which masks the failure). All S4 work is committed on local
branch `verify/windows-ci-fixes`; a new token (Contents+Workflows+PR write)
is required to push.

**Verification (COMPILED_AUDIT §6, evidence-tagged):**
- §6.A 12/12 + new A13 (Windows unit exe under Wine). ASan+UBSan clean
  (unit suite + CLI, incl. honest exit 1 on missing engine).
- §6.B: new offscreen harness `tests/test_gui_offscreen.cpp` — 81 checks,
  0 failures (batch N→N + frame counts, merge 13=12+1, explode `.NNN`,
  merge-empty-output refusal, honest failure, cancel mid-run on a
  4800-frame GIF, close-while-running kill, dedupe, multi-remove, live pane
  sync/quoting, Batch default, explicit-output E1). B5/B6-plumbing/B14-variant
  remain one-time desktop probes. Qt 6.4 note: synthetic QDropEvents are
  ignored without an active platform drag session → harness emits
  `filesDropped` (the signal the drop handler emits) instead.
- §6.C: C1 confirmed green (Actions linux, run #18). C7 simulation exposed a
  **new pit**: stale GUI binaries let `build.sh --all` claim "GUI built" with
  Qt hidden → build.sh now deletes stale GUI outputs before probing.
  C6 configure ±Qt, C8 honest default: green.
- §6.D: portable package now includes the GUI (D1/D2); **D5 fixed** —
  `reference_code/caesium-bin` untracked (62 files / 74 MB, `git rm
  --cached`), manifest documents re-fetch. History purge still optional.
- §6.E 8/8 green.
- One-command suite: `scripts/verify_audit.sh` → 21 PASS / 0 FAIL / 2 SKIP.

**Windows CI failure (run #18 step 4) root-caused & fixed:**
- gifsicle 1.96 sources do *unconditional* `#include <config.h>`; the
  `--windows` compile line lacked `-I.` → "config.h: No such file". Fixed
  per upstream `src/Makefile.mingw`: `-include src/win32cfg.h` first (owns
  `GIFSICLE_CONFIG_H` guard; root config.h resolves via `-I.` but is
  guard-neutralized), `-DHAVE_CONFIG_H=1 -DHAVE_UINTPTR_T -DHAVE_INTTYPES_H`,
  no `-DVERSION` (win32cfg.h defines `1.96 (Windows)`; the old flag only
  warned and never took effect).
- Reproduced the exact CI failure locally with mingw-w64, then verified the
  fix: valid PE x86-64, runs under Wine, optimizes GIFs, `--version` →
  `LCDF Gifsicle 1.96 (Windows)`.
- Wine CLI E2E (first ever): confs with `C:\gs\...` paths → exit 0 + valid
  outputs (12 frames preserved); spaces in input+output → exit 0 **after**
  fixing a second Windows-only bug: MinGW `_spawnvp` does **not quote**
  argv → space paths split. `ProcessRunner.h` now uses **CreateProcessA**
  with MSVCRT-rule quoting (`win_quote_arg`); unit test 19 guards the edge
  cases (empty arg, tabs, embedded quotes, trailing backslashes).
- Missing engine under Wine → exit 1 + honest stderr (A2 parity).
- **Static linking** for Windows CLI/tests (`-static`; KERNEL32+msvcrt only):
  dynamic exes die silently without MinGW runtime DLLs — unacceptable for
  portable artifacts. CMake `if(MINGW)` applies the same (GUI:
  `-static-libgcc -static-libstdc++`, Qt stays dynamic via windeployqt).

**Workflow hardening** (`build.yml` + `build.yml.proposed`, byte-identical):
`-static` flags; **Ninja generator** for the GUI step (windows-latest cmake
defaults to Visual Studio, which cannot consume aqt's MinGW Qt — latent
landmine); native **Windows engine+CLI E2E smoke step**; GUI offscreen
harness steps on linux + windows.

**Decisions:** keep `docs/ci/build.yml.proposed` synced with the live
workflow until retired; harness ships as a CMake target + ctest
(`QT_QPA_PLATFORM=offscreen`, TIMEOUT 600) so CI runs it on both OSes;
version stays 0.1.0; WebP/APNG stay blocked.

---

## 2026-09-07 — Docs sync + PR for P0/P1 remediation + compiled audit

- Updated `SESSION_HANDOFF.md`, `WORKLIST.md`, root/`working_code` READMEs,
  `PROJECT_VISION.md` status line, and `VERSION.md` “where version lives” to
  match implemented code and `COMPILED_AUDIT.md`.
- PR from `arena/01a07959-gifscythe` → `main` (version stays **0.1.0**).
- **CI note:** GitHub App lacks `workflows` permission, so the rewritten
  workflow ships as `docs/ci/build.yml.proposed` instead of replacing
  `.github/workflows/build.yml` in this PR. Next session (or a human with
  workflow rights) should copy it into place and confirm Windows CI green.

---

## 2026-09-07 — Merged compiled audit (S1+S2+S3)

- Fetched branch audit
  `codebase-review-and-optimization-3bbfe` / `WORKLIST_CODE_REVIEW.md` (30 items).
- Adjudicated each item against the forensic compilation:
  real duplicate, weaker restatement, false positive, or net-new backlog.
- S3 is strong as a **UX backlog** but **missed** critical silent-failure bugs
  already in S1/S2 (exit-0, Merge weld, empty-output loss, shell injection,
  CMake INTERFACE, SettingsIO UB, Win config.h static_assert, dangling Settings&).
- S3 false premises called out: SettingsIO is not JSON; CLI has no
  `--settings`/`settings.json`; `ci.yml` vs `build.yml`; `GIFSYCYTHE` typo in
  proposed version fix; installers vs portable vision.
- Wrote **`COMPILED_AUDIT.md`**: master checklist with fix status, §6
  next-session verification (including “new pit” probes), remaining work order.
- Prior long reviews remain on disk for history; day-to-day checklist is
  `COMPILED_AUDIT.md`.

---

## 2026-09-07 — P0/P1 remediation (silent failures + honesty)

Implemented the consolidated code-review plan without bumping past 0.1.0 and
without touching WebP/APNG.

**P0 — stop silent failures**
- CLI: replaced `system()` with argv `fork/execvp` (CreateProcess/`_spawnvp` on
  Windows); honest `WEXITSTATUS`; engine pre-flight; `std::filesystem` paths;
  `~` expansion; dropped `unistd.h`.
- CMake: `gifscythe_core` is `INTERFACE`; `version.h` generated from `VERSION.md`.
- Windows engine build uses `-include src/win32cfg.h` (no Linux `config.h`);
  engine `-DVERSION=1.96` (product version only names the release dir).
- GUI: default mode **Batch** (per-file `_opt.gif`); mode combo; require/auto
  output; verify output file exists before success; async `QProcess` + cancel +
  progress; timeout/zombie path removed.
- SettingsIO: safe `to_long`/`to_double`, unified `parse_bool`, load warnings,
  `save_settings` + round-trip, `load_settings_file` → `optional`.

**P1 — one truth per concept**
- `EngineLocator` shared by CLI and GUI; status bar shows engine path.
- Queue append+dedupe, Remove/Clear, working drag-and-drop (`DropListWidget`).
- Live command pane wired to all controls; `shell_quote` for display only.
- `GifsicleCommand` stores `Settings` by value (no dangling ref).
- Packaging probes `gifsicle`/`gifsicle.exe` and all GUI output dirs; ships
  `COPYING.gifsicle` + `LICENSE`; optional `windeployqt`.
- `build.sh` honest GUI branch (fails non-zero when `--all` and Qt missing).
- FEASIBILITY mapping table corrected (delay 1/100 s, crop `+` form); live pane
  doc descoped to honest **one-way** sync (two-way = optional later).
- Root `.gitignore`, `.gitattributes` (`*.sh eol=lf`), include-guard rename
  `GIFSYCYTHE_*` → `GIFSCYTHE_*`.
- Expanded unit tests + `scripts/smoke_cli.sh`; CI runs engine + smoke tests and
  uploads artifacts; Windows job uses aqtinstall + win32 config.

**Local proof (sandbox):** unit ALL PASSED (74 CHECKs); engine 5/5; smoke 7/7;
missing engine exits 1; engine version string 1.96; demo GIF written end-to-end.

**Intentionally not changed (verified correct):** `--loopcount=0` = forever,
`-O0` = off, gamma sentinel `-1`, crop plus-form emitter.

---

## 2026-09-06 — GUI MVP and packaging follow-up

- Improved the Qt GUI with an animation queue, optimization and lossy controls,
  output selection, generated command preview, status messages, and process
  error handling.
- Added portable and system-dependent packaging scripts.
- Fixed clean engine builds by creating the versioned release directory.
- Added SettingsIO parser coverage to the Qt-independent tests.
- Added GitHub Actions CI. Linux passes with Qt6; Windows currently fails and is
  the next investigation target.
- Confirmed WebP/APNG remain deferred until the GIF UI/UX retrofit is complete.

---

## 2026-09-06

- **Created** `FEASIBILITY_REVIEW.md` — feasibility verdict for the 4 product
  asks (Caesium GUI base, XNConvert-like ease-of-use + terminal control,
  portable click-and-run, exclusive GIF/APNG/WebP like eZgif).
- **Created** the project doc set for the next session: `PROJECT_VISION.md`,
  `SESSION_HANDOFF.md`, `IMPROVEMENT_LOG.md`, `WORKLIST.md`.
- **Resolved Blocker 1 (UI base):** use Caesium's UI/UX files as the base (not a
  full copy), modeled on XNConvert's "feels & vibe". Icons/styles are
  open-source/public with no exclusive trademark (logo excluded).
- **Deferred Blocker 2 (WebP/APNG):** moved to the bucket list, to be done only
  after the pending UI/UX retrofit task.
- **Separated reference from working code:** created `reference_code/` (read-only
  source material) and `working_code/` (the product). Moved gifsicle trees +
  Caesium bundle into `reference_code/`. Auto-fetched `gifsicle-upstream`
  (kohler/gifsicle master) and `caesium-source` (Lymphatus/caesium-image-compressor
  UI) from GitHub.
- **Named the product `gifscythe`** and set the version scheme: v0.1.0 →
  1.0.0–1.9.9 (finished GIF product) → 2.0.0–3.0.0 (WebP + APNG). Created
  `working_code/gifscythe/` skeleton + `VERSION.md`.
- **Built the gifsicle engine natively** (P0): hand-wrote `config.h` in
  `reference_code/gifsicle/` (no autotools in sandbox); created
  `scripts/build_engine.sh`; engine lives in `release/0.1.0/gifsicle`.
  Verified via `scripts/test_engine.sh` (info/optimize/lossy/resize/explode).
- **Built the engine control layer** (P1): `src/core/` with `GifsicleSettings`,
  `GifsicleCommand` (argv + live CLI string), `SettingsIO`; `src/cli/main.cpp`
  → `gifscythe-cli` (prints or runs the command). Unit + integration tests pass.
  Fixed optional-value gifsicle options to attached form (`--lossy=N`, `-O3`,
  `-j4`, `--loopcount=0`).
- **Constraint discovered:** cannot compile the Qt6 GUI or Windows `gifsicle.exe`
  in this sandbox — Qt not installed, apt offline, Qt mirrors fail SSL. Only
  GitHub HTTPS is reliable. These builds must happen on a machine/CI with the
  toolchains.
- **Reviewed the authored diff** (engine control layer, CLI driver, Qt GUI
  scaffold, build scripts): C++ compiles clean with `-Wall -Wextra -pedantic`;
  shell scripts pass `bash -n`. External clones (`caesium-source/`,
  `gifsicle-upstream/`) are treated as reference only and gitignored.
- **Created + merged PR** for the P0/P1 work (engine + control layer + Qt GUI
  scaffold + one-command build). Committed the authored work and repo
  reorganization to `main`.
