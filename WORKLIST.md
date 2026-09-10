# Worklist

**Version:** 0.1.0 · **Audit checklist:** `COMPILED_AUDIT.md`  
**Rule:** Do not mark an item done until `COMPILED_AUDIT.md` §6 has evidence for it.

## Direction decisions (2026-09-09 — see `docs/planning/OFFLINE_BUILD_REVIEW.md`)

- **Offline-only.** No server, no auto-update, no telemetry. The `web/` build
  is a demo / command-parity harness, not the product path.
- **Language: stay C++17 + Qt6 Widgets through 1.0.0** (already offline,
  portable, CI-verified). Revisit only if a documented trigger fires — then
  spike **Rust + Tauri**. Comparison matrix in the offline review doc.

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
- [x] Apply `docs/ci/build.yml.proposed` → `.github/workflows/build.yml` — applied
      by maintainer in `821a310`; S4 hardened both copies (static-link CLI/tests,
      Ninja generator, native Windows E2E smoke, GUI offscreen steps).
      **Push of S4 hardening blocked: provided PAT is invalid (see HANDOFF).**
- [x] Merged compiled audit (S1+S2+S3) → `COMPILED_AUDIT.md`

### Gates before more features
- [x] **Verify fixes** (`COMPILED_AUDIT.md` §6) — executed 2026-09-07 (S4) with
      evidence: §6.A 12/12 + new A13 (Wine), §6.B via 81-check offscreen harness,
      §6.C local items, §6.D local items, §6.E 8/8. One-command rerun:
      `working_code/gifscythe/scripts/verify_audit.sh` (21 PASS / 0 FAIL / 2 SKIP)
- [x] Windows engine + CLI **proven under Wine**: `gifsicle.exe` runs
      (`1.96 (Windows)`), CLI E2E with `C:\` paths + spaces + honest exit 1;
      found & fixed two Windows-only bugs (engine `-I.` recipe, `_spawnvp`
      space-splitting → CreateProcessA + quoting; static-linked exes)
- [x] **Valid push token** — token #3 worked (2026-09-07); PR #5 merged
      into `main` (merge `0ad1ff5`)
- [x] Windows GitHub Actions job **green** with downloadable artifact
      (main run #23: windows ✅ + linux ✅; `gifscythe-windows` 27.8 MB)
- [ ] Clean-machine portable smoke (esp. Windows + `windeployqt`) — from CI
      artifact after C2 (C4/D3/D4)
- [ ] One-time real-desktop GUI probes: B5 (kill engine mid-run), B6 physical
      drag-drop, B14 engine-missing GUI variant

### GIF UI/UX → 1.0.0  (S4b retrofit 2026-09-07 + S7 polish 2026-09-10 — harness T1–T16, 243 checks)
- [x] Input / Actions / Output tab flow (XNConvert feel) — QTabWidget + Preview
      pane in splitter; bottom live pane/progress/status bar kept
- [x] Before/after preview (debounced 1200 ms, fully async, seq-guarded;
      honest captions; savings readout; refuses in Explode mode)
- [x] File size/count display (row sizes + total label), output-folder actions
      (batch folder + Open folder via QDesktopServices)
- [x] Free-form naming templates — **S7**: Output-tab `Name template`, default
      `{name}_opt.gif` renders exactly the historical auto-name (E4 unchanged);
      separators stripped, `.gif` appended, constant-template multi-file
      collision REFUSED with dialog (S3-25, harness T16)
- [x] Expose remaining `GifsicleSettings` controls (~30 widgets, engine-truth
      value lists; harness T11 asserts each control → exact flag)
- [x] Queue reorder (move up/down) — **S7**: list+model stay index-aligned,
      selection follows, merge order = queue order (S3-9, harness T15)
- [ ] Optional: two-way CLI pane (`gs::parse_args`) **or** keep one-way forever
      (UI label now says one-way explicitly) — owner decision
- [x] **Persist GUI settings between sessions** — **S7**: SettingsIO-backed
      load/save (ctor/closeEvent) at `AppConfigLocation/gifscythe.conf`,
      override `GS_SETTINGS_PATH`; GUI keys `batch_dir`/`name_template`;
      queue + Save-as deliberately NOT persisted; warnings surfaced
      (S6 gap / audit row 15, harness T14)
- [x] Document release procedure — **S7**: `docs/release/RELEASE_PROCEDURE.md`
- [ ] Bump `VERSION.md` → **1.0.0** only after Windows CI green + desktop
      probes + the optional items above are decided (owner may take 0.2.0
      first per the minor-bump rule)

## Next actions (ordered)

1. ~~Obtain a valid PAT / push / merge~~ **DONE** — PR #5 merged, main run
   #23 green on both jobs with artifacts (2026-09-07); S5/S6 merged via PR #6.
2. **Push the S7 branch → let CI confirm** (persistence/reorder/templates +
   shim removal on linux + windows; harness now 243 checks).
3. Clean-VM windeployqt smoke from the `gifscythe-windows` artifact (C4/D3/D4)
   — checklist: `docs/ci/CLEAN_WINDOWS_SMOKE.md` (asset banked on Release
   `snapshot-2026-09-07`).
4. One-time real-desktop GUI probes: B5 (kill engine mid-run), B6 physical
   drag-drop, B14 engine-missing GUI variant.
5. Owner decisions: two-way CLI pane **or** keep one-way forever; version
   (0.2.0 for the S7 feature set per the minor-bump rule, vs straight 1.0.0
   once 3+4 are green). Release how-to: `docs/release/RELEASE_PROCEDURE.md`.
6. WebP/APNG stay blocked until all of the above ships.

## Deferred bucket list — after GIF `1.0.0`

- Common RGBA animation frame model (timing, disposal, blend, alpha, canvas, loop).
- Animated WebP (libwebp AnimDecoder/AnimEncoder).
- APNG (libpng/zlib with APNG support).
- GIF ⇄ APNG ⇄ WebP convert, explode, merge, reorder, loop controls.
- Frame editor, text/watermark overlays, presets, richer previews.
- Optional: logging framework, i18n, dark mode, system tray (see COMPILED_AUDIT S3 P2/P3).
- **Web (not the product path):** client-side `gifsicle.wasm` + web UI
  (`docs/web/WEB_FEASIBILITY.md` Option 4); `web/` server demo already exists.
- **Language migration (only if a trigger fires):** Rust + Tauri spike —
  see `docs/planning/OFFLINE_BUILD_REVIEW.md` §4.

## Build commands

```bash
cd working_code/gifscythe
./build.sh                 # engine + CLI + unit tests
./build.sh --all           # also GUI (requires Qt6; fails honestly if missing)
./scripts/test_engine.sh
./scripts/smoke_cli.sh
./scripts/verify_audit.sh  # whole COMPILED_AUDIT §6 suite in one command
./scripts/package_portable.sh
./scripts/package_system.sh
./scripts/build_engine.sh --windows   # needs mingw-w64
# GUI harness (needs Qt6): cmake -S . -B build-cmake && cmake --build build-cmake
#   && QT_QPA_PLATFORM=offscreen ./build-cmake/test_gui_offscreen

# Web demo (offline-unrelated; parity harness only)
node web/server.mjs 8000           # from the repo root
node web/test/command.test.mjs     # JS ⇄ C++ command parity
```

## Do not

- Bump to 1.0.0 as a placeholder.
- Add WebP/APNG before GIF UI is stable.
- “Fix” `--loopcount=0`, `-O0`, crop plus-form, or gamma sentinel (verified correct).
- Link gifsicle into the GUI binary (keep subprocess for GPL v2-only vs GPLv3).
- Edit `reference_code/` (read-only).
