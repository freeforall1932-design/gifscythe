# W-32: the per-file result contract (PROPOSED)

**Status: PROPOSED — not approved, not implemented.** This document is the
"propose the contract and the test cases" step that precedes any GUI redesign.
It changes no code. Per the standing rule recorded in `SESSION_HANDOFF.md`
(research → walkable draft → owner confirmation → implementation), the Status
tab is not built until the owner has confirmed the columns below.

Owner-facing name: **Status** — the fourth XnConvert-style tab.

---

## 1. Why this is a model, not a widget

`W-32` is currently OPEN because neither surface keeps a *result table*:

- Qt has an activity log (`logPane_` / `appendLog()`) and one summary label —
  lines of text, not rows.
- `web/server.mjs` returns `{ok, mode, outputs:[{name,bytes,data}], commands,
  inBytes, outBytes}` — a flat list with no per-input pairing and no failure
  rows at all.

A log cannot answer "which file failed, and why?" A flat list cannot answer
"how much did *this* file shrink?" Renaming either would close the row on paper
and leave the question unanswered. The work is the model underneath.

## 2. Cardinality — get this right or the totals lie

Modes do not all pair one input with one output. The contract must name the
relationship per operation, or size totals get double-counted.

| Mode | Inputs → Outputs | Engine reality |
| --- | --- | --- |
| **Batch** | N → N | One engine command **per file**, each in Auto mode. Qt never passes `-b`; the web server mirrors that exactly. |
| **Merge** | N → 1 | "All inputs welded" into one file. |
| **Explode** | 1 → N | One input, many frame files. Qt **refuses** N > 1 (the engine would scatter the other files' frames into the CWD and still exit 0). |
| **Auto** | N → 1 | The engine merges the inputs into one output; an explicit output is required or it goes to stdout. |

Consequence for the Status rows: a **merge** row lists N inputs in one row with
one output. An **explode** row lists one input and N outputs. **Batch** is the
only N-rows-of-1→1 case.

## 3. The proposed objects

### `Run`
| Field | Meaning |
| --- | --- |
| `runId` | Stable identifier for one Start press. |
| `startedAt` / `finishedAt` | Wall clock. Absent `finishedAt` = still running. |
| `settingsSnapshot` | The exact settings the run used — not a re-read of the current panel, which the user may already have changed. |
| `inputOrder` | The file order as queued, so row order is reproducible. |
| `enginePath`, `engineVersion` | Recorded, so a result can be explained later. |
| `state` | `running` / `succeeded` / `failed` / `cancelled` — with `succeeded` meaning **every operation succeeded**. |

### `Operation` (one row in the Status table)
| Field | Meaning |
| --- | --- |
| `operationId` | Stable within the run. |
| `inputs[]` | File names + input byte counts (N for merge/auto). |
| `outputs[]` | File names + verified output byte counts (N for explode). |
| `state` | See §4. |
| `failureReason` | See §5. Null unless `state == failed`. |
| `command` | The exact argv that ran, quoted for display. |
| `startedAt` / `finishedAt` | Per operation. |

### `ResultRow` (what the table shows the user)

| Column | Source |
| --- | --- |
| File | `inputs[0]` (or "merged.gif" / "`<stem>` frames") |
| Before | Sum of `inputs[].bytes` |
| After | Sum of `outputs[].bytes` |
| Change | `after - before` and a percentage — **only when `state == succeeded`** |
| State | Icon + word |
| Detail | `failureReason` message, or the command that ran |

## 4. States — and why "skipped" is not "failed"

`succeeded` · `failed` · `cancelled` · `skipped` · `running` · `pending`

- **`skipped`** = *never attempted*. The browser batch stops on the first
  failure (`for (let i = 0; i < files.length && done; i += 1)` in
  `web/server.mjs`), so every file after the failure was never run. Reporting
  those as failures would blame files for a mistake they never got to make.
- **`cancelled`** = *attempted and stopped*. Distinct from `skipped` and from
  `failed`.

**Rule: a row that is not `succeeded` shows no byte figures at all** — not
zero, not a dash that reads as zero. Fabricating "0 bytes saved" for work that
never happened is the specific honesty bug this contract exists to prevent.

## 5. Failure reasons — named, not numeric

`engineMissing` · `engineNotStartable` · `engineFailed` · `timeout` ·
`outputMissing` · `outputInvalid` · `outputUnverifiable` · `cancelledByUser` ·
`refusedBeforeRun` (the plan-time refusals Qt already surfaces, e.g. explode
with N > 1, or a constant batch template that would overwrite one file).

Each carries a human sentence plus the command. The engine's own exit code is
recorded but is never the whole story: **exit code 0 alone is not success** —
the existing `verifyOutput()` rule already says the output must exist and be a
valid GIF.

## 6. Cancellation — the honest scope

`web/request-guard.mjs` and its stale-result rules stay as they are. A browser
`AbortController` abort discards the request; it is **not** a promise that the
server's gifsicle subprocess stopped.

So: a Cancel control may be labelled **Cancelling**, and the run may be
reported `cancelled`, **only** once it is wired to run identity, server-side
process termination, cleanup, and a verified result state. Until then the
honest UI is "Stop after this file" (no new files started), which the
stop-on-first-failure policy already gives us for free.

## 7. Preserved decisions

- The current **stop-on-first-failure** policy is preserved. It is not changed
  to make the table look tidier. Files after the failure are `skipped`.
- Results already completed before a failure **stay visible**. A failed run
  does not blank the rows that succeeded.
- Originals are untouched; output is written atomically and verified.
- The run is one input→output pair per batch operation — no double counting.

## 8. Proposed test cases

Fixtures shared by Qt, Node/browser and the future WPF port.

| # | Case | Expected |
| --- | --- | --- |
| R1 | Batch, 3 files, all succeed | 3 rows, each with before/after/change; totals = sum of rows |
| R2 | Batch, 3 files, 2nd fails | Row 2 `failed` with a named reason; row 3 `skipped` with **no** byte figures; rows 1 visible; totals count 1 succeeded / 1 failed / 1 skipped |
| R3 | Merge, 3 inputs | **One** row, 3 inputs, 1 output; `before` = sum of the 3 inputs, counted once |
| R4 | Explode, 1 input → 12 frames | One row, 1 input, 12 outputs; `after` = sum of the frames |
| R5 | Explode, 2 inputs | Refused **before** the run (`refusedBeforeRun`); no engine command is executed |
| R6 | Engine exits 0, writes nothing | Row `failed` / `outputMissing` — never `succeeded` |
| R7 | Engine exits 0, writes a non-GIF | Row `failed` / `outputInvalid`; pre-existing good output at that path is unchanged |
| R8 | Engine exits non-zero | Row `failed` / `engineFailed`, with the stderr the engine produced |
| R9 | Engine stalls | Row `failed` / `timeout`; no child left running; good output preserved |
| R10 | Cancel mid-run | Completed rows keep their figures; the stopped row is `cancelled`; later files are `skipped` |
| R11 | Same file queued twice (batch) | Two independent rows, each with its own figures |
| R12 | Settings changed after Start | Row shows the settings from the **snapshot**, not the edited panel |
| R13 | Empty queue | No rows, no Start; not a crash |
| R14 | A file that is not a GIF | Row `failed` with a named reason; the run's other rows unaffected |

## 9. Open questions for the owner

1. Are **Before / After / Change / State / Detail** the right five columns?
2. Should the Status tab also show the **command** per row, or stay on the
   Actions/Output command pane only?
3. On a failed batch, should rows after the failure be `skipped` (proposed) or
   simply absent?
4. Keep results between runs, or clear the table on every Start?
