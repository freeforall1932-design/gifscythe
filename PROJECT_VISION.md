# Project Vision

**Product name:** Gifscythe — a GIF / APNG / WebP animation tool  
**Version:** 0.1.0 (in development). See `working_code/gifscythe/VERSION.md`.  
**Status:** Feasibility decided (`FEASIBILITY_REVIEW.md`). Engine, control layer,
CLI, and GUI MVP implemented; P0/P1 honesty fixes landed. Full GIF UI/UX retrofit
and verified portable Windows build remain before **1.0.0**. See `WORKLIST.md`
and `COMPILED_AUDIT.md`.

## Mission
A portable, click-and-use desktop app for **animated (moving) images only** —
**GIF, APNG, and WebP**. It should let a *normal person* edit, optimize, and
convert animated images without ever seeing "a pile of code scrap."

## Audience
Casual / non-technical users who just want it to work, **plus** power users who
want the full gifsicle terminal control underneath.

## UX references
- **"Feels & vibe" of XNConvert** for the overall layout and ease of use.
- **Feature-richness of eZgif** for GIF-style options + a "Convert / other"
  branch (convert between formats, explode, merge, reorder frames, and so on).

## UI approach (owner decision)
- Use **Caesium's UI/UX files as the base** — **not** a full copy/paste.
- Reuse their **open-source / public** icon and style assets; Caesium has no
  exclusive trademark on those (the XNConvert-style modeling applies, **logo
  excluded**).
- Model the layout/feel on **XNConvert** for ease of use.

## Hard scope constraints
- **Exclusively** animated GIF, APNG, and WebP. Not photos, not video —
  only "moving picture" / GIF-type features.
- Portable, click-and-run (no installer, no admin). Windows-first.
- Retain gifsicle terminal-level control for power users (live command pane;
  argv execution — never shell injection).
- gifsicle stays a **subprocess** (GPL v2-only engine vs GPLv3 UI).

## Format support plan
- **GIF:** gifsicle (existing, native) — current focus through 1.0.0.
- **APNG + animated WebP:** **bucket-list / future task** — only after the
  GIF UI/UX retrofit is complete and 1.0.0 ships. See `WORKLIST.md`.

## Progress snapshot (2026-09-07)
| Area | State |
|------|--------|
| GIF engine subprocess | Done (native + Windows config path) |
| Settings → argv control layer | Done |
| CLI driver + tests | Done (honest exits, smoke suite) |
| GUI MVP | Done enough to drive core; tabs/preview/full controls pending |
| Silent-failure class bugs | Fixed in code; re-verify via `COMPILED_AUDIT.md` §6 |
| 1.0.0 | **Not yet** — UI/UX retrofit + verified portable Windows still open |
