# Gifscythe — work-in-progress (GIF / APNG / WebP animation tool)

This repository deliberately separates **reference code** from **working code**
so the finished product is never confused with source-material we copied or
fetched.

## Repo layout

```
gifsicle-1.96/
  PROJECT_VISION.md       what the product is
  WORKLIST.md             task board (P0–P3)
  SESSION_HANDOFF.md      notes for the next session
  IMPROVEMENT_LOG.md      decision/change log
  FEASIBILITY_REVIEW.md   the original feasibility analysis
  README.md               this file (structure + how refs are organized)

  reference_code/                 SOURCE MATERIAL — do not edit, do not ship
    gifsicle/                canonical gifsicle 1.96 source (identical to upstream master)
    gifsicle-nested-1.96/    older alternate gifsicle variant (kept for reference)
    gifsicle-upstream/       shallow clone of kohler/gifsicle master (auto-fetched)
    caesium-source/          Caesium UI source (GPLv3) — the UI/UX base we adapt
    caesium-bin/             Caesium 2.8.5 Windows binary bundle (Qt6 runtime)

  working_code/                  THE PRODUCT — edit & ship this
    gifscythe/               the app (currently v0.1.0 skeleton)
```

## What is reference vs. working
- **reference_code/** — unmodified source material we reference, adapt, or bundle
  into releases. **Never edit** these; treat them as read-only imports.
- **working_code/** — our actual product. All edits happen here.
- Reference items fetched automatically from GitHub: `gifsicle-upstream`,
  `caesium-source`. If a future reference can't be auto-fetched, put it here and
  note that it was **manually uploaded** (see the note going forward).

## Versions
The *product* version lives in the app (`working_code/gifscythe/VERSION.md`).
0.1.0 (now) → 1.0.0–1.9.9 (finished GIF product) → 2.0.0–3.0.0 (WebP + APNG).
Never call it 1.0.0 until the UI/UX task is done.
