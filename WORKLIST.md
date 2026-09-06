# Worklist

## Current status

- [x] P0 project setup and reference/working separation
- [x] GIF engine build and verification
- [x] Qt-independent command/settings control layer
- [x] CLI driver and unit/integration tests
- [x] Qt6 GUI MVP scaffold and first UX retrofit
- [x] Portable and system-dependent packaging scripts
- [x] Linux GitHub Actions build with Qt6
- [ ] Windows GitHub Actions build (currently failing)
- [ ] Full GUI verification on Qt6/Windows
- [ ] Final GIF UI/UX retrofit and `1.0.0` release

## Next GIF release tasks

1. Diagnose the Windows CI failure and make the Windows engine/CLI build reliable.
2. Build the Qt6 GUI on Windows and deploy it with `windeployqt`.
3. Add an Input / Actions / Output tab flow inspired by XNConvert.
4. Add drag-and-drop, remove/clear queue controls, file size/count display,
   before/after preview, progress, cancel, and output-folder actions.
5. Expose the remaining `GifsicleSettings` controls in the GUI and retain the
   generated raw command for power users.
6. Test a clean portable folder and document the release procedure.
7. Bump to `1.0.0` only after the GIF UI is complete and verified.

## Deferred bucket list — after GIF `1.0.0`

- Add a common RGBA animation frame model with timing, disposal, blend, alpha,
  canvas, and loop metadata.
- Integrate animated WebP using libwebp animation APIs.
- Integrate APNG using libpng/zlib with APNG support.
- Add GIF/APNG/WebP conversion, explode, merge, reorder, and loop controls.
- Add frame editor, text/watermark overlays, presets, and richer previews.

## Build commands

```bash
cd working_code/gifscythe
./build.sh
./build.sh --all
./scripts/package_portable.sh
./scripts/package_system.sh
```
