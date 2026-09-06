# Gifscythe

**Version:** 0.1.0 (see `VERSION.md`)

Gifscythe is the *actual working product* — a portable, click-and-use desktop app
for **animated (moving) images only**: GIF, APNG, and WebP. It is built on top of
the gifsicle engine with an XNConvert-feel GUI (UI/UX base taken from Caesium),
while retaining the full gifsicle terminal control underneath.

This folder is the **working source code**. It is deliberately separated from
reference material (see the repo root `reference_code/`).

## Status
- **0.1.0** — engine + control layer done. UI/UX retrofit (Qt GUI) pending.
- Engine: native `gifsicle` built at `release/0.1.0/gifsicle` (P0 complete).
- Control layer built & verified (P1 engine side complete). Qt GUI blocked
  locally (no Qt in sandbox) — end-to-end pipeline verified via CLI driver.

## Build and test
```bash
# One command: build engine + CLI, run tests. Add --all/--gui to build Qt GUI.
./build.sh

# Or individually:
./scripts/build_gifsicle.sh          # engine (native) -> release/<version>/gifsicle
./scripts/build_gifsicle.sh --windows # Windows gifsicle.exe (needs mingw-w64)
./scripts/test_engine.sh             # verify engine (info/optimize/lossy/resize/explode)
./scripts/package_portable.sh         # create release/<version>/Gifscythe
```

`package_portable.sh` assembles a portable folder containing the engine, CLI,
and documentation. After building the Qt GUI on Windows, use `windeployqt` to
place the Qt runtime DLLs beside the GUI executable; no installer is required.

## Layout
```
working_code/gifscythe/
  src/
    core/   Qt-independent engine control layer:
              GifsicleSettings.h  (all UI controls -> gifsicle settings)
              GifsicleCommand.h   (builds argv + live CLI string)
              SettingsIO.h        (load/save settings as key=value)
    cli/    main.cpp -> gifscythe-cli (prints or runs the generated command)
  assets/     bundled app assets (icons, fonts, etc.) — app-owned
  resources/  Qt resource files, styles, lists
  scripts/    build_gifsicle.sh, test_engine.sh
  release/    portable output per version (e.g. release/0.1.0/)
  tests/      test_gifsicle_command.cpp (unit tests)
  examples/   animation.conf (sample settings file)
  VERSION.md  version scheme + current version
```

## The engine control layer (this is the heart of the "terminal control" feature)
- A UI widget sets a field on `gs::Settings`.
- `gs::GifsicleCommand(s)` converts it into the exact gifsicle argv and the
  human-readable CLI string shown in the **live "show me the command" pane**.
- `gifscythe-cli` is a headless proof: it loads a settings file, prints the
  command, and `--run`s it against the engine.

```bash
# Print the command the app would run:
./build/gifscythe-cli examples/animation.conf
# ...or run it against the engine:
./build/gifscythe-cli examples/animation.conf --run
```

## Versioning
0.1.0 → 1.0.0–1.9.9 (finished GIF product) → 2.0.0–3.0.0 (WebP + APNG). See
`VERSION.md` for the full rules. **Do not call it 1.0.0 until the UI/UX task is
done.**
