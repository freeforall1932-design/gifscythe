# Improvement Log

Chronological log of decisions and changes. **Newest at the top.**

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
  workflow. The two dated review snapshots (`gifscythe-comprehensive-review.md`,
  `gifscythe-final-code-review.md`) intentionally keep their historical line
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
