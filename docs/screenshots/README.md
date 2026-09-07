# UI screenshots (pre-release 0.1.0, session S4b)

Rendered offscreen (`QT_QPA_PLATFORM=offscreen`) on Linux/Qt 6.4.2 from the
real `MainWindow` with `logo.gif` + `logo1.gif` queued, lossy=30 and
colors=128 set in the Actions tab.

- `shot_input_tab.png` — Input tab: queue with per-file sizes, count/total
  label; Preview pane with Before (original) / After (debounced async
  single-file re-encode) and size-savings readout; live one-way command pane.
- `shot_actions_tab.png` — Actions tab: mode + optimize/quantize groups
  (full control surface scrolls).
- `shot_output_tab.png` — Output tab: Save-as, batch folder, Open-folder,
  per-mode summary.

Note: `logo.gif` is a color-cycling animation, so the Before/After movies
in the Input shot show different animation moments (independent playback —
expected, previews are not frame-synced).
