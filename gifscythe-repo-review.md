# Gifscythe Repository Review — Findings & Proposed Solutions

Repository: https://github.com/freeforall1932-design/gifscythe
Report generated: 2026-09-16T06:57:30.118Z
Reviewed branch: main (STATUS.md last regenerated at session S22, 2026-09-16 per the repo)

## Methodology & limitations (read this first)

- This review was produced by fetching the repository's documentation and source files directly from the GitHub API/raw content (README, STATUS.md, COMPILED_AUDIT.md, WORKLIST.md, PROJECT_VISION.md, FEASIBILITY_REVIEW.md, docs/planning/*, and directory/file listings for working_code/gifscythe/src and web/). It is a **static, documentation-and-structure level review**, not a local build, execution, or independent test run of the code.
- Findings are labeled by **source type**:
  - `repo-audit` — already tracked by the project's own audit system (STATUS.md / COMPILED_AUDIT.md) under the cited ID. This review re-frames and prioritizes these; it does not invent them.
  - `observed` — directly measured from repository structure/file sizes during this review.
  - `inferred` — a reasonable conclusion from the above that is not stated outright in the repo's own docs; verify before treating as settled fact.
- Byte-size metrics are a **documented lower bound**: several directories (docs/planning/*, docs/ci/*, docs/legal/*, most of scripts/*.sh, src/cli/*) were confirmed to exist via directory listings but not individually downloaded and sized. Where estimated, this is noted inline.
- Nothing in this report should be read as 'the code is broken' in the sense of failing to build — the project's own CI is reported green. The findings below are about correctness edge cases the project has already found in itself, plus structural/process risks observed from the outside.

## Executive summary

- Findings compiled in this report: **20** (1 critical, 5 high, 9 medium, 3 low, 2 process/meta).
- The project's own status register (STATUS.md) currently tracks **146** items: 96 DONE, 8 PARTIAL, 42 OPEN, 0 UNTRIAGED, across **22** logged work sessions.
- Exactly **1** release-blocker-severity (P0) item remains open: a cancel/failure path that can destroy a user's existing output file (F-01 / U-59). This should be fixed before any other work.
- Measured documentation + process-gate scripts total roughly **843.5 KB**, versus roughly **289.9 KB** of actual shipped source + tests this review was able to size — a ratio of about **2.9:1** in favor of documentation, and likely higher once un-sized files are counted.
- The core "portable Windows app" claim has never been verified by a human on a real Windows machine (F-09) — every Windows check so far is Wine emulation or headless CI, a fact the project's own README already flags.
- Product scope promises GIF + APNG + WebP; APNG and WebP are both 0% implemented and formally deferred (F-17). Today it is a GIF-only tool.

## Findings

### F-01 — Cancel / failed run can destroy the last good output file

- **Severity:** CRITICAL
- **Area:** Engine / Desktop GUI
- **Repo tracked state:** OPEN
- **Source type:** repo-audit
- **Source reference:** STATUS.md row U-59 · COMPILED_AUDIT.md §6 P0-7

**What's wrong:** gifsicle writes its result directly to the final -o path. If a run is cancelled mid-write, or fails, nothing removes the truncated file or restores the previous one. Re-optimizing an existing *_opt.gif and hitting Cancel can destroy the last good result with no undo.

**Evidence / how to reproduce or verify:** Tracked as P0 (release-blocker severity) in the project's own register and still OPEN as of the last recorded session (S22). This is the only P0-severity row in the entire 146-item register that is not yet closed.

**Proposed fix:** Write to a temp file beside the target and atomically rename on verified success only — the exact tmp+fsync+rename pattern the project already ships for settings persistence (U-16, src/core/SettingsIO.h). Apply it to OutputPlan/ProcessRunner for every engine invocation that overwrites an existing file, plus one regression test that SIGTERMs the engine mid-write and asserts the original file is untouched.

### F-02 — Self-hosted web server has no concurrency cap or run timeout

- **Severity:** HIGH
- **Area:** Web server
- **Repo tracked state:** PARTIAL
- **Source type:** repo-audit
- **Source reference:** STATUS.md row U-06 · WORKLIST P1-5

**What's wrong:** The web build is a supported, self-hosted product surface with an opt-in LAN bind (GS_WEB_HOST). Loopback-by-default and the 64MB body cap landed, but the concurrency cap, per-client rate limiting, and a hard wall-clock bound on engine runs named in the original finding are still missing.

**Evidence / how to reproduce or verify:** STATUS.md marks U-06 PARTIAL and explicitly lists the three missing controls. Any user who opts into a LAN bind (a documented, intended use) currently has no protection against one peer starting unbounded concurrent or long-running gifsicle processes.

**Proposed fix:** Add a bounded job queue (max N concurrent child processes), a per-IP token bucket in front of /run and /optimize, and a kill-on-timeout wrapper around the engine subprocess that also runs the cleanup path from F-01.

### F-03 — In-flight web run isn't invalidated when settings change

- **Severity:** HIGH
- **Area:** Web server
- **Repo tracked state:** OPEN
- **Source type:** repo-audit
- **Source reference:** STATUS.md row U-54 · P1-34

**What's wrong:** A request generation counter (requestGen) already exists and correctly cancels stale responses when the file queue changes (this fixed U-46). It does not yet advance when a settings field changes mid-request, so controls stay editable while a fetch('/run') is pending and an old response can be shown against new, different settings.

**Evidence / how to reproduce or verify:** Documented as OPEN, scoped P1-34, in STATUS.md.

**Proposed fix:** Reuse the existing requestGen mechanism: bump it on every settings-field input event, not only on queue mutation, exactly mirroring the pattern already proven for U-46.

### F-04 — WASM build can silently report success from a stale output file

- **Severity:** MEDIUM
- **Area:** Web / WASM build
- **Repo tracked state:** OPEN
- **Source type:** repo-audit
- **Source reference:** STATUS.md row U-57 · P1-37

**What's wrong:** wasm.js does not unlink or snapshot the virtual /out.gif before calling the engine's entry point. After one successful run, a later run that exits 0 without writing will read the previous GIF and report success for the wrong input.

**Evidence / how to reproduce or verify:** Documented OPEN in STATUS.md; also currently unshippable regardless, see F-15 (OD-16 licensing block).

**Proposed fix:** Unlink/snapshot /out.gif before every callMain() and verify identity/mtime changed afterward — the same postcondition check already implemented natively via OutputVerify.h.

### F-05 — Batch runs can silently mix settings across files in the same job

- **Severity:** MEDIUM
- **Area:** Desktop GUI
- **Repo tracked state:** OPEN
- **Source type:** repo-audit
- **Source reference:** STATUS.md row U-58 · P1-38

**What's wrong:** Batch output targets are correctly snapshotted once at batch start (this is what U-01 fixed), but currentSettings() is still read live during continuation, so changing a control mid-batch can apply different engine settings to different files in the same run without any indication to the user.

**Evidence / how to reproduce or verify:** Documented OPEN in STATUS.md, session S12 scoping note attached.

**Proposed fix:** Snapshot the full settings object at the same point batchTargets_ is snapshotted, and freeze the relevant controls (same lock pattern already used for the output-folder group, U-45) for the duration of the batch.

### F-06 — Stale UI state after failure / cancel can mislead the user

- **Severity:** MEDIUM
- **Area:** Desktop GUI
- **Repo tracked state:** OPEN
- **Source type:** repo-audit
- **Source reference:** STATUS.md rows U-69, U-72

**What's wrong:** Two related state-lifetime bugs: (1) after a failed run, the previous successful "After" preview can remain visible next to a "Failed" status; (2) the cancelling_ flag is cleared unconditionally 3 seconds after a cancel request, so if the engine actually dies later than that, the finished() handler shows an error dialog even though the user already cancelled on purpose.

**Evidence / how to reproduce or verify:** Both documented OPEN in STATUS.md (P2-16 and P1-42 respectively).

**Proposed fix:** Route both through the same invalidatePreview()/state-machine discipline already used to fix the very similar U-47 preview-staleness bug — clear result state on every failure, and gate the error dialog on the engine's actual process-exit reason rather than a fixed timer.

### F-07 — Two Unicode edge cases in filename safety on Windows

- **Severity:** MEDIUM
- **Area:** Engine / Core
- **Repo tracked state:** OPEN
- **Source type:** repo-audit
- **Source reference:** STATUS.md rows U-55, U-56

**What's wrong:** Windows output-collision detection folds case using ASCII tolower() only, so two names differing only by non-ASCII case (e.g. an accented capital letter) are not recognized as the same file and can silently overwrite one another. Separately, the reserved-device-name guard (COM/LPT) checks ASCII digits only and misses the Unicode superscript aliases (COM¹, LPT²) that Windows itself still treats as reserved.

**Evidence / how to reproduce or verify:** Both OPEN in STATUS.md, filed as new findings in the same review pass that closed the primary Windows Unicode bug (U-07/N-04).

**Proposed fix:** Extend the already-built path_key()/NameRules machinery (built for U-07/U-21) to use a full Unicode case-fold table instead of ASCII tolower, and add the superscript aliases to the reserved-name list.

### F-08 — The desktop GUI is the one surface without a shared output-correctness check

- **Severity:** HIGH
- **Area:** Desktop GUI
- **Repo tracked state:** PARTIAL
- **Source type:** repo-audit
- **Source reference:** STATUS.md row GS-203 · P1-25

**What's wrong:** A shared "did the engine actually produce a valid, changed, non-empty GIF" check now exists and is wired into the CLI and the web server. It has not yet been integrated into the Qt GUI's own run lifecycle, so the desktop app — the primary shipped artifact — is currently the only surface that can still show a bare "Success" without the same postcondition check the other two surfaces already enforce.

**Evidence / how to reproduce or verify:** STATUS.md marks GS-203 PARTIAL with the GUI integration explicitly named as the missing half.

**Proposed fix:** Call the existing core verifier (src/core/OutputVerify.h / ExplodeVerify.h) from MainWindow's run-finished handler before flipping status to Success, exactly as CLI/web already do.

### F-09 — "Portable Windows app" has never been run by a human on real Windows

- **Severity:** HIGH
- **Area:** Process / Release
- **Repo tracked state:** OPEN
- **Source type:** observed
- **Source reference:** STATUS.md rows W-18, W-19 · README "emulation signal, not Windows proof"

**What's wrong:** Every Windows verification claim in the README is either (a) Wine emulation on Linux, which the project's own README explicitly qualifies as "an emulation signal, not Windows proof", or (b) Qt's offscreen platform plugin in headless CI. The physical-desktop-only behaviors that make or break a click-and-run app — real drag-and-drop from Explorer, killing the engine mid-run, the engine-missing dialog, real DPI scaling and window chrome — have zero recorded runs on an actual Windows machine.

**Evidence / how to reproduce or verify:** STATUS.md: W-18 and W-19 both OPEN, blocked on "a clean Windows VM" / "a physical desktop" that has apparently never been available across 22 recorded sessions.

**Proposed fix:** This is cheap to close and disproportionately important: rent a Windows VM for under an hour (or use a personal Windows machine once), download the actual Release artifact, and manually run the checklist already written and waiting in docs/ci/CLEAN_WINDOWS_SMOKE.md. Do this before any further feature work — it is the only remaining unverified claim behind the product's core pitch.

### F-10 — Engine-selection semantics differ silently between desktop and web

- **Severity:** MEDIUM
- **Area:** Engine / Web server
- **Repo tracked state:** OPEN
- **Source type:** repo-audit
- **Source reference:** STATUS.md rows U-63, U-65, U-66

**What's wrong:** Three related gaps: --no-loopcount ("play once") has no control anywhere in the UI; CLI engine discovery beside a symlinked executable is weaker than its own PATH-based discovery; and, most importantly, the desktop app pins to the exact GS_VERSION engine folder while the web server's findEngine() auto-selects the newest version directory it finds — so the desktop build and the web build of the very same commit can end up executing two different gifsicle binaries.

**Evidence / how to reproduce or verify:** All three OPEN in STATUS.md, filed together under the same review pass (P1-40/P1-41).

**Proposed fix:** Make both surfaces resolve the engine version the same way — either both pin to GS_VERSION, or both accept an explicit override — and add loop-once as a tri-state alongside the existing unchanged/forever/N loop control.

### F-11 — Documentation and process tooling now outweigh the product code by roughly 3:1

- **Severity:** PROCESS/META
- **Area:** Process / Repo hygiene
- **Repo tracked state:** N/A
- **Source type:** observed
- **Source reference:** Measured via GitHub API blob sizes across all top-level docs, docs/, and scripts/*.sh

**What's wrong:** COMPILED_AUDIT.md alone is 214 KB — larger than the entire Qt GUI implementation (MainWindow.cpp + SettingsPanel.cpp + PreviewPanel.cpp combined, ~110 KB) plus the whole src/core control layer (~50 KB). check_docs.sh, a shell script whose only job is verifying the documentation is internally consistent, is 55.6 KB — bigger than MainWindow.cpp itself. Across README/STATUS/COMPILED_AUDIT/WORKLIST/SESSION_HANDOFF/IMPROVEMENT_LOG/archive files/planning docs/gate scripts, this review measured roughly 860 KB of process artifacts against roughly 320 KB of actual shipped source + tests (see Metrics tab for the full breakdown and method).

**Evidence / how to reproduce or verify:** Direct byte-size comparison of fetched blobs; see Metrics tab for the itemized table. This is a structural observation, not a bug, but it is the largest single lever available to make the project ship faster.

**Proposed fix:** See Roadmap: replace the hand-rolled STATUS.md/COMPILED_AUDIT.md/check_docs.sh gate system with a short, human-maintained CHANGELOG plus a normal issue tracker. The self-audit process has become a second codebase that itself needs maintaining (see F-12).

### F-12 — The audit/gate system has already broken itself at least twice

- **Severity:** PROCESS/META
- **Area:** Process / Repo hygiene
- **Repo tracked state:** DONE
- **Source type:** repo-audit
- **Source reference:** STATUS.md rows N-01, N-06

**What's wrong:** N-01: a 'pending workflow change' marker outlived the change it described, leaving eight separate documentation locations quoting a stale gate count. N-06: the documentation-count generator behaved differently under gawk vs. mawk (character-count vs. byte-count semantics), so the exact same committed STATUS.md passed its own consistency gate under one AWK implementation and failed under another, undetected until a sandbox happened to lose gawk mid-session.

**Evidence / how to reproduce or verify:** Both are logged and fixed (DONE) in the project's own register — cited here not as still-open bugs, but as evidence that a 55KB+ bespoke documentation-verification script is itself a nontrivial source of bugs, on top of the product it's meant to protect.

**Proposed fix:** Reduce the surface area of the thing that verifies the docs, by reducing the size and mutability of the docs it verifies (see F-11 and Roadmap).

### F-13 — Three parallel UI/build stacks are being maintained at once

- **Severity:** MEDIUM
- **Area:** Process / Architecture
- **Repo tracked state:** PARTIAL
- **Source type:** observed
- **Source reference:** Repo layout: CMakeLists.txt + gifscythe.pro, web/, csharp/spike; STATUS.md row GS-210

**What's wrong:** The Qt desktop GUI is built by two independently maintained build systems (CMake and qmake/.pro) whose dispatch order is called out in the project's own audit as undecided (GS-210, PARTIAL). A third UI stack — a C# spike — is parked in csharp/spike/ with no committed revisit date. A fourth, the Node.js web/ app, is a full separate reimplementation of the settings/validation/command-building logic in JavaScript, kept in parity with the C++ core by hand-written parity test suites rather than a single shared implementation.

**Evidence / how to reproduce or verify:** Directory listing confirms all four surfaces exist simultaneously in the current tree.

**Proposed fix:** Pick one desktop build system and delete the other's config file. Set an explicit revisit-or-delete date for csharp/spike. Longer term, see Roadmap for collapsing the UI stack count from effectively 3 (Qt, Node web, parked C#) to 1.

### F-14 — A superseded duplicate of the vendored engine source still ships in the tree

- **Severity:** LOW
- **Area:** Repo hygiene
- **Repo tracked state:** N/A
- **Source type:** observed
- **Source reference:** reference_code/gifsicle-nested-1.96/, called out as an "oddity" in FEASIBILITY_REVIEW.md

**What's wrong:** The project's own original feasibility review flagged having both a root gifsicle tree and a nested duplicate variant as something to resolve by picking one tree. The duplicate (reference_code/gifsicle-nested-1.96) is still present, now explicitly labeled 'reference only', adding clone size and reviewer confusion for no active purpose.

**Evidence / how to reproduce or verify:** Confirmed present in the current repo tree listing.

**Proposed fix:** Delete it, or move it out of the git history entirely if it must be kept for provenance, per the project's own reference_code policy.

### F-15 — The in-browser WASM path is fully blocked on an unanswered legal question

- **Severity:** MEDIUM
- **Area:** Licensing / Web
- **Repo tracked state:** OPEN
- **Source type:** repo-audit
- **Source reference:** docs/planning/OWNER_DECISIONS.md OD-16 · STATUS.md row D-07

**What's wrong:** web/wasm/ is scaffolded (build script, one-screen UI, byte-proof script) but cannot ship: it would run the GPLv2 engine in-process with the Ms-PL UI inside a single WASM module, and the FSF lists Ms-PL as GPL-incompatible. The project's own recommendation is 'no, stays experimental' until counsel answers — but no owner or counsel has answered as of the last recorded session.

**Evidence / how to reproduce or verify:** Documented directly in OWNER_DECISIONS.md as OD-16, unanswered.

**Proposed fix:** Either get an explicit legal answer, or formally drop the in-process WASM direction and keep the existing self-hosted Node server (which has no such conflict, since gifsicle stays an out-of-process subprocess there) as the only web path. Stop investing further engineering time in web/wasm/ until this is resolved either way — it is currently guaranteed-unshippable code.

### F-16 — The only banked Windows release artifact is already known to be stale

- **Severity:** HIGH
- **Area:** Process / Release
- **Repo tracked state:** OPEN
- **Source type:** repo-audit
- **Source reference:** STATUS.md row U-09 · P0-4

**What's wrong:** The Windows binaries banked on the 'snapshot-2026-09-07' release are recorded by the project's own audit as 5 commits behind the SHA the release notes claim, and no re-cut has happened since. Anyone downloading that Release today is not running current main.

**Evidence / how to reproduce or verify:** OPEN in STATUS.md, scoped as P0-4 (release blocker), still unresolved.

**Proposed fix:** Re-cut the release from one reviewed, current SHA per the project's own docs/release/RELEASE_PROCEDURE.md, then combine with the F-09 real-hardware smoke test in the same pass so both blockers close together.

### F-17 — Product name/pitch promises 3 formats; 2 of 3 are 0% built and deferred

- **Severity:** MEDIUM
- **Area:** Product scope
- **Repo tracked state:** N/A
- **Source type:** observed
- **Source reference:** PROJECT_VISION.md mission statement vs. STATUS.md rows D-01..D-03 (all OPEN, deferred past 1.0.0)

**What's wrong:** The stated mission is 'GIF, APNG, and WebP' animation editing. The common frame model, WebP support, and APNG support are all explicitly deferred until after a 1.0.0 that itself hasn't shipped. Today the product is, functionally, a GUI and web front-end over the gifsicle CLI — GIF only. That's a perfectly reasonable v1 scope, but it isn't how the project introduces itself.

**Evidence / how to reproduce or verify:** Cross-referenced mission statement against the deferred-item rows in STATUS.md.

**Proposed fix:** Rename the public-facing pitch to 'a focused GIF optimizer/editor, with APNG and WebP on the roadmap' until D-01 lands. This is a documentation change, not a code change, and removes a real expectation-mismatch risk for the very first users.

### F-18 — Input validation gaps: unchecked format admission and unchecked numeric ranges

- **Severity:** MEDIUM
- **Area:** Desktop GUI / Core
- **Repo tracked state:** OPEN
- **Source type:** repo-audit
- **Source reference:** STATUS.md rows GS-205, GS-206

**What's wrong:** The file picker offers an 'All files' option and appendInputs() does not validate file content; drag-and-drop only checks that a dropped path exists, not that it is actually a GIF. Separately, several numeric settings (loop count, thread count, gamma, method-name enums) are narrowed from long to int with no range checks and no validation rule at all in Validate.h, so out-of-range values can reach the engine partially unchecked.

**Evidence / how to reproduce or verify:** Both OPEN in STATUS.md, filed with exact file:line references in the project's own intake (SettingsPanel.cpp, MainWindow.cpp, Validate.h).

**Proposed fix:** Add one shared admitInputs() (existing, readable, regular file + real GIF magic-byte check) used by both the picker and the drop handler; parse numeric settings with std::from_chars into the destination width and add the missing Validate.h domains, mirrored in the JS validator for web parity.

### F-19 — CI cannot enforce its own documentation gate on the branch that matters

- **Severity:** LOW
- **Area:** Process / CI
- **Repo tracked state:** OPEN
- **Source type:** repo-audit
- **Source reference:** STATUS.md rows R-03, W-30

**What's wrong:** The credential available to the automated sessions has no `workflows` GitHub App scope, so the actual .github/workflows/build.yml cannot be updated directly. A hand-maintained byte-identical copy (docs/ci/build.yml.proposed) has to be kept in sync by a separate drift-detection gate instead of just being the live file.

**Evidence / how to reproduce or verify:** Documented OPEN in STATUS.md; blocks W-30 specifically.

**Proposed fix:** A maintainer with a workflows-scoped token applies the pending change once and deletes the marker file in the same commit — a five-minute human task that has been open across multiple sessions because no such token has been available to the agent.

### F-20 — Oversized web request bodies return an inaccurate status code

- **Severity:** LOW
- **Area:** Web server
- **Repo tracked state:** PARTIAL
- **Source type:** repo-audit
- **Source reference:** STATUS.md row U-68

**What's wrong:** readBody() rejects an oversized body with a generic error that a caller maps to HTTP 400 (bad request) in one path, where 413 (payload too large) is now correctly produced elsewhere after the fix. The mapping is inconsistent between the two call sites.

**Evidence / how to reproduce or verify:** Marked PARTIAL in STATUS.md with the remaining call site named.

**Proposed fix:** Route every oversized-body rejection through the same typed BodyTooLargeError already introduced for the fixed path, so both call sites map to 413 identically.

## Documentation vs. code size (measured)

| File / area | Size | Kind |
|---|---|---|
| src/core (C++ control-layer headers) _(8 of the headers this review fetched directly; Validate.h/SettingsIO.h/WinUnicode.h exist but were not individually sized — real total is higher)_ | 48.0 KB | code |
| src/qtui (Qt6 desktop GUI) | 107.8 KB | code |
| src/cli (CLI driver) _(estimated lower bound — directory listing confirmed present, main.cpp not individually sized)_ | 7.8 KB | code |
| web/ app (server.mjs, app.js, command.mjs, static assets) | 61.1 KB | code |
| web/test (5 Node test suites) | 65.1 KB | code |
| COMPILED_AUDIT.md | 208.8 KB | docs |
| IMPROVEMENT_LOG.md | 156.6 KB | docs |
| docs/archive/ (2 superseded review snapshots) | 125.0 KB | docs |
| SESSION_HANDOFF.md | 64.3 KB | docs |
| check_docs.sh (doc-consistency gate script) | 54.4 KB | docs |
| docs/audit/EXTERNAL_REVIEW_INTAKE_2026-09-12.md | 45.8 KB | docs |
| STATUS.md | 41.4 KB | docs |
| WORKLIST.md | 40.4 KB | docs |
| docs/audit/CONSOLIDATED_AUDIT_2026-09-10.md | 23.4 KB | docs |
| FEASIBILITY_REVIEW.md | 16.3 KB | docs |
| web/WEB_PLAN_TEMPLATE.md | 15.2 KB | docs |
| pr_preflight.sh (PR-gate script) | 11.4 KB | docs |
| web/README.md | 10.3 KB | docs |
| PROJECT_VISION.md | 8.4 KB | docs |
| working_code/gifscythe/README.md | 8.7 KB | docs |
| root README.md | 13.1 KB | docs |

**Totals measured:** code+tests ≈ 289.9 KB · docs+gate scripts ≈ 843.5 KB · ratio ≈ 2.91:1

## Proposed roadmap

### Phase 0 — Stop the bleeding (this week) (1–3 days)

**Goal:** Close the one bug in the entire register that can destroy user data, before anything else.

- Fix F-01 / U-59: cancel or a failed run must never leave a truncated file over a pre-existing good output. Write-to-temp + atomic-rename-on-verified-success only (reuse the pattern already shipped for settings persistence).
- Add a regression test that SIGTERMs the engine mid-write and asserts the original output file is byte-identical afterward.
- Ship this alone as a patch release note — it is the highest-value single commit available in the whole backlog.

_Citation: STATUS.md U-59 / COMPILED_AUDIT.md §6 P0-7_

### Phase 1 — Verify the one claim that was never actually tested (1 day, mostly waiting)

**Goal:** Replace Wine emulation and headless CI with one real run on real Windows.

- Rent a Windows VM for under an hour (or use any spare Windows machine) — this has apparently never happened across 22 recorded work sessions.
- Download the actual current Release build (after Phase 2's re-cut) and manually run the checklist already written in docs/ci/CLEAN_WINDOWS_SMOKE.md.
- Record the three physical-only probes the project has flagged as impossible in CI: drag-and-drop from Explorer, killing the engine mid-run, and the engine-missing dialog.

_Citation: STATUS.md W-18 / W-19; README's own "emulation signal, not Windows proof" caveat_

### Phase 2 — Re-cut a trustworthy release artifact (1 day)

**Goal:** Make sure the thing a user downloads matches the commit the notes claim.

- Re-cut the Windows package from one current, reviewed main SHA per the project's own docs/release/RELEASE_PROCEDURE.md.
- Fold in the Phase 0 fix and the Phase 1 verification evidence into the same release.
- Update the README hero copy to say 'GIF optimizer, APNG/WebP on the roadmap' instead of implying all three ship today (F-17) — a docs-only change with real expectation-setting value.

_Citation: STATUS.md U-09 / P0-4_

### Phase 3 — Cut the process overhead down to something sustainable (1 week, then ongoing)

**Goal:** Stop maintaining a documentation system that is now larger and more fragile than the product it describes.

- Freeze COMPILED_AUDIT.md and IMPROVEMENT_LOG.md as historical archives — do not keep growing them.
- Replace STATUS.md + check_docs.sh's generated register with a plain GitHub Issues board (labels: P0/P1/P2, area) plus a short, hand-written CHANGELOG.md. Issues already give you state, history, and search for free — a bespoke 42KB register enforced by a 55KB shell script does not need to be reinvented.
- Keep one short (1–2 page) STATUS snapshot for newcomers, updated weekly, not machine-regenerated per session.
- This is the single highest-leverage change available: today, documentation and gate scripts alone (~860KB, see Metrics) outweigh the entire shipped product (~320KB) by roughly 3 to 1.

_Citation: Observed: F-11, F-12_

### Phase 4 — Consolidate the UI stack (spike first, then decide) (2–3 day time-boxed spike)

**Goal:** Stop maintaining three UI stacks (Qt/C++, Node web, parked C#) and two build systems (CMake + qmake) for one product.

- Time-box a spike: wrap the already-built, already best-tested web/ front end (vanilla JS + command.mjs/validate.mjs, which already has 5 passing Node test suites) in Tauri, keeping gifsicle as an untouched local subprocess.
- Compare the result against the current Qt build on: bundle size, build time, and how much of the ~90KB MainWindow.cpp/SettingsPanel.cpp needs to be rewritten vs. deleted outright.
- If the spike wins (it is favored — see the language comparison below), delete the qmake file, freeze/retire the Qt GUI, and delete the parked csharp/spike/ experiment.
- This exactly matches the trigger condition the project's own OFFLINE_BUILD_REVIEW.md already names for revisiting the language choice: 'UI must be web-tech' — which became true the day the parallel web/ app was built.

_Citation: docs/planning/OFFLINE_BUILD_REVIEW.md §4 trigger conditions; observed F-13_

### Phase 5 — Only then: APNG / WebP (after 1.0.0 has real users)

**Goal:** Avoid building speculative format support before the GIF-only product has been used by anyone outside the project.

- Keep the same subprocess-boundary pattern that already keeps the GPLv2 engine license-clean: add libwebp/libpng-apng as separate local helper subprocesses, never linked in-process.
- Build the common RGBA frame model (D-01) only once real usage data shows which operations (crop/resize/reorder/etc.) actually need to be format-agnostic.
- Formally resolve OD-16 (WASM + GPLv2 in-process legal question) before investing further in web/wasm/, or drop that direction entirely.

_Citation: STATUS.md D-01..D-03, D-07 / OD-16_

## Stack / language options for the next UI iteration

| Option | Bundle size | Code reuse | Dev velocity | Key risk | Verdict |
|---|---|---|---|---|---|
| C++17 + Qt6 Widgets (current desktop GUI) | ~30–40 MB (windeployqt folder) | Highest — it's already built and CI-verified end to end | Slow — two build systems, ~90KB of hand-written widget code, never run on real hardware | Never verified on physical Windows (F-09); dual build system maintenance (F-13) | KEEP-AS-FALLBACK |
| Tauri (Rust shell) + the existing web/ front end | ~10–15 MB + WebView2 (present on nearly all Win10/11 machines) | Very high — reuses the already best-tested code in the repo (command.mjs, validate.mjs, 5 passing Node suites) | Fast — front end already exists; only a thin Rust shell + subprocess wiring is new | WebView2 must be present or bundled; still needs the same real-hardware Windows check as any option | RECOMMENDED |
| Electron + the existing web/ front end | ~150 MB+ (bundles full Chromium) | High — same reuse story as Tauri | Fast, easiest of all options to get running | Bundle size directly contradicts the project's own 'lightweight, portable' goal | NOT-RECOMMENDED |
| Keep self-hosted Node web server as the primary surface, no native shell | N/A — runs in an existing browser | Total — zero new code beyond what already exists | Fastest possible — ship what's already built and tested today | Not 'click an .exe' — needs 'start a local server, open a browser tab', a real UX cost for non-technical users | KEEP-AS-FALLBACK |

Recommendation: keep gifsicle as an untouched local subprocess (unchanged — this is what keeps the GPLv2/Ms-PL license boundary clean), and run a short, time-boxed spike wrapping the existing web/ front end in Tauri before investing further in the Qt GUI. Full rationale in the Roadmap section above.

---
_End of report. Generated by the Gifscythe Repo Review dashboard._
