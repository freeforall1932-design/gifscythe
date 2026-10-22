# UI review — S41, before merging PR #17

**Requested by the owner:** review the UI before merging. **Reviewed at:** `0dee5b2`, PR #17,
CI run `39786162032` (all eight jobs green, `ci-ok` included).
**Scope:** every change on this branch that a user can *see*, plus the UI surface around them.

## What this review can and cannot see

- **Can:** the real widgets, rendered by the real Qt on both CI platforms (`offscreen`), sampled
  pixel by pixel, plus the full assertion suite (T1–T26) and its event-loop timings.
- **Cannot:** desktop appearance, window-manager chrome, animation smoothness, HiDPI scaling,
  a dark theme (`D-06` is not implemented), or a human judgement of "looks right". The PNGs are
  attached to run `39786162032` as `ui-screenshots-linux` and `ui-screenshots-windows` — this
  sandbox cannot fetch artifact blobs (they redirect to storage outside the allowed hosts), so
  **the images are yours to eyeball; the numbers below are what I could verify**.

## 1. The preview canvas changed colour — the one visible change in this PR

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

## 3. The tab shape is Input → Actions → Output → **Guide**, not → Status

The dionysus drop-in (and the audit's UI note) describe the target four-tab XnConvert shape as
Input → Actions → Output → **Status**. What actually ships is a **Guide** tab in the fourth slot.
That is not a regression and not a defect — `W-32` (Status tab / per-file results) is still OPEN,
with its contract proposed in `docs/planning/W32_RESULT_CONTRACT.md`. Recorded so nobody "fixes"
the tab titles from the drop-in's table.

Consequence for a user today: a batch shows **one status line plus the log pane**; there is no
per-file results table yet. That is unchanged by this PR.

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

**Recommended:** merge, accepting §1. If the dark canvas was deliberate, say so first and it is a
small, lint-clean change to make it dark again *by role* — after which the same numbers should read
a dark value at the corners instead of `#ffffff`.

## Evidence index

| Item | Where |
|---|---|
| Pixel + tab facts, both platforms | annotations on run `39786162032` (check-runs API) |
| Screenshots (7 PNGs per platform) | artifacts `ui-screenshots-linux`, `ui-screenshots-windows` on that run |
| Harness assertions, both platforms | `linux`/`windows` jobs, *GUI offscreen tests* steps |
| Two failed implementations | `STATUS.md` `N-52`, this file §1 |
| Changelog rows | `N-48` (cancel), `N-51` (lint gate), `N-52` (palette), `N-53` (`ci-ok`), `N-54` (evidence) |
