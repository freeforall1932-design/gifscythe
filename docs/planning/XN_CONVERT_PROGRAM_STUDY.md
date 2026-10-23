# XnConvert — study of the real program: UI/UX and the logic wired to its GUI

**Session:** 2026-10-23 · **Status:** complete for the purpose it was written for · **Blocks:** slices
**C → E → D** (this file was their precondition and it now exists).

**Why this file exists.** Until now the UI target was described from four photographs
(`XN_CONVERT_REFERENCE.md`, N-55). The owner rejected that method — *"You kept messing it… I thought
giving you the program to see the inner workings is better than guessing from photo."* Both supplied
archives were therefore unpacked and the **shipped XnConvert 1.118.0 binary was driven live**, under
Qt's `vnc` platform plugin, at full resolution, with the app's own strings, its `.qm` translation
catalogues and its persisted log harvested alongside. Everything below that is marked `[OBS]` was
seen on screen or in the app's own files; `[STR]` is a string from the binary/catalogues; `[INF]` is
my inference and is labelled as such.

**What this file is not.** It is not a spec for gifscythe, and it is not permission to copy XnConvert.
No binary, screenshot or asset from the vendor enters this tree (`OD-22`/`N-36` still govern). The
study lives in this repo as **text**; the captures it cites live outside it, in the session workspace,
and are listed in Appendix A only so a future session knows what exists.

---

## 0. Scope guard — we are not building a picture converter

This is the single most important section, because the reference program is the wrong shape and its
wrongness is invisible in screenshots.

**gifscythe's product, restated:** *moving pictures in → moving pictures out.* Inputs are animated
GIF/APNG/WebP and — as a conversion endpoint only, per `OD-20 = a` — video. Outputs are animated
GIF/APNG/WebP. gifsicle remains the only GIF encoder; the FFmpeg sidecar decodes only; there is no
editor, timeline, recorder or player (`PROJECT_VISION.md`, and `OD-20 = a` explicitly keeps
ScreenToGif's **recorder/editor** scope out — what we borrow from ScreenToGif is its *file type +
preset* vocabulary, `S.SaveAs.Type`, not its capture pipeline).

**XnConvert is a still-image converter with a page model.** That is not an impression, it is measured
on the shipped binary:

| Run | Input (gifsicle's count) | Output (gifsicle's count) | Status log line |
|---|---|---|---|
| 7 mixed fixtures | `banner.gif … 320x240x6` | `banner_result.gif` = **1 image** | `27.75 KiB -> 66.52 KiB (139%)` |
| 40 small GIFs, *Oil painting 2* | `pic01.gif (1) 60x132x6` | `pic01_result.gif` = **1 image** | `8.50 KiB -> 635 Bytes (-92%)` |
| 24 heavy GIFs, *Oil painting 2* | `h07.gif (1) 1200x1200x6` | `h07_result.gif` = **1 image** | `276.99 KiB -> 16.17 KiB (-94%)` |

The mechanism is visible in the GUI: Output tab → **Multipage**, two checkboxes, **both cleared by
default** — `Convert multipage file to multipage file (when possible)` and `Convert all pages from
multipage file` `[OBS]`. With them off, XnConvert writes page 1 and drops the rest. The −92 %/−94 %
"compression" in those runs is mostly **frames being deleted**.

Three consequences, and they are the reason this study can be trusted:

1. **The shell transfers; the data model does not.** Tabs, counters, card stack, status grammar and
   presets are about *driving a batch*, and a batch of animations is still a batch. The page/bitmap
   model underneath is about *one image*, and that is precisely what gifscythe must not adopt.
2. **The action catalogue is vocabulary, not a roadmap.** XnConvert ships 12 action groups (131
   actions in `.qm` context `Convert`). Most are single-bitmap photo filters. gifscythe's action set
   is bounded by what gifsicle can do to *every frame of a stream*; anything else needs a raster
   engine and a re-scope, not a copy.
3. **The animation must be a first-class, visible object** — in columns, in the preview, and in the
   Output form. XnConvert gives it one disabled-by-default checkbox group. This study therefore
   spends §10 on the **inverse** of several reference defaults.

---

## 1. How the program was studied (so this can be repeated or challenged)

- **Unpacked:** `XnConvert windows.7z` and the two-part Linux archive
  (`XnConvert-linux-x64.7z.001`, `XnConvert-linux-x64.7z.002`) with py7zr 1.1.4 (no `7z` binary in
  the sandbox) into `_xnconvert/windows/XnConvert/` and `_xnconvert/linux/XnConvert/`.
  The study used the **Linux x64 Qt 6.8.9 build**, version **1.118.0** (unix x64, 2026-10-10,
  Libformat 7.310).
- **Run headless-but-real:** the binary cannot use a normal X server here (no system Qt deps), so it
  runs on the `vnc` platform plugin with minimal `libGL/libEGL/libxkbcommon/libdbus` stubs. It is the
  genuine program: same widgets, same logic, same worker threads.
- **Captured:** a small RFB driver (`analysis/vnc.py`) that sends `SetPixelFormat` + `SetEncodings(Raw)`
  before requesting updates (without that, Qt's VNC plugin returns 1-pixel stripes). 26 full-frame
  captures, listed in Appendix A.
- **Harvested:** `strings` over the binary → UI text and logic strings; a `.qm` parser over the Qt
  translation catalogues → 4,637 rows, 131 action names, widget object names (`sbAngle`, `cbCrop`,
  `leFilename`, …); the app's own `~/.xnconvert/xnconvert.ini` and `~/.xnconvert/batch_convert_log.txt`.
- **Not available:** source code, debug symbols (`nm -C` is empty — the binary is stripped), and any
  way to single-step. All *timing* and *threading* statements below are behaviour observed from the
  outside, not code read.

---

## 2. The shell

Six tabs, one window, one bottom bar. The tab titles carry state, which is why they are worth copying
verbatim `[OBS]`:

```
Input: 24 file(s) │ Actions [1/1] │ Output │ Status │ Settings │ About
```

- `Input:` always shows the queue length, including while a run is in progress.
- `Actions [enabled/total]` — the counter is **enabled actions over total actions**, not "current
  step". Disabling a card changes `[1/1]` → `[0/1]` while the card stays visible (§4.4).
- `Status` is **greyed out until a run exists**, then becomes the live report tab (§7).
- Bottom bar, left to right: `Presets:` combo + three 16-px buttons (save / load / clear), then
  `Export for NConvert…`; on the right `Convert` and `Close` `[OBS]`. While a run is in flight the
  right-hand button becomes **`Stop`** and the presets combo plus its buttons go **disabled** `[OBS]`.
- While running, the tabs `Input`, `Actions` and `Output` are **disabled**; `Status`, `Settings` and
  `About` stay usable `[OBS]`. That is a strong hint about intent: you may read state and change
  preferences during a run, but you may not change the job.
- A **startup wizard** (`General`) appears on first run: Language `<Autodetect>`, Theme `Default`,
  settings location `Current user (recommended)`, update check `Daily`, `OK`/`Cancel` `[OBS]`.
- Window-title grammar during a run: **`XnConvert − 21% − 19 second(s) remaining`** `[OBS]` in the
  owner's reference screenshot. The percentage and the remaining time are the *same* numbers shown in
  the Status footer.

---

## 3. Input tab

`[OBS]` on a 24-file and a 40-file queue:

- Toolbar: `🌱 Add files…`, `Add folder…`, `− Remove`, `Remove all`, a funnel button and a
  `Quick filter` line edit, and a small columns menu button on the right.
- A **thumbnail grid**: cells roughly 100 px square, image centred on a checkerboard for transparency,
  file name centred underneath, selected cell painted **green** (multi-select) with a blue focus ring
  on the current one.
- Column headers are the sort keys, and they are the *only* visible columns:
  **Name · Size · Modified date · Created date · EXIF Date T… · Properties · Print size · Type ·
  Rating · Colour label · Check**.
- Footer strip inside the tab: `☐ Hot folders…` at bottom-left.
- Selection drives the preview: `1/24  h01.gif` in the preview footer matches the highlighted cell.

**Reading it for gifscythe:** the grid is the part users actually see first, and every one of its
columns is a *file* property. There is no `Frames`, `Duration`, `fps` or `Loop` column anywhere in
the program `[OBS]`. For an animation tool that is the defining omission — see §11.1.

---

## 4. Actions tab — an ordered stack of cards

### 4.1 Pane anatomy `[OBS]`
Title `Actions [1/1]`, then a header row: `🌱 Add action >` (left) and `🌿 Clear all` (right). Below,
a scroll area holding the cards; a vertical scrollbar appears only when the stack overflows.

### 4.2 Card anatomy `[OBS]`
One collapsed card, top row:

```
▾ │ Oil painting 2 │        −  ⧉  ↻ │      Enabled ▾
```

- left: collapse chevron; then the action title in bold;
- middle-right: **`−` (delete this action)**, duplicate, reset;
- right: the word **`Enabled`** with a ▾ menu.
- Below the title row, the action's own body: parameters only, no OK/Apply.

### 4.3 Insertion order is stack order `[OBS]`
A new action is appended at the bottom; processing runs top → bottom. With `Rotate` added before
`Resize` the tab read `Actions [2/2]` and the cards read Rotate, Resize in that order. Reordering is
by the drag handle at the card's left edge.

### 4.4 `−` deletes; `Enabled` disables `[OBS]`
This was mis-read from the photos and is worth stating plainly: clicking the card's **`−` removed the
action entirely** (stack went `[1/1]` → `[0/0]`). Disabling is a *different* control: the `Enabled`
label opens a menu, and turning it off keeps the card on screen while the tab counter drops to
`[0/1]`. **Delete and disable are separate affordances, and the counter counts enabled ones.**

### 4.5 Add menu taxonomy `[OBS]`
`Add action >` opens a flat list of 12 groups, each expanding to the right:

`Add · Adjust · Blur · Channel · Enhance · Frame · Metadata · Noise · Pixelate · Stylize · Transform · Video`

Counts harvested from the `.qm` context `Convert` (131 names total). Two groups matter for us because
they are geometry- and time-adjacent:

- **Transform (15):** Aspect Pad, Automatic crop, Automatic deskew, Canvas resize, Change color
  depth, Clear transparent pixels, Crop, Mirror, Offset, Replace color, Resize, Rotate, set DPI,
  Symmetry, Transparent color.
- **Stylize (23):** Band, Bloom, Edge detect, Emboss, Emboss more, Fantasy, Film Simulation, Lens,
  Oil painting, Oil painting 2, Old camera, Photo Spread, Posterize, Retro, Sepia, Shear, Slice,
  Solarize, Spread, Swirl, Tile, Vignetting, Waves.

Note what the group *Video* is not: `[INF]` it holds actions *about* video files (thumbnail extraction,
frame grabbing), not a timeline. There is no trim, no frame-rate, no duration control anywhere in the
action catalogue `[OBS]`.

### 4.6 The parameter idiom `[OBS]`
Every numeric parameter is the same triple: **slider + spin box + a small orange reset button**.

- `Rotate`: `Angle` radio + slider −180…180 + `Angle 0.00` spin + `>` reset, `EXIF Orientation` radio,
  `Background color` swatch, `Auto crop`, `Use anti-aliasing`, channel combo (`All`).
- `Resize`: `Presets [Custom ▾]`, `Mode [Fit ▾]`, `Width 800` `pixels`, `Height 600`.
- `Oil painting 2`: one row — `Intensity` slider + spin + reset.

The slider/spin/reset triple is the single most copyable piece of the reference: it is why the panel
feels calm despite 131 actions. `[INF]` it also means the model stores one number per parameter and
the widgets are three views of it — exactly the shape a `QAbstractItemModel`-backed card stack needs.

### 4.7 Preview is bound to the card `[OBS]`
Editing `Intensity` re-rendered the preview immediately (visible texture change at 46 % zoom), with no
Apply. Heavy parameters are presumably debounced `[INF]`; there was no visible lag at 60×132.

---

## 5. Preview pane

`[OBS]`: a titled `Preview` pane to the right of the stack, with a thin icon column on its left
(refresh, and an apply/commit icon), a large canvas with a checkerboard for transparency, and a
footer:

```
        ◀◀   ◀   ▶   ▶▶          100%
1/40
pic01.gif
```

- **The stepper walks the input files, not the frames.** `1/40 … pic01.gif`, `1/24 … h01.gif` — the
  arrows change *which file* is previewed, and the readout is `index/count` of the queue.
- Zoom is `− / + / fit / 1:1` plus a percentage readout (`46 %`, `100 %`).
- The canvas is a **drop target** for files.
- The preview background colour is a **Settings** item (`Preview background color` swatch), not a
  per-session toggle.

**This is the clearest structural evidence of the page model:** a program that previewed animations
would preview *motion* (play/pause, frame step, duration). XnConvert previews **one still frame** and
uses the footer arrows to move between *files*. `[INF]` the animation is decoded, reduced to page 1,
and never shown as time.

---

## 6. Output tab

`[OBS]` on 1.118.0, top to bottom. Group boxes, dense, two columns for the lower half.

### 6.1 Output
| Control | Type | Observed default |
|---|---|---|
| `Source folder` | radio | **selected** |
| `Folder` | radio + path combo + `…` | unselected; `Multiple resolution…` greyed |

### 6.2 Filename
| Control | Type | Observed |
|---|---|---|
| `{Filename}_result` | editable combo + `>>` insert-tag button | `{Filename}_result` |
| `Case` | combo | `No change` |
| `Start index` | spin | `1` |

Tokens visible in the catalogue `[STR]`: `{Filename}`, `{Filename With Ext}`, `{DstWidth}`,
`{DstHeight}`, `{Current Date [m_d_Y]}`, `{Modified Date}`, `{Annotation}`.

### 6.3 Format
| Control | Type | Observed |
|---|---|---|
| format combo | combo | `Same as original` |
| `Settings…` | button | **disabled** while format is `Same as original`, enabled per format |

Per-format write dialogs `[STR]`: **GIF** (interlace, 87a), **JPEG** (quality, progressive, optimise
Huffman, DCT, 4:2:2/4:2:0/4:4:4, `Use estimated quality`), **PNG** (interlace, level, Sub/Avg/Paeth),
**WebP**, **TIFF**, **PDF** (PackBits/LZW), **JP2K**, **JPEG-XL**, **AVIF** (speed 0–10). This is the
"gear opens *that* format's settings" shape `OD-21 = a` already adopted.

### 6.4 Options
| Control | Type | Observed default |
|---|---|---|
| `If an output file already exists` | combo | **`Ask`** |
| `Keep original file when encoded result is larger (experimental)` | check | off |
| `Keep folder structure` | check | off (greyed when Source folder) |
| `Keep parent folder` | check | off, greyed |
| `Keep original date/time attributes` | check | off |
| `Preserve extension` | check | off |
| `Preserve Metadata` | check | **on** |
| `Preserve color profile` | check | off |
| `Delete original` | check | off |
| `Use multiple CPU Cores` + slider | check + slider | off, value `1` |

`[STR]` The core slider refuses to coexist with prompting or multipage PDF:
`The 'Use multiple CPU Cores' setting was disabled. This setting can't be combined with 'Ask' mode or
multipage PDF files.`

### 6.5 Multipage — the decisive group
| Control | Default |
|---|---|
| `Convert multipage file to multipage file (when possible)` | **off** |
| `Convert all pages from multipage file` | **off** |

`[OBS]` With both off, a 6-page GIF produced a 1-page GIF. This group *is* XnConvert's animation
switch, and it is off and unlabelled as "animation". `[INF]` a user converting a GIF with defaults
silently loses their animation; the size delta in the log (−92 %) hides that inside "compression".

### 6.6 After conversion
`Open File Manager` · `Clear the 'Input' file(s)` · `Beep at the end of process` (**on**) ·
`Exit after conversion` — all checkboxes `[OBS]`.

There is **no** `Recipient:`/`Open in Browser` in this form — those come from sibling XnView, and the
screenshot-based reference in `XN_CONVERT_REFERENCE.md` §3 does not conflict with this.

---

## 7. Status tab

### 7.1 Header and time `[OBS]`
```
Processing list: 13 of 24 files
Started at: 23 Oct 2026 03:36:29
```
`Processing list: k of m` — **k is the 1-based index of the file currently being processed**, so at
completion k = m (`7 of 7`), and `13 of 24` means twelve are done and the thirteenth is in flight.
Observed caveat: at the instant a run starts the header still shows the previous run's value until the
first progress signal arrives (a batch was caught showing `24 of 24` with only one file loaded).
`Started at:` is written immediately.

### 7.2 Per-file log grammar `[OBS]`
Two lines per file plus an indented size delta:
```
/home/user/.../h07.gif (1) 1200x1200x6 : loaded
/home/user/.../h07_result.gif : saved
            276.99 KiB -> 16.17 KiB (-94%)
```
- `(1)` is the page index taken from the source; the trailing `x6` is XnConvert's page count.
- **Caution for our implementation:** these counts do not map 1:1 onto gifsicle's. A resized fixture
  that gifsicle reports as **12 images** is logged by XnConvert as **x6**. Never equate a
  `page count` from one engine with `image count` from the other `[OBS]`+`[INF]`.
- One variant without the page parenthetical was observed (`logo1.gif 48x68x6 : loaded`); the rule
  that decides it is unknown.

### 7.3 Summary block `[OBS]`
```
Input files: 24
Extracted pages: 24
Overwritten: 24            <- only present when overwriting happened
New files: 24              <- seen when nothing pre-existed

Total input file size: 6.49 MiB
Total output file size: 388.01 KiB
File size ratio: -94%

Total Duration: 22 second(s)

Finished at: 23 Oct 2026 03:36:52
```
`Extracted pages` equals `Input files` when Multipage is off — a direct readout that no extra pages
were carried. `Total Duration` tracks wall clock (03:36:29 → 03:36:52 reported as 22 s; a 17 s batch
reported 16 s) and rounds to `0 second(s)` for very fast batches.

### 7.4 Progress bar, ETA and remaining `[OBS]`
Footer under a log that is separated by a rule:
```
[▮▮▮▮▮▮▮▮▮▮▮▮▮▮▮▮▮▮                ]        (segmented blue blocks)
Estimated end time: 03:36:24      Remaining: 12 file(s) - 50% - 7 second(s) remaining
```
- The bar is **segmented** (one block per file) and fills with **completed** files.
- `Estimated end time` is a **wall-clock time**, not a duration, and it is recomputed as the run goes
  (`03:36:24` at 13/24, `03:38:21` at 2/24 of the next batch).
- `Remaining:` counts **files not yet finished**, *including the one in flight*:
  `m − (k−1)`, with the percentage `Remaining/m`.

**The two percentages are complements, and this is a real papercut worth not copying.** At `2 of 24`
the bar is ~8 % filled while the text says `96%`. Both are defensible; together they are confusing.
`[INF]` our Status tab must choose one convention (recommended: bar = completed, and label the number
`completed`/`done`, not a bare percentage).

### 7.5 The primary button changes `[OBS]`
Idle: `Convert` … `Close`. Running: the `Convert` slot becomes **`Stop`**; presets and the
`Export for NConvert…` button grey out. Clicking `Stop` is guarded by `[STR]`
`Do you want to cancel current batch processing?`

### 7.6 The log is a file `[OBS]`
The whole tab is mirrored to `~/.xnconvert/batch_convert_log.txt` (28 KB after a few runs), including
`Started at:`/`Finished at:` and the summary block. The tab is a *view* of a persisted artifact —
which is why the same text survives restarts. `[INF]` this is why the reference tab reads like a
console and not like a table: it is literally a log sink being tailed.

---

## 8. Settings, About, and what persists

`[OBS]` Settings tab:
`Language` combo (`<Autodetect>`) · `Theme` (`Default`) · `Check for program updates` (`Daily`) ·
`Registration…` · `Load last script used at startup` · **`Preview background color`** swatch ·
`Save actions list at exiting` (**on**) · `Clear filename field at startup` · `Restore previous input
file list` · `Rotate images according to EXIF orientation tag` · `Use color based on file format` ·
`Load format settings…`.

`[OBS]` `~/.xnconvert/xnconvert.ini` holds only two sections, `[%General]` and `[Start]`, with ~60
keys (`allFormats`, `confirmDel`, `exifToolPath`, `lastTab`, `maxRecent`, `useRecycleBin`,
`checkFreq`, `language`, `startupType`, `style`, …). **The job itself** — actions, output form,
filename template — is not in the ini; it is the *script* that `Save actions list at exiting` persists
separately, and is what the presets combo saves/loads. `[INF]` two stores: preferences (ini) and
reusable job definitions (script/preset). We should keep that split.

About tab: version, build date, Libformat version, and third-party notices (IJG, libpng, zlib,
dcraw) `[STR]`.

---

## 9. The GUI logic, reconstructed

Marked `[INF]` throughout: the binary is stripped and this is inference from behaviour plus the signal
names that survive in the binary.

### 9.1 State machine `[OBS]`+`[INF]`
```
IDLE ──Convert──► RUNNING ──file needs decision──► PROMPTING (modal) ──answer──► RUNNING
                     │                                                      
                     ├──Stop──► CONFIRM ──yes──► STOPPING ──► IDLE
                     └──last file──► FINISHED ──► IDLE (log + summary retained)
```
- During `RUNNING`: Input/Actions/Output tabs disabled, presets disabled, `Convert`→`Stop`.
- During `PROMPTING`: the batch is **frozen**, not merely slowed. With the overwrite dialog open the
  log held exactly one `loaded` line and no `saved` line `[OBS]`. So the prompt blocks the worker's
  continuation — the modal is on the flow, not a notification.

### 9.2 Worker and signals `[STR]`
Slot names harvested from the binary make the threading model explicit:
`onConvert`, `onThreadLoad`, `onThreadLoaded(…MapInfo…)`, `onThreadSaved(…)`, `onThreadInfo`,
`onThreadOverwrite`, `onThreadProgress(int, int, int)`, `onPreviewFinished(const Bitmap&)`,
`onTabIndexChanged(int)`, `updateListCount`. `[INF]` a QThread pool emitting per-file
load/saved/progress signals into the GUI thread; `onThreadOverwrite` is the modal decision point;
`onPreviewFinished` is why the preview is asynchronous even for one file.

### 9.3 Progress arithmetic `[OBS]`
`onThreadProgress(int,int,int)` plausibly carries `(done, total, percent)` `[INF]`. The observed
contract is: header shows current index, bar shows done, footer shows remaining. Three renderings of
one counter — and the mismatch in §7.4.

### 9.4 When output exists `[OBS]`
Default policy `Ask`, and the prompt is:
```
Destination already has a file named:
/path/h01_result.gif

Do you want to overwrite it?
[ Yes to all ] [ Yes ] [ No ] [ Rename ] [ Cancel ]
```
`Yes to all` resolves the rest of the batch without asking again (log then shows `Overwritten: 24`).
`Cancel` aborts the batch. `[STR]` a non-prompting policy logs `skipped, file already exists!`;
the batch-level warning `Output filename will be the same for all converted files.` guards a template
with no per-file token.

### 9.5 What can go wrong, and how it is worded `[STR]`
- `Do you want to cancel current batch processing?`
- `Destination already has a file named: %1 / Do you want to overwrite it?`
- `skipped, file already exists!`
- `Output filename will be the same for all converted files.`
- `The 'Use multiple CPU Cores' setting was disabled. This setting can't be combined with 'Ask' mode
  or multipage PDF files.`

Note the pattern: **every refusal names the conflict and the control that caused it**, and none of
them are bare "Error". That is a writing standard worth adopting, and it is consistent with the
repo's existing honesty rule (`OD-21`, never hide what the engine gets).

---

## 10. What gifscythe adopts, adapts and rejects

The table is the deliverable. "Shell" means structure only — no assets, no wording that implies a
still-image product.

| Reference element | Verdict | For gifscythe |
|---|---|---|
| Six tabs, titles carrying counts, bottom `Presets`/primary/`Close` bar | **ADOPT** | Already shipped in slice B; keep the grammar `Input: n file(s)`, `Actions [enabled/total]` |
| Tabs disabled during a run except Status/Settings/About | **ADOPT** | Cheap, and it prevents the "change the job mid-flight" class of bug |
| Primary button `Convert` → `Stop` while running | **ADOPT** | Shipped in slice F; keep `Stop`, not `Cancel` |
| `Actions [enabled/total]` with delete ≠ disable | **ADOPT** | Slice D's counter; the two affordances must be visually distinct |
| Card = collapse + title + `−` + `Enabled` + parameter body | **ADOPT** | Slice D's card widget |
| Slider + spin + reset triple per numeric parameter | **ADOPT** | Adopt as a *widget*, not as a licence to add photo filters |
| Ordered stack, top→bottom, drag to reorder | **ADOPT** | Slice D |
| Add-action menu of 12 groups / 131 actions | **REJECT as-is** | Vocabulary only; gifscythe's actions are gifsicle operations over a stream (§11.3) |
| Preview = one still frame, arrows step *files* | **REJECT** | Ours previews **motion**: play/pause, frame step, duration, loop. The arrows stay for files, but motion needs its own control |
| Input columns (Name…Print size, no animation fields) | **ADAPT** | Keep the 7 file columns; **add Frames, Duration, fps, Loop** |
| Output `Multipage` group, both off by default | **REJECT, invert** | Replace with an `Animation` group whose **preserve-animation default is ON** |
| Format `Same as original` + `Settings…` per format | **ADOPT** | With GIF/APNG/WebP only; `Settings…` opens that format's sliders (`OD-21`) |
| Filename tokens `{Filename}`/`{DstWidth}`/`{Current Date}` | **ADOPT (subset)** | Keep `{Filename}`, `{DstWidth}`, `{DstHeight}`, dates; drop nothing that gifsicle can't deliver |
| `If an output file already exists: Ask/…` + `Yes to all` | **ADOPT** | Same modal, same 5 answers; the freeze-the-batch behaviour is correct |
| `Keep original file when encoded result is larger` | **ADOPT** | Directly useful for GIF→GIF |
| Status log: per-file lines + size delta + summary + `Started at`/`Finished at` | **ADOPT** | Slice A shipped the contract; keep this grammar for the *log*, keep the table for the *grid* |
| Status footer: segmented bar + `Estimated end time` + `Remaining` | **ADAPT** | Adopt bar and wall-clock ETA; **drop the dual percentage** (§7.4) |
| Log persisted to disk | **ADAPT** | Optional export, not a hidden file in the user's home |
| `Use multiple CPU Cores` + its conflict warning | **HOLD** | Depends on engine parallelism; out of slice scope |
| Photo-studio action catalogue, `Film Simulation`, `Fantasy`, DPI/print size, colour profiles | **REJECT** | Stills pipeline; no gifsicle equivalent |
| Page model, `Extracted pages`, `(1)` page indices | **REJECT** | gifscythe's unit is a **frame with a delay**, not a page |

---

## 11. What this means for slices C, E and D (in that order)

### 11.1 Slice C — Input grid
Copy the structure (toolbar, thumbnail grid, sortable columns, Quick filter, green multi-select,
Hot folders) and **change the information model**:

- Keep: `Name · Size · Modified · Created · Type`.
- Drop: `EXIF Date Taken · Properties · Print size · Rating · Colour label` (stills metadata; either
  meaningless or unavailable for our inputs).
- **Add, and make them default-visible: `Frames`, `Duration`, `fps`, `Loop`.** This is the difference
  between "an image grid" and "an animation queue"; without it the tab is a picture converter's tab.
- Keep the tab title `Input: n file(s)`, and add the animation summary in the same grammar
  (`Input: 24 file(s) · 288 frames`) `[INF]` — one extra fact, same shape.

### 11.2 Slice E — Output form
Adopt the group boxes (`Output`, `Filename`, `Format`, `Options`, `After conversion`) and invert the
one group that matters:

- **`Animation` replaces `Multipage`**, and its *first* control is `Preserve animation` — **checked**.
  Under it: frame delay / fps, loop count (`forever` default), and "drop duplicate frames" (a
  genuinely useful gifsicle operation, not a photo filter).
- `Format`: GIF / APNG / WebP only. `Same as original` stays meaningful (GIF→GIF) and should stay the
  default, with `Settings…` enabling per-format sliders (`OD-21 = a`: colour detail % → `-k`,
  extra compression % → `--lossy`, engine argument printed under each).
- Everything in `Options` and `After conversion` transfers as-is, except `Use multiple CPU Cores`
  (hold) and `Preserve color profile` (needs an engine capability check).
- The status of a *saved* file must state frame count, so the log can never hide a dropped animation
  the way `8.50 KiB -> 635 Bytes (-92%)` does.

### 11.3 Slice D — Actions as a card stack
The reference's architecture is right: **a list of actions, each with enable/disable, order, and a
parameter body**, applied to every file and reported per file. Adopt it wholesale, then bound it:

- Card = collapse, title, `−` delete, `Enabled ▾`, body of slider+spin+reset rows.
- Counter `Actions [enabled/total]` in the tab title.
- The **action vocabulary is gifsicle's**, not XnConvert's 131: optimize/compression, resize, crop,
  rotate, colour reduction, delay/time, loop, frame selection, and later the FFmpeg endpoint actions
  once `P3-19` exists. Anything requiring per-pixel raster filters is out of 1.0.0 and needs an owner
  decision, not a port.
- Every action's semantics must be **stream-level**: "resize" means resize *each frame and the logical
  screen*; "rotate" means rotate every frame; "delay" means edit per-frame delays. This is the part
  the reference cannot teach us, because it has no such actions.

### 11.4 The gate that produced this file
`XN_CONVERT_REFERENCE.md` §7 lists C → E → D as the remaining order. This study does not change the
order; it changes **what gets built inside each slice** (§11.1–§11.3), and it removes the last excuse
for guessing: the reference is now documented from the running program, not from four photographs.

---

## 12. Limits of this study

- **No source.** Threading, debounce and internal ordering are inferred from behaviour and from
  surviving slot names, and are marked `[INF]`.
- **String catalogue ≠ UI.** The binary shares a great deal of code with sibling XnView
  (`tabsOnBottom`, `useRecycleBin`, ZIP, `Recipient`). A string existing in the binary does **not**
  mean a control exists in XnConvert; only `[OBS]` items are asserted as UI.
- **Linux build only.** Windows-only dialogs and shell integration were not exercised.
- **Geometry is screen-specific** (window 190…1290 × 170…965 in a 1500×1200 VNC frame); do not treat
  pixel offsets as design values.
- **One unanswered detail:** the page-parenthetical rule in §7.2 and the `x6` vs 12-images mismatch
  (§7.2) are observed but not explained.
- **Vendor licence.** XnConvert is freeware for private/educational use; nothing here is copied, and
  `OD-22`/`N-36` continue to forbid shipping its binaries or assets.

---

## Appendix A — captures (outside the repo, session workspace only)

`_xnconvert/shots/` — 26 full-frame PNGs at 1400×900 or 1500×1200:

| File | Shows |
|---|---|
| `xnc-02-main.png` | shell at rest, 6 tabs, empty Actions |
| `xnc-04-input-list.png` | Input grid with columns |
| `xnc-05-output.png` | full Output form (§6) |
| `xnc-06-settings.png` | Settings (§8) |
| `xnc-07-about.png` | About + notices |
| `xnc-08-addaction-menu.png` | the 12-group add menu |
| `xnc-09-transform-submenu.png` | Transform's 15 actions |
| `xnc-11/12-enabled*.png` | `Enabled ▾` menu and the toggle |
| `xnc-13-state.png` | `−` deleting an action, `[0/0]` |
| `xnc-15-two-cards.png` | Rotate above Resize, `[2/2]` |
| `xnc-18-status-run.png` | completed Status log (7 files) |
| `xnc-22-40.png` | 40-file grid, `1/40` preview stepper |
| `xnc-26-midrun.png` | **live** run: `13 of 24`, partial segmented bar, ETA, `Stop` |
| `xnc-25-running.png` | overwrite modal freezing the batch |

No screenshot is committed (`N-36`); this appendix is an index, not a payload.

## Appendix B — reproducing the study

```bash
# unpack (no 7z binary available)
python3 -m py7zr x "XnConvert-linux-x64.7z.001" -o _xnconvert/linux
python3 -m py7zr x "XnConvert windows.7z"          -o _xnconvert/windows

# run the real binary headless-but-real (Qt vnc platform, stubbed GL/dbus)
cd _xnconvert/linux/XnConvert
LD_LIBRARY_PATH=/tmp/xstub:$PWD/lib:$PWD/Plugins \
XDG_CONFIG_HOME=$PWD/../../run/config LANG=C.UTF-8 \
QT_QPA_PLATFORM="vnc:size=1500x1200:port=5901" ./XnConvert <files...>

# capture (SetPixelFormat + Raw are mandatory, or the frames stripe)
python3 analysis/vnc.py shot out.png
python3 analysis/vnc.py click 1149 944     # bottom bar; Alt+C also starts a run
```

Kill by `pkill -f "\./XnConvert"` (the argv ends with file paths, so `XnConvert$` never matches), and
always confirm the port is free before relaunching — otherwise the new instance silently fails to
bind and you photograph a stale window.
