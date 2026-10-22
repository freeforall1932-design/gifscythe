# UI review — S41, before merging PR #17

**Requested by the owner:** review the UI before merging. **Reviewed at:** `f5b7d7a`, PR #17, CI run `39810897814` (all eight jobs green, `ci-ok` included).
**Scope:** every change on this branch that a user can *see*, plus the UI surface around them.

## What this review can and cannot see

- **Can:** the real widgets, rendered by the real Qt on both CI platforms (`offscreen`), sampled
  pixel by pixel, plus the full assertion suite (T1–T26) and its event-loop timings.
- **Cannot:** desktop appearance, window-manager chrome, animation smoothness, HiDPI scaling,
  a dark theme (`D-06` is not implemented), or a human judgement of "looks right". The PNGs are
  attached to run `39810897814` as `ui-screenshots-linux` and `ui-screenshots-windows` — this
  sandbox cannot fetch artifact blobs (they redirect to storage outside the allowed hosts), so
  **the images are yours to eyeball; the numbers below are what I could verify**.

## 1. The preview canvas — dark, derived from the palette (owner's call)

**Owner's decision (2026-10-22):** keep the dark film strip; do not keep the light canvas. The
"Before" column below therefore describes the original code, and "After" describes the shipped fix,
which restores the look without the literal.

| | Before (QL006 literal) | After (owner's call) |
|---|---|---|
| Canvas fill | fixed `#202020` | `mediaCanvasPalette()`: the palette's darker role lifted 13% toward the other → `#212121` **measured in CI, both platforms** |
| Hint text | fixed `#909090` | `QPalette::Mid` → `#b8b8b8` |
| Hint contrast | 5.10:1 | 8.12:1 (computed; AA/AAA pass, where the old value only scraped AA) |
| Measured corners (both platforms) | `#202020` hard-coded | **`#212121` ×4**, run `39810897814` at `f5b7d7a` |
| Dark mode (`D-06`) | impossible — literal overrides the palette | follows the theme automatically |

The earlier light-canvas measurement in this file's history is kept below rather than deleted,
because it is what caught the two broken attempts.

## 1a. The rejected alternative — a light canvas

| | Before | After |
|---|---|---|
| Canvas fill | fixed `#202020` (dark) | `QPalette::Base` — `#ffffff` in the light palette |
| Hint text | fixed `#909090` | `QPalette::Mid` — `#b8b8b8` in the light palette |

Measured on **both** platforms at this commit:

```
(ui) tabs=Input>Actions>Output>Guide | palette base=#ffffff text=#000000
     | label text="no file selected" px=#ffffff/#ffffff/#ffffff/#ffffff/#b8b8b8 (622x180)
     | canvas px=#ffffff/#ffffff/#ffffff/#ffffff/#b8b8b8 (624x180)      [windows]
     | label ... px=#ffffff/#ffffff/#ffffff/#ffffff/#ecf8fe (426x180)
     | canvas px=#ffffff/#ffffff/#ffffff/#ffffff/#ffffff (428x180)      [linux]
```

Four corners plus centre; `#b8b8b8` / `#ecf8fe` at the centre is the hint glyph itself.

**This is the decision the owner should make.** The old dark canvas is gone, on purpose — it was
the QL006 finding, and it is why dark mode could never work. But it is a *look* change: a GIF with
transparency or pale frames now reads against white instead of near-black. If the dark "film
strip" was wanted, say so and I will restore a dark canvas **from a palette role** (so the lint and
dark mode both stay satisfied) rather than the old literal.

### The measurement earned its keep — twice

Neither wrong version would have been caught by any test in the suite, because **no test asserts a
pixel**. Both measured `#000000` (an unpainted canvas) on both platforms:

1. `setBackgroundRole(Base)` + `setAutoFillBackground(true)` — Qt skips `autoFillBackground` for a
   widget with `WA_OpaquePaintEvent`, which the label sets, so nothing painted it.
2. A stylesheet of `background: palette(base)` — the QSS role did not resolve to a fill (its
   `color: palette(mid)` half did work, which is what made the mistake look plausible).

The fix is palette roles with no stylesheet and no `WA_OpaquePaintEvent`. Recorded in `N-52` so the
next session does not re-invent either failure.

## 2. Cancel now says a flat "Cancelled." — reviewed, and honest

`AUD-02` (`N-48`) removed the N-10 "frames already written may be incomplete" text. That text
described the old behaviour, where Explode wrote frames straight onto the user's prefix and a
cancel truncated them. Explode now writes to a claimed `*.gs-partial[.N]` prefix and promotes only
on success, so **a cancel leaves the previous frame set untouched** and "Cancelled." is the
truthful message. Asserted by harness T21, which also checks the previously promoted frames are
still intact.

## 3. The tab shape is Input → Actions → Output → **Guide** — CORRECTED 2026-10-22

> **This section was wrong and is corrected here.** It said the fourth tab being `Guide` rather than
> `Status` was "not a regression", and treated the drop-in's Status claim as the thing to distrust.
> On 2026-10-22 the owner supplied four screenshots of XnConvert showing **Status as a live tab**
> (`Processing list: 9 of 38 files`, per-file size deltas, ETA, `Stop`). The drop-in was right and
> this review had it backwards. The authoritative transcription and the measured gap now live in
> `docs/planning/XN_CONVERT_REFERENCE.md`.

What actually ships today is a **Guide** tab in the fourth slot, and a batch reports **one status
line plus the log pane**. Against the reference that is a **gap, not a design choice**: `W-32`
(Status tab / per-file results) is OPEN, its contract is proposed in
`docs/planning/W32_RESULT_CONTRACT.md`, and it is slice **A** in the reference's proposed order.

The Guide content itself is not wasted: it is plain-language help for gifsicle's terms, which the
reference has no equivalent of (XnConvert ships a Help button instead). Whether it stays as a tab or
becomes a Help dialog is part of slice **B** in `XN_CONVERT_REFERENCE.md` §7.

## 4. Things I checked and found unchanged

- **objectNames**: no harness-visible widget was renamed by `N-52`; T1–T26 stay green on both
  platforms, so the selection contract still holds.
- **Layout**: the canvas measures 622–624×180 px on Windows and 426–428×180 on Linux, i.e. it takes
  the width the splitter gives it while holding its 240×180 minimum. No clipping observed in the
  sampled frames.
- **UI-thread responsiveness** (T26, the standing guard for `OD-19 = a`): worst gap 11 ms on
  Windows, 10 ms on Linux, against a 2500 ms assertion. Unchanged by this PR, but re-measured
  because the preview code was touched.
- **Hint contrast**: mid-grey on white is low contrast, as was `#909090` on `#202020`. It is
  placeholder text ("no file selected"), not content, so this is noted rather than changed — a
  palette or `D-06` decision, not this PR's.

## 5. Verdict

**No UI regression found in the final state**, with one caveat that is the owner's call, not a
defect: **the preview canvas is now light where it used to be dark** (§1). Everything the harness
asserts is green on both platforms, `ci-ok` is green, and the two subtle failures this review did
find were both introduced and fixed inside this branch, with their measurements kept as the record.

**Recommended:** merge this PR (it is a correctness PR, not the redesign). The owner's call on §1 is implemented and measured: the dark film strip is
back (`#212121`, one step off the old `#202020`), it is derived from palette roles rather than typed
in, and the hint text is more legible than it has ever been. **The owner is reviewing the PNGs
themselves before the merge** — that is the one judgement a pixel measurement cannot make.

## 6. The reference's grammar landed — slices A + B + F (2026-10-23)

`XN_CONVERT_REFERENCE.md` costed six slices; the owner chose **A + B + F as one batch** and
**`OD-21 = a`** (the Output tab follows the reference, which unblocks D and E later). This section
records what a reviewer can see now, and what is still deliberately missing.

**B — the tab set.** Six tabs, in the reference's order: `Input: 0 file(s)` · `Actions` · `Output` ·
**`Status`** · `Settings` · **`About`**. The old `Guide` tab did not disappear; it became **About**,
with a version/licence block above it, and kept the `guideScroll` / `guideIntro` /
`guideDescription` objectNames, so every probe written against the old tab still finds its widget.
`Settings` shows the three app-level facts the probes already produce — the resolved engine path,
the version it reports, and the output folder with a working *Open folder* button. The Input title
carries its count and follows the queue as files are added or cleared.

**A — the Status tab.** The `W-32` contract is now implemented, not just written down:

| Contract clause | What the tab does |
|---|---|
| one row per **operation** | Batch installs one row per queued file; Merge/Auto install one row whose *Before* is the sum of its inputs; Explode installs one row whose *After* is the sum of its frames |
| rows come from the **plan** | `beginResultRows()` runs after `planBatch()`, not from the live queue — editing the queue mid-run cannot desynchronise the table from what is actually running |
| states | `pending` → `running` → `succeeded` / `failed` / `cancelled`; `skipped` = never attempted; `refusedBeforeRun` = the planner refused before any process started |
| named reasons | `engineFailed: exit N`, `engineNotStartable: …`, `outputMissing: …`, `outputInvalid: …`, `outputUnverifiable: …` |
| no figures on dishonesty | a non-`succeeded` row shows **no** After size and **no** delta — the contract's rule, asserted in T27 |
| stop-on-first-failure | preserved; later rows become `skipped` with "not attempted", completed rows keep their figures |
| `n of m` | `Processing list: n of m files` above the table, plus a `succeeded · failed · skipped · cancelled` summary line |

**F — the footer.** The primary control reads **`Convert`**; the cancel control reads **`Stop`**
while a run is in flight and **`Cancel`** when idle. The semantics did not change: `U-72`'s latch
still stops *this file*, and the app still refuses to claim it is cancelling the whole run — which is
why the tab labels the row `cancelled` / "stopped by the user" rather than promising a rollback.

**What a reviewer should still expect to be missing** (the remaining slices, in the owner's order
C → E → D): the Input tab is still a flat list, not a thumbnail grid with sortable columns and a
quick filter; Actions is still one settings form, not a stack of enable/disable cards; the Output
form is still gifsicle-shaped rather than the reference's format/case/collision-policy form; and the
Status tab has no ETA or in-tab progress bar (the bar remains in the bottom strip).

**Registered honestly:** `W-32` is **PARTIAL**, not DONE — the Qt surface is done, the web surface
(`web/app.js`) still lists outputs without per-file before/after/delta or failure states. `N-56`/
`N-57` carry the evidence rows.


## Evidence index

| Item | Where |
|---|---|
| Pixel + tab facts, both platforms | annotations on run `39810897814` at `f5b7d7a` (check-runs API) |
| Screenshots (7 PNGs per platform: idle, 4 tabs, preview panel, with-input) | artifacts `ui-screenshots-linux`, `ui-screenshots-windows` on run `39810897814` |
| Harness assertions, both platforms | `linux`/`windows` jobs, *GUI offscreen tests* steps |
| Two failed implementations | `STATUS.md` `N-52`, this file §1 |
| Changelog rows | `N-48` (cancel), `N-51` (lint gate), `N-52` (palette), `N-53` (`ci-ok`), `N-54` (evidence) |
