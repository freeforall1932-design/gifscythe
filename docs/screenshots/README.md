# UI screenshots (pre-release 0.1.0, session S10 refresh)

Rendered offscreen (`QT_QPA_PLATFORM=offscreen`) on Linux/Qt 6.4.2 (Debian
bookworm) from the real `MainWindow` with `logo.gif` + `logo1.gif` queued,
lossy=30 and colors=128 set in the Actions tab, and the debounced async preview
finished (savings readout visible) — the same scenario as the S7 shots these
replace. Re-shot on 2026-09-11 (S10) because S8 changed `src/qtui/` after the
S7 capture and the old PNGs no longer matched the shipped UI (finding **N-03**,
now closed: re-shot on a Qt machine **and** linked from the root README).

How to re-shoot: drive the real `MainWindow` offscreen (queue via the
`filesDropped` signal, set the Actions controls, wait for the preview savings
readout, then `QWidget::grab()` once per tab). The capture driver used for
these shots is a throwaway dev tool kept OUTSIDE the repo (`~/devtools/capture`
in the S10 sandbox) on purpose — it is a screenshot rig, not product code, and
shipping it would add a fourth Qt binary to every CI matrix. Any Qt machine
can reproduce the shots from the recipe above.

- `shot_input_tab.png` — Input tab: queue with per-file sizes, count/total
  label, **Order: Move Up / Move Down** (S7); Preview pane with Before
  (original) / After (debounced async single-file re-encode) and size-savings
  readout; live one-way command pane (one honest engine command per queued
  file — never a single `-b`).
- `shot_actions_tab.png` — Actions tab: mode + optimize/quantize groups (full
  control surface scrolls).
- `shot_output_tab.png` — Output tab: Save-as, batch folder, **name template**
  `{name}_opt.gif` (S7; `{name}` = input base name) with its two Browse
  buttons (locked during runs since S10, audit U-45), Open-folder, per-mode
  summary of what will be written.

Note: `logo.gif` is a color-cycling animation, so the Before/After movies show
different animation moments in each shot (independent playback — expected,
previews are not frame-synced). The long paths in the command pane are an
artifact of the capture sandbox's temp dirs; on a normal install they are the
user's own paths.
