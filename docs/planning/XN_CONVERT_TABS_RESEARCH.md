# XnConvert tabs, researched — and the ScreenToGif engine that backs them

**Owner's instruction (2026-10-24):** *"research what XnConvert do in input, action, output,
status tab in original XnConvert but you can change it with screentogif function I ask time and
time again use screentogif function and feature but make it almost 1:1 copy to XnConvert but
adapted to my project."*

So there are two authorities and they answer different questions:

| Question | Authority |
|---|---|
| What does each tab **look like** and **do**? | **XnConvert 1.118.0**, driven live (`XN_CONVERT_PROGRAM_STUDY.md`) + the owner's four screenshots |
| What can we **encode/decode**, and with which settings? | **ScreenToGif fork** (MS-PL, `freeforall1932-design/ScreenToGif-fork`) — its export presets are the battle-tested recipes |
| What do we actually **ship today**? | gifsicle 1.96 for GIF; an FFmpeg sidecar is *not* shipped (`OD-20 = a`, parked) |

This file is the research the owner asked for before slice D, and the mapping every later slice
follows. Facts marked `[OBS]` were seen on screen; `[SRC]` are read out of the ScreenToGif fork at
`master`; `[PLAN]` is what this project will do.

---

## 1. Input tab

**XnConvert `[OBS]`** — toolbar (`Add files…`, `Add folder…`, `Remove`, `Remove all`), a funnel +
`Quick filter`, a columns menu, a **thumbnail grid** (≈100 px cells, name centred, selected cell
green), column headers that double as sort keys (`Name · Size · Modified date · Created date · EXIF
Date Taken · Properties · Print size · Type · Rating · Colour label · Check`), `Hot folders…` at the
bottom, and the count in the tab title (`Input: 24 file(s)`). Every column is a **still-image**
property — there is no Frames/Duration/loop column anywhere, because XnConvert treats an animation
as page 1 of a multipage image.

**ScreenToGif `[SRC]`** contributes nothing structural here — it is a *recorder*, so its "input" is a
capture session, not a file queue. Its only relevant lesson is negative: it does not pretend to be a
file manager either.

**gifscythe `[PLAN]`, delivered in slice C:** the same shape, with the columns replaced by
`Queue order · Name · Size · Modified · Frames · Duration` and `Frames · Duration · fps · Loop` in the
cell caption and tooltip. `Add folder…` walks one level through the same admission rule as a drop.

**Not copied, on purpose:** `Hot folders…` (a watcher is a background feature with its own failure
modes, not a view) and the columns menu (no configurable column model).

---

## 2. Actions tab

**XnConvert `[OBS]`** — a stack of **cards**, top to bottom, each:

```
[▾] Title                         −  ⧉  ↻      Enabled ▾
    label  [slider———————]  [ 5 ]  (orange reset)
```

- `Add action >` opens 12 groups → 131 actions; `Clear all` empties the stack.
- The tab title counts **enabled/total** (`Actions [5/5]`); disabling a card leaves it visible and
  drops the counter to `[4/5]`.
- The `−` **deletes** the card; `Enabled` **disables** it. Two different affordances, and the
  counter counts enables.
- Every numeric parameter is the same triple: **slider + spin box + reset**. The reference's
  `Photo Spread` card shows it exactly: `Count [——] 2`, `Angle [——] 0.00`.
- Cards are reorderable by a drag handle; insertion order is stack order.
- The **Preview lives in this tab**, right-hand pane, with `Before` / `After` toggles.

**ScreenToGif `[SRC]`** is where the *features* differ: it edits a frame list (timeline, selection,
per-frame delays), which is an **editor** and is explicitly out of scope for gifscythe
(`OD-20 = a`). What we take from it is the **encoder capability list**, below.

**gifscythe `[PLAN]` — slice D:** one card per action group (Mode, Optimize, Resize, Geometry, Crop,
Animation, Colors, Metadata), each with collapse / `−` / reset / `Enabled ▾`, an `Add action >` menu
that re-adds a removed card, `Clear all`, and `Actions [enabled/total]` in the tab title. Parameter
rows keep the slider+spin+reset idiom where a range exists. A disabled or absent card contributes
**nothing** to the command — the honest meaning of "disabled".

**Honest divergence:** we do **not** claim free reordering. gifsicle applies its operation set in a
fixed pipeline, so a drag-handle that reorders nothing would be a lie; cards are ordered by the
pipeline and the UI says so.

---

## 3. Output tab

**XnConvert `[OBS]`** — group boxes: **Output** (`Source folder` / `Folder` + browse), **Filename**
(`{Filename}_result` + tag menu, `Case`, `Start index`), **Format** (combo + `Settings…`),
**Options** (`If an output file already exists` = `Ask`, keep-original-when-larger, keep folder
structure/parent, keep date/time, preserve extension/metadata/colour profile, delete original,
`Use multiple CPU Cores`), **Multipage** (two boxes, **both off by default** — this is the switch that
silently flattens animations), **After conversion** (`Open File Manager`, `Clear the 'Input' file(s)`,
`Beep at the end`, `Exit`).

### 3.1 The formats, and where each encoder comes from

**ScreenToGif `[SRC]`** — `ScreenToGif.Model/Enums/ExportFormats.cs`:

> `Gif, Apng, Webp, Bpg, Avif, Avi, Mkv, Mov, Mp4, Webm, Bmp, Jpeg, Png, Stg, Psd, Zip`

and `EncoderTypes.cs`:

> `ScreenToGif // Gif, Apng` · `KGySoft // Gif` · `System // Gif, Video` · `FFmpeg // Gif, Webp, Apng, Avif, Video` · `Gifski // Gif`

So ScreenToGif's answer to "which encoder?" is **FFmpeg for everything animated except its own
in-house GIF/APNG encoders**, with Gifski and KGySoft as alternative GIF encoders. That is the
architecture we adopt — with **gifsicle kept as our GIF encoder** (it is pinned, it is what this
project's whole contract is built on), and FFmpeg used for APNG / WebP / video.

The parameter recipes are not guesses; they are the presets' own strings `[SRC]`:

| Format | ScreenToGif preset file | Recipe (verbatim) |
|---|---|---|
| GIF | `AnimatedImage/Gif/FfmpegGifPreset.cs` | `-vsync passthrough {I} -loop 0 -lavfi palettegen=stats_mode=diff[pal],[0:v][pal]paletteuse=new=1:dither=sierra2_4a:diff_mode=rectangle -f gif {O}` |
| APNG | `AnimatedImage/Apng/FfmpegApngPreset.cs` | `-vsync passthrough {I} -pred mixed -plays 0 -pix_fmt rgba -f apng {O}` |
| WebP | `AnimatedImage/Webp/FfmpegWebpPreset.cs` | `-vsync passthrough {I} -c:v libwebp_anim -lossless 0 -quality 75 -loop 0 -f webp {O}` |
| Video | `Video/Codecs/*.cs` | one codec class per encoder: `X264, Vp9, LibAom, SvtAv1, Rav1E, Mpeg4, Mpeg2, Vp8, H264Nvenc/Qsv/Amf, HevcNvenc/Qsv/Amf` |

`{I}` = input, `{O}` = output; one filter argument per line is their convention.
`[PLAN]` we ship those recipes as the catalogue, not as free-text fields.

### 3.2 gifscythe today, stated plainly

| Format | Available | Why |
|---|---|---|
| GIF / "Same as original" | **yes** | gifsicle 1.96, pinned |
| APNG, animated WebP | **no — disabled in the combo** | needs the FFmpeg sidecar |
| MP4 (H.264 / H.265), WebM (VP9 / AV1) | **no — disabled** | needs the FFmpeg sidecar |
| Video *in* (mp4 → gif) | **no** | same sidecar (`OD-20 = a`, parked behind `P3-19`/`U-90`) |

The combo lists them and refuses them, with the reason on hover: a selectable format that writes
nothing would be the exact failure this project keeps deleting.

---

## 4. Status tab

**XnConvert `[OBS]`** — this is a **log sink**, not a table:

- `Processing list: 13 of 24 files` (k = the file in flight), then `Started at: …`.
- Two lines per file plus a size delta:
  `…/h07.gif (1) 1200x1200x6 : loaded` / `…/h07_result.gif : saved` / `276.99 KiB -> 16.17 KiB (-94%)`.
- A summary block: `Input files`, `Extracted pages`, `Overwritten`/`New files`, total input/output
  size, `File size ratio`, `Total Duration`, `Finished at`.
- A **segmented** progress bar (one block per file), `Estimated end time` (wall clock) and
  `Remaining: 12 file(s) - 50% - 7 second(s) remaining`.
- The primary button becomes **`Stop`** while running; Input/Actions/Output are disabled.
- The whole tab is mirrored to `~/.xnconvert/batch_convert_log.txt`.

**gifscythe `[PLAN]`** — keeps the *grammar* (header counter, per-file lines with size delta, summary,
progress, `Stop`) in the **Status tab**, and keeps the per-operation **table** from `W-32` on the same
tab, because a table is the only place where a failed row can honestly show no byte figures. The
command preview and activity log now live here too (they used to sit under every tab, which is not
the reference's chrome at all).

**Deliberate divergence:** one percentage, not two. XnConvert can show a bar at 8 % and
`Remaining … 96 %` at the same instant (bar counts completed, text counts remaining including the
file in flight). We use the bar, plus `n of m`, and no second percentage.

---

## 5. The one control that must be inverted

XnConvert's **Multipage** group is off by default and is the switch that decodes an animation into
pages; with it off, a 6-page GIF comes out as 1 page `[OBS]` (`276.99 KiB -> 16.17 KiB`, −94 % — the
frames were **dropped**, not compressed). gifscythe exists to do the opposite, so its Output tab
carries an **`Animation`** group whose *preserve animation* is **on** by default, and whose controls
mirror ScreenToGif's real exporter options (`-loop 0`, per-frame delay, `-plays 0` for APNG,
`-loop 0` for WebP).

---

## 6. What "almost 1:1" means here, precisely

| Element | XnConvert | gifscythe | Status |
|---|---|---|---|
| Six tabs, counts in titles | yes | yes | **1:1** |
| Input grid + toolbar + filter | yes | yes, animation-aware columns | 1:1 shell, adapted data |
| Actions card stack | yes | yes (slice **D**, `actionCard_0..7`) | **1:1** shell, gifsicle-bounded actions |
| Preview inside Actions, Before/After | yes | yes (owner's correction, 2026-10-24) | **1:1** |
| Footer: `Presets:` + Convert/Close | yes | yes (presets are real files) | **1:1** |
| Output: groups + Format + Settings… | yes | yes, formats the build can write stay enabled | 1:1 shell, honest capability |
| Status: log grammar, Stop, progress | yes | yes + the W-32 table | 1:1 shell, plus a table |
| Multipage (drops animation) | on by default, off | **inverted** — `Animation`, on by default | **deliberate** |
| 131 photo actions | yes | no — gifsicle's operations only | **scope** |
| Recorder / timeline / editor | no (not its job) | no (ScreenToGif's editor scope is out) | **scope** |
| Encoding catalogue | per-format dialogs | Gif (gifsicle) + the ScreenToGif FFmpeg presets when the sidecar lands | **staged** |

---

## 7. Limits of this research

- `[OBS]` facts come from the Linux 1.118.0 build in the session that produced
  `XN_CONVERT_PROGRAM_STUDY.md`; the unpacked tree is not in the repo (size budget), so re-verify by
  re-extracting the owner's archives.
- `[SRC]` facts are from `ScreenToGif-fork@master`; the parameter strings are the presets' defaults
  and can be edited by users in ScreenToGif, so they are *recipes we adopt*, not a frozen ABI.
- Nothing here authorises copying ScreenToGif code into this tree; the checklist for when it might
  is in `legal/README.md`.

---

## 8. What slice D actually shipped (2026-10-24)

Delivered against §2, element by element:

- **One card per gifsicle operation group**, in the engine's fixed order:
  `Mode`, `Optimize`, `Resize`, `Geometry`, `Crop`, `Animation`, `Colors / Transparency`,
  `Metadata`. Object names `actionCard_<0..7>`; each header carries `actionCardChevron_<id>`
  (fold), `actionCardRemove_<id>` (`-`), `actionCardReset_<id>` (`↺`) and
  `actionCardEnabled_<id>` (`Enabled`). The group box keeps its own title **stripped**, because the
  header already says it (`ActionCard::setBody`).
- **The two verbs stay two verbs.** `-` takes the card out of the stack (counter shrinks on both
  sides, e.g. `[7/7]`), `Enabled` keeps it and drops only the numerator (`[7/8]`). A card that is
  out or off contributes **the default of its fields**, i.e. nothing: `writeInto()` copies the
  default value of that card's fields over whatever the controls hold, so a typed-then-disabled
  value cannot leak back into the command. `writeSessionInto()` deliberately bypasses that gate, so
  a disabled action still comes back with its values next launch.
- **Removing never destroys a control.** The body is parked in `bodies_`, re-parented to the panel
  and hidden; `Add action >` re-attaches the same widget, so every object name and every value
  survives (the harness contract is not negotiable). `Reset` restores the *construction* defaults
  captured once into `defaults_`.
- **Toolbar:** `Add action >` (a menu that greys out the actions already in the stack, plus
  `Add all actions`), the `Actions [on/total]` label, and `Clear all` on the right. The same
  `Actions [on/total]` string rides in the **tab title** (`MainWindow::refreshTabTitles`).
- **Persistence:** the stack travels as one `card_state = id:present:enabled:collapsed;…` GUI key in
  the session file **and** in every preset `.conf`, so a preset really is an action set (XnConvert's
  own preset model), not just a bag of numbers.
- **Divergences kept, on purpose (§6):** no drag-reorder (the gifsicle pipeline order is fixed and
  the UI does not pretend otherwise — the intro text says so), no per-action sliders
  (spin + reset is the whole affordance), no 131 photo actions, no recorder.

Proof: `tests/test_gui_offscreen.cpp` **T30 / T30b** — counter and title, per-card affordances,
fold, disable (flags leave the command, others stay), reset (this card only), remove + re-add
(values intact), `Clear all`, and a restart that restores presence, enabled state and the values of
a disabled card. Run by CI only: there is no Qt SDK in the agent sandbox.

---

## 9. Naming and storage, measured (2026-10-26)

Re-read from the shipped 1.118.0 binary's own strings, because the Output tab's naming was the
owner's next priority and memory is not evidence:

| Element | The reference's own symbols / strings | gifscythe before this slice |
|---|---|---|
| Filename field | `leFilename`, `tbFilename`, `cbFilename`, `filenameChanged` | `outputEdit` ("Save as…", a literal path) + a separate `name_template` |
| Template tokens | `{Filename}`, `{Filename With Ext}`, `{File Type}`, `{File Index}`, `{Width}`, `{Height}`, `{DstWidth}`, `{DstHeight}`, `{Directory}`, `{Folder name}`, `{Format}`, `{Size KB}`, `{Modified Date}`, `{Current Date [...]}`, plus EXIF/Rating/Categories/Annotation | `{name}` only |
| Default template | `{Filename}_result` (`_result` also exists as a bare default-suffix string); an example is `{Filename}_{Current Date [m_d_Y]}` | `{name}_opt.gif` |
| Extension | supplied by the Format, never typed in the template | typed into the template, then force-appended `.gif` |
| Start index | `sbStartIndex`, `startIndex`, `onIndexChanged` | - |
| Keep folder structure | `cbKeepFolder`, `keepFolder`, `keepParentFolder` | - |
| Existing file | `cbOverwrite`, `overwriteQuestion`, `onThreadOverwrite`; strings `Skip`, `skipped, file already exists!`, `Destination already has a file named: ` | refuses the whole plan up front |
| Destination | `rbSrcFolder` ("Source folder") / `rbDstFolder` ("Destination"), `wDstFolder`, `changeFolder` | one `batch_dir`, empty = beside each input |
| Constant name | `Output filename will be the same for all converted files. ` | refuses a template without `{name}` |
| Presets | `Presets:` ribbon, `lePreset`, `tbPresetSave`, `tbPresetDel`, `Preset name:`, `.xbs` files, `xnconvert.ini` | `presets/*.conf` + `gifscythe.conf` |

**The date letters are XnView's, not Qt's and not strftime's.** The four example templates in the
binary (`[m_d_Y]`, `[d_m_Y]`, `[Y-m-d_H-M-S]`, `[Y-m-d_H-M]`) are self-consistent under exactly one
reading: `m` = month, `M` = minute. Under Qt's letters `[Y-m-d_H-M-S]` would be
year-minute-day_hour-month-second. `src/core/NameTemplate.h` follows the measured reading and says so
in the code.

### 9.1 The owner's decisions (2026-10-26)

1. **Tokens:** ship the full set, but the **default is `{Filename}_result`**.
2. **Duplicates are numbered, not refused:** if two inputs render `go_result.webp`, the first one to
   be processed keeps the plain name and the others become `_result_1`, `_result_2`, ... — case-only
   collisions count, because Windows and macOS would otherwise lose a file.
3. **The Format owns the extension** (`OD-24 = a`); a template that still carries one loses it, with a
   visible note.
4. **"Save as…" is dropped** (`OD-25 = a`): Destination + Filename explains every mode.
5. **Storage:** whatever fits this project without bugs (`OD-26`); the payload is reshaped rather than
   the file format thrown away.
6. **Typing `{` offers the tokens**, with a scrolling list, filtering as the user types
   (`{d` → Duration, `{fi` → Filename / File Index / File Type ...).

### 9.2 What landed in this pass, and what did not

- **Landed, and proven here:** `src/core/NameTemplate.h` — the token renderer, the format-owned
  extension rule, `template_is_constant()`, `template_uses_index()`, and `disambiguate_names()`;
  pinned by `tests/test_name_template.cpp` (**60 checks**, run by `build.sh` and ctest, on every
  platform). Two real bugs were found by that suite before any GUI code existed: the date letters
  above, and a numbering step that could steal a literal name (`{a, a, a_1}` must give
  `a, a_2, a_1`, not `a, a_1, a_1_1`).
- **Not yet wired:** the Output tab still shows the old pair (Save-as + `{name}` template). The
  service that will use this header (`MainWindow::renderedOutputName`, `plannedBatchTargets`) and the
  completion popup are the next commit — the rule is already frozen and tested, so that work changes
  UI only, not naming semantics.
