# Reference Code Manifest

This folder holds **read-only source material** — reference code we adapt, bundle,
or consult. **Never edit or ship these directly.** All edits happen in
`../working_code/`.

## What's here and where it came from

| Folder | What it is | Source / origin | Retrieved |
|---|---|---|---|
| `gifsicle/` | gifsicle 1.96 source (GPL v2-only) — the tree `scripts/build_engine.sh` compiles. **NOT verified identical to upstream**: see the deltas listed below. | Was already in this repo (repo root tree). | Already present |
| `gifsicle-nested-1.96/` | Older **alternate** gifsicle variant (3 files differ: `gifsicle.c`, `gifsicle.h`, `Makefile.w32`). Kept only as an alternate reference. | Was already in this repo (nested `gifsicle-1.96/` tree). | Already present |
| `gifsicle-upstream/` | Shallow clone of upstream gifsicle **master**. Commit `07f5c4c3de1306156e1d8f33e62971d4664c8f7d`. | `https://github.com/kohler/gifsicle.git` | Auto-fetched (GitHub) |
| `caesium-source/` | Caesium **UI** source (GPLv3) — the UI/UX base we adapt. Commit `867c7d5ce6efec599b87cd773fbe659bd5d1263f`. | `https://github.com/Lymphatus/caesium-image-compressor.git` | Auto-fetched (GitHub) |
| `caesium-bin/` | Caesium 2.8.5 **Windows binary bundle** (Qt6 runtime: Qt6*.dll, platforms/, imageformats/ incl. `qgif.dll` + `qwebp.dll`). Used as the reference for the **portable Qt runtime** pattern. | Was already in this repo (bundled `.exe` + DLLs). **Untracked 2026-09-07** (74 MB of third-party binaries; `.gitignore`d) — re-fetch from the Caesium GitHub releases (`Lymphatus/caesium-image-compressor` 2.8.5 Windows bundle) if needed. | Untracked (gitignored) |

## Notes
- **Auto-fetched** items (network worked): `gifsicle-upstream`, `caesium-source`.
- If a future reference **cannot be auto-fetched**, it must be **manually
  uploaded** here; append it to this manifest and mark `Retrieved: manually
  uploaded`.
- **`gifsicle/` is not proven to match upstream master** (audit U-10 — this
  bullet previously claimed it did). `diff -rq` against the nested 1.96 copy
  shows: `src/gifsicle.c`, `src/gifsicle.h` and `src/Makefile.w32` differ, a
  hand-written `config.h` exists only here, and `test/012-framechange.testie` is
  extra. `src/gifsicle.h:346` defines `FRAME_SELECTION_MODE_MASK 0x1F`, used at
  `src/gifsicle.c:432` with `frames_done |= 1 << mode` at `:521`. Whether that is
  upstream master after 1.96 or a local edit is **open** — answering it needs a
  fresh clone of `kohler/gifsicle` at the pinned `07f5c4c3`. Until then treat
  `gifsicle/` as *our build tree*, not as a pristine upstream snapshot.
- `gifsicle-nested-1.96/` is the older alternate variant, kept for comparing
  frame-selection behaviour. It is not the build input.
- The three **Auto-fetched** rows above (`gifsicle-upstream/`, `caesium-source/`,
  `caesium-bin/`) are gitignored and therefore **absent from a fresh checkout**;
  re-fetch them before relying on this manifest.
- The nested `.git` directories of the shallow clones were removed; this folder is
  a plain read-only snapshot, and provenance is documented here instead.
