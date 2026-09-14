# Third-party notices — web/wasm track

## gifsicle (the engine)

- What: the GIF engine this track compiles to WebAssembly, Copyright (C)
  Eddie Kohler.
- Licence: GNU General Public License, Version 2 ONLY.
- Full text: staged into the build output directory as `COPYING.gifsicle`
  by `build_wasm.sh` (every build ships it); the same text lives at the
  repo root as `COPYING.gifsicle`.
- Source: `reference_code/gifsicle/` in this repo, or upstream
  https://github.com/kohler/gifsicle.

## Everything else in this track

The page, the glue script, the build script, and the proof script are
first-party Gifscythe code under the Ms-PL (`LICENSE`,
`COPYING.ms-pl`). No other third-party code ships in this track.

The in-process licence question for this track is tracked in
`docs/legal/WASM_LICENSE_QUESTION.md` and decided by `OD-16`
(`docs/planning/OWNER_DECISIONS.md`).
