# Worklist

**Version:** 0.1.0 · **Audit checklist:** `COMPILED_AUDIT.md`  
**Rule:** Do not mark an item done until `COMPILED_AUDIT.md` §6 has evidence for it.

## Current status

### Foundation (done)
- [x] P0 project setup and reference/working separation
- [x] GIF engine build and verification (`release/<ver>/gifsicle`, identity 1.96)
- [x] Qt-independent command/settings control layer
- [x] CLI driver + unit tests + integration smoke (`smoke_cli.sh`)
- [x] Qt6 GUI MVP scaffold (Batch default, mode combo, async run, queue, DnD)
- [x] Portable and system-dependent packaging scripts (`.exe` probe + licenses)
- [x] Linux GitHub Actions path with Qt6 + artifacts (recipe present)
- [x] Root LICENSE / COPYING.gifsicle / `.gitignore` / `.gitattributes`

### Review remediation (implemented 2026-09-07 — re-verify via §6)
- [x] **P0 silent-failure fixes** — CLI exit codes, argv exec, CMake INTERFACE,
      GUI batch default, output validation, SettingsIO safety, Windows `win32cfg.h`
- [x] **P1 honesty work** — single version source, EngineLocator, queue UX,
      live pane sync, `save_settings`/`validate`, packaging, `shell_quote`,
      Settings-by-value
- [x] Smoke suite + engine tests scripts; CI recipe proposed in `docs/ci/build.yml.proposed`
- [ ] Apply `docs/ci/build.yml.proposed` → `.github/workflows/build.yml` (needs `workflows` permission on the GitHub App / manual commit)
- [x] Merged compiled audit (S1+S2+S3) → `COMPILED_AUDIT.md`

### Gates before more features
- [ ] **Verify fixes** (`COMPILED_AUDIT.md` §6) — CLI/GUI/CI/packaging + new-pit probes
- [ ] Windows GitHub Actions job **green** with downloadable artifact
- [ ] Full GUI verification on Qt6 (Linux + Windows desktop)
- [ ] Clean-machine portable smoke (esp. Windows + `windeployqt`)

### GIF UI/UX → 1.0.0
- [ ] Input / Actions / Output tab flow (XNConvert feel)
- [ ] Before/after preview (debounced, async — do not re-block UI)
- [ ] File size/count display, output-folder actions, naming templates
- [ ] Expose remaining `GifsicleSettings` controls (core already models most)
- [ ] Optional: two-way CLI pane (`gs::parse_args`) **or** keep one-way forever
- [ ] Document release procedure
- [ ] Bump `VERSION.md` → **1.0.0** only after the above is verified

## Next actions (ordered)

1. Run `COMPILED_AUDIT.md` §6 on Linux (and Windows when available).
2. Confirm rewritten Windows CI (aqtinstall + win32cfg engine + windeployqt).
3. Finish XNConvert-style tabs + preview + remaining controls.
4. Clean portable package on a machine without Qt installed.
5. **Only then** 1.0.0. WebP/APNG stay blocked.

## Deferred bucket list — after GIF `1.0.0`

- Common RGBA animation frame model (timing, disposal, blend, alpha, canvas, loop).
- Animated WebP (libwebp AnimDecoder/AnimEncoder).
- APNG (libpng/zlib with APNG support).
- GIF ⇄ APNG ⇄ WebP convert, explode, merge, reorder, loop controls.
- Frame editor, text/watermark overlays, presets, richer previews.
- Optional: logging framework, i18n, dark mode, system tray (see COMPILED_AUDIT S3 P2/P3).

## Build commands

```bash
cd working_code/gifscythe
./build.sh                 # engine + CLI + unit tests
./build.sh --all           # also GUI (requires Qt6; fails honestly if missing)
./scripts/test_engine.sh
./scripts/smoke_cli.sh
./scripts/package_portable.sh
./scripts/package_system.sh
./scripts/build_gifsicle.sh --windows   # needs mingw-w64
```

## Do not

- Bump to 1.0.0 as a placeholder.
- Add WebP/APNG before GIF UI is stable.
- “Fix” `--loopcount=0`, `-O0`, crop plus-form, or gamma sentinel (verified correct).
- Link gifsicle into the GUI binary (keep subprocess for GPL v2-only vs GPLv3).
- Edit `reference_code/` (read-only).
