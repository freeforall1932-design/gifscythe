# XnConvert reference model — the target UI, transcribed (owner-supplied, 2026-10-22)

**Why this file exists.** On 2026-10-22 the owner supplied four screenshots of XnConvert — Input,
Actions, Output, and Status mid-run — after stating that the UI work had been aimed at "the wrong
model". This file is the authoritative transcription of *that* model, and the measured gap between
it and Gifscythe today. It is text-only on purpose (see §5).

**Correction to the record.** `UI_REVIEW_S41.md` §3 said the fourth tab being **Guide** rather than
**Status** was "not a regression", and treated the Status tab as a mere future item. That was
backwards: in the reference, **Status is a live tab during a run**, and it is central to how the
product reports work. The review is corrected in the same commit that adds this file.

## 1. Input — the file grid

- Toolbar: `+ Add files…`, `Add folder…`, `Remove`, `Remove all`, and a **Quick filter** text box with a
  filter-shaped dropdown.
- A **thumbnail grid**, five cells across, each cell: the image itself, the file name underneath
  (ellipsized), and small per-cell action icons.
- Column headers, which the grid sorts by: **Name · Size · Modified date · Created date · EXIF Date
  Taken · Properties · Print size**.
- `Hot folders…` checkbox, bottom-left.
- Footer strip: `Presets:` combo + save/delete, `Export for NConvert…`, then `Convert` and `Close`.
- The tab title itself carries the count: **`Input: 10 file(s)`**.

## 2. Actions — a stack of cards, preview alongside

- Left pane, titled `Actions [5/5]`, header buttons `+ Add action >` and `Clear all`.
- A scrollable stack of **action cards**. Each card is a collapsible group with: move up / move down,
  delete, and an **Enabled** checkbox on the right (with a dropdown for further options).
- Each parameter is a **slider + numeric spin box + a reset button**, not a bare spin box:
  `Photo Spread` → Count, Angle; `HLS` → Hue, Lightness, Saturation (+ a `Colorize` checkbox);
  `Spread` → Intensity; and so on, one card per action, five stacked.
- Right pane: `Preview`, with `Before` / `After` **toggle buttons** (Before active in the shot), a large
  canvas, zoom controls (fit / − / +), page-flip arrows, and a position/zoom readout `1/2 [41%]`.
- A small ▶ collapse handle sits between the panes.

## 3. Output — one dense form

- **Output**: radio `Source folder` / `Folder`, a folder path combo + `…` browse, and a small
  `Folder ▾` split button on the left.
- **Filename**: a template combo showing `{Filename}_result`, a `>` button to insert a tag, and a
  **Case** combo (`No change`).
- **Format**: combo (`As original`) + `Settings…` (disabled because "as original").
- **Options**: `When output files already exist` combo (`Ask`), plus checkboxes `Keep folder
  structure`, `Keep original date/time attributes`, `Preserve Metadata` (checked), `Preserve color
  profile`, `Delete original`.
- **Multipage**: `Convert multipage file to multipage file (when possible)`, `Convert all pages from
  multipage file` (both disabled for a non-multipage source).
- **After conversion**: `Open file explorer`, `Clear the 'Input' file(s)`.
- Footer: `Presets:`, `Export for NConvert…`, `Convert`, `Cancel`.

## 4. Status — the live run report

- Tab title while running: the window title reads **`XnConvert − 21% − 19 second(s) remaining`**.
- Header line: **`Processing list: 9 of 38 files`**.
- A monospaced, scrollable log with a `Started at:` line, then **two lines per file**: the source with
  its dimensions (`…5824x8736x24 : loaded`), the destination (`…_result.jpg : saved`), then a size
  delta line (`12.60 MiB -> 2.34 MiB (-81%)`).
- A green progress bar under the log.
- Bottom row: `Estimated end time: 0:27` on the left; **`Remaining: 30 file(s) − 79% − 19 second(s)
  remaining`** on the right.
- Footer: the primary button is **`Stop`** while running (not `Cancel`, and not `Convert`).

## 5. What the four references are, and where they live

| Attachment | Tab shown |
|---|---|
| `com.xnview.XnConvert-461adbcd7cdb0e2a7e01b211c63fb92c.png` (1252×706) | Actions, with Preview |
| `xnconvert-convert-heic-files.jpg` (719×621) | Input grid |
| `4806388_6.jpg` (871×584) | Output |
| `xnconvert-big-3.png` (1394×758) | Status, 21% into a 38-file run |

They are **not committed to this repo**: `N-36` records the owner's decision that the repo holds no
image files, and `OD-22` (may XnView/XnConvert material live here?) is still unanswered with `(a)
text-only` on file as the recommendation. This transcription satisfies that recommendation. If the
owner wants the PNGs themselves in-tree, that needs `OD-22` answered and an explicit `N-36` exception
— two decisions, not a copy-paste.

## 6. The gap, measured against the code

| Reference element | Gifscythe today | Gap |
|---|---|---|
| Tabs: Input · Actions · Output · **Status** · Settings · About | Input · Actions · Output · **Guide** | **Status and Settings tabs missing**; Guide is an extra the reference does not have |
| Tab titles carry counts (`Input: 10 file(s)`, `Actions [5/5]`) | None | missing |
| Input: thumbnail grid, 7 sortable columns, Quick filter, Add folder, Hot folders | Flat text list (`queueList`) + Add/Remove/Clear + Move Up/Down | **large** — needs a model item + view + tests |
| Actions: stack of cards, each enable/disable/reorder/delete, slider+spin+reset per parameter | One fixed settings form (`SettingsPanel`) | **architectural** — Gifscythe has *one* settings model, not a list of actions |
| Preview inside the Actions tab, Before/After + zoom + page readout | Separate splitter pane, Before/After + Play/Stop + savings | close in spirit; placement and zoom differ |
| Output: Format, Case, existing-file policy, metadata/colour-profile, Delete original, After-conversion | Mode combo, output path + browse, batch dir, summary | **mostly missing** — the reference's fields are format-agnostic where Gifscythe's are gifsicle-specific |
| Status: per-file log with size deltas, `n of m`, ETA, progress, Stop | Status label + progress bar + activity log in the bottom strip; `W-32` per-file model still OPEN | **the model exists on paper only** (`W32_RESULT_CONTRACT.md`, 14 cases) |
| `Stop` while running | `Cancel` (cancel latch, U-72) | wording + semantics to reconcile |

**The honest summary:** two of the four reference tabs are close in *spirit* (Actions+Preview,
Output) and far in *detail*; two (Input grid, Status) do not exist as designed. This is not a
restyle — the reference assumes an **action-list processing model** (N actions applied to N files,
reported per file), where Gifscythe today has **one settings set per run**. That is the real
distance, and it is why the previous sessions kept producing screenshots of "the wrong model".

## 7. Proposed slices, cheapest first

Each slice is independently shippable and keeps the harness contract (objectNames named by T1–T26)
intact, with its own tests.

| # | Slice | Why first | Cost | Gate |
|---|---|---|---|---|
| **A** | **Status tab** (per-file results, `n of m`, size deltas, ETA, `Stop`) | The contract already exists (`W32_RESULT_CONTRACT.md`, 14 cases) and the reference shows exactly what it must contain. It also retires `W-32`. | medium | none — `W-32` is already the registered item |
| **B** | Tab set + tab titles with counts (`Input: n file(s)`, `Actions [n/n]`), `Settings`/`About` | Mechanical, visible, cheap; moves the chrome onto the reference's grammar | small | none |
| **C** | Input grid: thumbnails + columns + Quick filter | Large but self-contained; needs a `QAbstractTableModel` over the existing queue | large | none |
| **D** | Actions as a card stack (enable/disable, reorder, per-parameter slider+spin+reset) | The architectural one: turns the settings model into an action list | very large | **`OD-21`** |
| **E** | Output form: Format + Case + existing-file policy + metadata + after-conversion | Needs `OD-21` (preset/format mapping) and a gifsicle-format decision | medium | **`OD-21`** |
| **F** | `Convert`/`Stop` wording and footer layout | Cosmetic, touches the cancel-latch tests | small | none |

**Recommended order: A → B → F → C → E → D.** A is the biggest *honest* win per unit of risk: the
model is already specified, it is what image 3 shows, and it is the one thing an owner looking at
these four screenshots would miss most.

## 8. What this file does not claim

- That the reference's *engine semantics* transfer: XnConvert is a general image converter (dozens of
  formats, multipage, NConvert export); Gifscythe drives one engine (gifsicle) for GIF-to-GIF. The
  **layout and reporting** transfer; the **field set** does not, and several Output fields have no
  gifsicle equivalent (`Case`, `Print size`, multipage).
- That any of this is scheduled: slices D and E are gated by `OD-21`, which is the owner's to answer,
  and the whole route is one `OD-21`/`W-32` decision away from being unblocked.
