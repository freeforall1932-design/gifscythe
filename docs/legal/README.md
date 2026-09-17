# Legal — licence single source of truth (consolidated S24)

**Current state (S19, 2026-09-14):** first-party code is **Ms-PL**
(`COPYING.ms-pl`); the gifsicle engine is a **GPL v2-only** subprocess
(`COPYING.gifsicle`); Qt is **LGPL v3** (`COPYING.lgplv3` + companion
`COPYING.gplv3`, generated `QT_NOTICE.txt` in GUI packages). The UI carries no
GPL code and no Caesium content. The experimental `web/wasm/` track has its own
open licence question (§4 here, decided by `OD-16`) and is **not shippable**
until it is answered.

**Consolidated 2026-09-17 (S24):** this file absorbed `WHY_MSPL.md` (§1),
`COPYING_RULES.md` (§2) and `WASM_LICENSE_QUESTION.md` (§3–§4); the originals'
full text is in git history at `3c67e14`.

**Rule:** licence rationale lives in this folder. Everywhere else only *points
here* (one line) or *lists files* (packagers, manifests). If any doc outside
this folder disagrees with it, this folder + `LICENSE` win — fix the other doc,
not this one. Dated history (old log entries, dated reviews, audit evidence)
keeps its original wording as evidence; it is superseded, not edited. Nothing
here is legal advice.

**Maintenance contract — to change the licence again, touch exactly two sets:**
1. **This file** — rewrite the rationale + rules (the *why* and *how*).
2. **The mechanical set** (file names + grant text, no prose reasons):
   `LICENSE`, `COPYING.*`, `working_code/gifscythe/scripts/package_common.sh`
   (required-file lines), `working_code/gifscythe/scripts/test_package.sh`
   (fixtures), `.github/workflows/build.yml` + `docs/ci/build.yml.proposed`
   (manifest list), `STATUS.md` rows `W-08`/`U-08`/`GS-204` (via the normal
   audit flow), `docs/release/RELEASE_PROCEDURE.md` (file lists).
Everything else points here and needs no edit.

## 1. Why the first-party code is Ms-PL (`OD-09 = b`, `OD-C6` — S18, 2026-09-14)

**The decision.** Owner relicensed all first-party code (GUI, control layer,
CLI, web client, future C# shell) from "intended GPLv3" to **Ms-PL**. Engine
untouched (GPLv2-only subprocess). Caesium base dropped at the same time.

**Context.** The ScreenToGif reference fork
(`freeforall1932-design/ScreenToGif-fork`) is **Ms-PL**, and the FSF lists
Ms-PL as GPL-incompatible weak copyleft
(https://www.gnu.org/licenses/license-list.en.html). The UI's GPLv3 intent
existed *only* to match a Caesium-derived UX base — and pre-relicense
verification found that base was vapor: zero Caesium files in the product tree,
empty `assets/` + `resources/`, no foreign copyright headers in shipped code
(`working_code/gifscythe/src/`, `working_code/gifscythe/tests/`, `web/`). Sole-
author tree; the subprocess engine boundary works under any UI licence. So no
third-party permission was needed to relicense.

**Why Ms-PL won.** (1) *Velocity* — licences match the fork, so fork files
(editor windows, export pipeline, settings infrastructure) can be copied and
adapted with notices retained instead of rewritten from patterns; the
maximum-speed path for the (now parked) WPF shell. (2) *The GPLv3 reason
evaporated* — its whole justification was Caesium, which contributed nothing.
(3) *Clean execution* — sole-author tree + subprocess engine = relicense by
owner fiat, no negotiations, no dual-licence debt.

**Rejected alternatives.** Stay GPLv3 + reference-only (viable, kept as the
fallback until the relicense; rejected for speed) · permission letter from
upstream (would need the author plus every substantial contributor — 1,394
commits; moot after) · dual-licence our code (helps downstream, fixes nothing
upstream) · relicense the fork (impossible — not our copyright).

**Accepted costs.** (1) Strong copyleft lost — Ms-PL is weak: someone may fork,
improve privately, and ship binaries owing nothing back; the owner priced this
and accepted it. (2) Caesium door closed — GPLv3 assets can never enter an
Ms-PL tree; replaced by policy (system fonts + MIT/Apache icon sets only, per
`PROJECT_VISION.md`). (3) Ecosystem signal — Ms-PL is OSI-approved but the FSF
urges against it (GPL incompatibility) and it has no "or later" path;
contributors will pause and google — this file is the answer they find.
(4) Qt LGPL notices were still open at the time (closed S19, U-08).

**One-way-door warning.** The relicense commit itself is reversible
(sole-author code). It becomes effectively permanent the moment the **first
fork file lands** — that file is Nicke Manarin's, Ms-PL forever, and a
pure-GPL tree could never contain it again. Rule: every copied file is named in
`IMPROVEMENT_LOG.md` with its source commit (§2), so the door's state is always
auditable.

**What reopens this.** Routine copying does NOT reopen it — follow §2. A full
revisit needs a trigger this section does not anticipate (e.g. a hard
requirement to absorb GPL-licensed code) + a new owner decision + a rewrite of
this folder. Deliberately hard: licence churn is worse than either licence.

**Verification appendix (S18).** Copyright-header sweep over shipped code: no
foreign headers · `assets/` + `resources/`: `.gitkeep` only · Caesium on disk:
absent (reference rows retired, ignore lines removed) · FSF incompatibility
note quoted from the FSF licence list (URL above), not interpreted here.

## 2. Copying ScreenToGif fork files — compliance checklist (S18)

Licences match (both Ms-PL), so copying is allowed. "Allowed" still has rules —
Ms-PL §3. Follow them per file, every time.

**Before you copy.** (1) Confirm the source file is Ms-PL (licence header or
upstream `LICENSE.txt`; the whole upstream tree is Ms-PL — if that ever
changes, stop and re-read §1). (2) Copy from a **pinned** upstream commit;
record it. (3) Copy **into** the matching product tree (the C# shell tree once
it exists; never into the engine or `reference_code/`).

**When you copy.** (4) Retain **all** copyright, patent, trademark and
attribution notices (§3C) — in the file header and anywhere they appear; never
strip or "clean up" a header. (5) Keep `COPYING.ms-pl` shipped (packagers + CI
already require it; do not remove that requirement). (6) Name the file + source
commit in `IMPROVEMENT_LOG.md` (the one-way-door audit trail — §1).

**Never.** (7) Never copy a file whose licence you have not checked. (8) Never
bring GPL-licensed material in through the side door — including **icons and
assets** (MIT/Apache/system sources only, per vision) and snippets from GPL
examples. (9) Never "rewrite from memory" to dodge attribution: a rewrite
guided file-by-file is still a derivative — copy it properly with notices.
(10) Never touch the engine boundary: no engine code into the UI, no UI code
into the engine, whatever the licence.

**If in doubt:** stop, ask the owner, log the question. An unlogged copy is a
future audit finding with your name on it.

## 3. WASM in-process licence question — OPEN, blocks shippable (`OD-16`, S19)

**The setup.** Every shipped Gifscythe build keeps the GPLv2-only gifsicle
engine as a **subprocess** — never linked, only spawned with an argv array.
That boundary is what lets a non-GPL UI (now Ms-PL) distribute alongside a
GPLv2 engine (§1). The `web/wasm/` track compiles the engine to WebAssembly and
runs it **in-process** (one `.wasm` module driven from the page's own script).
The subprocess boundary does not exist there.

**The question.** The first-party UI is Ms-PL, which the FSF lists as
GPL-incompatible; the engine is GPLv2-only. Under what terms, if any, may a
bundle that runs GPLv2 engine code in-process with Ms-PL UI code ship — and
what notices, source offers, or additional grants does that take? That needs an
explicit owner answer, on counsel's terms, via `OD-16`
(`docs/planning/OWNER_DECISIONS.md`). The MVP's small scope does not shrink the
question: one in-process call is still in-process.

**Already handled (necessary, not sufficient).** Whatever `OD-16` decides, the
wasm build ships gifsicle's own licence text and source offer: `web/wasm/`
carries its third-party notices (in `web/wasm/README.md` since S24) and its
build stages `COPYING.gifsicle` into the output directory. That covers the
engine-redistribution half every build owes; it does not answer the in-process
half.

## 4. Rule for the wasm track

`web/wasm/README.md` carries **NOT SHIPPABLE** until `OD-16` is answered. When
the answer lands, it is recorded here (§3 — this file is updated, not deleted)
and the `OD-16` row points at it — the same pattern as the `OD-09` answer and
§1. Do not extend the in-process pattern anywhere else
(`SESSION_HANDOFF.md` product constraints).
