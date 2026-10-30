# Capturing the desktop UI

The old product screenshots were removed; extracted third-party reference images
under `docs/planning/reference/` are design references, not product screenshots.
Keep generated captures outside Git (the `ui-shots/` directory is ignored).

Build the engine and Qt GUI test harness as described in `INSTALL.md`, then run
on a machine with Qt 6 and the offscreen platform and GIF image plugins:

```bash
cd working_code/gifscythe
./scripts/build_engine.sh
export GS_TEST_REF_DIR="$(./scripts/fixtures.sh)"
export GS_SHOT_DIR="$PWD/ui-shots"
QT_QPA_PLATFORM=offscreen ./build-cmake/test_gui_offscreen
```

The harness activates and settles each tab before `QWidget::grab()`. It captures
the idle window and all tabs, the complete preview frame with and without a
selected input, the full Actions window, and the After preview after clicking
**Apply setting preview**. `01-idle.png` through `08-actions-after.png` are
written under `GS_SHOT_DIR`; the assertions still run as normal. Paths and the
visible animation frame depend on the machine and capture timing.
