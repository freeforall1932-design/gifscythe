# Gifscythe independent source audit

- Repository: [freeforall1932-design/gifscythe](https://github.com/freeforall1932-design/gifscythe)
- Snapshot: [dcb92794ccf81654ae2a34a9bd493dbe970c17b4](https://github.com/freeforall1932-design/gifscythe/commit/dcb92794ccf81654ae2a34a9bd493dbe970c17b4)
- Product version: 0.1.0
- Review date: 2026-09-14
- Latest upstream CI checked: [run 34818891106](https://github.com/freeforall1932-design/gifscythe/actions/runs/34818891106) - success on Linux, Windows, and csharp-spike

## Verdict

The project is still viable and should not be rewritten before 1.0.0. Keep C++17 + Qt6 for the Windows desktop, Node ESM for the self-hosted web surface, and gifsicle as a subprocess. Five source-confirmed gaps below are not represented in the compiled audit. They require targeted regression tests before being marked closed.

## Verification boundary

This is an independent source review of the pinned public repository. The target repository was not cloned or executed inside this audit viewer environment. A finding marked Source confirmed means the control flow is directly present in the source; proposed reproductions still need to be run in the target repository, especially the native-Windows cases. The green upstream CI run proves the existing suite passed, not that these new cases are covered.

## Review coverage

- Fetched the current main Git tree and pinned the review to dcb9279.
- Read README.md, STATUS.md, COMPILED_AUDIT.md, WORKLIST.md, SESSION_HANDOFF.md, PROJECT_VISION.md, and planning/legal context.
- Reviewed first-party C++ core/CLI/Qt paths, Node web server/UI, WASM scaffold, C# spike, tests, build scripts, packaging, and workflow configuration.
- Treated reference_code/gifsicle as imported upstream source; reviewed its provenance and integration boundary, not upstream gifsicle internals line by line.
- Cross-checked the latest upstream GitHub Actions run: Linux, Windows, and the parked C# spike were green for the pinned commit.
- Platform reference: [Microsoft - Naming Files, Paths, and Namespaces](https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file)

## New findings beyond COMPILED_AUDIT.md

### NA-01 - A malformed position pair can still emit a valid-looking -p X,0

- Severity: High
- Class: Broken logic
- Surface: Core / CLI
- Confidence: Source confirmed; runtime reproduction pending
- Source: [SettingsIO.h: set_field() and load_settings()](https://github.com/freeforall1932-design/gifscythe/blob/main/working_code/gifscythe/src/core/SettingsIO.h)

**Problem**

The U-33 fix checks whether both position keys were present, not whether both parsed successfully. One valid coordinate and one invalid coordinate leaves has_position enabled.

**Impact**

The default, non-strict CLI path warns and continues. It can therefore move every frame to an unintended zero coordinate even though the audit says half-specified positions are closed.

**Source evidence**

- set_field() sets has_position = true as soon as either coordinate parses.
- load_settings() records saw_position_x / saw_position_y before parsing the values.
- The final guard only compares key presence. With both keys present it does not clear the partly parsed pair.

**Reproduce**

1. Create a conf with position_x = 12 and position_y = nope, plus a valid input and output.
2. Run gifscythe-cli without --strict and inspect the printed command or engine argv.
3. Expected: position is omitted. Current source path: a warning is emitted, but -p 12,0 remains reachable.

**Proposed solution**

1. Stop mutating has_position inside set_field(). Parse each coordinate into an optional temporary value.
2. After the file is read, enable has_position only when both keys were present and both conversions succeeded.
3. If either conversion fails, clear both coordinates and emit one pair-level warning. Keep --strict behavior unchanged.

**Acceptance tests**

- Unit: valid X + invalid Y produces a warning and no -p argument.
- Unit: invalid X + valid Y produces a warning and no -p argument.
- Smoke: non-strict print mode warns but omits -p; --strict exits 3.

### NA-02 - Changing web settings during a run does not invalidate that run

- Severity: High
- Class: Race / stale state
- Surface: Web UI
- Confidence: Source confirmed; runtime reproduction pending
- Source: [web/app.js: control wiring, requestGen, and run handler](https://github.com/freeforall1932-design/gifscythe/blob/main/web/app.js)

**Problem**

requestGen advances when the file queue changes, but not when any settings control changes. Controls remain editable while fetch('/run') is pending.

**Impact**

A response produced with settings A can be accepted while the controls show settings B. The completion then renders and downloads the old result, creating an attribution error that the existing U-46 queue-only guard does not cover.

**Source evidence**

- onQueueChanged() increments requestGen; control handlers only call updateControlState() and refreshCommand().
- The run handler accepts a response whenever its queue generation still matches.
- The server executes the captured payload correctly; the defect is client ownership and presentation, not server argv execution.

**Reproduce**

1. Use a test engine wrapper that delays completion for two seconds.
2. Start a web run, then change optimization, resize, loop, or delay before the response arrives.
3. Observe that the old response is rendered even though the controls now describe a different run.

**Proposed solution**

1. Create one invalidateRun() path used by queue changes and every settings change.
2. Track an AbortController for the active fetch, increment the generation, and mark or clear existing output when controls change.
3. Capture an immutable settings snapshot at launch. Only that generation may update status, command output, object URLs, or Run state.

**Acceptance tests**

- Browser/DOM test: mutate optimization while a deferred response is pending; stale success changes no output DOM.
- Browser/DOM test: stale failure does not replace the active status.
- Transport fixture: the accepted response command must correspond to the visible settings snapshot.

### NA-03 - Windows output collision keys fold ASCII only

- Severity: High
- Class: Data safety
- Surface: Desktop / Core
- Confidence: Source confirmed; runtime reproduction pending
- Source: [OutputPlan.h: path_key()](https://github.com/freeforall1932-design/gifscythe/blob/main/working_code/gifscythe/src/core/OutputPlan.h)

**Problem**

OutputPlan lowercases a UTF-8 path byte by byte with std::tolower. That does not implement Windows Unicode case-insensitive filename comparison.

**Impact**

Two batch targets that differ only by non-ASCII case can pass preflight and resolve to the same Windows file. The later engine run can replace the earlier result, reopening the data-loss class U-01 intended to close.

**Source evidence**

- path_key() converts the normalized path to UTF-8, then applies std::tolower to each byte under _WIN32.
- UTF-8 bytes for non-ASCII letters are not Unicode code points and remain effectively unchanged.
- The web planner already uses Unicode-aware lowercasing, so desktop and web collision policy are not equivalent.

**Reproduce**

1. On native Windows, place inputs named with U+00C4 and U+00E4 stems in different source folders.
2. Batch both into one destination folder with the default name template.
3. Assert preflight refuses if both derived names address the same case-insensitive target. Current path_key() cannot prove that equivalence.

**Proposed solution**

1. Keep paths as UTF-16 on Windows and compare collision keys with an ordinal, case-insensitive Windows API rather than bytewise std::tolower.
2. For existing paths, additionally compare file identity where handles are available. For not-yet-created targets, use the same UTF-16 case-fold rule for every planned name.
3. Make the comparison policy injectable so Linux unit tests can cover Windows rules, then add one native Windows E2E case.

**Acceptance tests**

- Unit: Windows policy treats U+00C4 and U+00E4 target components as a collision.
- Unit: POSIX policy keeps those names distinct.
- Windows E2E: preflight refuses before either engine process starts.

### NA-04 - Desktop filename sanitization misses Windows superscript device aliases

- Severity: Medium
- Class: Platform mismatch
- Surface: Desktop / Core
- Confidence: Source confirmed; runtime reproduction pending
- Source: [OutputName.h and Microsoft naming rules](https://github.com/freeforall1932-design/gifscythe/blob/main/working_code/gifscythe/src/core/OutputName.h)

**Problem**

The desktop recognizes COM/LPT aliases only when the fourth byte is an ASCII digit. Windows also reserves the superscript 1, 2, and 3 forms; the web admission policy already accounts for them.

**Impact**

A template that renders a superscript COM/LPT device name can survive desktop sanitization and then fail or address a device namespace instead of a normal output file.

**Source evidence**

- is_windows_reserved_device_name() requires stem.size() == 4 and std::isdigit(stem[3]).
- The superscript characters are multi-byte in UTF-8, so none can satisfy that branch.
- Microsoft's current file naming documentation explicitly lists the superscript aliases as reserved in every directory.

**Reproduce**

1. Call sanitize_output_name() with Windows rules and a COM/LPT stem ending in U+00B9, U+00B2, or U+00B3.
2. Expected: the name is defused like COM1.gif. Current source leaves it unchanged.
3. Confirm behavior on native Windows before closing the finding.

**Proposed solution**

1. Extend the portable reserved-name matcher to recognize the exact UTF-8 sequences for superscript 1, 2, and 3, or compare decoded Unicode scalars.
2. Share one test table between desktop naming tests and web upload-name tests to prevent another policy split.

**Acceptance tests**

- Unit: all six COM/LPT superscript aliases, with and without extensions, are defused.
- Unit: similar ordinary names remain untouched.
- Windows smoke: creating the sanitized output writes a regular file.

### NA-05 - The reusable WASM module can accept a stale /out.gif as new output

- Severity: Medium
- Class: Broken logic
- Surface: WASM
- Confidence: Source confirmed; runtime reproduction pending
- Source: [web/wasm/wasm.js and build_wasm.sh](https://github.com/freeforall1932-design/gifscythe/blob/main/web/wasm/wasm.js)

**Problem**

The WASM UI intentionally reuses one Emscripten module across runs, but it does not unlink or snapshot /out.gif before callMain().

**Impact**

After one successful run, a later exit-zero/no-write execution can read the previous GIF and report success for the wrong input. The missing-output check only works on the first run of a page session.

**Source evidence**

- ensureEngine() creates a singleton module and build_wasm.sh keeps the runtime alive with EXIT_RUNTIME=0.
- run() overwrites /in.gif, calls the engine, then reads /out.gif without removing its previous value.
- The glue harness tests one successful run and one validation refusal, but not two engine invocations with a no-write second call.

**Reproduce**

1. Run once with a fake module that writes a valid /out.gif.
2. Run a second time with callMain returning success but not writing /out.gif.
3. Current source reads the first output and reaches done; expected behavior is a missing-output refusal.

**Proposed solution**

1. Unlink /out.gif immediately before every callMain(), ignoring only ENOENT.
2. Read and verify output only after a zero exit, then remove temporary virtual files in a finally block.
3. Retain the current signature check. Do not mark the WASM path shippable until OD-16 and the real emcc byte proof are also closed.

**Acceptance tests**

- Glue harness: success followed by exit-zero/no-write must refuse the second run.
- Glue harness: two genuine writes return the second run's bytes, not the first.
- Real WASM proof: repeat two inputs in one module instance when emcc is available.

## Already tracked release debt

### GS-203 - PARTIAL

GUI still lacks the shared ordinary-output postcondition verifier.

Next: Integrate OutputVerify into single, merge, and batch QProcess lifecycles.

### U-06 - PARTIAL

The web server remains single-user with no concurrency or rate cap.

Next: Add bounded admission, per-client limits, and tested timeout cleanup.

### GS-205 / GS-206 - OPEN

Desktop input admission and numeric/domain validation remain incomplete.

Next: Centralize GIF admission; parse into destination widths; validate all enums and sentinels.

### U-12 - OPEN

Five GUI waits can still block the UI thread.

Next: Replace synchronous waits with a tested QProcess state machine.

### GS-204 / U-09 - PARTIAL / OPEN

Packaging still needs architecture checks, clean-Windows proof, and a release re-cut.

Next: Prove the exact artifact on a clean VM, then publish from one reviewed tagged SHA.

## Recommended delivery path

### 01. Close the new honesty gaps

Target: 1-2 focused PRs

Fix NA-01 and NA-02 first, then land NA-03 and NA-04 behind platform-specific tests. Patch NA-05 before any further WASM work.

### 02. Finish known product correctness

Target: Before release work

Complete GS-203 in Qt, centralize GIF input admission, finish numeric and enum domains, and bound the self-hosted server. Keep changes contract-first across C++ and JS.

### 03. Prove the Windows artifact

Target: Release candidate

Add binary architecture inspection, run the clean-Windows checklist and real desktop probes, apply the one-line workflow drift, then re-cut from an exact tagged SHA.

### 04. Ship a truthful pre-1.0 build

Target: After gates are green

Prefer 0.2.0 if a public checkpoint is needed. Reserve 1.0.0 for the documented clean-machine, UI, and no-open-High bar. APNG and WebP stay after GIF 1.0.

## Language and architecture recommendation

- Desktop: stay on C++17 + Qt6 through 1.0.0. A rewrite would discard the offscreen harness, Windows packaging, and proven subprocess behavior.
- Web: keep zero-dependency Node ESM and browser JavaScript for now. Do not add a framework or TypeScript build step solely for this remediation pass.
- Shared semantics: reduce drift by centralizing fixtures and contract tests now; revisit routing the server through gifscythe-cli only after the owner makes OD-05 explicit.
- WASM: keep experimental and unshipped until NA-05, a real emcc proof, and the OD-16 license decision are closed.
- Future rewrite trigger: only spike Rust/Tauri after 1.0.0 if a measured bundle-size or web-UI requirement justifies it.

## Suggested validation order

```bash
cd working_code/gifscythe
./build.sh --all
./scripts/smoke_cli.sh
QT_QPA_PLATFORM=offscreen ./build-cmake/test_gui_offscreen
cd ../..
node web/test/command.test.mjs
node web/test/validate.test.mjs
node web/test/transport.test.mjs
node web/wasm/glue_harness.mjs
working_code/gifscythe/scripts/verify_audit.sh
```

Run the new focused regressions before the broad suite so a failure identifies the patch being tested. Native Windows proof remains mandatory for NA-03 and NA-04.