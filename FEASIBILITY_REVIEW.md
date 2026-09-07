# Feasibility Review — "Gifscythe" (UI front-end over gifsicle, GIF/APNG/WebP)

> **Naming note.** The working title here was "GifSqueezer"; the product is now
> formally named **Gifscythe** (`v0.1.0`). Everything in this review still holds.

**Repo reviewed:** `gifsicle-1.96` (Gifsicle 1.96 + a bundled Caesium Image Compressor 2.8.5 Windows build)
**Reviewer date:** 2026-09-06

> **Owner decision (2026-09-06):** Blocker 1 is **resolved by design** — use
> Caesium's UI/UX files as the base (not a full copy), modeled on XNConvert's
> feel/vibe; the icon/style assets are open-source/public with no exclusive
> trademark (logo excluded). Blocker 2 (WebP/APNG) is **deferred** to the bucket
> list, to be done only after the UI/UX retrofit task on the worklist is
> complete. See `PROJECT_VISION.md`, `SESSION_HANDOFF.md`, and `WORKLIST.md`.

---

## Verdict at a glance

| # | Your ask | Possible? | Effort | Main blocker |
|---|----------|-----------|--------|--------------|
| 1 | Use Caesium GUI as the base GUI, adapt to gifsicle | **Yes** (with source fork) | Medium | Caesium source is **not** in this repo; license (GPLv3) vs gifsicle (GPL v2-only) conflict |
| 2 | Ease-of-use like XnConvert, **still keep raw terminal control** (bind functions to a slider/button) | **Yes — strongest fit** | Medium | Pure UI/architecture work; gifsicle's CLI maps cleanly to widgets |
| 3 | Click-and-use, portable, "not just a pile of code scrap" | **Yes — easy** | Low | Packaging/bundling; already proven by the Caesium Qt6 bundle pattern |
| 4 | Exclusive **GIF + APNG + WebP**, feature-rich like ezgif, with a "Convert" branch | **Yes, but heavy lift** | High | **Gifsicle natively only reads/writes GIF.** WebP/APNG are NOT in the source. Needs new codecs + a frame model + converter |

**Overall conclusion: YES, this is buildable — but it is a new desktop app *around* gifsicle, not a tweak inside gifsicle.** Two things are genuinely hard: (a) the license mixing if you fork Caesium and embed gifsicle code, and (b) adding WebP/APNG codec support at all (gifsicle can't do it today). Everything else is UI engineering with a very good CLI-to-widget mapping.

---

## 1. What is actually in this repository

- **Gifsicle 1.96 source (C)** — complete, plain C, command-line tools only:
  - `gifsicle` (the main program — batch/merge/explode, resize, crop, optimize, quantize, lossy, dither, frame ops, delay/disposal/loop, transparency, colormaps, comments).
  - `gifview` (X11 viewer), `gifdiff` (comparison). Companion man pages exist.
  - Internals: `clp.c` (CLI parser), `gifread.c`/`gifwrite.c`/`giffunc.c`/`gifx.c`/`merge.c`/`optimize.c`/`quantize.c`/`xform.c`/`support.c`/`kcolor.c`, headers under `include/lcdfgif/` + `include/lcdf/`.
  - README/nothing about APNG or WebP. `grep` for `webp|apng|png|libpng` across `src/` returns **zero hits**. So GIF is the **only** codec.
- **Caesium Image Compressor 2.8.5 (Windows binary bundle)** — `caesium-image-compressor-2.8.5-win/`:
  - `Caesium Image Compressor.exe` + `caesium.dll` + `libcaesium`-style DLLs.
  - Full Qt6 runtime: `Qt6Core/Gui/Widgets/Concurrent/Network/Svg.dll`, `platforms/qwindows.dll`, `imageformats/qgif.dll`, `qwebp.dll`, `qjpeg.dll`, `qtiff.dll`, `qsvg.dll`, `qtga.dll`, etc.
  - **No `.cpp/.h/.pro` — it is binaries only.** There is no Caesium source in this repo.
- Oddity: the gifsicle sources appear **both** at the repo root *and* in a nested `gifsicle-1.96/` duplicate. You'll want to pick one tree for the real project.

**Practical reading of this:** the repo is a "bring-your-own-binary" collection. You have a ready Qt6 shell (Caesium) and a ready GIF engine (gifsicle). The glue — a GUI that drives gifsicle and understands APNG/WebP — does not exist and is what you'd build.

---

## 2. The architecture that makes all four asks work

Recommended shape (staged, low-risk):

```
  +---------------------------------------------------------------+
  |  GUI shell  (Qt6 Widgets — fork Caesium's UI, or build fresh)  |
  |   • file list / batch queue   • per-file property panel        |
  |   • before/after preview      • slider <-> control mapping     |
  |   • workflow tabs (Optimize / Resize / Crop / Convert /        |
  |     Frame Editor / Extract / Merge)  — the "ezgif branch" feel |
  |   • RAW COMMAND-LINE pane (live, editable, two-way synced)     |
  +-------------------------------+-------------------------------+
                                  | builds a gifsicle command line
                                  | (and/or calls codec functions)
  +-------------------------------+-------------------------------+
                                  v
  +---------------------------------------------------------------+
  |  ENGINE LAYER (3 small subprocesses or in-process libs)        |
  |   • gifsicle.exe            -> GIF read/write + GIF optimize   |
  |   • libwebp (AnimEncoder)   -> animated WebP read/write        |
  |   • libpng (+APNG) + zlib   -> APNG read/write                 |
  |   + a common Frame model: RGBA frames + delay + loop +         |
  |     disposal/blend, so any edit works on any format            |
  +---------------------------------------------------------------+
```

Two viable ways to wire the engine:

- **MVP — subprocess wrapper (recommended for v1).** GUI runs `gifsicle.exe <args>` and parses its output. Cleanest for licensing (see §5), zero refactor of the C code, portable, and you already have cross-platform Windows binaries. WebP/APNG handled the same way — delegate to `gif2webp`/`img2webp`/`apng2webp` or small C helpers.
- **Later — in-process libgifsicle.** Refactor gifsicle `.c` files into a library (it's mostly already factored into functions) and link it into the GUI. Higher performance, no process spawn, but now gifsicle's GPL v2 license is inside your binary — the license math changes (see §5).

---

## 3. Feasibility of each ask, in detail

### (1) Use the Caesium GUI as the base — **mostly yes, with caveats**

- **Yes technically:** Caesium's Qt6 shell is exactly the right UX skeleton for this task: a table of images, a right-hand property panel, live before-after comparison, per-item compression settings, and a broad layout you can relabel/re-target at GIF/APNG/WebP. Its `qgif.dll`/`qwebp.dll` image-format plugins also confirm Qt already speaks GIF and WebP at the `QImage` level, so previews are trivial.
- **Caveat A — no source in the repo:** To *adapt* the GUI you must start from Caesium's source (GPL-3.0), not the binaries. That's a `git clone` of `Lymphatus/caesium-image-compressor` (GPL-3.0) and a re-skin + engine swap. Fine, but it's a fork, not a patch.
- **Caveat B — codec limit in the shell:** Qt's `QImage` gives you static/large-frame-capable GIF+WebP for *preview*, but per-frame **delay / disposal / transparency** control for animated content is **not** reliably exposed by Qt `QImageReader` across formats/versions. Do the real animated encoding/decoding in the codec layer (gifsicle / libwebp / libpng), and keep Qt for the UI + preview only.
- **Caveat C — don't reuse `caesium.dll`:** its focus is JPEG/PNG compression; it's not an animation editor. Treat it as a UI reference, not the engine.

### (2) XnConvert-like ease-of-use **while keeping raw terminal control** — **strongest fit, yes**

This is gifsicle's sweet spot. The whole CLI is a flat set of operations, and each one has a natural widget. Crucially, you satisfy "keep the control of the terminal" by having the GUI generate the exact command line and exposing a **live raw-CLI pane** (change a slider → the CLI text updates; the displayed line is shell-quoted and matches the argv actually executed). A full reverse parser (edit the CLI text → the widgets update) is a desirable power-user enhancement for a later 1.x release — the MVP ships honest one-way sync so the pane never lies. Power users can copy the command into a real terminal; casual users never need to see it.

A concrete slider/option → gifsicle-arg mapping:

| GUI control | gifsicle argument |
|---|---|
| Optimization level slider | `--optimize=0..3` |
| Lossiness slider | `--lossy=0..200` (Kornel's lossy GIF) |
| Colors dropdown/slider | `--colors=N` (2–256) |
| Dither dropdown | `--dither=floyd-steinberg|ro64|none` |
| Resize (fit/exact) | `--resize WxH`, `--resize-fit`, `--resize-touch` |
| Scale % (X/Y) | `--scale XxY` |
| Rotate | `--rotate-90/-180/-270` |
| Flip | `--flip-horizontal`, `--flip-vertical` |
| Crop box | `--crop X,Y+WxH` (plus form), `--crop-transparency` |
| Loop count | `--loopcount` (on output) |
| Per-frame delay (1/100 s, **not** ms) | `-d` |
| Disposal dropdown | `--disposal=N` |
| Transparency color picker | `--transparent=#RRGGBB`, `--no-transparent` |
| Background color | `--background=#RRGGBB` |
| Logical screen | `--logical-screen WxH` |
| Colormap transform | `--change-color`, `--color-transform`, `--use-colormap`, `--colormap-algorithm` |
| Comments | `--comment="...", --no-comments` |
| Gamma / color math | `--gamma=srgb` / `--gamma=oklab` / `--gamma=NUM` (string form) |
| Frame ops | `#0 insert-1 #2 ...`, `--delete`, `--replace`, `--append` |
| Mode | batch (`-b`), merge (`-m`), explode (`-e`) |
| Robustness | `--careful`, `--conserve-memory`, `--no-warnings`, `--ignore-errors`, `--threads=N` |

That list *is* the product. There is also genuine room for high-value "smart" defaults (e.g. "Auto-optimize" runs `-O3 --lossy=20` then reports byte savings) that keep it approachable.

### (3) Portable click-and-run, not "code scrap" — **yes, and easy**

- gifsicle is a single small C `gifsicle.exe`. The Caesium folder already demonstrates the exact portable-Qt6 pattern: `.exe` + a handful of `Qt6*.dll` + `platforms/` + `imageformats/` + a couple of runtime DLLs (`libgcc_s`, `libstdc++`, `libwinpthread`, `opengl32sw`, `D3Dcompiler_47`).
- Ship a folder like `GifSqueezer/` with `GifSqueezer.exe`, `gifsicle.exe` (and the codec helpers), and the Qt runtime. Double-click to launch. No installer, no admin, runs from USB. This is a solved distribution problem.
- To keep the "not code scrap" feel: add a first-run welcome wizard, a one-click "Optimize" preset, drag-and-drop, tooltips that explain each control in plain words, and a "Show me the command" toggle for the curious.

### (4) Exclusive GIF / APNG / WebP, feature-rich like ezgif, with a Convert branch — **yes, but this is the big one**

This is where gifsicle alone is insufficient. **Gifsicle reads/writes GIF only; there are zero WebP or APNG symbols in `src/`.** So "exclusive GIF+APNG+WebP" is new engineering, not a config change. Here's the realistic route:

- **Common frame model.** Decode each input (GIF via gifsicle `-e` to frames; APNG via libpng APNG; WebP via libwebp `WebPAnimDecoder`) into a format-neutral list of RGBA frames, each with `(delay, disposal, blend, loop, transparent)`. Once everything is a frame model, every gifsicle operation (optimize, crop, resize, lossy, recolor, reorder, insert/delete) applies uniformly.
- **Encode to target.** GIF → gifsicle; APNG → libpng (`png_set_acTL/fcTL/fdAT`, supported via the upstream APNG merge in recent libpng or the `libpng-apng` patch, plus zlib); WebP → libwebp `WebPAnimEncoder` (lossy/gif2webp, lossless/img2webp).
- **Convert branch.** A "Convert" tab becomes GIF ⇄ APNG ⇄ WebP + static capture of any frame, plus "Extract/Explode frames", "Merge a folder of images into a GIF/APNG/WebP", "Reorder frames", "Set loop", "Strip/replace transparency" — the ezgif feature set. All reusing the same frame model.
- **The deliberate limit.** Constrain the file-type pickers to GIF/PNG(APNG)/WebP so the app never drifts into "media editor." That's a UI policy, trivially enforced.

> Note on APNG support: recent libpng merged APNG upstream (pnggroup/libpng PR #706); otherwise use the well-known `libpng-apng` patch. Either way it's mature and small.

---

## 4. Where the real work is (honest effort split)

| Workstream | Size | Notes |
|---|---|---|
| GUI shell (fork Caesium Qt or build) | Medium | Reuse the batch/panel/preview skeleton; rebrand + retarget |
| CLI ⇄ widget binding + live raw-CLI pane | Medium | 1:1 mapping table in §3(2); this *is* the retain-terminal-control feature |
| GIF ✓ via gifsicle | Trivial | Already works |
| **WebP animated codec** (libwebp AnimDecoder/AnimEncoder) | High | New library + frame model integration |
| **APNG codec** (libpng+APNG + zlib) | High | New library + frame model integration |
| Converter + frame-editor UX (GIF⇄APNG⇄WebP, explode/merge/reorder/effects) | Medium-High | ezgif-style feature set |
| Portable packaging | Low | Proven pattern from the Caesium bundle |

Roughly: **days for a usable MVP (GIF-only GUI shell over gifsicle); weeks to add WebP+APNG + converter to a polished multi-tab app.** Not a weekend, but squarely achievable.

---

## 5. Licensing — the one thing you cannot skip

- **Gifsicle: GPL v2, and *only* v2.** (README: "GNU General Public License, Version 2 (and only Version 2).") It also offers an *alternative* license, with the explicit note that anyone embedding gifsicle code in a product whose source is **not** made available "**MUST contact the author [Eddie Kohler] and obtain permission before doing so.**"
- **Caesium: GPL v3** (the `Lymphatus/caesium-image-compressor` project). Qt is LGPL-3.0 (or GPL/commercial).

GPL **v2-only code is not compatible with GPL v3 code** in the same program — you can't just merge them. Therefore:

- **Clean path (recommended): keep gifsicle as a separate process.** Your GUI shells out to `gifsicle.exe`. Your GUI can be GPL v3 (with Caesium) and gifsicle stays a GPL v2 executable you ship alongside; you comply with gifsicle's distribution terms by providing its source and the license text. No license conflict, no permission needed.
- **If you link gifsicle C code into a single binary:** you either (a) open-sourcing the whole thing still won't fix v2-only ⊕ v3-onward (v2-only can't be combined into v3 at all — the "v3" file would be incompatible), so you'd need to **contact Eddie Kohler** for permission / a GPL v3 grant under gifsicle's alternative-license clause, or (b) build **your own** re-encoding in libwebp/libpng for the APNG/WebP side and keep the GIF side purely as a subprocess.

This is the one genuine *legal* constraint on ask #1 and #4. If you plan to distribute a closed-source app, you **must** use the subprocess approach (or get written permission). If you're open-sourcing under GPLv3 but still want gifsicle linked in, you need the author's OK because of v2-only.

---

## 6. Recommended go/no-go + phased roadmap

**Go, with this sequencing (each phase shippable):**

1. **Phase 0 — Setup.** Pick one gifsicle tree (drop the nested duplicate). Get a clean cross-compiled `gifsicle.exe`. Confirm GIF in → GIF out + `--info` works.
2. **Phase 1 — GIF GUI MVP (proves ask 1, 2, 3).** Build a Qt GUI that lists files, exposes the §3(2) control mapping, shows before/after preview, and emits the gifsicle command line. Ship portable (Qt runtime + gifsicle.exe). This alone delivers: "Caesium-like GUI + terminal control + portable."
3. **Phase 2 — Convert branch.** Add the frame model + libwebp + libpng/APNG decoders/encoders. Implement GIF ⇄ APNG ⇄ WebP, explode, merge, reorder, loop, transparency. This unlocks ask 4.
4. **Phase 3 — ezgif-style richness + polish.** Frame editor, effects (overlay text/watermark), presets, keyboard-first power users, localized tooltips, "show me the command" toggle, update/auto-update flow.

**Go only if** you're comfortable with GPL obligations (or going the subprocess route) and the effort of adding two codecs. **Don't attempt** it as a modification of gifsicle's own codebase expecting APNG/WebP to "just work" — they won't; it needs the extra layer described above.

---

## 7. Key libraries / tools to adopt

- GIF: **gifsicle** (keep as-is, subprocess).
- Animated WebP: **libwebp** — `WebPAnimDecoder` / `WebPAnimEncoder`; reference tools `gif2webp`, `img2webp`.
- APNG: **libpng** (APNG-capable build; upstream merged APNG in recent libpng, else `libpng-apng` patch) + **zlib**; reference tool `apng2webp`/`apngdis`.
- GUI: **Qt 6 Widgets** (fork Caesium's shell), or **Tauri/Electron** if you prefer web-tech UI (Tauri gives a smaller footprint; Electron eases iteration).
- Build/packaging: CMake; a portable folder (no NSIS/installer) mirroring the Caesium bundle layout.
