# Architecture Synthesis: XnConvert Frontend & ScreenToGif Backend Model for Gifscythe

This document preserves and compiles the architectural breakdown synthesized from the XnConvert program workflows (as documented in the guide materials) and the ScreenToGif engine model.

---

## 1. High-Level Vision: The Hybrid Architecture

Gifscythe couples:
1. **Frontend**: Inspired by **XnConvert** — an intuitive, professional batch processing GUI organized across clear functional tabs (Input, Actions + Preview, Output, Status, Settings, About).
2. **Backend**: Inspired by **ScreenToGif** & **Gifsicle/FFmpeg** — rich animation/codec pipeline capabilities (frame manipulation, quantizers, encoding profiles, size estimation, container conversions) executing under a decoupled, verifiable engine layer.

```
+-----------------------------------------------------------------------------------+
|                           GIFSCYTHE FRONTEND (XnConvert)                          |
+-----------------------------------------------------------------------------------+
| [ Input ]     [ Actions + Preview ]     [ Output ]     [ Status ]     [ Settings ]|
|  Thumbnail     Action Card Stack         Destination    Processing     FFmpeg/Eng |
|  Grid, Sort,   & On-Demand Mirror        Templates,     Table, Live    Path & Env |
|  Filter & Del  Preview Viewport          Policies       Telemetry                 |
+-----------------------------------------------------------------------------------+
                                        |
                             Task Request / Settings
                                        v
+-----------------------------------------------------------------------------------+
|                        GIFSCYTHE CORE / ENGINE (ScreenToGif)                      |
+-----------------------------------------------------------------------------------+
| Frame Extraction & Parsing  -->  Transform Pipeline  -->  Optimized Encoders     |
| (GifFacts, InputAdmission)       (Resize, Crop, Colors)   (Gifsicle, FFmpeg, etc.)|
|                                                           - GIF (Gifsicle engine) |
|                                                           - APNG, WebP, MP4, WebM |
+-----------------------------------------------------------------------------------+
```

---

## 2. Frontend Breakdown (XnConvert UI Paradigm)

### A. Input Tab (Queue & Ingestion)
* **Ingestion Controls**: Top toolbar with `Add files…`, `Add folder…`, `Remove`, and `Remove all`.
* **Search / Filter Bar**:
  * In-line search text box with a clean placeholder (`Filter`).
  * Dedicated Filter action menu with two core batch actions:
    * `Delete filtered items` (removes matching items from the processing queue).
    * `Delete non-filtered items` (retains only matching items, clearing everything else).
* **Grid Presentation**:
  * Unified thumbnail view showing scaled previews with file details (dimensions, duration, frame counts, size).
  * Direct thumbnail size selector to toggle between compact and expanded icon sizes.
  * Multi-selection and drag-and-drop support.

### B. Actions Tab & Mirror Preview
* **Action Cards Stack**:
  * Collapsible operation cards for discrete operations (Optimize, Resize, Geometry, Crop, Colors, Metadata).
  * Single-state pointer glyphs (`▼` closed / `▲` open) without trailing arrows.
  * Direct toggleable enablement per card without discarding configurations.
* **On-Demand Mirror Preview**:
  * Integrated directly beside the action stack in a unified large viewport frame.
  * Viewport tabs (`[Before]` / `[After]`) allowing instantaneous visual comparison.
  * **"Apply setting preview"** manual button to prevent disk thrashing and redundant temporary re-encodes during rapid slider/option tweaks.

### C. Output Tab
* **Destination**: Direct radio selection between `Source folder` and designated `Folder` (with path entry and Browse button).
* **Filename Template**:
  * Dynamic name template (`{Filename}_result`).
  * Live completion on typing `{` displaying context tokens (`{Width}`, `{Height}`, `{File Index}`, etc.).
* **Collision & Guard Options**:
  * Single compact collision selector with pointer chevron (`▼`/`▲`), providing `Ask each time`, `Rename`, `Overwrite`, and `Skip`.
  * Left-aligned options:
    * `Keep folder structure` (re-creates relative subfolder trees inside designated destination folders).
    * `Discard result if larger than original (same format only)` (safeguards against optimization bloating).
* **Format & Encoder Settings**:
  * Clean format selector (GIF, APNG, WebP, MP4, WebM).
  * Dedicated `Settings…` dialog for codec quality parameters (CRF, QP, framerate).

### D. Status Tab (Execution & Telemetry)
* Real-time batch progress header (`Processing list: X of Y files`).
* Honest result table (File, Original Size, Final Size, Savings %, State, Details).
* Skipped or collided files clearly flagged without reporting synthetic savings.

---

## 3. Backend Engine Breakdown (ScreenToGif Engine Paradigm)

### A. Modular Encoder Abstraction
* **Gifsicle Core**: Native, low-overhead GIF optimization, color reduction, and frame disposal tuning.
* **FFmpeg Pipeline**: Multi-container video and animated image encoding with explicit parameter mapping (libwebp-anim, libx264, libx265, libvpx-vp9, libsvtav1).
* **Frame Analysis & Metadata**: Ingestion inspection (`GifFacts`) reading duration, canvas geometry, transparency flags, and color table depth before dispatching encoder processes.

### B. Safe Staging & Non-Destructive Operations
* Atomic writes using staging files (`.gs-partial`) to protect original files from crash-induced corruption.
* Size-guard validation comparing byte sizes before final promotion.


### C. Delivered versus reference ideas

The shipped desktop accepts GIF inputs and uses gifsicle for GIF output, with
FFmpeg-backed formats available when a suitable sidecar is found. The preview
plays GIFs; it does not simulate APNG, WebP, video or multi-file Explode output.
The ScreenToGif research is an inspiration for future animation tooling, **not**
a claim that its frame editor, recording, quantizers or encoders are implemented
here. See `STATUS.md` and `WORKLIST.md` for the actual scope and remaining work.

A filter action removes queue entries only, never the source files. An empty
filter disables both destructive queue actions. New filename templates use a
1-based file index; legacy session/preset `start_index` values are still read
and saved so older configurations retain their output names.
