# AUDIT A (GPT 5.6 sol xhigh) — Gifscythe Codebase Audit — full findings text


## GS-001 [Critical] (GUI) — Batch output planning can overwrite another result or the source GIF
- path: working_code/gifscythe/src/qtui/MainWindow.cpp :: defaultOutputFor(), templateIsConstant(), runCommand()
- certainty: Confirmed
- impact: Silent data loss. Two inputs with the same base name and one batch folder both resolve to the same target. A template such as {name}.gif with no separate batch folder can also resolve the output to the input itself.
- evidence: The preflight rejects only templates with no {name} token. It never builds and compares the actual target paths, never compares targets with input paths, and does not ask before replacing an existing file.
- reproduce:
    mkdir -p /tmp/gs-a /tmp/gs-b /tmp/gs-out
    cp reference_code/gifsicle/logo.gif /tmp/gs-a/foo.gif
    cp reference_code/gifsicle/logo1.gif /tmp/gs-b/foo.gif
    In the GUI, add both files, choose Batch, set /tmp/gs-out, keep {name}_opt.gif, then run.
    Inspect /tmp/gs-out: only foo_opt.gif remains; the second job replaced the first.
- expected: The app refuses colliding targets or creates two unique outputs, and it never replaces a source file silently.
- actual: The same path is used twice. Existing targets are overwritten without a confirmation step.
- fix:
    Add planBatchOutputs() and compute every source/target pair before setBusy(true).
    Compare normalized absolute paths; use case-insensitive comparison on Windows and macOS where appropriate.
    Reject duplicate targets and target-equals-source. Prompt once for pre-existing files or default to non-destructive suffixing.
    Write to a temporary sibling file and rename only after a valid non-empty GIF is produced.
- regression: GUI test: same basename from two folders, self-target template, case-only collision on Windows, and pre-existing output.

## GS-002 [Critical] (Packaging) — The portable packager can report success for an incomplete or stale release
- path: working_code/gifscythe/scripts/package_portable.sh :: optional copy branches and final sanity check
- certainty: Confirmed
- impact: A release can be published without the CLI or GUI, with old files from a prior build, or with an incomplete Qt runtime while the packaging command still exits zero.
- evidence: The script does not clear the destination, treats CLI and GUI as optional, converts windeployqt failure into a note, and verifies only that the engine exists.
- reproduce:
    cd working_code/gifscythe && ./scripts/build_engine.sh
    rm -rf build release/0.1.0/Gifscythe
    ./scripts/package_portable.sh; echo $?
    The command exits 0 and creates an engine-and-docs folder without an application binary.
- expected: A desktop portable package is produced completely or the command fails.
- actual: The script prints 'Portable package created' for a partial bundle and can retain stale binaries or DLLs.
- fix:
    Stage into a fresh mktemp directory and atomically replace the destination only after validation.
    Require the exact engine, CLI, GUI, license set, and platform runtime for the selected package type.
    Require windeployqt on Windows and propagate its non-zero status.
    Run the packaged CLI from the staging folder and inspect dynamic dependencies before success.
- regression: Negative packaging tests: missing CLI, missing GUI, failing windeployqt, stale destination, wrong-platform engine, and absent license.

## GS-003 [High] (CLI / core) — CLI run without output corrupts its own binary stdout stream
- path: working_code/gifscythe/src/cli/main.cpp :: main(): command print followed by run_argv()
- certainty: Confirmed
- impact: A valid engine run exits zero but redirected output is not a valid GIF because human-readable status lines are written to stdout before the engine bytes.
- evidence: Validation does not require output. main() always prints the banner and command to stdout, then run_argv() lets an output-less gifsicle inherit that same stream.
- reproduce:
    printf 'mode = auto\ninput = /absolute/path/to/gifscythe/reference_code/gifsicle/logo.gif\n' > /tmp/no-output.conf
    working_code/gifscythe/build/gifscythe-cli /tmp/no-output.conf --run > /tmp/result.gif
    head -c 6 /tmp/result.gif
    The file begins with '# Gifs' rather than GIF87a or GIF89a, even if the exit code is 0.
- expected: Either require an output path or reserve stdout exclusively for GIF bytes.
- actual: Status text and binary data share stdout.
- fix:
    Simplest: reject --run when output is empty, matching the GUI's non-Explode policy.
    If stdout output is a supported feature, send all diagnostics and the command preview to stderr before exec.
    Add an explicit --stdout mode so binary behavior cannot happen accidentally.
- regression: CLI E2E: output-less --run must fail, or redirected bytes must start with GIF8 and contain no status prefix.

## GS-004 [High] (CLI / core) — Documented PATH engine fallback is rejected by the CLI preflight
- path: working_code/gifscythe/src/core/EngineLocator.h :: locate_engine() and path_is_executable()
- certainty: Confirmed
- impact: The CLI says it supports a gifsicle found on PATH, but --run fails unless that bare name also exists in the current directory.
- evidence: locate_engine() falls back to the string 'gifsicle'. path_is_executable() checks that string with filesystem::is_regular_file instead of searching PATH. exe_path_of() also does not actually resolve argv[0] through PATH.
- reproduce:
    mkdir -p /tmp/gs-cli /tmp/gs-engine /tmp/gs-empty
    cp working_code/gifscythe/build/gifscythe-cli /tmp/gs-cli/
    cp working_code/gifscythe/release/0.1.0/gifsicle /tmp/gs-engine/
    cd /tmp/gs-empty && PATH=/tmp/gs-engine:$PATH /tmp/gs-cli/gifscythe-cli /absolute/test.conf --run
    Observe 'engine not found at gifsicle' although command -v gifsicle succeeds.
- expected: PATH fallback resolves to an absolute executable and runs.
- actual: The fallback is a bare name that fails the preflight.
- fix:
    Search each PATH entry in EngineLocator and return the first executable absolute path.
    On Windows, honor PATHEXT and use wide-character filesystem APIs.
    Resolve argv[0] through PATH before deriving the executable directory.
    Treat a non-empty GS_ENGINE as a strict override instead of silently falling through when it is invalid.
- regression: Unit and CLI smoke tests with only PATH, invocation by bare CLI name, invalid GS_ENGINE, and paths containing spaces.

## GS-005 [High] (Web POC) — The demo server is network-exposed and has no resource isolation
- path: web/server.mjs :: server.listen(), readBody(), run()
- certainty: Confirmed
- impact: Any host that can reach port 8000 can start concurrent native image-processing jobs. Uploads can be 64 MB, settings can request extreme resize work, concurrency is unlimited, and stderr is buffered without a cap.
- evidence: The server binds 0.0.0.0, has no authentication or request queue, and passes client-controlled settings to a native subprocess. The README correctly says not to expose it publicly, but the default bind does exactly that on a local network.
- reproduce:
    node web/server.mjs 8000
    From another host on the same network, POST a GIF to http://HOST:8000/optimize.
    Repeat requests in parallel with large files and aggressive dimensions; each request can create a process for up to 120 seconds.
- expected: A local demo is loopback-only and bounded, or a network service has production controls.
- actual: The zero-auth endpoint is reachable on every interface and can exhaust CPU, memory, process slots, and disk.
- fix:
    Bind to 127.0.0.1 by default and require an explicit --host flag for remote access.
    Validate GIF magic, settings types, pixel dimensions, frame count, and total work before spawning.
    Add a small concurrency semaphore, queue limit, output/stderr caps, and per-client rate limit.
    Sandbox the engine and document that the POC is not a deployable service.
- regression: API tests for loopback bind, 413 handling, malformed GIF, extreme settings, concurrency limit, timeout cleanup, and absent output.

## GS-006 [High] (CLI / core) — Windows CLI execution is not Unicode-safe
- path: working_code/gifscythe/src/core/ProcessRunner.h :: CreateProcessA() and narrow std::string paths
- certainty: Strong risk
- impact: Input, output, engine, or settings paths containing characters outside the active Windows code page can fail or resolve to the wrong path for international users.
- evidence: The Windows branch deliberately uses CreateProcessA. Settings and filesystem paths cross the API boundary as std::string. Correct argument quoting fixes spaces but does not fix character encoding.
- reproduce:
    On Windows, create C:\gifscythe-test\<non-ASCII-name>\input.gif.
    Reference that path in a settings file and run gifscythe-cli.exe --run.
    Repeat under a Windows user whose profile name is not representable in the current ANSI code page.
- expected: Every valid Windows Unicode path works.
- actual: CreateProcessA interprets bytes through the active ANSI code page, so UTF-8 paths are not reliably representable.
- fix:
    Use CreateProcessW and build a UTF-16 command line with the same quoting rules.
    Use std::filesystem::path/wstring at Windows filesystem boundaries.
    Define UTF-8 as the settings-file encoding and convert once at the boundary.
- regression: Native Windows E2E with CJK, accented, and supplementary-plane characters in every relevant path.

## GS-007 [High] (Packaging) — Release bundles do not carry a complete, unambiguous license set
- path: LICENSE, scripts/package_portable.sh, scripts/package_system.sh :: license copy steps
- certainty: Confirmed
- impact: Release readiness and redistribution compliance are at risk. The root LICENSE is a short notice saying the UI is 'intended' for GPLv3, not the full GPLv3 text. Qt LGPL notices are not staged, and the system package omits the root UI license entirely.
- evidence: package_portable copies LICENSE and COPYING.gifsicle only. package_system looks for a non-existent root COPYING and copies only the engine license from reference_code. Neither script includes a full GPLv3 or LGPLv3 text or Qt attribution materials.
- reproduce:
    cd working_code/gifscythe && ./scripts/package_portable.sh && ./scripts/package_system.sh
    find release/0.1.0/Gifscythe* -maxdepth 1 -type f -printf '%f\n'
    Verify that the system package lacks LICENSE and neither package contains the complete GPLv3/LGPLv3 texts or Qt notices.
- expected: Each distributed bundle contains the applicable grants, complete license texts, notices, and required source/relink information.
- actual: The staged legal material is incomplete and differs by package type.
- fix:
    Choose and state the first-party license as an actual grant, not only an intent statement.
    Ship complete GPLv3, GPLv2-only, LGPLv3, and relevant Qt notices in a LICENSES directory.
    Make both packagers require the same audited legal manifest.
    Have release counsel confirm the final Qt and bundled-engine obligations before 1.0.0.
- regression: Manifest test that hashes and requires every legal file in every package variant.

## GS-008 [High] (Docs / release) — The banked Windows snapshot is not tied to the source revision claimed by its notes
- path: GitHub Release snapshot-2026-09-07 and CLEAN_WINDOWS_SMOKE.md :: tag, release body, preferred smoke asset
- certainty: Confirmed
- impact: Binary provenance is ambiguous, and the preferred clean-Windows test asset predates the S7 code now on main. Passing that smoke cannot validate the current release candidate.
- evidence: The snapshot tag resolves to 5934339, while the release body says source of truth is d3544b1. Current main is 8190c08. The checklist still recommends the old release asset even though run #36 produced current-main artifacts.
- reproduce:
    Open the GitHub tags API and note snapshot-2026-09-07 -> 5934339f.
    Open the release body and note its d3544b1 source claim.
    Compare both with current main 8190c085 and Actions run #36.
- expected: The tag, source archive, workflow SHA, checksums, and attached binaries all identify one commit.
- actual: At least three revisions are involved in the current release and smoke instructions.
- fix:
    Cut a new snapshot from exact SHA 8190c085 or its reviewed successor after fixes.
    Attach artifacts from that SHA's green run and record artifact digests and run IDs.
    Create the release tag at the exact build commit and add build provenance/attestation.
    Run the clean-Windows checklist against that new published zip, not the S4-era asset.
- regression: Release gate script that compares tag SHA, workflow head SHA, artifact digest manifest, and VERSION.md.

## GS-009 [High] (Build / CI) — The read-only upstream boundary is already modified and Linux-specific
- path: reference_code/gifsicle/config.h and REFERENCE_MANIFEST.md :: handwritten config.h used by build_engine.sh
- certainty: Confirmed
- impact: The manifest's 'identical to upstream master' claim is false, reproducibility is weakened, and the native build silently assumes 64-bit glibc/Linux values even though the script advertises Linux/macOS.
- evidence: Upstream commit 07f5c4c has no tracked root config.h. This repository adds a handwritten config.h under reference_code with SIZEOF_UNSIGNED_LONG=8, SIZEOF_VOID_P=8, glibc random(), and Linux headers. The product build depends on it despite the rule that reference_code is never edited.
- reproduce:
    Compare reference_code/gifsicle/config.h with upstream commit 07f5c4c3.
    Observe that upstream has no tracked root config.h and the local file says it is handwritten for Linux/gcc.
    Attempt the advertised native build on macOS, 32-bit Linux, or a non-glibc target.
- expected: The upstream tree is immutable and platform configuration is generated in the product build tree.
- actual: A product-specific Linux configuration lives inside and mutates the canonical reference snapshot.
- fix:
    Move product-owned configuration under working_code/gifscythe/build_support.
    Generate config.h per target with configure/CMake feature checks or maintain explicitly named target configs.
    Pin and verify upstream tree hashes in CI; document the one intentional patch series if patches are needed.
    Correct the manifest's identity claim.
- regression: CI diff check against the pinned upstream tree plus native builds on every claimed platform/architecture.

## GS-010 [Medium] (CLI / core) — Unknown CLI arguments print an error but can still exit successfully
- path: working_code/gifscythe/src/cli/main.cpp :: argument loop
- certainty: Confirmed
- impact: Automation can contain a misspelled option, receive an error message, and still treat the command as successful in print mode.
- evidence: The parser writes 'unknown argument' but does not set an error state or return usage status 2.
- reproduce:
    working_code/gifscythe/build/gifscythe-cli working_code/gifscythe/examples/animation.conf --rnu
    echo $?
    The command continues to print the generated line and returns 0 in non-run mode.
- expected: Any unknown or incomplete option returns 2 before loading or executing a settings file.
- actual: The error is non-fatal.
- fix:
    Parse into a typed Options struct.
    Return 2 immediately for unknown flags, missing --engine values, duplicate incompatible arguments, and misplaced operands.
- regression: Table-driven CLI parser tests for every accepted and rejected form.

## GS-011 [Medium] (CLI / core) — Malformed booleans and several numeric states degrade silently
- path: working_code/gifscythe/src/core/SettingsIO.h and Validate.h :: parse_bool(), set_field(), validate()
- certainty: Confirmed
- impact: A typo can turn a feature off with no warning, while non-finite or nonsensical scale/thread/loop values are incompletely validated or simply omitted by the command builder.
- evidence: parse_bool() returns false for every value outside its small true set, so careful=treu is indistinguishable from careful=false. Validation covers only selected ranges and the CLI treats all validation findings as warnings even for --run.
- reproduce:
    printf 'careful = treu\nscale_x = nan\nloopcount = -9\ninput = a.gif\n' > /tmp/bad-state.conf
    working_code/gifscythe/build/gifscythe-cli /tmp/bad-state.conf 2>/tmp/warnings
    Observe no warning for the invalid boolean and incomplete rejection of the invalid states.
- expected: Invalid syntax is distinguished from false, and impossible execution settings block a run.
- actual: Some malformed values silently become false/default or vanish from argv.
- fix:
    Replace parse_bool with optional<bool> and warn on values outside explicit true/false sets.
    Validate finite positive scale, resize dimensions, loopcount, threads, gamma, mode/output requirements, and allowed enum strings.
    Separate errors from warnings; make --run fail on errors.
- regression: Property tests for malformed values, boundaries, NaN/Inf, overflow, and save/load round trips.

## GS-012 [Medium] (GUI) — The 'fully async' GUI still has synchronous waits on the UI thread
- path: working_code/gifscythe/src/qtui/MainWindow.cpp :: waitForStarted(5000), waitForFinished(3000/1000)
- certainty: Confirmed
- impact: A slow process start, network filesystem, security scanner, or stubborn process can freeze the interface for up to several seconds during run, preview cancellation, or user cancellation.
- evidence: runCommand waits up to five seconds after QProcess::start. cancelRun and killPreview also block. The offscreen harness proves steady-state processing is async, not that these waits never stall.
- reproduce:
    Point GS_ENGINE at an executable on a deliberately slow/unavailable network path or a launcher that delays process creation.
    Click Optimize and observe the event loop during the five-second start window.
    Use an engine process that ignores termination and click Cancel to exercise the three-second wait.
- expected: Run and cancel transitions are signal/timer driven and never wait in a GUI callback.
- actual: Several callbacks synchronously wait on QProcess.
- fix:
    Use started, finished, and errorOccurred signals for the entire state machine.
    Use a QTimer for start/cancel deadlines; escalate terminate to kill asynchronously.
    Increment the preview generation when killing so old completions cannot update the panel.
- regression: Event-loop tick tests for delayed start, delayed kill, failed later batch start, and preview cancellation.

## GS-013 [Medium] (GUI) — Drag-and-drop accepts every existing local path, not only GIF files
- path: working_code/gifscythe/src/qtui/MainWindow.cpp :: onFilesDropped()
- certainty: Confirmed
- impact: PNG, text files, and directories can enter a GIF-only queue. The UI labels them as GIF files and enables Run, deferring failure to the engine.
- evidence: The condition is effectively suffix-is-gif OR QFileInfo::exists, so any existing path passes. The audit already listed this as a residual risk but the current status still calls drag/drop done.
- reproduce:
    Create /tmp/not-a-gif.txt.
    Drag it onto the Input queue.
    Observe that it is added and the run control becomes available.
- expected: Only existing regular GIF files are accepted, with rejected-item feedback.
- actual: Existence bypasses the extension check.
- fix:
    Require QFileInfo(f).isFile() and a .gif suffix.
    Read the first six bytes and require GIF87a or GIF89a before queue insertion.
    Report how many dropped paths were rejected and why.
- regression: Real QDropEvent tests for GIF, uppercase GIF, PNG, text, directory, URL, missing path, and duplicate path.

## GS-014 [Medium] (Build / CI) — Green CI does not enforce several claims used as release gates
- path: .github/workflows/build.yml :: Windows deploy, packaging, and test steps
- certainty: Confirmed
- impact: Run #36 is genuinely green, but it can still produce an incomplete deploy and does not exercise the web parity test, negative packaging cases, sanitizers, or lint/static analysis.
- evidence: windeployqt is guarded by 'if command -v', the Linux package check inherits the weak packager, and node web/test/command.test.mjs is absent. Dependencies such as aqtinstall and actions are not pinned to immutable revisions.
- reproduce:
    Review .github/workflows/build.yml and compare it with docs/release/RELEASE_PROCEDURE.md preflight requirements.
    Note that the release procedure requires web parity but the workflow never runs it.
    Note that missing windeployqt does not fail the Windows job.
- expected: Every objective release gate is machine-enforced in CI.
- actual: Some gates are optional, local-only, or documented without enforcement.
- fix:
    Fail unless windeployqt is present and verify staged Qt DLLs/plugins.
    Run web parity/API tests, package negative tests, ShellCheck, compiler warnings-as-errors, and sanitizer jobs.
    Pin actions by commit SHA and aqtinstall by version/hash.
    Upload only validated package roots, not a mixture of raw build directories.
- regression: A CI self-test branch that intentionally removes each required artifact and confirms the job fails.

## GS-015 [Medium] (Build / CI) — CMake mutates the source tree and the secondary qmake path drifts
- path: working_code/gifscythe/CMakeLists.txt and gifscythe.pro :: configure_file() and VERSION = 0.1.0
- certainty: Confirmed
- impact: Read-only or hermetic builds can fail, parallel build trees can race over a committed header, and qmake retains a hardcoded product version despite the single-source version rule.
- evidence: CMake writes generated version.h both to the build tree and back into src/core. Source files include the relative source header, so the generated include is not actually the sole build input. build.sh also tries qmake before its stated preferred CMake path.
- reproduce:
    Make working_code/gifscythe/src read-only and run cmake -S . -B /tmp/gs-build.
    Search gifscythe.pro for VERSION = 0.1.0.
    Run ./build.sh --all where qmake6 is installed and observe that qmake is attempted before CMake.
- expected: Out-of-tree builds leave the checkout untouched and every build system consumes one generated version source.
- actual: Configure writes into source and duplicate build paths can disagree.
- fix:
    Generate version.h only under CMAKE_BINARY_DIR and include core/version.h through generated-first include paths.
    Stop committing the generated fallback or generate it only in the explicit build script.
    Remove qmake if it is not maintained, or derive its version from VERSION.md and test it in CI.
- regression: Read-only-source CMake build plus CI jobs for every officially supported build system.

## GS-016 [Medium] (GUI) — Settings persistence is non-atomic and crosses a narrow-path loader
- path: working_code/gifscythe/src/qtui/MainWindow.cpp :: saveSessionState() and loadSessionState()
- certainty: Strong risk
- impact: A crash, full disk, or interrupted close can truncate the only settings file. On Windows, a non-ASCII config path is converted to std::string before std::ifstream opens it.
- evidence: The save path opens QFile with WriteOnly|Truncate and then writes in place. The load path calls load_settings_file(path.toStdString()). Qt already provides QSaveFile for atomic replacement and Unicode-safe QFile access.
- reproduce:
    Change settings, then interrupt the process while the close handler is writing gifscythe.conf.
    Simulate a short write/full disk and relaunch.
    On Windows, repeat under a profile/config path containing characters outside the active code page.
- expected: The old file survives until a complete new file is flushed and committed; every Unicode path loads.
- actual: The existing file is truncated before the replacement is known to be complete.
- fix:
    Use QSaveFile, check write(), flush/commit(), and preserve the previous file on failure.
    Load with QFile/QTextStream, then pass bytes through load_settings(std::istream&) rather than reopening a narrow path.
    Add a format version and explicit UTF-8 encoding.
- regression: Fault-injection tests for short write, failed commit, corrupt file, and Unicode config locations.

## GS-017 [Medium] (GUI) — Explode mode can claim success without verifying any frame output
- path: working_code/gifscythe/src/qtui/MainWindow.cpp :: onProcessFinished()
- certainty: Confirmed
- impact: The UI can display 'Optimization complete' after exit 0 even if no expected .000/.001 frame artifacts exist.
- evidence: Output existence is checked only when batchMode_ is not Explode. There is no explode-specific enumeration or before/after artifact count.
- reproduce:
    Use Explode mode with a writable-looking prefix whose output is removed or redirected by a test engine that exits 0 without writing.
    Run and observe the completion status despite zero frame files.
- expected: At least one newly-created, non-empty, valid frame is required before success is reported.
- actual: Exit code alone is treated as success for Explode.
- fix:
    Snapshot matching files before start and enumerate new prefix.NNN files after finish.
    Require a non-zero count and validate GIF headers; surface the exact prefix searched on failure.
- regression: Explode E2E for 0, 1, and many frames, stale pre-existing frames, unwritable prefix, and fake exit-0 engine.

## GS-018 [Medium] (Build / CI) — The current regression suite misses the failure classes above
- path: working_code/gifscythe/tests and scripts/*test*.sh :: unit, smoke, and offscreen coverage
- certainty: Confirmed
- impact: The documented 243 GUI checks and green smoke suite create useful confidence, but they do not protect output collisions, PATH-only engine discovery, strict CLI parsing, binary stdout, packaging failures, Unicode paths, or the web server.
- evidence: EngineLocator is not included by the core unit test, the CLI smoke has no unknown-option/PATH/stdout cases, T16 covers only constant templates, and the web test checks command parity rather than the server lifecycle.
- reproduce:
    Run ./scripts/verify_audit.sh and observe green results.
    Then run the GS-001, GS-003, GS-004, or GS-010 reproducer; those behaviors are outside the suite.
- expected: Tests fail when a known high-impact failure is present.
- actual: The suite can stay green while reproducible release blockers remain.
- fix:
    Add focused tests before fixing each finding so the red-to-green transition is visible.
    Split the monolithic custom test main into named tests or at least named executables for clearer CI evidence.
    Add fuzz targets for SettingsIO and the GIF engine boundary, plus ASan/UBSan coverage in CI.
- regression: The new tests are the regression gate; require them on pull requests and release tags.

## GS-019 [Low] (Web POC) — Web engine discovery sorts semantic versions lexicographically
- path: web/server.mjs :: findEngine(): versions.sort().reverse()
- certainty: Confirmed
- impact: Once release/ contains 0.9.0 and 0.10.0, the demo can select the older engine folder unexpectedly.
- evidence: String ordering places '0.9.0' after '0.10.0', so reverse lexicographic order is not semantic-version order.
- reproduce:
    Create release/0.9.0 and release/0.10.0 with engine stubs.
    Start the web server and inspect the logged engine path.
- expected: The current product version or highest semantic version is selected.
- actual: The lexically greatest directory is selected.
- fix:
    Read the version from VERSION.md, or parse numeric semver components before sorting.
    Prefer an explicit GS_ENGINE and fail strictly when it is invalid.
- regression: Unit test ordering 0.9.0, 0.10.0, 1.0.0, prereleases, and non-version directories.

## GS-020 [Low] (Docs / release) — Current-state documents still describe an already-completed CI action
- path: SESSION_HANDOFF.md, WORKLIST.md, docs/ci/README.md :: S7 push/CI status
- certainty: Confirmed
- impact: A maintainer following the ordered worklist may repeat completed work or misunderstand which verification is still open.
- evidence: The documents say the S7 branch must be pushed and CI must be confirmed. It was merged as PR #7, and Actions run #36 completed successfully on both Linux and Windows for main SHA 8190c085.
- reproduce:
    Compare the three documents with GitHub Actions run 34425977060 and commit 8190c085.
- expected: The operational handoff reflects current main and clearly labels historical snapshots.
- actual: The handoff and CI summary stop immediately before the completed merge/run.
- fix:
    Update the worklist to mark S7 merge and run #36 green, including artifact digests and expiry.
    Keep dated reviews immutable but put a prominent 'superseded by' header on historical status sections.
    Generate volatile CI status in one file rather than copying it through several documents.
- regression: Docs check that current main SHA/run metadata is referenced from one canonical status page.
---

## A page — non-finding sections

### Header / decision summary
- "Independent review / 2026-09-10 snapshot"; CI #36: Linux + Windows passed (run 34425977060); "Release hold recommended"
- Risk distribution: Critical 02 / High 07 / Medium 09 / Low 02 = 20 actionable findings; 02 critical blockers; 243 existing GUI checks; 0.1.0 pre-release version
- "Do not cut 1.0.0 yet."
  - Prevent overwrite: Plan and validate every output before the first engine process starts.
  - Harden packaging: Make a partial GUI, CLI, runtime, or license bundle fail closed.
  - Re-cut evidence: Test a current-main Windows zip, not the older S4 snapshot asset.

### 03 / Recommended fix order ("Close the data-loss path before polishing the product.")
1. Stop silent replacement — Plan outputs, reject source/cross-job collisions, and define the overwrite policy. (GS-001)
2. Make packaging fail closed — Fresh staging, required binaries/runtime, complete legal manifest, package E2E. (GS-002, 007)
3. Repair CLI contracts — Strict parser, safe stdout, PATH resolution, Unicode process APIs, validation errors. (GS-003, 004, 006, 010, 011)
4. Harden execution surfaces — Bound web work, remove GUI waits, validate input, atomic settings, verify Explode. (GS-005, 012, 013, 016, 017)
5. Turn fixes into gates — Hermetic builds, immutable upstream, negative packaging tests, sanitizer and web CI. (GS-009, 014, 015, 018)
6. Re-cut release evidence — Current-SHA artifacts, matching tag/provenance, clean Windows smoke, status sync. (GS-008, 019, 020)
- Suggested release criterion: "No Critical/High findings open, package-negative tests green, and clean-Windows smoke run against the exact tagged SHA."

### 04 / Coverage and positives ("What is already working well.")
- "This is not a blanket rejection of the repository. Several previously severe defects were repaired correctly and have meaningful regression coverage."
- Argv execution: The C++ and Node paths spawn with argument arrays rather than shell-concatenated commands. Space-path quoting has dedicated Windows coverage.
- Honest engine exit codes: The CLI's old system() wait-status bug was replaced with platform process APIs, and missing-engine behavior is checked on Windows and Linux.
- GUI process ownership: Main runs use member QProcess state, cancellation is explicit, controls are disabled while busy, and most non-Explode outputs are checked.
- Command parity: The web command builder mirrors the C++ builder and has 12 cross-language fixtures plus direct argv checks, even though CI should enforce them.
- Settings lifetime: GifsicleCommand stores Settings by value, removing the prior dangling-reference defect. Parsing no longer reads uninitialized numeric values.
- Real CI breadth: Current main run #36 completed both OS jobs, native Windows CLI/engine smoke, GUI builds, and the 243-check offscreen harness.
- Inspected: Core/settings/runner, CLI, Qt window and panels, web POC, both tests, six scripts, CMake/qmake, workflow, release metadata, and current-state docs.
- Corroborated: Main SHA, PR #7 merge, Actions run #36 job results, current artifact digests/expiry, release tag SHA, and snapshot asset metadata through GitHub's public API.
- Not re-audited: The vendored million-line upstream gifsicle implementation was treated as a pinned third-party dependency. Its integration boundary was reviewed; upstream internals were not line-audited here.

### 05 / Method and limits
- "I reviewed current main rather than accepting COMPILED_AUDIT.md as proof. Existing audit claims were checked against primary source, current workflow metadata, and release/tag APIs. Findings marked Confirmed follow directly from reachable code paths or metadata. Strong risk items depend on a platform fault or encoding condition and should be reproduced on the target OS."
- "The target C++/Qt repository was not cloned or executed in this environment. The reproductions are source-derived and intended for a disposable checkout or VM. I did verify the public source revision, CI outcomes, artifacts, tags, and release metadata live. The report application itself was compiled after implementation."
- Footer: "20 findings. 2 immediate release blockers."
