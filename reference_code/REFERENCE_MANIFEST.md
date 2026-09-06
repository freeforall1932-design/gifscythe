# Reference Code Manifest

This folder holds **read-only source material** — reference code we adapt, bundle,
or consult. **Never edit or ship these directly.** All edits happen in
`../working_code/`.

## What's here and where it came from

| Folder | What it is | Source / origin | Retrieved |
|---|---|---|---|
| `gifsicle/` | Canonical gifsicle 1.96 source (GPL v2-only). **Identical to upstream master.** | Was already in this repo (repo root tree). | Already present |
| `gifsicle-nested-1.96/` | Older **alternate** gifsicle variant (3 files differ: `gifsicle.c`, `gifsicle.h`, `Makefile.w32`). Kept only as an alternate reference. | Was already in this repo (nested `gifsicle-1.96/` tree). | Already present |
| `gifsicle-upstream/` | Shallow clone of upstream gifsicle **master**. Commit `07f5c4c3de1306156e1d8f33e62971d4664c8f7d`. | `https://github.com/kohler/gifsicle.git` | Auto-fetched (GitHub) |
| `caesium-source/` | Caesium **UI** source (GPLv3) — the UI/UX base we adapt. Commit `867c7d5ce6efec599b87cd773fbe659bd5d1263f`. | `https://github.com/Lymphatus/caesium-image-compressor.git` | Auto-fetched (GitHub) |
| `caesium-bin/` | Caesium 2.8.5 **Windows binary bundle** (Qt6 runtime: Qt6*.dll, platforms/, imageformats/ incl. `qgif.dll` + `qwebp.dll`). Used as the reference for the **portable Qt runtime** pattern. | Was already in this repo (bundled `.exe` + DLLs). | Already present |

## Notes
- **Auto-fetched** items (network worked): `gifsicle-upstream`, `caesium-source`.
- If a future reference **cannot be auto-fetched**, it must be **manually
  uploaded** here; append it to this manifest and mark `Retrieved: manually
  uploaded`.
- `gifsicle/` (repo root tree) matches upstream master exactly, so it is the
  **canonical** reference tree for the GIF engine. `gifsicle-nested-1.96/` is
  only interesting if we want to compare against the older frame-selection
  behavior.
- The nested `.git` directories of the shallow clones were removed; this folder is
  a plain read-only snapshot, and provenance is documented here instead.
