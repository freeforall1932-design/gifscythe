# Project Vision

**Product name:** Gifscythe — a GIF / APNG / WebP animation tool
**Version:** 0.1.0 (in development). See `working_code/gifscythe/VERSION.md`.
**Status:** Design & feasibility decided. See `FEASIBILITY_REVIEW.md`.

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
- Retain gifsicle terminal-level control for power users.

## Format support plan
- **GIF:** gifsicle (existing, native).
- **APNG + animated WebP:** **bucket-list / future task** — only after the
  pending **UI/UX retrofit** task on the worklist is complete. See
  `WORKLIST.md`.
