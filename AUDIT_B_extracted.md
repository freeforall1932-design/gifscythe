# AUDIT B (Seed 2.1 Pro Preview) — Gifscythe Code Audit Report — full findings text


## BUG-01 [high] (C++ core/Logic bug) verified=True — Threads 'Auto' setting does not enable auto-threading
- file: src/core/GifsicleCommand.h:210
- summary: The GUI labels threads=0 as 'Auto' (implying auto-detect CPU count with bare `-j`), but the command builder only emits `-jN` when threads > 0. threads=0 emits NO flag, so gifsicle runs single-threaded (its default). The Settings.h comment even says `-j; <=0 = auto`, but the build logic doesn't implement 0 as bare `-j`.
- reproduction: Run CLI with `threads = 0` in conf or set GUI Threads to 'Auto' (0). Observe generated command lacks `-j`. Gifsicle runs with 1 thread, not auto-detected count.
- current: `if (s.threads > 0) { add(args_, "-j" + i2s(s.threads)); }`  → threads=0 adds nothing.
- expected: threads=-1: no flag (gifsicle default, 1 thread). threads=0: bare `-j` (auto-detect online processors). threads>=1: `-jN`.
- fix: Replace line 210 with:
```cpp
if (s.threads == 0) {
  add(args_, "-j");
} else if (s.threads > 0) {
  add(args_, "-j" + i2s(s.threads));
}
```
Update Validate.h to allow threads == 0. Ensure CLI/GUI default of -1 stays 'no flag' and GUI 0 maps to bare `-j`.

## BUG-02 [medium] (Qt GUI/Logic bug) verified=True — Drag-and-drop filter accepts ANY existing file, not just GIFs
- file: src/qtui/MainWindow.cpp:426
- summary: onFilesDropped uses OR: `endsWith(".gif") || QFileInfo::exists(f)`. Any existing file (e.g. .exe, .txt, .jpg) passes the filter and is added to the queue. The user only discovers the problem at run-time when gifsicle fails.
- reproduction: Drag a file that exists on disk but is not a .gif (e.g. a README.txt) onto the queue. It is added. Click 'Optimize GIF' → gifsicle errors on non-GIF input.
- current: `if (f.endsWith(".gif", Qt::CaseInsensitive) || QFileInfo::exists(f)) gifs << f;`
- expected: Only files that are GIFs should be added. Condition should require BOTH: extension is .gif AND file exists (and perhaps also sniff the header for non-.gif extensions).
- fix: Change `||` to `&&`: `if (f.endsWith(".gif", Qt::CaseInsensitive) && QFileInfo::exists(f)) gifs << f;`. Also consider adding a failed-to-add feedback for non-GIF drops.

## BUG-03 [low] (Qt GUI/Code quality) verified= — runCommand: missing `return` after waitForStarted failure on non-batch path
- file: src/qtui/MainWindow.cpp:804-808
- summary: When Merge/Auto/Explode engine fails to start, the code shows a critical message box but does NOT `return;` afterwards (unlike the Batch path which does return at line 773). Function falls through. Today this is harmless because no code follows the if-block, but any future code added after line 808 would run in the error path.
- reproduction: Code inspection only. Trigger waitForStarted failure (e.g. rename engine binary after launch), run Merge mode.
- current: After `QMessageBox::critical(...)` there's no return; closing brace ends the function.
- expected: Consistent early return like the batch branch.
- fix: Add `return;` after the QMessageBox::critical block (line ~808), mirroring line 772.

## BUG-04 [medium] (Web UI/Misalignment) verified=True — Web UI defaults 'Scale %' to 50, desktop defaults to 100
- file: web/index.html:81
- summary: Desktop scale spinboxes default to 100 % (= no scale factor change). Web scalePct defaults to 50 (= half size). A user who selects 'Scale %' expects 'no change' (100 %) as the default, not 50 %.
- reproduction: Open web UI, select resize 'Scale %'. The scale field shows 50, which would halve the image on run.
- current: `<input type="number" id="scalePct" min="1" max="1000" value="50" step="0.1" />`. Desktop SettingsPanel.cpp line 183: `scaleXSpin_->setValue(100.0);`
- expected: Default 100 (no-op scale) to match desktop.
- fix: Change value from 50 to 100 in web/index.html line 81.

## BUG-05 [low] (Web UI/Missing feature) verified= — Web missing 'Touch' resize option in HTML dropdown
- file: web/index.html:70-77
- summary: Desktop Resize has 6 modes (None, Fit, Touch, Exact, Scale, Width, Height). command.mjs implements the 'touch' case correctly (line 91), but the web <select> omits the option, so users can't select it from the UI.
- reproduction: Inspect resize select in web/index.html — no 'touch' option.
- current: Options: none, fit, exact, width, height, scale. 'touch' is supported by command.mjs but not exposed.
- expected: Include Touch option for parity.
- fix: Add `<option value="touch">Touch W×H (resize to fit within WxH if larger)</option>` after the 'Fit' option.

## BUG-06 [medium] (Web server/Logic bug) verified= — Web server doesn't verify that out.gif exists / is non-empty after engine exits 0
- file: web/server.mjs:169
- summary: The desktop GUI explicitly verifies after exit code 0 that the output file exists and is non-empty before declaring success (MainWindow.cpp lines 850-862). The Node server skips this check and goes straight to `readFile(outFile)`. If gifsicle exits 0 but writes no file (or zero bytes), the server throws ENOENT which becomes a generic HTTP 500 rather than a helpful 422.
- reproduction: Craft settings that produce no output (e.g. info mode or gifsicle that exits 0 without writing). Server stack-traces to 500.
- current: After `result.code === 0`: immediately `const outBytes = await readFile(outFile);` with no existence/size check.
- expected: Check `existsSync(outFile)` and size before reading; return structured 422 if missing/empty.
- fix: Insert after result.code check: `if (result.code !== 0) { … existing 422 … } const st = await stat(outFile).catch(() => null); if (!st || st.size === 0) { res.writeHead(422, …); res.end(JSON.stringify({ok:false, error:'engine produced no output', …})); return; }`

## BUG-07 [low] (Web server/Missing feature) verified= — Web server has no Validate-equivalent; out-of-range values passed straight to engine
- file: web/server.mjs:123-166
- summary: The desktop GUI runs gs::validate() and refuses to run with out-of-range values (colors, lossy, disposal, delay, empty inputs, crop w/h of 0, etc.). The web server spreads user settings directly into buildArgs, relying on buildArgs' own range checks and the engine to reject. Most invalid flags are silently dropped by buildArgs (e.g. color_count=999 fails >=2 && <=256 and is skipped) but some (e.g. malformed dither method strings) are passed to gifsicle which returns 422 via its stderr. This is acceptable but gives inconsistent UX vs. desktop.
- reproduction: POST /optimize with `{"color_count": 999}` — flag is silently dropped. No warning returned to user.
- current: No validation layer server-side.
- expected: Either port Validate.h to JS or document that engine's stderr is the source of truth.
- fix: Add a JS equivalent of the Validate.h checks in server.mjs and return 422 with a user-friendly list of issues before invoking the engine. This provides parity with the GUI's pre-flight dialog.

## BUG-08 [medium] (Qt GUI/Logic bug) verified= — Explode mode never verifies that frame files were produced
- file: src/qtui/MainWindow.cpp:850-862
- summary: Non-explode modes verify the output file exists and is non-empty (pessimal check against silent engine failure). Explode mode is explicitly skipped (`batchMode_ != gs::Mode::Explode`). A failed explode (gifsicle exits 0 but writes zero frames — e.g. wrong permissions in target dir) is reported as 'Optimization complete.' even though no frames were written.
- reproduction: Run Explode into a directory where you have no write permission (but the output prefix points there). Gifsicle may exit non-zero (caught), but a scenario where exit=0 but no frames exist (empty file set) is not verified.
- current: Only checks `pendingOutput_` for non-explode. Explode writes `<prefix>.000`, `<prefix>.001`, etc.
- expected: For explode, verify that at least one frame file matching `<prefix>.*` exists.
- fix: After an Explode run completes with exit 0, QDir the parent and look for files starting with the prefix+'.' — fail with the same 'No output produced' dialog if none exist.

## BUG-09 [info] (Web UI/Missing feature) verified= — Web POC only supports single-file Auto mode; no batch/merge/explode
- file: web/app.js:17
- summary: The web UI hardcodes mode:'auto' and only supports single-file upload. Multi-file batch, merge, and explode modes that exist in the desktop app are not present in the web POC. This is called out as a POC but is a major feature gap for a web port.
- reproduction: web/app.js settings() returns `mode: 'auto'` with no mode selector in HTML.
- current: Single-file optimization only.
- expected: At minimum document the limitation clearly, or add a mode selector.
- fix: Add a <select id="mode"> to the HTML and read it in settings(). Batch mode requires multiple file uploads; Merge needs an output filename. Document as roadmap.

## BUG-10 [info] (Web UI/Misalignment) verified= — Web uses a single scalePct for both axes; desktop has separate X and Y
- file: web/app.js:27-28
- summary: Desktop exposes independent X and Y scale percentages (allowing anamorphic scaling). The web UI links both axes to one slider. Acceptable for POC but a parity gap.
- reproduction: Only one scalePct input in web/index.html line 81.
- current: Uniform scaling only.
- expected: Two inputs for non-uniform scaling.
- fix: Add scaleXPct and scaleYPct inputs; default both to 100.

## BUG-11 [low] (Build/Portability) verified= — CLI compile via build.sh may fail on older g++ (no -lstdc++fs link)
- file: working_code/gifscythe/build.sh:58-60
- summary: EngineLocator.h uses std::filesystem which on g++ < 9 requires linking with `-lstdc++fs`. build.sh does not add this link flag. On Debian 12 / g++ 12 it works, but older distros (Ubuntu 18.04 / g++ 7) will fail at link time with undefined references to std::filesystem symbols.
- reproduction: Build on a system with g++ 8 or earlier.
- current: No `-lstdc++fs` in link line.
- expected: Portable link.
- fix: Add a small autodetect in build.sh (try linking with and without -lstdc++fs). CMake handles this via C++17 standard + proper compiler detection; the ad-hoc g++ line does not.

## BUG-12 [low] (C++ core/Logic bug) verified= — Posix ProcessRunner returns 1 on signal/kill instead of 128+signum convention
- file: src/core/ProcessRunner.h:117-120
- summary: If child is killed by a signal, the POSIX branch returns generic 1 (printing a message). Unix convention (and what bash returns) is 128 + signal number. This makes it impossible for callers to distinguish crashes from user-requested kills (though the GUI uses cancelling_ flag).
- reproduction: Send SIGSEGV to child process.
- current: `return 1;` after WIFSIGNALED.
- expected: Return 128 + WTERMSIG(status).
- fix: Replace `return 1;` with `return 128 + WTERMSIG(status);`. Update Windows branch for consistency if desired.

## BUG-13 [low] (C++ core/Logic bug) verified= — SettingsIO: setting only position_x OR position_y auto-enables has_position
- file: src/core/SettingsIO.h:105-110
- summary: When loading a conf, setting position_x alone (or position_y alone) sets has_position = true, leaving the other coordinate at default 0. This may produce an unexpected -p X,0 flag. It's a corner case because valid configs always write both coordinates together (as save_settings does), but a hand-written partial config will produce a partial position.
- reproduction: Write a config containing `position_x = 100` but no position_y. Loading sets has_position=true, position_y=0.
- current: Each key independently sets has_position = true.
- expected: Position should only be enabled when BOTH coordinates are provided, or neither. Otherwise it should warn and not set has_position.
- fix: Change loader so setting position_x or position_y doesn't immediately flip has_position; instead, after parsing all fields, set has_position = (position_x_was_set && position_y_was_set). Alternatively warn if exactly one was provided.

## BUG-14 [low] (Qt GUI/Logic bug) verified= — Temp preview files older than seq-1 can accumulate during long sessions
- file: src/qtui/MainWindow.cpp:992-993
- summary: The completion lambda only removes preview file `seq - 1`. If a preview is killed before its lambda fires (stale preview, killed by a newer preview), the killed preview's partial output file is never removed. Over a long session with many rapid setting changes this leaks preview_1.gif … preview_{n-2}.gif in the temp directory. The whole directory is removed at app exit, so it's only a per-session disk leak.
- reproduction: Change settings rapidly for several minutes, ls the temp preview dir: multiple preview_*.gif exist.
- current: Only the immediately preceding seq file is removed on completion of seq. Killed/stale previews don't clean up.
- expected: All old preview files should be cleaned up when a new run starts, regardless of whether the previous run completed.
- fix: At the top of startPreview(), before launching a new process, remove all preview_*.gif in previewDir_ instead of just the predecessor. Or track all created preview paths in a set and clean up in killPreview or when a new one starts.

## BUG-15 [info] (C++ core/Misalignment) verified= — CLI prints warnings but still executes --run even with blocking issues
- file: src/cli/main.cpp:131-174
- summary: The GUI refuses to run when validate() returns non-empty warnings (after removing the 'empty inputs' item). The CLI prints the warnings to stderr but proceeds with run anyway. This is an intentional design difference (CLI is scriptable, warnings are advisory), but it means the CLI can run with `colors=999` or `crop=0x0` etc. without blocking, which may surprise users coming from the GUI.
- reproduction: Run `gifscythe-cli bad.conf --run`. Warnings are printed but execution continues.
- current: Non-zero exit code only from engine, not from validate warnings.
- expected: Add a `--strict` flag (or document the behavior).
- fix: Document the behavior in README. Optionally add --strict that treats warnings as fatal.

## BUG-16 [low] (Qt GUI/Logic bug) verified= — GUI setBusy(false) re-enables Run button without re-checking engine existence
- file: src/qtui/MainWindow.cpp:667-684
- summary: setBusy(true) stops the preview and disables controls; setBusy(false) re-enables the Run button with `!busy && !inputs_.isEmpty()`. It does not call ensureEngine() like appendInputs does. If the engine binary is deleted while a run is in progress (unusual), Run is re-enabled and clicking it will fail at runCommand's ensureEngine() check, but the button is misleadingly enabled.
- reproduction: Engine file deleted mid-run; after run ends Run is enabled despite engine missing.
- current: No engine check when clearing busy state.
- expected: Run button enabled only when engine is present AND queue non-empty.
- fix: After setBusy(false) call, use the same pattern as appendInputs: `runButton_->setEnabled(!busy && !inputs_.isEmpty() && ensureEngine());` (ensureEngine re-probes and updates status).
---

## B page — "What's working well" section (non-finding text)

- Core compiles cleanly: g++ -std=c++17 -Wall -Wextra -pedantic produces zero warnings on the CLI + test target. All 20 unit tests pass.
- shell / argv discipline: Both POSIX fork+execvp and Windows CreateProcessA paths use argv arrays (never shell). Windows quoting for MSVCRT is implemented correctly and tested.
- Batch per-file semantics: N inputs -> N outputs with {name} template; constant-template collision refused; single-file Save-as honored.
- --crop X,Y+WxH matches gifsicle's preferred syntax; not the comma-separated form known to trip people up.
- Delay units labeled 1/100 s: Not milliseconds - avoids the off-by-10 error common in GIF tools.
- Session persistence: Saves only Actions state (not queue, not Save-as); corrupt file produces honest status warning instead of silent defaults.
- Preview: Debounced, async, uses seq# to discard stale completions; temp dir cleaned on exit.
- Parity test scaffolding: web/test/command.test.mjs cross-checks JS command.mjs output against the C++ gifscythe-cli binary, ensuring the web and desktop build identical argv.
- Settings round-trips: save_settings/load_settings tested; unknown keys ignored for forward-compat (allows GUI-specific keys in the same file).
- MinGW _spawnvp was splitting on spaces; custom CreateProcessA + MSVCRT quoting added (regression test in unit test #19).

## B page — Method note (tail)

"Audit performed by static analysis + compile + unit-test execution. Core engine binary (gifsicle) was not built in this environment; tests involving actual GIF processing require running [build_engine.sh]. [Some Verified-marked bugs] were reproduced with a throwaway test driver."

Severity tally on page: Critical 0 / High 1 / Medium 4 / Low 8 (+ 3 info) = 16 findings.
