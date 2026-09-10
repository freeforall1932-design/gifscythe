# UI screenshots (pre-release 0.1.0, session S7 refresh)

Rendered offscreen (`QT_QPA_PLATFORM=offscreen`) on Linux/Qt 6.4 from the
real `MainWindow` with `logo.gif` + `logo1.gif` queued, lossy=30 and
colors=128 set in the Actions tab, and the debounced async preview finished
(savings readout visible). The S4b-era shots were refreshed on 2026-09-10 to
show the S7 additions (queue reorder buttons, name-template row).

- `shot_input_tab.png` — Input tab: queue with per-file sizes, count/total
  label, **Order: Move Up / Move Down** (S7); Preview pane with Before
  (original) / After (debounced async single-file re-encode) and
  size-savings readout; live one-way command pane (one honest engine command
  per queued file — never a single `-b`).
- `shot_actions_tab.png` — Actions tab: mode + optimize/quantize groups
  (full control surface scrolls).
- `shot_output_tab.png` — Output tab: Save-as, batch folder, **name
  template** `{name}_opt.gif` (S7; `{name}` = input base name), Open-folder,
  per-mode summary of what will be written.

Note: `logo.gif` is a color-cycling animation, so the Before/After movies
in the Input shot show different animation moments (independent playback —
expected, previews are not frame-synced).
