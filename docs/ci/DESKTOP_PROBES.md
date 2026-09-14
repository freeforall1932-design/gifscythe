# Real-desktop GUI probes — W-19 (B5/B6/B14-adjacent)

**Purpose:** the three GUI behaviors the offscreen harness
(`tests/test_gui_offscreen.cpp`) cannot execute — they need a physical
desktop with a real window manager. Status: **OPEN** (nothing here has been
executed yet). Run on the same build the clean-Windows smoke uses
(`docs/ci/CLEAN_WINDOWS_SMOKE.md`), after its GUI double-click step passes.

**Why offscreen cannot do these:** the harness drives widgets in-process
(Qt offscreen platform) — it can emit the drop *signal* and it can cancel
or close around a run, but it cannot receive an OS-level file drop from
Explorer, cannot have a *third party* kill the engine mid-run, and never
shows a real engine-missing dialog to a user. Each probe below names the
harness coverage it complements so a green probe is not mistaken for a
duplicate.

## Probe 1 — kill the engine mid-run (external kill, not Cancel)

Harness complement: T8 (a failing engine is reported honestly), T9 (Cancel
mid-run sets `Cancelled.`), T10 (closing the window kills the engine).
None of them kills the engine *externally* while a run is in flight: an
external kill arrives with no `cancelling_` flag set, so it must take the
failure branch of `onProcessFinished`, not the cancel branch.

1. Queue a large GIF (or several), pick Optimize, start the run.
2. While the progress bar is visible, kill `gifsicle.exe` from Task
   Manager (Details tab) — do NOT press Cancel and do NOT close the window.
3. Expect, with no hang and no success claim:
   - status `Optimization failed (exit …).` (`src/qtui/MainWindow.cpp`,
     `onProcessFinished` failure branch);
   - a warning dialog carrying the engine's stderr (or the exit-code text
     when stderr is empty);
   - Run re-enabled, Cancel hidden, queue intact — a second Run works.
4. Failure modes that fail the probe: a dialog claiming completion, a
   frozen window, or a run that can never start again without relaunch.

## Probe 2 — physical drag-and-drop from Explorer

Harness complement: T3 (the `filesDropped` signal appends, dedupes, and
rejects `.txt`). T3 emits the signal directly; it cannot prove the OS
delivers a real drop to `src/qtui/DropListWidget.h`.

1. Drag two real `.gif` files from Explorer onto the queue → both
   appended (B5 shape), queue count 0→2.
2. Drag the same two again plus one new GIF → only the new one appended,
   duplicates ignored (B6 shape; `appendInputs` skips `inputs_`
   members).
3. Drag a `.txt` file → NOT appended (B15 shape: `onFilesDropped` keeps
   only existing `*.gif`). Known gap, not a probe failure: the rejection
   is currently silent (no feedback line) — that feedback is tracked as
   `GS-205` / fix-order P1-27, and this probe does not waive it.
4. Failure modes that fail the probe: a drop that appends nothing, a
   duplicate that creates a second queue row, or a `.txt` that lands in
   the queue.

## Probe 3 — engine-missing GUI (launch, run, and recovery)

Harness complement: T1 (status names the engine path and Run is disabled
on an empty queue when the engine IS present), T18 (Run re-enables
through `ensureEngine()`). No harness case removes the engine binary.

1. Rename `gifsicle.exe` beside `gifscythe.exe` to `gifsicle.exe.hidden`
   and launch the GUI → status `Engine not found — build with
   ./scripts/build_engine.sh` (`MainWindow::ensureEngine`), Run disabled.
2. Add a GIF and press Run anyway → critical dialog `Could not find the
   GIF engine (gifsicle).` — no process starts, no output is written.
3. Rename the engine back while the GUI is still open, then add another
   file (or finish any run) → `ensureEngine()` re-probes, status shows
   the engine path again, Run re-enables.
4. Failure modes that fail the probe: a crash or a silent no-op at step
   2, an output file written with no engine, or a GUI that needs a
   relaunch to notice the restored engine.

## Record evidence

Note the OS build, the app commit, and for each probe the observed
status text + dialog text (screenshot or transcription). Then tick W-19
in `STATUS.md` (via the normal register flow) with an `IMPROVEMENT_LOG.md`
line. A probe that fails stays **open** — it does not waive, and it does
not become a harness case by itself (the harness cannot run it).
