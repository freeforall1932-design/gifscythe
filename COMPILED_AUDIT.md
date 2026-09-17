# Gifscythe — Compiled Audit (Master) — v4

> **v4 consolidation (2026-09-17, S24).** The four external review files uploaded to the repo root on
> 2026-09-16 (`gifscythe-audit-delta-2026-09-16-1357.md`, `gifscythe-code-review-794a996.md`,
> `gifscythe-review-794a996.md`, `gifscythe-repo-review.md`) were incorporated into **§20** and then
> deleted, per owner instruction and the S21 precedent: every finding got a disposition — already
> fixed (S23/PR #30), already tracked, **new → §5 rows U-77..U-96**, or refuted with evidence — so no
> problem was dropped and no already-worked problem was re-added. The register grows **76 → 96**;
> new fix-order ids P1-44..P1-46, P2-18..P2-22, P3-13..P3-19 were scoped in §6. The seven dated
> audit snapshots under docs/audit/ (now empty and removed) and docs/archive/ were folded into
> `docs/archive/AUDIT_HISTORY.md` (§10 and §20.5 record where each landed; full texts stay in git
> history at `3c67e14`). The stale header base line (finding U-82 / H:F-06) was fixed and rewritten
> into the G10-enforced shape.

> **v3 consolidation (2026-09-15).** This file is now the **single** compiled audit md: the two
> scattered root-level intake copies (`GIFSCYTHE_REVIEW_INTAKE_…7b91….md` and `gifscythe-audit_…729d….md`)
> were folded in (§2E/§2F/§5 findings; §17 reviewer prose) and removed. v3 also **recovers content v2
> had dropped** — the Audit A/B non-finding sections (§16), the VP-1..VP-5 + false-positive guardrails
> that §12 cites but v2 never defined (§15), and the intake E/F verdicts / delivery paths / per-finding
> regression cases (§17) — and adds the **merge-completeness checklist** (§18) and the **next-session
> review ask** (§19). **No finding was added or dropped: the register is still 76 (§5).** Removing the
> `…7b91…` copy also clears the live **G17/S2** doc-gate failure (it quoted a stale register tally).
> See §18 for the source-by-source traceability and §14 for the filename-vs-URL discrepancy log.

> **Remediation status (2026-09-10, session S8): 31 findings closed outright
> (21 in batch 1 + 10 in batch 2), 3 closed in part (U-10/U-14/U-18), 15 still
> open, 2 register rows corrected (U-19/U-20) — see
> `docs/archive/AUDIT_HISTORY.md` (file 6) for the per-finding before/after
> evidence pointer; the full S8 record is in git history at `3c67e14`. Rows below carry a
> `✅ FIXED (S8)` / `◐ PARTIAL (S8)` / `☑ CORRECTED (S8)` marker in the Status
> column. CI on **PR #11** (run `34471563229`) is **green on linux and
> windows**, which is the first compilation of the S8 Qt edits.**

> **Current state (2026-09-12, S14) — the red main is FIXED.** History: `main` reached `2d51347`
> (the PR #15 merge) with run `34705247115` **failing the Linux documentation status gate** — every
> later Linux step was skipped, Windows passed. The failure was a stale base-commit line in this
> file (gate **G10**); **PR #16 merged as `629135a` (2026-09-12); `main` run `34709202307` is GREEN on both jobs**. A newer merge on top of this branch keeps `main` red
> only while a run is in flight; check the tip run before claiming green. The S8 banner above is a dated snapshot of that
> session, **not** the current state. §13 is the external-review intake — **triaged
> into §6 in S15**; **GS-201 closed S16** (P0-5), **GS-202, DS-13 and GS-207 closed S17** (P0-6 / P1-32 / P1-29). DS-11 is also closed S17 (P2-14). GS-203, GS-204 and GS-210 are PARTIAL; as of S24 the not-DONE intake rows are those three (PARTIAL) plus GS-205, GS-209, DS-10 (OPEN) — DS-06..DS-09, GS-206 and GS-208 closed in S22/S23/S24.
> Narrative `**Status:**` lines in §2/§3/§4 now name their §5 register row; where the original
> audit text disagreed with the register, the original wording is kept after *"Original report:"*
> and is superseded by the register.

**Compiled:** 2026-09-10 · **Verification sessions:** S4 (2026-09-07), S7…S17 (2026-09-13), S18–S20 (2026-09-14), S21 (2026-09-15 — v3 consolidation + U-68 413), S22 (2026-09-16 — U-67 + web gates + U-53/U-60/U-61/U-62/U-64/DS-09), S23 (2026-09-16 — Tier-1 batch, PR #29), PR #30 (2026-09-16 — U-77/U-79/U-80), **S24 (2026-09-17 — v4: external-review intake §20, doc consolidation, U-82 fix, stale-row closures W-30/R-03/GS-208)**, S25 (2026-09-17 — P1-44 implementation, PR #32), **S26 (2026-09-17 — P1-44 proof + closeout U-78/U-87, DS-09 register sync, G10 base-line repair)**
**Base:** `main` at `e885d58` (the PR #32 merge; re-confirm with
`gh api repos/freeforall1932-design/gifscythe/branches/main --jq .commit.sha`;
`check_docs.sh` gate **G10** matches this line shape and fails if it names anything
else — S24 note: the previous "**Branch:** … at …" shape was NOT matched by G10's
trigger list, which is how it stayed six merges stale; finding U-82 / H:F-06)
**Product version:** 0.1.0 (do **not** bump to 1.0.0 yet)
**Companion docs:** `STATUS.md` · `SESSION_HANDOFF.md` · `WORKLIST.md` · `IMPROVEMENT_LOG.md`

> **This file is the DETAIL; `STATUS.md` is the ROLL-UP.** The §5 register below
> keeps the per-finding evidence — source audit, verification mark, `file:line`,
> before/after. `STATUS.md` is the one-row-per-item status register that answers
> *"how much is done?"* in a single line, and its `U-nn` rows are **generated
> from this file** by `working_code/gifscythe/scripts/check_docs.sh --emit`.
> Neither is a summary of the other: edit §5 here, re-emit, and both stay true.
> `check_docs.sh` gates **G5/G5b** fail if the two ever disagree, and **G0**
> fails if `STATUS.md` has drifted from what the emitter produces.

This document merges **ten independent reviews** (audits A–F, plus the four
2026-09-16 uploads G–J incorporated in v4 — see §20):

| ID | Source | What it is | Trust rank |
|----|--------|------------|------------|
| **A** | `AUDIT_A_extracted.md` — GPT 5.6 sol xhigh | 20 findings (2 Critical / 7 High / 9 Medium / 2 Low) | **1 — highest** |
| **B** | `AUDIT_B_extracted.md` — Seed 2.1 Pro Preview | 16 findings (0 Critical / 1 High / 4 Medium / 8 Low / 3 info) | **2** |
| **C** | `docs/archive/AUDIT_HISTORY.md` file 3 (was docs/audit/POST_S7_AUDIT.md) — Arena agent session | 13 findings (1 High / 4 Medium / 5 Low / 3 Nit) | **3** (executed code) |
| **D** | GPT 6 Astra Medium Audit (`arena.site/01a089b0…`) | 8 new findings (1 High / 6 Medium / 1 Low) | **1 — highest** (same rank as A) |
| **E** | Independent Source Audit (`arena.site/01a0a4f2-59e2-7b91-a71d-c630bb77209a`) — **gpt 5.6 sol xhigh** | 5 new findings (3 High / 2 Medium) — **from repo commit ca48bf8, live URL verified 5 findings** | **1 — highest** |
| **F** | Code Review Intake (`arena.site/01a0a4f2-59e2-729d-ba6e-9030c6b52dcb`) — **fable 5.1 low — WINNER** | 19 new findings (2 High / 7 Medium / 10 Low) — **from repo commit ca48bf8, live URL verified 19 findings = WINNER** | **1 — highest** |
| **G** | `gifscythe-audit-delta-2026-09-16-1357.md` (uploaded, pinned `794a996`) | 18 findings GN-01..GN-18 (0 P0 / 12 P1 / 4 P2 / 2 P3) + a shipping plan; source-read only, nothing executed; every finding carries a mechanical falsify line | **2** (honest about its own limits) |
| **H** | `gifscythe-code-review-794a996.md` (uploaded, pinned `794a996`) | 9 findings F-01..F-09 + 3 minor notes M-1..M-3; byte-exact source re-fetch; 4 of 9 were already fixed by S23/PR #30 at intake time | **2** |
| **I** | `gifscythe-review-794a996.md` (uploaded, pinned `794a996`) | 4 findings (proposed U-77..U-80) with **live executed proof** — the reviewer re-ran the whole suite (build/unit/smoke/web/gates) before judging; 3 of the 4 were fixed by PR #30 | **1** (executed) |
| **J** | `gifscythe-repo-review.md` (uploaded, static doc-level review) | 20 findings F-01..F-20 — 16 are re-frames of already-tracked rows, 4 are new observations (meta/hygiene/pitch); no code execution | **3** |

**Ranking rule:** A and D are both GPT-class audits and are treated as **highest priority** —
higher than B (Seed) and higher than the compiled audit (C). Where A/D conflict with B/C,
trust A/D + primary source. Where A and D disagree, treat D as the fresher observation.

**Verification marks**
| Mark | Meaning |
|---|---|
| ✅ **EXEC** | Confirmed by running the real code in this sandbox |
| ✅ **SRC** | Confirmed by reading the cited source; not executed (needs Qt/Windows/old toolchain) |
| ⚠️ **PART** | Real, but overstated or understated — corrected wording given |
| ❌ **NOT-REPRO** | Could not reproduce as stated |
| ⏸ **BLOCKED** | Cannot be checked in this sandbox (no Qt6/cmake/Windows) |

> **Status:** Audit A and B extracted files (`AUDIT_A_extracted.md`, `AUDIT_B_extracted.md`)
> were incorporated into this compiled document and then deleted from `main`. They still exist
> on the closed branch `codebase-review-and-fix-implementation-b8d7e` (PR #9, never merged),
> which is where the v3 completeness sweep re-read them from — the branch is the backup copy,
> `main` is not. This file is the single source of truth for all findings from **all six audits**
> (A, B, C, D, E, F). v3 also recovered their *non-finding* sections, which the v2 merge had
> dropped: see §16 (A/B positives, method, fix-order rationale) and §15 (the VP / false-positive
> guardrails §12 cites).

---

## 0. How to use this file

1. Do **not** re-open items marked ✅ FIXED without first running the regression checklist in §7.
2. Prefer fixing ⬜ OPEN items in the order of §6 (P0 data-loss → P1 hardening → P2 gates → P3 polish).
3. After any fix, run §7 and tick the boxes with evidence (command + exit code / screenshot).
4. Watch for **new pits** (§8): closing one hole by opening another is a failed fix.
5. WebP / APNG / frame editor stay **blocked** until GIF UI is stable (`PROJECT_VISION.md`).
6. **Narrative vs register:** §2/§3/§4 are the audits as filed. S14 reconciled their
   `**Status:**` lines with §5 — each one now names its row, and the original wording follows
   after *"Original report:"*. **§5 is the only source of state**; `STATUS.md` is generated from it.

---

## 1. Executive cross-audit summary

**All six audits converge on the same top failure class:**
**silent data loss / false success** — the tool reports success while destroying data,
writing nothing, or running single-threaded when the user asked for auto-threading.

| Metric | Count |
|--------|------:|
| Unique findings across all 10 reviews | **96** (52 + 5 + 19 + 20 new U-77..U-96 from the 2026-09-16 intake §20) |
| Confirmed BROKEN (wrong result / silent failure at runtime) | **23** (14 + 2 + 7) + intake G/H/I additions |
| Confirmed MISALIGNED (code contradicts docs/labels) | **20** (11 + 1 + 8) + intake additions |
| Confirmed MISSING-logic (documented behavior that doesn't exist) | **20** (16 + 2 + 2) + intake additions |
| **New from GPT 6 Astra Medium (D)** | **8** (GS-101…GS-108) |
| **New from 7b91 (E) — NA-01..NA-05 — gpt 5.6 sol xhigh** | **5** (3 High / 2 Medium) |
| **New from 729d (F) — NF-01..NF-19 — fable 5.1 low — WINNER** | **19** (2 High / 7 Medium / 10 Low) |
| **New from the 2026-09-16 uploads (G/H/I/J) — §20** | **20 rows** (4 fixed before/at intake: U-77/U-79/U-80 by PR #30, U-82 by S24; 16 OPEN) |
| Audits A+B items already in prior compilations | 36 |
| Items fixed in code this session | see §4 |
| Items still **open** before 1.0.0 | see §6 |

### Critical gaps across audits

| ID | Problem | Audits | Why it matters |
|----|---------|--------|----------------|
| U-01 | Batch auto-naming overwrites other outputs **and** the source GIF | A (GS-001), C (F-01), D (GS-101) | Silent data destruction; the project's core trust guarantee |
| U-02 | Portable packager reports success for an incomplete release | A (GS-002) | A release can ship without CLI/GUI/runtime |
| U-03 | Threads "Auto" (0) runs single-threaded, not auto-detected | B (BUG-01), C (F-02 correction) | User sets "Auto", gets 1 thread — the opposite of the label |
| U-04 | CLI `--run` without output corrupts stdout with status text | A (GS-003) | Redirected output is not a valid GIF |
| U-05 | Documented PATH engine fallback is dead code | A (GS-004) | README says PATH works; it doesn't |
| U-06 | Web demo binds `0.0.0.0`, no auth, no concurrency cap | A (GS-005), D (GS-102) | Anyone on the network can burn CPU; concurrent requests can race |
| U-07 | Windows CLI execution is ANSI-only (CreateProcessA) | A (GS-006) | Non-ASCII paths fail on Windows |
| U-08 | License set can ship incomplete, silently | A (GS-007) | GPL-v2 engine shipped with no license text |

---

## 2. GPT 6 Astra Medium Audit (D) — 8 new findings

**Source:** https://01a089b0-ef16-7451-bd81-a1c6a80d3252.arena.site/
**Rank:** Highest (same as Audit A — GPT-class review)
**Date:** 2026-09-10

### D-01 / GS-101 [High] — Batch output destination can change during a run

**File:** `src/qtui/MainWindow.cpp` — `setBusy()`, `chooseBatchDir()`, `onProcessFinished()`
**Source:** https://github.com/freeforall1932-design/gifscythe/blob/7a0a8b8883e35e3233944095e1ca9544cd10f500/working_code/gifscythe/src/qtui/MainWindow.cpp
**Evidence level:** source review; runtime reproduction pending

**Finding.** The batch folder text field (`batchDirEdit_`) is disabled while busy via
`setBusy()` → `batchDirEdit_->setEnabled(!busy)`, **but the sibling Browse button
(`browseBatchDirButton`) is NOT disabled.** The picker can still call `setText()` on the
disabled field. Each subsequent batch job computes its output from the *current* folder value,
so a running batch can silently split its outputs across destinations without repeating preflight.

**Evidence:**
```cpp
// setBusy():
batchDirEdit_->setEnabled(!busy);
// browseBatchDirButton is NOT disabled

void MainWindow::chooseBatchDir() {
  // no busy_ guard
  if (!dir.isEmpty()) batchDirEdit_->setText(dir);
}

// next batch item (in onProcessFinished batch continuation):
pendingOutput_ = defaultOutputFor(in);
```

**Proposed reproduction (not executed):**
1. Queue at least two GIFs with a deliberately slow test engine and select output folder A.
2. Start Batch, then use the still-enabled batch-folder Browse button to select folder B.
3. Observe whether later jobs write into B while the first job writes into A. Run only with
   disposable inputs.

**Suggested fix:** Snapshot a complete, validated job plan before starting. Disable the entire
output group (including Browse buttons) and guard chooser slots while busy; subsequent jobs must
use immutable planned destinations.

**Prior-audit distinction:** U-01 / A:GS-001 concerns collisions in the initial plan. This is a
separate time-of-use mutation after preflight; U-35 only concerns Run re-enablement.

**Status:** ✅ **FIXED (S10)** — resolved; register §5 `U-45`. Original report: ⬜ **OPEN** — `setBusy()` only disables the text field, not the Browse button.

---

### D-02 / GS-102 [Medium] — A previous web request can replace the current result

**File:** `web/app.js` — `setFile()`, run click handler
**Source:** https://github.com/freeforall1932-design/gifscythe/blob/7a0a8b8883e35e3233944095e1ca9544cd10f500/web/app.js
**Evidence level:** source review; runtime reproduction pending

**Finding.** Selecting a new file while optimization is pending resets the preview and re-enables
Run. The old request has no generation check or cancellation. Its completion then populates After
and the download link alongside the new Before image. Multiple requests can overwrite results out
of order.

**Evidence:**
```javascript
function setFile(f) {
  currentFile = f;
  $("run").disabled = false;   // re-enables Run even if a request is in flight
}

// run click handler:
const resp = await fetch(url, { method: "POST", body: currentFile });
const blob = await resp.blob();
// no file identity or request-generation check
$("after").src = URL.createObjectURL(blob);
```

**Proposed reproduction (not executed):**
1. Start optimizing GIF A with an artificially delayed response.
2. Select GIF B before A completes, then optionally start B.
3. Let A finish last; compare the Before image, After image, and downloaded GIF.

**Suggested fix:** Capture the input and a request generation at launch. Abort obsolete requests
and ignore every stale success, error, and finally handler. Only the active generation should
change Run state.

**Prior-audit distinction:** Not the desktop temp-file leak in B:BUG-14 / U-34. This is web
response ownership, not file cleanup or missing web modes.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-46`. Original report: ⬜ **OPEN** — confirmed by source read (`app.js:48-69`).

---

### D-03 / GS-103 [Medium] — Desktop preview invalidation happens too late

**File:** `src/qtui/MainWindow.cpp` — `onSelectionChanged()`, `schedulePreview()`, `startPreview()`
**Source:** https://github.com/freeforall1932-design/gifscythe/blob/7a0a8b8883e35e3233944095e1ca9544cd10f500/working_code/gifscythe/src/qtui/MainWindow.cpp
**Evidence level:** source review; runtime reproduction pending

**Finding.** The preview generation (`previewSeq_`) advances only when a **new eligible preview
starts**, not when selection or settings change. During the 1.2-second debounce, the old
completion still passes its sequence guard. Clearing the queue or selecting Explode causes the
next preview to return **before** incrementing `previewSeq_`, so the old result can reappear and
remain visible.

**Evidence:**
```cpp
void MainWindow::schedulePreview() {
  if (busy_) return;
  previewTimer_->start();  // generation (previewSeq_) unchanged
}

// startPreview(): invalid input / Explode return FIRST, before incrementing seq
const int seq = ++previewSeq_;   // only reached for eligible input + non-Explode

// completion lambda:
if (seq != previewSeq_) return;  // stale guard — but seq never advanced on clear/Explode
```

**Proposed reproduction (not executed):**
1. Start a slow preview of GIF A.
2. Select B, clear the queue, or switch to Explode while A is still encoding.
3. Allow A to finish before a new valid preview begins; inspect After for a stale image.

**Suggested fix:** Invalidate the generation immediately on every input/settings change, queue
clear, and cancellation. Cancel obsolete work independently of whether a replacement preview can
start.

**Prior-audit distinction:** Related to A:GS-012 cancellation guidance, but adds the uncovered
debounce and early-return paths. U-34 concerns leaked files, not stale visual correctness.

**Status:** ✅ **FIXED (S10)** — resolved; register §5 `U-47`. Original report: ⬜ **OPEN** — confirmed by source read. **Distinct from U-34** (which is about leaked
temp files, not stale visual results).

---

### D-04 / GS-104 [Medium] — Empty comments remove a required argv operand

**File:** `src/core/GifsicleCommand.h` — `add()`, comments loop
**Source:** https://github.com/freeforall1932-design/gifscythe/blob/7a0a8b8883e35e3233944095e1ca9544cd10f500/working_code/gifscythe/src/core/GifsicleCommand.h
**Evidence level:** source review; runtime reproduction pending

**Finding.** SettingsIO accepts an empty comment value. The command builder adds `--comment` but
its shared `add()` helper discards the empty string operand. This creates a malformed argv
sequence: the following token can be consumed as comment text or trigger an option error instead
of keeping the intended argument boundaries.

**Evidence:**
```cpp
inline void add(std::vector<std::string>& v, const std::string& s) {
  if (!s.empty()) v.push_back(s);
}

for (const auto& c : s.comments) {
  add(args_, "--comment");
  add(args_, c);  // empty operand disappears
}
```

If `c` is empty (`""`), `add(args_, "--comment")` emits `--comment` but `add(args_, c)` does
nothing. The result is `--comment` with **no following argument** — gifsicle sees `--comment` and
then the next argv (e.g. `-o`) as the "comment text", consuming `-o` as a comment and breaking the
command.

**Proposed reproduction (not executed):**
1. Load a disposable settings file containing an empty `comment =` line and an input path.
2. Inspect `GifsicleCommand::args()`: `--comment` has no operand.
3. Add a regression asserting that the input stays an input and the empty operand is retained or
   explicitly rejected.

**Suggested fix:** Never filter required argv operands. Push both tokens together, including an
empty string, or reject/skip the entire empty-comment option according to a documented policy.

**Prior-audit distinction:** A:GS-011 covers malformed booleans and numeric validation, not
argument cardinality. Shell quoting cannot repair an already-missing argv element.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-48`. Original report: ⬜ **OPEN** — confirmed by source read. The GUI's "Add comment" button
(`SettingsPanel.cpp`) checks `if (!t.isEmpty())` before adding, so the GUI path is protected. But
a hand-written `.conf` with `comment = ` (empty value after `=`) loads as an empty string in
`s.comments` (SettingsIO.h: `s.comments.push_back(v)` where `v` is trimmed to `""`). Then
`GifsicleCommand` emits `--comment` with no argument.

---

### D-05 / GS-105 [Medium] — Settings query values are decoded twice

**File:** `web/server.mjs` — `handleOptimize()`
**Source:** https://github.com/freeforall1932-design/gifscythe/blob/7a0a8b8883e35e3233944095e1ca9544cd10f500/web/server.mjs
**Evidence level:** source review; runtime reproduction pending

**Finding.** `URLSearchParams.get()` already percent-decodes the query value. The additional
`decodeURIComponent()` corrupts literal percent sequences inside valid JSON. A comment containing
`"100%"` throws and returns HTTP 400; `"%20"` is silently turned into a space.

**Evidence:**
```javascript
const raw = url.searchParams.get("settings") || "{}";
settings = JSON.parse(decodeURIComponent(raw));

// client already encodes once:
"/optimize?settings=" + encodeURIComponent(JSON.stringify(s))
```

**Proposed reproduction (not executed):**
1. Send a correctly encoded settings object with comments: `["100%"]`.
2. The extra decode throws `URIError` and the server returns "bad settings JSON".
3. Repeat with comments: `["%20"]` and check that the literal value changes.

**Suggested fix:** Parse JSON directly from `url.searchParams.get("settings")`. Add transport
round-trip tests for `%`, `%20`, `%22`, plus signs, Unicode, and malformed JSON.

**Prior-audit distinction:** U-30 is missing semantic validation. This is corruption of
syntactically valid transport data before validation or engine execution.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-49`. Original report: ⬜ **OPEN** — confirmed by source read (`server.mjs:146-148`).

---

### D-06 / GS-106 [Medium] — Valid command text can break the HTTP response header

**File:** `web/server.mjs` — `handleOptimize()`, `X-Gifscythe-Command` header
**Source:** https://github.com/freeforall1932-design/gifscythe/blob/7a0a8b8883e35e3233944095e1ca9544cd10f500/web/server.mjs
**Evidence level:** source review; runtime reproduction pending

**Finding.** The full command is inserted into an HTTP header without header-safe encoding. Valid
GIF comment arguments can contain newlines or characters outside Latin-1, and the engine path can
also contain Unicode. Node rejects invalid header characters, turning a successful engine result
into a response failure.

**Evidence:**
```javascript
res.writeHead(200, {
  "Content-Type": "image/gif",
  "X-Gifscythe-Command": argv.map(shellQuote).join(" "),
});
// POSIX shell quoting is NOT HTTP header encoding
```

**Proposed reproduction (not executed):**
1. Use a valid GIF and settings with a CJK comment or an embedded newline.
2. Allow the engine to complete successfully.
3. Check for an invalid-header exception instead of a successful GIF download. Also test a
   Unicode engine path.

**Suggested fix:** Return command metadata separately as JSON or use a documented ASCII-safe
encoding and decode it in the client. Bound metadata size; never treat shell quoting as header
sanitization.

**Prior-audit distinction:** A:GS-006 concerns Windows process encoding. This is Node HTTP output
encoding and can fail on any supported host.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-50`. Original report: ⬜ **OPEN** — confirmed by source read (`server.mjs:173-181`).

---

### D-07 / GS-107 [Medium] — Unescaped settings values can become additional keys

**File:** `src/core/SettingsIO.h` — `save_settings()`, `load_settings()`
**Source:** https://github.com/freeforall1932-design/gifscythe/blob/7a0a8b8883e35e3233944095e1ca9544cd10f500/working_code/gifscythe/src/core/SettingsIO.h
**Evidence level:** source review; runtime reproduction pending

**Finding.** The line-based serializer writes string values verbatim, while the loader splits on
newlines and treats each line as a new key. A programmatically supplied multiline comment or a
valid POSIX path containing a newline cannot round-trip; its later lines can override settings.
Leading and trailing whitespace is also lost.

**Evidence:**
```cpp
for (const auto& c : s.comments)
  out << "comment = " << c << "\n";

// load_settings reads each physical line as a key
while (std::getline(in, line)) {
  // ... set_field(s, key, val, warnings);
}
```

**Proposed reproduction (not executed):**
1. Create a `Settings` object with a comment containing a newline followed by `mode = merge`.
2. Serialize it and load the result into a new `Settings` object.
3. Assert both the comment and mode are unchanged; source analysis predicts the comment is
   truncated and mode is overwritten.

**Suggested fix:** Define a backward-compatible escaping/quoting scheme or reject
unrepresentable string values before saving. Test newline, carriage return, whitespace, equals
signs, and Unicode round trips.

**Prior-audit distinction:** U-16 is atomicity; U-19 is omission of inactive values. Neither
covers injection of new logical keys through unescaped string serialization. This is not a claim
of remote code execution.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-51`. Original report: ⬜ **OPEN** — confirmed by source read (`SettingsIO.h:275-276` for save,
`:290` for load).

---

### D-08 / GS-108 [Low] — Before-image object URLs are never released

**File:** `web/app.js` — `setFile()`
**Source:** https://github.com/freeforall1932-design/gifscythe/blob/7a0a8b8883e35e3233944095e1ca9544cd10f500/web/app.js
**Evidence level:** source review; runtime reproduction pending

**Finding.** Each file selection creates a new object URL for Before, but only After URLs are
retained and revoked. Replacing the `src` does not release the earlier blob URL, so repeatedly
choosing large files keeps them reachable until the page unloads.

**Evidence:**
```javascript
if (afterUrl) {
  URL.revokeObjectURL(afterUrl);
  afterUrl = null;
}
$("before").src = URL.createObjectURL(f);
// previous Before URL is not retained or revoked
```

**Proposed reproduction (not executed):**
1. Instrument `URL.createObjectURL` and `URL.revokeObjectURL`.
2. Select multiple GIFs without running optimization.
3. Verify that Before URLs are created repeatedly with no corresponding revocation.

**Suggested fix:** Track a `beforeUrl` alongside `afterUrl`. Revoke the previous Before URL on
replacement and release both URLs during teardown.

**Prior-audit distinction:** B:BUG-14 / U-34 is desktop preview disk cleanup. This is browser blob
lifetime on the input side.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-52`. Original report: ⬜ **OPEN** — confirmed by source read (`app.js:64-66`).

---

## 2E. Independent Source Audit (E) — 5 findings — 01a0a4f2-59e2-7b91-a71d-c630bb77209a — gpt 5.6 sol xhigh

**Source:** https://01a0a4f2-59e2-7b91-a71d-c630bb77209a.arena.site/
**Repo file:** gifscythe-audit-01a0a4f2-59e2-729d-ba6e-9030c6b52dcb 2026-09-15.md from commit ca48bf8 (307 lines, 5 findings) — **removed from `main` in v3 (2026-09-15): folded into this file (§2E findings + §17.1 prose); full text preserved in git history at `c4f9e1c`.** *note: file name 729d on repo contains 5 findings but live URL 7b91 also contains 5 findings; content matched by finding count, URL verified 2026-09-15 via fetch_page*
**Model:** gpt 5.6 sol xhigh (per owner mapping, non-winner)
**Rank:** Highest (source-confirmed, 0 runtime claims, CI green check 34818891106)
**Date:** 2026-09-15
**Trust:** 1 — highest, same as A/D

### E-01 / NA-01 [High] — A malformed position pair can still emit a valid-looking -p X,0 — U-33 fix regression

**File:** `src/core/SettingsIO.h` — `set_field()` and `load_settings()`
**Confidence:** verified-by-source | **Nearest:** U-33 (half-spec) — fix checked presence not parse success
**Evidence level:** source review; U-33 fix regression
**Finding.** The U-33 fix checks whether both position keys were present, not whether both parsed successfully. One valid coordinate and one invalid coordinate leaves `has_position` enabled. `set_field()` sets `has_position=true` as soon as either coordinate parses. `load_settings()` records `saw_position_x/y` before parsing. Final guard only compares key presence.
**Evidence:**
```cpp
else if (k == "position_x") {
  if (need_ulong_nonneg(&s.position_x)) s.has_position = true;
}
else if (k == "position_y") {
  if (need_ulong_nonneg(&s.position_y)) s.has_position = true;
}
// load_settings:
if (lk == "position_x") saw_position_x = true;
else if (lk == "position_y") saw_position_y = true;
if (saw_position_x != saw_position_y) { clear has_position }
```
With `position_x=12` valid + `position_y=nope` invalid, both saw true, `has_position` stays true with `x=12,y=0` → emits `-p 12,0`.
**Repro:**
1. conf with `position_x = 12` and `position_y = nope`, valid input/output
2. Run cli without --strict, inspect printed command
3. Expected: position omitted. Current: warning + `-p 12,0` reachable
**Suggested fix:** Stop mutating `has_position` inside `set_field()`. Parse into optional temps. Enable only when both present and both conversions succeeded. Emit one pair-level warning. Keep --strict rc=3. Unit test: `x=12,y=nope` → no `-p`.
**Status:** ✅ **FIXED (S22)** — register §5 `U-53`. `set_field()` no longer toggles `has_position`; `load_settings()` now enables the pair only when both keys were seen and both parsed, otherwise it emits one pair-level warning and clears both coordinates. Executed proof: native unit suite `./build.sh` → **308/308**, plus `scripts/smoke_cli.sh` → **45 passed / 0 failed** with the `position_x=12`, `position_y=nope` plain-print and `--strict` regressions pinned.

---

### E-02 / NA-02 [High] — Changing web settings during a run does not invalidate that run — extends U-46

**File:** `web/app.js` — `requestGen`, control wiring, run handler for `POST /run`
**Confidence:** verified-by-source | **Nearest:** U-46 (queue guard), D:GS-102
**Evidence level:** source review
**Finding.** `requestGen` advances when file queue changes (`onQueueChanged()`), but not when any settings control changes. Controls remain editable while `fetch('/run')` pending. Old response can be accepted while controls show new settings.
**Evidence:**
```javascript
function onQueueChanged() { requestGen += 1; ... }
for (const id of [...controls]) { $(id).addEventListener("input", refreshCommand); }
// run handler:
const gen = requestGen; await fetch("/run", ...); if (gen !== requestGen) return;
```
**Repro:** Use delayed engine wrapper (2s), start web run, change optimization/resize/loop/delay before response, observe old response rendered with new controls.
**Suggested fix:** Create one `invalidateRun()` used by queue and every settings change. Track AbortController, increment generation, clear output on change. Capture immutable settings snapshot at launch.
**Status:** ✅ **FIXED (S23)** — resolved; register §5 `U-54`. The ownership counter is now a pure module (`web/request-guard.mjs`) invalidated by every control change, snapshotting the launch-time settings and aborting the fetch; `web/test/request-guard.test.mjs` pins it (13 assertions). Original report: ⬜ **OPEN** — confirmed by source read; extends U-46 (queue-only guard).

---

### E-03 / NA-03 [High] — Windows output collision keys fold ASCII only — reopens U-01 class

**File:** `src/core/OutputPlan.h` — `path_key()`
**Confidence:** verified-by-source | **Nearest:** U-01/U-45
**Evidence level:** source review; needs native Windows
**Finding.** `path_key()` lowercases UTF-8 path byte-by-byte with `std::tolower` under `_WIN32`. Does not implement Windows Unicode case-insensitive comparison. Two batch targets differing only by non-ASCII case can pass preflight and overwrite same Windows file, reopening U-01 class.
**Evidence:**
```cpp
#ifdef _WIN32
  for (auto& c : s) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (c == '\') c = '/';
  }
#endif
```
**Repro:** On Windows, batch inputs with U+00C4 vs U+00E4 stems into one folder, assert preflight should refuse.
**Suggested fix:** Keep paths as UTF-16 on Windows, compare with ordinal case-insensitive Windows API (`CompareStringW` / `CompareStringOrdinal NORM_IGNORECASE`), injectable policy so Linux unit tests cover Windows rules.
**Status:** ⬜ **OPEN** — confirmed by source read; register §5 `U-55`.

---

### E-04 / NA-04 [Medium] — Desktop filename sanitization misses Windows superscript device aliases

**File:** `src/core/OutputName.h` — `is_windows_reserved_device_name()`
**Confidence:** verified-by-source | **Nearest:** U-21, web run-paths.mjs already handles
**Evidence level:** source review
**Finding.** Desktop recognizes COM/LPT only when fourth byte is ASCII digit (`isdigit`). Windows also reserves superscript ¹ (U+00B9), ² (U+00B2), ³ (U+00B3) forms: `COM¹`, `LPT²`, etc. Web `run-paths.mjs` already handles `/com[0-9¹²³]/iu`.
**Evidence:**
```cpp
if (stem.size() == 4 && (stem.compare(0,3,"com")==0 || stem.compare(0,3,"lpt")==0) && std::isdigit(stem[3])) return true;
```
**Repro:** Call `sanitize_output_name()` with Windows rules and COM/LPT + superscript, expect defused.
**Suggested fix:** Extend matcher to recognize exact UTF-8 sequences for superscript 1,2,3 (C2 B9/C2 B2/C2 B3) or compare decoded Unicode scalars. Share test table with web.
**Status:** ✅ **FIXED (S23)** — resolved; register §5 `U-56`. Original report: ⬜ **OPEN** — confirmed by source read; register §5 `U-56`.

---

### E-05 / NA-05 [Medium] — The reusable WASM module can accept a stale /out.gif as new output — GS-203 pattern

**File:** `web/wasm/wasm.js` — `M.FS.writeFile("/in.gif")`, `callMain()`, `readFile("/out.gif")`
**Confidence:** verified-by-source | **Nearest:** GS-203 (output verification)
**Evidence level:** source review
**Finding.** WASM UI reuses one Emscripten module (singleton, EXIT_RUNTIME=0) but does not unlink/snapshot `/out.gif` before `callMain()`. After one success, a later exit-zero/no-write run reads previous GIF and reports success for wrong input.
**Evidence:**
```javascript
M.FS.writeFile("/in.gif", inputBytes);
M.callMain(args);
outBytes = M.FS.readFile("/out.gif");
```
**Repro:** Run once with fake module writing valid `/out.gif`, run second time with success but no write, observe second reads first output.
**Suggested fix:** Unlink `/out.gif` before every `callMain()` (ignore ENOENT), verify GIF magic after, remove temps in finally. Do not ship WASM until OD-16 closed.
**Status:** ⬜ **OPEN** — confirmed by source read; register §5 `U-57`. Mirrors GS-203 pattern.

---


## 2F. Code Review Intake (F) — 19 findings — 01a0a4f2-59e2-729d-ba6e-9030c6b52dcb — fable 5.1 low — WINNER

**Source:** https://01a0a4f2-59e2-729d-ba6e-9030c6b52dcb.arena.site/
**Repo file:** GIFSCYTHE_REVIEW_INTAKE_01a0a4f2-59e2-7b91-a71d-c630bb77209a 2026-09-15.md from commit ca48bf8 (703 lines, 19 findings) — **removed from `main` in v3 (2026-09-15): folded into this file (§2F findings + §17.2 prose); full text preserved in git history at `c4f9e1c`. Its stale register tally was the live G17/S2 gate failure that v3 clears.** *note: file name 7b91 on repo contains 19 findings but live URL 729d also contains 19 findings; content matched by finding count, URL verified 2026-09-15 via fetch_page — 19 findings = fable WINNER per owner check*
**Model:** fable 5.1 low — WINNER (per owner latest check, arena comparison winner, 19 findings)
**Rank:** Highest (source-confirmed, checked against 122 rows, 0 runtime, needs smoke/harness)
**Date:** 2026-09-15
**Trust:** 1 — highest

**Method (from original):** Fetched main live, read COMPILED_AUDIT §1–§13 and STATUS.md first to avoid duplicates, read all core headers, CLI driver, web server/UI/builder, MainWindow.h, SettingsPanel.h, gifsicle 1.96 man page as truth. Every row verified-by-source.

### F-01 / NF-01 [High] — Batch continuation re-reads LIVE settings for every file after the first

**File:** `MainWindow.cpp` — `onProcessFinished()` batch branch, `MainWindow.h` — no settings snapshot member
**Confidence:** verified-by-source | **Nearest:** U-45 (output group lock) and U-01 (target plan) — neither covers settings snapshotting
**Finding.** `runCommand()` plans TARGETS once (`batchTargets_`, U-01) but SETTINGS not snapshotted. Every subsequent batch item calls `currentSettings()` live. Actions tab is time-of-use input for files 2..N. One batch can apply different settings to different files while UI reports one complete.
**Evidence:**
```cpp
pendingOutput_ = batchTargets_.value(batchIndex_); // targets planned
auto settings = currentSettings(); // settings NOT — read again, live
// MainWindow.h run-state: enginePath_, inputs_, process_, busy_, cancelling_, pendingOutput_, batchTargets_, batchIndex_, batchQueue_, batchMode_, explodeSnapshot_ — no batchSettings_
```
**Repro:** Queue 3 large GIFs, Batch Optimize=1, start, while file1 encoding change Actions Optimize=3 Colors=16, compare 3 opt.gif sizes / gifsicle --info colour tables. Command pane shows only last argv.
**Fix:** Snapshot `batchSettings_ = currentSettings()` once at batch start next to batchTargets_, use in continuation, disable `settingsPanel_` in `setBusy()`. Harness T18-style: change control mid-batch, assert argv run2 == run1.
**Status:** ⬜ **OPEN** — register §5 `U-58`.

---

### F-02 / NF-02 [High] — Cancel (or engine failure) leaves a truncated file over a PRE-EXISTING output — P0 data loss

**File:** `MainWindow.cpp` — `cancelRun()`, `onProcessFinished()` failure path, `cli/main.cpp`, `OutputPlan.h` header comment declining temp+rename
**Confidence:** verified-by-source | **Nearest:** U-01/U-45 (planning), GS-203 (post-conditions) — none address partial-output cleanup on cancel
**Finding.** gifsicle writes straight to `-o <target>`. Cancel kills engine mid-write; failure branch shows dialog but never removes/restores partial. OutputPlan.h says pre-existing is normal and declines temp+rename because it would only protect previous output from crashed engine. Cancel is first-class button, not crash: re-optimising existing `<name>_opt.gif` and pressing Cancel replaces last good result with 0-byte/truncated file.
**Evidence:**
```cpp
void MainWindow::cancelRun() { cancelling_=true; process_->kill(); waitForFinished(3000); cancelling_=false; setBusy(false); updateStatus("Cancelled."); }
```
**Repro:** Optimize large GIF to out.gif once, note size. Run again to same out.gif and Cancel (deterministic with stub engine that opens -o, truncates, sleeps 5s). `ls -l out.gif` → 0 bytes/truncated, `gifsicle --info` error, status only "Cancelled." CLI equivalent: kill engine during write → partial left, rc reflects signal (U-32) but file not cleaned.
**Fix:** Minimum: after cancel/rc≠0, if pre-run snapshot said target did NOT exist → delete partial; if DID exist → warn damaged. Proper: run to `<target>.gs-partial`, verify GIF magic + non-empty, rename atomic. Live pane shows real argv + note. Harness: cancel-with-preexisting keeps old bytes.
**Status:** ⬜ **OPEN** — register §5 `U-59`. P0 data loss, same class as U-01, **requires tmp+rename guard not covered by OutputPlan existing**.

---

### F-03 / NF-03 [Medium] — CLI turns gifsicle frame selections into bogus file paths

**File:** `cli/main.cpp` — `resolve_path()` applied to every `s.inputs`, `GifsicleSettings.h` documents `#0`
**Finding.** `GifsicleSettings.h` documents frames `#0` as legal input, man page defines them, but CLI resolves every input relative to conf dir, so `#0` becomes `/abs/dir/#0` and fails.
**Evidence:** `for (auto& in : s.inputs) in = resolve_path(in, base_dir);`
**Fix:** Skip resolution when value starts with `#` or equals `-`, exclude from `plan_outputs()` input keys.
**Status:** ✅ **FIXED (S22)** — register §5 `U-60`. CLI path resolution now keeps frame selectors and stdin tokens literal, and the planner ignores those non-path inputs. Executed proof: `web/test/command.test.mjs` now pins `#0` and `-` parity against the real CLI, and `scripts/smoke_cli.sh` verifies `input = #0` reaches the engine unchanged and produces a one-frame GIF.

---

### F-04 / NF-04 [Medium] — `output = -` treated as file → false failure rc=1

**File:** `cli/main.cpp` — `verify_file` / `plan_outputs`
**Finding.** Man page: `-o file … special filename - means stdout`. CLI plans `-` as CWD file, snapshots it, streams GIF to stdout correctly, then `verify_output("-")` reports no file and rc becomes 1. Valid GIF + failure code.
**Evidence:** `const bool verify_file = !s.output.empty() && ...` // "-" non-empty
**Fix:** Normalise `output == "-"` to streaming contract before planning/verification.
**Status:** ✅ **FIXED (S22)** — register §5 `U-61`. `output = -` is now treated as streaming stdout before path resolution, planning, and verification. Executed proof: `web/test/command.test.mjs` pins literal `-o -`, and `scripts/smoke_cli.sh` compares `gifscythe-cli --run` stdout byte-for-byte with `gifsicle -o -` while asserting no literal `-` file is created.

---

### F-05 / NF-05 [Medium] — Validate.h refuses crop width/height 0, engine allows 0 = extend to edge

**File:** `Validate.h` — crop rule, `GifsicleSettings.h` unsigned fields
**Finding.** Man page `--crop x1,y1+WxH`: width/height can be zero or negative, zero = to edge. Validator rejects 0 outright, unsigned makes negative unrepresentable. False refusal.
**Evidence:** `if (s.crop && (s.crop_w == 0 || s.crop_h == 0)) add("crop", "0x0", ...)`
**Fix:** Allow 0 (and consider int + negative), update JS mirror + parity fixture.
**Status:** ✅ **FIXED (S22)** — register §5 `U-62`. Native and web validation now accept crop width/height `0` so the engine's "extend to edge" syntax is no longer refused. Executed proof: native unit suite `./build.sh` → **308/308**, `web/test/validate.test.mjs` now expects zero warnings for `crop 0x0`, and `scripts/smoke_cli.sh` proves `--run --strict` accepts `crop_w = 0`, `crop_h = 0` and produces output.

---

### F-06 / NF-06 [Medium] — `--no-loopcount` (play once) unrepresentable

**File:** `GifsicleSettings.h` `loopcount = -1`, `GifsicleCommand.h`, `command.mjs`, `SettingsPanel.h`
**Finding.** Engine has three states: unchanged, forever (`--loopcount=0`), N, and OFF (`--no-loopcount`, show once). Model only has unchanged/forever/N. Cannot produce non-looping GIF from any surface.
**Evidence:** `if (s.loopcount == 0) add("--loopcount=0"); else if (>0) ...` // nothing emits --no-loopcount
**Fix:** Add tri-state+off: e.g. `loopcount = -2` → `--no-loopcount`, GUI item Play once, web `once` option.
**Status:** ✅ **FIXED (S23)** — resolved; register §5 `U-63`. Original report: ⬜ **OPEN** — register §5 `U-63`.

---

### F-07 / NF-07 [Medium] — `/run` and `/optimize` accept `info:true` then misleading 422

**File:** `web/server.mjs` — `handleOptimize()`/`runOne()` → `verifyOutput()`, `cli/main.cpp` exempts info from verification, web does not
**Finding.** `--info -o file` makes gifsicle write TEXT into output. CLI exempts info from verification. Web does not: validate lets info through in auto mode, engine exits 0, `verifyOutput` sees no GIF signature and API blames engine.
**Evidence:** `const verified = await verifyOutput(outPath, before); if (verified.error) sendJson(422, {exitCode:0, stderr: verified.error})`
**Fix:** Reject `info` at web validation with 400, or return text/plain info. Add transport test.
**Status:** ✅ **FIXED (S22)** — register §5 `U-64`. The web server now rejects `info:true` early with a clear HTTP 400 instead of falling through to GIF verification and blaming the engine with exitCode 0. Executed proof: `web/test/transport.test.mjs` pins both `/optimize` and `/run` to the new 400 path and asserts the `/run` request does not spawn the engine.

---

### F-08 / NF-08 [Medium] — CLI symlink/PATH loses engine-beside-executable discovery

**File:** `cli/main.cpp` — `exe_path_of(argv0)`, `EngineLocator.h`
**Finding.** `exe_path_of()` trusts argv[0]: symlink not resolved, bare name not found in CWD → `exe_dir = CWD`. Packaged layout fails when CLI installed via `ln -s` into /usr/local/bin.
**Evidence:**
```cpp
fs::path p = u8path_compat(argv0);
if (p.is_absolute()) return p; // symlink kept
if (fs::exists(p, ec)) return absolute(p, ec); // only if exists relative to CWD
return p; // bare name -> CWD
```
**Fix:** Resolve real exe: `/proc/self/exe` Linux, `_NSGetExecutablePath` macOS, `GetModuleFileNameW` Windows, then `canonical()`. Smoke: symlinked CLI finds sibling engine.
**Status:** ✅ **FIXED (S23)** — resolved; register §5 `U-65`. Original report: ⬜ **OPEN** — register §5 `U-65`.

---

### F-09 / NF-09 [Medium] — Desktop pinned to GS_VERSION while web picks newest — VERSION bump breaks CLI/GUI

**File:** `EngineLocator.h` candidates use `release/<GS_VERSION>/`, `build_engine.sh` writes `release/<VERSION.md>/`, `server.mjs` `findEngine()` newest numeric dir
**Finding.** After editing VERSION.md to 0.2.0, CLI/GUI look only in release/0.2.0/ which does not exist until rebuilt, while web keeps using release/0.1.0/. Two policies for same binary.
**Evidence:** `candidates.push_back(exe_dir / ".." / "release" / GS_VERSION / base);` vs `versions.sort(...newest first...)`
**Fix:** Pick one policy: version-independent release/engine/ or release/current symlink + newest fallback in EngineLocator, log chosen dir.
**Status:** ✅ **FIXED (S23)** — resolved; register §5 `U-66`. Original report: ⬜ **OPEN** — register §5 `U-66`.

---

### F-10 / NF-10 [Low] — serveStatic raw prefix containment, serves source/tests, HEAD body

**File:** `web/server.mjs` — `serveStatic()`, `run-paths.mjs` `assertContainedPath()` exists
**Finding.** `file.startsWith(ROOT)` with ROOT=/repo/web would admit /repo/web-anything. Shielded by `new URL()` dot-segment collapse, so not exploitable today, but hygiene and exact pattern GS-202 replaced for uploads. Every file under web/ served, HEAD gets body.
**Fix:** Reuse `assertContainedPath(ROOT, file)`, allow-list served files, return headers only for HEAD.
**Status:** ✅ **FIXED (S22)** — register §5 `U-67`. S21 first narrowed the finding to what actually reproduced; S22 completed the implementation proof: `serveStatic()` now serves only the shipped UI allow-list (`/`, `/index.html`, `/style.css`, `/app.js`, `/command.mjs`), reuses `assertContainedPath()` as defence in depth, pins a single explicit HEAD contract with `Content-Length`, and leaves the API paths untouched. Executed proof: `web/test/static-hygiene.test.mjs` **43/43, red→green**; `web/test/transport.test.mjs` re-run **67/67** after the static rewrite; the regression now also runs in CI and in `verify_audit.sh` (**W5**). Original report: ⬜ **OPEN**.

---

### F-11 / NF-11 [Low] — Oversized bodies 400 not 413, real /run cap ~48MB not 64MB

**File:** `web/server.mjs` — `readBody()`, `handleRun()` catch
**Finding.** `readBody` rejects generic Error, `handleRun` converts to bad JSON, hiding cause. MAX_BODY applies to JSON envelope, base64 +33% lowers effective upload to ~48MB while README says 64MB.
**Fix:** Throw typed 413 from `readBody` and map, document effective limit or raise MAX_BODY for /run.
**Status:** ◐ **PARTIAL (S22)** — register §5 `U-68`. The 413 mapping is now executed end-to-end and wired into the automated gates: `web/test/body-limit.test.mjs` stays **8/8** (`/run` 400→413, `/optimize` 500→413, no real engine — `/run` is pre-discovery, `/optimize` uses an inert `GS_ENGINE` discovery stub never executed), `web/test/transport.test.mjs` re-ran **67/67**, and the regression now runs in CI and `verify_audit.sh` (**W4**). **Remaining:** the limit value itself is still left at a 64 MB HTTP-envelope cap, which means about a 48 MB effective decoded GIF for `/run`; that reality is documented in source, not changed. Original report: ⬜ **OPEN**.

---

### F-12 / NF-12 [Low] — After failed run previous After image stays under Failed status

**File:** `web/app.js` — run handler `!resp.ok` branch
**Finding.** `revokeResults()`/hide #outputs only on success and queue change. Changing setting and re-running to 422 leaves OLD results visible next to new failure — stale-visual of U-47 class on web.
**Fix:** Call `revokeResults()`, hide #outputs, clear #after before every run or on failure.
**Status:** ✅ **FIXED (S23)** — resolved; register §5 `U-69`. Original report: ⬜ **OPEN** — register §5 `U-69`.

---

### F-13 / NF-13 [Low] — Preview engine check bypasses UTF-8 boundary

**File:** `MainWindow.cpp` — `startPreview()`
**Finding.** `ensureEngine()` wraps in `u8path_compat()`, `startPreview()` passes `toStdString()` straight into `path_is_executable(fs::path)` — narrow conversion byte-mangling on MinGW. Non-ASCII engine path: main run works but preview says engine not found.
**Evidence:** `ensureEngine(): u8path_compat(enginePath_.toStdString())` vs `startPreview(): path_is_executable(enginePath_.toStdString())`
**Fix:** Wrap in `u8path_compat()`, grep other `.toStdString()`→`fs::path` boundaries.
**Status:** ⬜ **OPEN** — register §5 `U-70`.

---

### F-14 / NF-14 [Low] — Windows exit masked `&0xff` collapses NTSTATUS crash to success

**File:** `ProcessRunner.h` — Windows branch `run_argv()`
**Finding.** Windows exit codes 32-bit, crashed child returns NTSTATUS like 0xC0000005. Masking to low byte keeps most non-zero, but status ending 0x00 becomes 0 and honest exit contract broken in crash case.
**Evidence:** `return static_cast<int>(code) & 0xff;`
**Fix:** `if (code==0) return 0; int low=code &0xff; return low ? low : 1;` log raw hex when >255.
**Status:** ⬜ **OPEN** — register §5 `U-71`.

---

### F-15 / NF-15 [Low] — cancelling_ cleared after 3s wait → spurious failure dialog after Cancelled

**File:** `MainWindow.cpp` — `cancelRun()`, `onProcessFinished()`
**Finding.** `cancelRun()` resets `cancelling_=false` right after `waitForFinished(3000)`. If engine takes longer to die, `finished()` arrives later with `cancelling_==false` and failure branch shows error after Cancelled.
**Fix:** Clear `cancelling_` inside `onProcessFinished()`/`onProcessError()`, not in `cancelRun()`, fold into P1-24.
**Status:** ⬜ **OPEN** — register §5 `U-72`.

---

### F-16 / NF-16 [Low] — resolve_path falls back to CWD contradicting CWD-independent contract

**File:** `cli/main.cpp` — `resolve_path()`
**Finding.** Relative input missing next to conf but exists in CWD picked up from CWD. Same conf produces different runs from different dirs — contradicts comment about CWD-independent.
**Evidence:** `if (exists(candidate)) return candidate; if (exists(path)) return path; // CWD fallback`
**Fix:** Drop CWD fallback (fail with input not found next to conf) or print NOTE naming resolution.
**Status:** ✅ **FIXED (S23)** — resolved; register §5 `U-73`. Original report: ⬜ **OPEN** — register §5 `U-73`.

---

### F-17 / NF-17 [Low] — Batch + output + N>1 passes planner in merge shape, engine semantics undocumented

**File:** `cli/main.cpp` — `plan_outputs(s.inputs, {s.output})` for Batch, `OutputPlan.h` one_target shape
**Finding.** GS-201 made CLI refuse Batch WITHOUT output. Batch WITH one output and several inputs accepted because `plan_outputs` treats N→1 as legal merge shape. Man page defines `-b` as modify in place, says nothing about `-o` in batch, so CLI allows command whose outcome nobody pinned.
**Fix:** Refuse Batch with >1 input and single output in CLI (GUI never emits -b, runs per-file Auto). Long term make CLI batch identical to GUI/web batch.
**Status:** ✅ **FIXED (S23)** — resolved; register §5 `U-74`. Original report: ⬜ **OPEN** — register §5 `U-74`.

---

### F-18 / NF-18 [Low] — `-E` exposed but `--name` not, several engine options absent

**File:** `GifsicleSettings.h`, `SettingsPanel.h` `explodeByNameCheck_`
**Finding.** Without `--name` only way `-E` differs from `-e` is input already carries name extensions, so checkbox mostly inert. Gaps relative to ~30 engine-truth controls claim.
**Fix:** Add per-frame `--name` list or grey out `-E` with tooltip, add Not-exposed list to README.
**Status:** ✅ **FIXED (S23)** — resolved; register §5 `U-75`. Original report: ⬜ **OPEN** — register §5 `U-75`.

---

### F-19 / NF-19 [Low] — Explode default prefix differs per surface

**File:** `MainWindow.cpp` explode branch, `server.mjs` targets, `ExplodeVerify.h` `explode_prefix_for()`
**Finding.** GUI persists sessions with output cleared. Conf exported from GUI and run by CLI in Explode mode scatters `<basename>.NNN` into CWD, while GUI would write `<dir>/<stem>_frame.NNN` next to input.
**Evidence:** GUI: `fi.absolutePath()+"/"+fi.completeBaseName()+"_frame"` vs CLI: `first input's basename (CWD)`
**Fix:** CLI default explode prefix to `<input dir>/<stem>_frame` like GUI/web.
**Status:** ◐ **PARTIAL (S23)** — register §5 `U-76`. The NAME is unified (`<stem>_frame`, under `--run` so print mode and the JS⇄C++ parity stay exact). The DIRECTORY was deliberately NOT moved beside the input as suggested: tried literally, it wrote 12 frames into `reference_code/`. That policy is owner decision `OD-18`. Original report: ⬜ **OPEN** — register §5 `U-76`.

---


---

## 3. Audit A (GPT 5.6 sol xhigh) — 20 findings (full text)

*Source: `AUDIT_A_extracted.md` (now deleted — content merged below)*

### A-01 / GS-001 [Critical] — Batch output planning can overwrite another result or the source GIF

**Path:** `working_code/gifscythe/src/qtui/MainWindow.cpp` :: `defaultOutputFor()`,
`templateIsConstant()`, `runCommand()`

**Certainty:** Confirmed

**Impact:** Silent data loss. Two inputs with the same base name and one batch folder both
resolve to the same target. A template such as `{name}.gif` with no separate batch folder can
also resolve the output to the input itself.

**Evidence:** The preflight rejects only templates with no `{name}` token. It never builds and
compares the actual target paths, never compares targets with input paths, and does not ask
before replacing an existing file.

**Reproduce:**
```
mkdir -p /tmp/gs-a /tmp/gs-b /tmp/gs-out
cp reference_code/gifsicle/logo.gif /tmp/gs-a/foo.gif
cp reference_code/gifsicle/logo1.gif /tmp/gs-b/foo.gif
```
In the GUI, add both files, choose Batch, set `/tmp/gs-out`, keep `{name}_opt.gif`, then run.
Inspect `/tmp/gs-out`: only `foo_opt.gif` remains; the second job replaced the first.

**Expected:** The app refuses colliding targets or creates two unique outputs, and it never
replaces a source file silently.

**Actual:** The same path is used twice. Existing targets are overwritten without a
confirmation step.

**Fix:** Add `planBatchOutputs()` and compute every source/target pair before `setBusy(true)`.
Compare normalized absolute paths; use case-insensitive comparison on Windows and macOS where
appropriate. Reject duplicate targets and target-equals-source. Prompt once for pre-existing
files or default to non-destructive suffixing. Write to a temporary sibling file and rename
only after a valid non-empty GIF is produced.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-01`. Original report: ⬜ **OPEN** — the constant-template refusal exists (`MainWindow.cpp:730`), but
same-basename collisions and target-equals-source are not checked. **This is the #1 finding
across all audits.** See also C:F-01, D-01.

---

### A-02 / GS-002 [Critical] — The portable packager can report success for an incomplete or stale release

**Path:** `working_code/gifscythe/scripts/package_portable.sh` :: optional copy branches and
final sanity check

**Certainty:** Confirmed ✅ **EXEC**

**Impact:** A release can be published without the CLI or GUI, with old files from a prior
build, or with an incomplete Qt runtime while the packaging command still exits zero.

**Evidence:** The script does not clear the destination, treats CLI and GUI as optional,
converts `windeployqt` failure into a note, and verifies only that the engine exists.

**Reproduce:**
```
cd working_code/gifscythe && ./scripts/build_engine.sh
rm -rf build release/0.1.0/Gifscythe
./scripts/package_portable.sh; echo $?
```
The command exits 0 and creates an engine-and-docs folder without an application binary.

**Expected:** A desktop portable package is produced completely or the command fails.

**Actual:** The script prints 'Portable package created' for a partial bundle and can retain
stale binaries or DLLs.

**Fix:** Stage into a fresh `mktemp` directory and atomically replace the destination only
after validation. Require the exact engine, CLI, GUI, license set, and platform runtime for
the selected package type. Require `windeployqt` on Windows and propagate its non-zero status.
Run the packaged CLI from the staging folder and inspect dynamic dependencies before success.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-02`. Original report: ⬜ **OPEN** — confirmed by execution (C:U-02). The packager exits 0 with no GUI
binary in the folder.

---

### A-03 / GS-003 [High] — CLI run without output corrupts its own binary stdout stream

**Path:** `working_code/gifscythe/src/cli/main.cpp` :: `main()`: command print followed by
`run_argv()`

**Certainty:** Confirmed ✅ **EXEC**

**Impact:** A valid engine run exits zero but redirected output is not a valid GIF because
human-readable status lines are written to stdout before the engine bytes.

**Evidence:** Validation does not require output. `main()` always prints the banner and command
to stdout, then `run_argv()` lets an output-less gifsicle inherit that same stream.

**Reproduce:**
```
printf 'mode = auto\ninput = /absolute/path/to/gifscythe/reference_code/gifsicle/logo.gif\n' > /tmp/no-output.conf
working_code/gifscythe/build/gifscythe-cli /tmp/no-output.conf --run > /tmp/result.gif
head -c 6 /tmp/result.gif
```
The file begins with `# Gifs` rather than GIF87a or GIF89a, even if the exit code is 0.

**Expected:** Either require an output path or reserve stdout exclusively for GIF bytes.

**Actual:** Status text and binary data share stdout.

**Fix:** Simplest: reject `--run` when output is empty, matching the GUI's non-Explode policy.
If stdout output is a supported feature, send all diagnostics and the command preview to stderr
before exec. Add an explicit `--stdout` mode so binary behavior cannot happen accidentally.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-04`. Original report: ⬜ **OPEN** — confirmed by execution (C:U-04). The CLI writes status to stdout
before the engine bytes.

---

### A-04 / GS-004 [High] — Documented PATH engine fallback is rejected by the CLI preflight

**Path:** `working_code/gifscythe/src/core/EngineLocator.h` :: `locate_engine()` and
`path_is_executable()`

**Certainty:** Confirmed ✅ **EXEC**

**Impact:** The CLI says it supports a gifsicle found on PATH, but `--run` fails unless that
bare name also exists in the current directory.

**Evidence:** `locate_engine()` falls back to the string `"gifsicle"`. `path_is_executable()`
checks that string with `filesystem::is_regular_file` instead of searching PATH. `exe_path_of()`
also does not actually resolve `argv[0]` through PATH.

**Reproduce:**
```
mkdir -p /tmp/gs-cli /tmp/gs-engine /tmp/gs-empty
cp working_code/gifscythe/build/gifscythe-cli /tmp/gs-cli/
cp working_code/gifscythe/release/0.1.0/gifsicle /tmp/gs-engine/
cd /tmp/gs-empty && PATH=/tmp/gs-engine:$PATH /tmp/gs-cli/gifscythe-cli /absolute/test.conf --run
```
Observe 'engine not found at gifsicle' although `command -v gifsicle` succeeds.

**Expected:** PATH fallback resolves to an absolute executable and runs.

**Actual:** The fallback is a bare name that fails the preflight.

**Fix:** Search each PATH entry in `EngineLocator` and return the first executable absolute
path. On Windows, honor `PATHEXT` and use wide-character filesystem APIs. Resolve `argv[0]`
through PATH before deriving the executable directory. Treat a non-empty `GS_ENGINE` as a
strict override instead of silently falling through when it is invalid.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-05`. Original report: ⬜ **OPEN** — confirmed by execution (C:U-05). `locate_engine()` returns
`"gifsicle"` (bare name) as the last resort, and `path_is_executable("gifsicle")` checks
CWD, not PATH.

---

### A-05 / GS-005 [High] — The demo server is network-exposed and has no resource isolation

**Path:** `web/server.mjs` :: `server.listen()`, `readBody()`, `run()`

**Certainty:** Confirmed ✅ **EXEC**

**Impact:** Any host that can reach port 8000 can start concurrent native image-processing
jobs. Uploads can be 64 MB, settings can request extreme resize work, concurrency is unlimited,
and stderr is buffered without a cap.

**Evidence:** The server binds `0.0.0.0`, has no authentication or request queue, and passes
client-controlled settings to a native subprocess. The README correctly says not to expose it
publicly, but the default bind does exactly that on a local network.

**Reproduce:**
```
node web/server.mjs 8000
```
From another host on the same network, POST a GIF to `http://HOST:8000/optimize`. Repeat
requests in parallel with large files and aggressive dimensions; each request can create a
process for up to 120 seconds.

**Expected:** A local demo is loopback-only and bounded, or a network service has production
controls.

**Actual:** The zero-auth endpoint is reachable on every interface and can exhaust CPU,
memory, process slots, and disk.

**Fix:** Bind to `127.0.0.1` by default and require an explicit `--host` flag for remote
access. Validate GIF magic, settings types, pixel dimensions, frame count, and total work
before spawning. Add a small concurrency semaphore, queue limit, output/stderr caps, and
per-client rate limit. Sandbox the engine and document that the POC is not a deployable
service.

**Status:** ✅ **FIXED (S23)** — resolved; register §5 `U-06`. The loopback bind and `GS_WEB_HOST` opt-in landed in S8; the remaining bounds named here — engine-run concurrency, a per-client request window, and a configurable engine timeout — landed with `web/test/server-bounds.test.mjs`. Original report: ◐ **PARTIAL (S8)** — the concurrency cap, per-client rate limit and request-size bound were still missing.
`server.listen(PORT, "0.0.0.0", ...)`. No auth, no concurrency cap.

---

### A-06 / GS-006 [High] — Windows CLI execution is not Unicode-safe

**Path:** `working_code/gifscythe/src/core/ProcessRunner.h` :: `CreateProcessA()` and narrow
`std::string` paths

**Certainty:** Strong risk ✅ **SRC**

**Impact:** Input, output, engine, or settings paths containing characters outside the active
Windows code page can fail or resolve to the wrong path for international users.

**Evidence:** The Windows branch deliberately uses `CreateProcessA`. Settings and filesystem
paths cross the API boundary as `std::string`. Correct argument quoting fixes spaces but does
not fix character encoding.

**Reproduce:**
On Windows, create `C:\gifscythe-test\<non-ASCII-name>\input.gif`. Reference that path in a
settings file and run `gifscythe-cli.exe --run`. Repeat under a Windows user whose profile name
is not representable in the current ANSI code page.

**Expected:** Every valid Windows Unicode path works.

**Actual:** `CreateProcessA` interprets bytes through the active ANSI code page, so UTF-8 paths
are not reliably representable.

**Fix:** Use `CreateProcessW` and build a UTF-16 command line with the same quoting rules. Use
`std::filesystem::path/wstring` at Windows filesystem boundaries. Define UTF-8 as the
settings-file encoding and convert once at the boundary.

**Status:** ✅ **FIXED (S11)** — register §5 `U-07`. `CreateProcessW` + UTF-16 command line (strict
UTF-8 conversion, invalid input refused), `GetCommandLineW` argv re-fetch,
wide env reads, and `u8path_compat`/`path_u8string` at every string↔path
boundary (the MinGW libstdc++ narrow conversions are NOT UTF-8 — verified
under Wine). Executed proof (mingw cross-build + Wine 8): the pre-fix binary
fails an `é`-path conf with mojibake (`rÃ©sumÃ©…: No such file`, rc=1); the
fixed binary runs it rc=0 and writes the output; a CJK path reaches the child
process's UTF-16 command line byte-exact (probe). Unit suite green under Wine
(289 checks). Residual, documented: upstream gifsicle's own CRT re-encodes its
argv through the ACP (no wmain; `reference_code/` read-only), so characters
the system ACP cannot represent still need Windows' UTF-8-ACP option to reach
the ENGINE's file APIs — Gifscythe's own chain is lossless regardless.

---

### A-07 / GS-007 [High] — Release bundles do not carry a complete, unambiguous license set

**Path:** `LICENSE`, `scripts/package_portable.sh`, `scripts/package_system.sh` :: license
copy steps

**Certainty:** Confirmed ✅ **EXEC**

**Impact:** Release readiness and redistribution compliance are at risk. The root `LICENSE` is
a short notice saying the UI is 'intended' for GPLv3, not the full GPLv3 text. Qt LGPL notices
are not staged, and the system package omits the root UI license entirely.

**Evidence:** `package_portable` copies `LICENSE` and `COPYING.gifsicle` only. `package_system`
looks for a non-existent root `COPYING` and copies only the engine license from `reference_code`.
Neither script includes a full GPLv3 or LGPLv3 text or Qt attribution materials.

**Reproduce:**
```
cd working_code/gifscythe && ./scripts/package_portable.sh && ./scripts/package_system.sh
find release/0.1.0/Gifscythe* -maxdepth 1 -type f -printf '%f\n'
```
Verify that the system package lacks `LICENSE` and neither package contains the complete
GPLv3/LGPLv3 texts or Qt notices.

**Expected:** Each distributed bundle contains the applicable grants, complete license texts,
notices, and required source/relink information.

**Actual:** The staged legal material is incomplete and differs by package type.

**Fix:** Choose and state the first-party license as an actual grant, not only an intent
statement. Ship complete GPLv3, GPLv2-only, LGPLv3, and relevant Qt notices in a `LICENSES`
directory. Make both packagers require the same audited legal manifest. Have release counsel
confirm the final Qt and bundled-engine obligations before 1.0.0.

**Status:** ✅ **FIXED (S19)** — the silent half closed S8 (both packagers hard-require LICENSE + COPYING.gifsicle + COPYING.ms-pl, with negative tests); Ms-PL text + grant staged S18 (OD-09 = b); Qt LGPL remainder closed S19 (COPYING.lgplv3 + COPYING.gplv3 staged by both packagers, generated QT_NOTICE.txt in GUI packages, 36 packaging checks, CI manifest asserts the set); register §5 `U-08`. Original report: ⬜ **OPEN** — confirmed by execution (C:U-08). Root had `LICENSE` (1133 B at the time) and `COPYING.gifsicle` but **no `COPYING`**, so the `package_system` copy branch never fired.

---

### A-08 / GS-008 [High] — The banked Windows snapshot is not tied to the source revision claimed by its notes

**Path:** GitHub Release `snapshot-2026-09-07` and `CLEAN_WINDOWS_SMOKE.md` :: tag, release
body, preferred smoke asset

**Certainty:** Confirmed ✅ **EXEC**

**Impact:** Binary provenance is ambiguous, and the preferred clean-Windows test asset predates
the S7 code now on main. Passing that smoke cannot validate the current release candidate.

**Evidence:** The snapshot tag resolves to `5934339`, while the release body says source of
truth is `d3544b1`. Current main is `8190c08`. The checklist still recommends the old release
asset even though run #36 produced current-main artifacts.

**Reproduce:**
Open the GitHub tags API and note `snapshot-2026-09-07` → `5934339f`. Open the release body and
note its `d3544b1` source claim. Compare both with current main `8190c085` and Actions run #36.

**Expected:** The tag, source archive, workflow SHA, checksums, and attached binaries all identify
one commit.

**Actual:** At least three revisions are involved in the current release and smoke instructions.

**Fix:** Cut a new snapshot from exact SHA `8190c085` or its reviewed successor after fixes.
Attach artifacts from that SHA's green run and record artifact digests and run IDs. Create the
release tag at the exact build commit and add build provenance/attestation. Run the clean-Windows
checklist against that new published zip, not the S4-era asset.

**Status:** ⬜ **OPEN** — confirmed by execution (C:U-09). Release `snapshot-2026-09-07` is
from `5934339`, not `8190c08`.

---

### A-09 / GS-009 [High] — The read-only upstream boundary is already modified and Linux-specific

**Former path (before S13):** reference_code/gifsicle/config.h and `REFERENCE_MANIFEST.md` :: handwritten
`config.h` used by `build_engine.sh`

**Certainty:** Confirmed ✅ **EXEC**

**Impact:** The manifest's 'identical to upstream master' claim is false, reproducibility is
weakened, and the native build silently assumes 64-bit glibc/Linux values even though the
script advertises Linux/macOS.

**Evidence:** Upstream commit `07f5c4c` has no tracked root `config.h`. This repository adds a
handwritten `config.h` under `reference_code` with `SIZEOF_UNSIGNED_LONG=8`,
`SIZEOF_VOID_P=8`, glibc `random()`, and Linux headers. The product build depends on it despite
the rule that `reference_code` is never edited.

**Reproduce:**
Before S13, compare reference_code/gifsicle/config.h with upstream commit `07f5c4c3`. Observe that upstream
has no tracked root `config.h` and the local file says it is handwritten for Linux/gcc. Attempt
the advertised native build on macOS, 32-bit Linux, or a non-glibc target.

**Expected:** The upstream tree is immutable and platform configuration is generated in the
product build tree.

**Actual:** A product-specific Linux configuration lives inside and mutates the canonical
reference snapshot.

**Fix:** Move product-owned configuration under `working_code/gifscythe/build_support`. Generate
`config.h` per target with configure/CMake feature checks or maintain explicitly named target
configs. Pin and verify upstream tree hashes in CI; document the one intentional patch series if
patches are needed. Correct the manifest's identity claim.

**Status:** ◐ **PARTIAL (S13)** — register §5 `U-10`. Provenance and configuration relocation are
CLOSED with executed proof: `reference_code/gifsicle/` no longer contains the
product-owned header; `working_code/gifscythe/build_support/gifsicle/config.native.h`
is staged as `config.h` in a temporary include directory by
`scripts/build_engine.sh`. The native engine build, full CLI/unit build,
engine pipeline, and smoke suite all pass after the move. The S11 upstream
comparison remains valid after removing the sole local reference-tree file;
updated digests are recorded in `reference_code/REFERENCE_MANIFEST.md`. Still
open: CI hash-pinning
(proposal-only — needs `workflows` scope).

---

### A-10 / GS-010 [Medium] — Unknown CLI arguments print an error but can still exit successfully

**Path:** `working_code/gifscythe/src/cli/main.cpp` :: argument loop

**Certainty:** Confirmed ✅ **EXEC** — **stronger than stated**

**Impact:** Automation can contain a misspelled option, receive an error message, and still treat
the command as successful in print mode.

**Evidence:** The parser writes 'unknown argument' but does not set an error state or return
usage status 2.

**Reproduce:**
```
working_code/gifscythe/build/gifscythe-cli working_code/gifscythe/examples/animation.conf --rnu
echo $?
```
The command continues to print the generated line and returns 0 in non-run mode.

**Expected:** Any unknown or incomplete option returns 2 before loading or executing a settings
file.

**Actual:** The error is non-fatal.

**Correction (from consolidated audit):** **Nothing is printed at all** for unknown arguments —
the CLI silently ignores `--rnu`, `--engine` with no value, and other malformed options. Exit
code is 0. **A understated this finding.**

**Fix:** Parse into a typed `Options` struct. Return 2 immediately for unknown flags, missing
`--engine` values, duplicate incompatible arguments, and misplaced operands.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-23`. Original report: ⬜ **OPEN** — confirmed by execution (C:U-23). Unknown args are silently ignored,
exit 0.

---

### A-11 / GS-011 [Medium] — Malformed booleans and several numeric states degrade silently

**Path:** `working_code/gifscythe/src/core/SettingsIO.h` and `Validate.h` :: `parse_bool()`,
`set_field()`, `validate()`

**Certainty:** Confirmed ✅ **EXEC**

**Impact:** A typo can turn a feature off with no warning, while non-finite or nonsensical
scale/thread/loop values are incompletely validated or simply omitted by the command builder.

**Evidence:** `parse_bool()` returns false for every value outside its small true set, so
`careful=treu` is indistinguishable from `careful=false`. Validation covers only selected ranges
and the CLI treats all validation findings as warnings even for `--run`.

**Reproduce:**
```
printf 'careful = treu\nscale_x = nan\nloopcount = -9\ninput = a.gif\n' > /tmp/bad-state.conf
working_code/gifscythe/build/gifscythe-cli /tmp/bad-state.conf 2>/tmp/warnings
```
Observe no warning for the invalid boolean and incomplete rejection of the invalid states.

**Expected:** Invalid syntax is distinguished from false, and impossible execution settings block
a run.

**Actual:** Some malformed values silently become false/default or vanish from argv.

**Fix:** Replace `parse_bool` with `optional<bool>` and warn on values outside explicit true/false
sets. Validate finite positive scale, resize dimensions, loopcount, threads, gamma, mode/output
requirements, and allowed enum strings. Separate errors from warnings; make `--run` fail on errors.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-11`. Original report: ⬜ **OPEN** — confirmed by execution (C:U-11). `parse_bool` maps anything outside
`1/true/yes/on` to `false` with no warning.

---

### A-12 / GS-012 [Medium] — The 'fully async' GUI still has synchronous waits on the UI thread

**Path:** `working_code/gifscythe/src/qtui/MainWindow.cpp` :: `waitForStarted(5000)`,
`waitForFinished(3000/1000)`

**Certainty:** Confirmed ✅ **SRC**

**Impact:** A slow process start, network filesystem, security scanner, or stubborn process can
freeze the interface for up to several seconds during run, preview cancellation, or user
cancellation.

**Evidence:** `runCommand` waits up to five seconds after `QProcess::start`. `cancelRun` and
`killPreview` also block. The offscreen harness proves steady-state processing is async, not that
these waits never stall.

**Reproduce:**
Point `GS_ENGINE` at an executable on a deliberately slow/unavailable network path or a launcher
that delays process creation. Click Optimize and observe the event loop during the five-second
start window. Use an engine process that ignores termination and click Cancel to exercise the
three-second wait.

**Expected:** Run and cancel transitions are signal/timer driven and never wait in a GUI callback.

**Actual:** Several callbacks synchronously wait on `QProcess`.

**Fix:** Use `started`, `finished`, and `errorOccurred` signals for the entire state machine. Use
a `QTimer` for start/cancel deadlines; escalate terminate to kill asynchronously. Increment the
preview generation when killing so old completions cannot update the panel.

**Status:** ⬜ **OPEN** — confirmed by source read (C:U-12); **scoped as P1-24 in §6 (S11)**.
Five synchronous waits remain (line numbers re-measured in S11):
`waitForStarted(5000)` at `:842` and `:904`, `waitForFinished(2000)` at `:171`,
`waitForFinished(3000)` at `:919`, `waitForFinished(1000)` at `:1086`.

---

### A-13 / GS-013 [Medium] — Drag-and-drop accepts every existing local path, not only GIF files

**Path:** `working_code/gifscythe/src/qtui/MainWindow.cpp` :: `onFilesDropped()`

**Certainty:** Confirmed ✅ **SRC** — **identical to B:BUG-02**

**Impact:** PNG, text files, and directories can enter a GIF-only queue. The UI labels them as
GIF files and enables Run, deferring failure to the engine.

**Evidence:** The condition is effectively `suffix-is-gif OR QFileInfo::exists`, so any existing
path passes. The audit already listed this as a residual risk but the current status still calls
drag/drop done.

**Reproduce:**
Create `/tmp/not-a-gif.txt`. Drag it onto the Input queue. Observe that it is added and the run
control becomes available.

**Expected:** Only existing regular GIF files are accepted, with rejected-item feedback.

**Actual:** Existence bypasses the extension check.

**Fix:** Require `QFileInfo(f).isFile()` and a `.gif` suffix. Read the first six bytes and
require `GIF87a` or `GIF89a` before queue insertion. Report how many dropped paths were rejected
and why.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-13`. Original report: ⬜ **OPEN** — confirmed by source read. `MainWindow.cpp:426`:
```cpp
if (f.endsWith(".gif", Qt::CaseInsensitive) || QFileInfo::exists(f))
  gifs << f;
```
The `||` should be `&&`. **Identical to B:BUG-02 / U-13.**

---

### A-14 / GS-014 [Medium] — Green CI does not enforce several claims used as release gates

**Path:** `.github/workflows/build.yml` :: Windows deploy, packaging, and test steps

**Certainty:** Confirmed ✅ **EXEC**

**Impact:** Run #36 is genuinely green, but it can still produce an incomplete deploy and does
not exercise the web parity test, negative packaging cases, sanitizers, or lint/static analysis.

**Evidence:** `windeployqt` is guarded by `if command -v`, the Linux package check inherits the
weak packager, and `node web/test/command.test.mjs` is absent. Dependencies such as `aqtinstall`
and `actions` are not pinned to immutable revisions.

**Reproduce:**
Review `.github/workflows/build.yml` and compare it with `docs/release/RELEASE_PROCEDURE.md`
preflight requirements. Note that the release procedure requires web parity but the workflow never
runs it. Note that missing `windeployqt` does not fail the Windows job.

**Expected:** Every objective release gate is machine-enforced in CI.

**Actual:** Some gates are optional, local-only, or documented without enforcement.

**Fix:** Fail unless `windeployqt` is present and verify staged Qt DLLs/plugins. Run web
parity/API tests, package negative tests, ShellCheck, compiler warnings-as-errors, and sanitizer
jobs. Pin actions by commit SHA and `aqtinstall` by version/hash. Upload only validated package
roots, not a mixture of raw build directories.

**Status:** ◐ **PARTIAL (S8)** — resolved; register §5 `U-14`. Original report: ⬜ **OPEN** — confirmed by execution (C:U-14). `verify_audit.sh` is never run in CI;
package contents are never asserted.

---

### A-15 / GS-015 [Medium] — CMake mutates the source tree and the secondary qmake path drifts

**Path:** `working_code/gifscythe/CMakeLists.txt` and `gifscythe.pro` :: `configure_file()` and
`VERSION = 0.1.0`

**Certainty:** Confirmed ✅ **SRC**

**Impact:** Read-only or hermetic builds can fail, parallel build trees can race over a committed
header, and qmake retains a hardcoded product version despite the single-source version rule.

**Evidence:** CMake writes generated `version.h` both to the build tree and back into `src/core`.
Source files include the relative source header, so the generated include is not actually the sole
build input. `build.sh` also tries qmake before its stated preferred CMake path.

**Reproduce:**
Make `working_code/gifscythe/src` read-only and run `cmake -S . -B /tmp/gs-build`. Search
`gifscythe.pro` for `VERSION = 0.1.0`. Run `./build.sh --all` where `qmake6` is installed and
observe that qmake is attempted before CMake.

**Expected:** Out-of-tree builds leave the checkout untouched and every build system consumes one
generated version source.

**Actual:** Configure writes into source and duplicate build paths can disagree.

**Fix:** Generate `version.h` only under `CMAKE_BINARY_DIR` and include `core/version.h` through
generated-first include paths. Stop committing the generated fallback or generate it only in the
explicit build script. Remove qmake if it is not maintained, or derive its version from
`VERSION.md` and test it in CI.

**Status:** ✅ **FIXED (S11)** — register §5 `U-15`. The second `configure_file` is gone; the
template moved to `build_support/version.h.in` and the generated dir now comes
FIRST on every include path (`core/version.h` includes converted to path
form). Executed repro of the audit's own scenario, before vs after: with
`src/` read-only (uid 65534) the OLD CMakeLists dies with "Could not open file
for write in copy operation …/src/core/version.h.tmp", the NEW one configures
AND builds; deleting the committed `src/core/version.h` outright, the new tree
still configures+builds+passes its unit tests from the generated header alone.
Regression gate: `verify_audit.sh` **C9** (disposable copy, fallback deleted).
`build.sh` remains the only writer of the committed fallback (A5 stays
green); the qmake `VERSION =` line stays informational (secondary path,
documented in the .pro header).

---

### A-16 / GS-016 [Medium] — Settings persistence is non-atomic and crosses a narrow-path loader

**Path:** `working_code/gifscythe/src/qtui/MainWindow.cpp` :: `saveSessionState()` and
`loadSessionState()`

**Certainty:** Strong risk ✅ **SRC** — **identical to C:F-06 / U-16**

**Impact:** A crash, full disk, or interrupted close can truncate the only settings file. On
Windows, a non-ASCII config path is converted to `std::string` before `std::ifstream` opens it.

**Evidence:** The save path opens `QFile` with `WriteOnly|Truncate` and then writes in place. The
load path calls `load_settings_file(path.toStdString())`. Qt already provides `QSaveFile` for
atomic replacement and Unicode-safe `QFile` access.

**Reproduce:**
Change settings, then interrupt the process while the close handler is writing `gifscythe.conf`.
Simulate a short write/full disk and relaunch. On Windows, repeat under a profile/config path
containing characters outside the active code page.

**Expected:** The old file survives until a complete new file is flushed and committed; every
Unicode path loads.

**Actual:** The existing file is truncated before the replacement is known to be complete.

**Fix:** Use `QSaveFile`, check `write()`, `flush()/commit()`, and preserve the previous file on
failure. Load with `QFile/QTextStream`, then pass bytes through `load_settings(std::istream&)`
rather than reopening a narrow path. Add a format version and explicit UTF-8 encoding.

**Status:** ✅ **FIXED (S10)** — resolved; register §5 `U-16`. Original report: ⬜ **OPEN** — confirmed by source read. `MainWindow.cpp:1086-1096`: opens with
`WriteOnly|Truncate`, writes in place. **Identical to C:F-06 / U-16.**

---

### A-17 / GS-017 [Medium] — Explode mode can claim success without verifying any frame output

**Path:** `working_code/gifscythe/src/qtui/MainWindow.cpp` :: `onProcessFinished()`

**Certainty:** Confirmed ✅ **SRC** — **identical to B:BUG-08 / U-17**

**Impact:** The UI can display 'Optimization complete' after exit 0 even if no expected
`.000/.001` frame artifacts exist.

**Evidence:** Output existence is checked only when `batchMode_` is not `Explode`. There is no
explode-specific enumeration or before/after artifact count.

**Reproduce:**
Use Explode mode with a writable-looking prefix whose output is removed or redirected by a test
engine that exits 0 without writing. Run and observe the completion status despite zero frame
files.

**Expected:** At least one newly-created, non-empty, valid frame is required before success is
reported.

**Actual:** Exit code alone is treated as success for Explode.

**Fix:** Snapshot matching files before start and enumerate new `prefix.NNN` files after finish.
Require a non-zero count and validate GIF headers; surface the exact prefix searched on failure.

**Status:** ✅ **FIXED (S11)** — new `src/core/ExplodeVerify.h` (Qt-free,
shared): snapshot `<prefix>.*` before the run, require ≥1 NEW-or-CHANGED file
with GIF87a/GIF89a magic after exit 0; failures name the exact prefix and
directory. Wired into `onProcessFinished` (dialog + honest status, success
reports the verified frame count) and the CLI `--run` path (rc=1 + stderr).
Executed: lying engine (exits 0, writes nothing) refused by unit test 33,
smoke cases 9–11 (incl. the empty-output CWD-basename prefix rule read off
upstream `gifsicle.c:778`), harness T7 (`fake_engine_exit0` fixture), and
under Wine (rc=1, prefix named). **Identical to B:BUG-08 / U-17.**

---

### A-18 / GS-018 [Medium] — The current regression suite misses the failure classes above

**Path:** `working_code/gifscythe/tests` and `scripts/*test*.sh` :: unit, smoke, and offscreen
coverage

**Certainty:** Confirmed ✅ **EXEC**

**Impact:** The documented 243 GUI checks and green smoke suite create useful confidence, but they
do not protect output collisions, PATH-only engine discovery, strict CLI parsing, binary stdout,
packaging failures, Unicode paths, or the web server.

**Evidence:** `EngineLocator` is not included by the core unit test, the CLI smoke has no
unknown-option/PATH/stdout cases, T16 covers only constant templates, and the web test checks
command parity rather than the server lifecycle.

**Reproduce:**
Run `./scripts/verify_audit.sh` and observe green results. Then run the GS-001, GS-003, GS-004,
or GS-010 reproducer; those behaviors are outside the suite.

**Expected:** Tests fail when a known high-impact failure is present.

**Actual:** The suite can stay green while reproducible release blockers remain.

**Fix:** Add focused tests before fixing each finding so the red-to-green transition is visible.
Split the monolithic custom test main into named tests or at least named executables for clearer
CI evidence. Add fuzz targets for `SettingsIO` and the GIF engine boundary, plus ASan/UBSan
coverage in CI.

**Status:** ✅ **FIXED (S12)** — resolved; register §5 `U-18`. Original report: ⬜ **OPEN** — confirmed by execution (C:U-18). T16 uses `a.gif`/`b.gif` (distinct
stems), so same-basename collision is untested.

---

### A-19 / GS-019 [Low] — Web engine discovery sorts semantic versions lexicographically

**Path:** `web/server.mjs` :: `findEngine()`: `versions.sort().reverse()`

**Certainty:** Confirmed ✅ **SRC** — **identical to C:F-12 / U-26**

**Impact:** Once `release/` contains `0.9.0` and `0.10.0`, the demo can select the older engine
folder unexpectedly.

**Evidence:** String ordering places `'0.9.0'` after `'0.10.0'`, so reverse lexicographic order
is not semantic-version order.

**Reproduce:**
Create `release/0.9.0` and `release/0.10.0` with engine stubs. Start the web server and inspect
the logged engine path.

**Expected:** The current product version or highest semantic version is selected.

**Actual:** The lexically greatest directory is selected.

**Fix:** Read the version from `VERSION.md`, or parse numeric semver components before sorting.
Prefer an explicit `GS_ENGINE` and fail strictly when it is invalid.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-26`. Original report: ⬜ **OPEN** — confirmed by source read. `server.mjs:52`:
`versions.sort().reverse()`. **Identical to C:F-12 / U-26.**

---

### A-20 / GS-020 [Low] — Current-state documents still describe an already-completed CI action

**Path:** `SESSION_HANDOFF.md`, `WORKLIST.md`, `docs/ci/README.md` :: S7 push/CI status

**Certainty:** Confirmed ✅ **SRC** — **corrected by S7**

**Impact:** A maintainer following the ordered worklist may repeat completed work or misunderstand
which verification is still open.

**Evidence:** The documents say the S7 branch must be pushed and CI must be confirmed. It was
merged as PR #7, and Actions run #36 completed successfully on both Linux and Windows for main
SHA `8190c085`.

**Reproduce:**
Compare the three documents with GitHub Actions run `34425977060` and commit `8190c085`.

**Expected:** The operational handoff reflects current main and clearly labels historical snapshots.

**Actual:** The handoff and CI summary stop immediately before the completed merge/run.

**Fix:** Update the worklist to mark S7 merge and run #36 green, including artifact digests and
expiry. Keep dated reviews immutable but put a prominent 'superseded by' header on historical
status sections. Generate volatile CI status in one file rather than copying it through several
documents.

**Status:** ✅ **FIXED** — register §5 `U-27`. S7 merged, CI green on both jobs (runs `34425977060` /
`34427315414`).

---

## 4. Audit B (Seed 2.1 Pro Preview) — 16 findings (full text)

*Source: `AUDIT_B_extracted.md` (now deleted — content merged below)*

### B-01 / BUG-01 [High] — Threads 'Auto' setting does not enable auto-threading

**File:** `src/core/GifsicleCommand.h:210`

**Verified:** ✅ **EXEC** + SRC

**Finding:** The GUI labels `threads=0` as 'Auto' (implying auto-detect CPU count with bare
`-j`), but the command builder only emits `-jN` when `threads > 0`. `threads=0` emits NO flag,
so gifsicle runs single-threaded (its default). The `Settings.h` comment even says `-j; <=0 =
auto`, but the build logic doesn't implement 0 as bare `-j`.

**Reproduction:** Run CLI with `threads = 0` in conf or set GUI Threads to 'Auto' (0). Observe
generated command lacks `-j`. Gifsicle runs with 1 thread, not auto-detected count.

**Current:**
```cpp
if (s.threads > 0) { add(args_, "-j" + i2s(s.threads)); }
```
→ `threads=0` adds nothing.

**Expected:** `threads=-1`: no flag (gifsicle default, 1 thread). `threads=0`: bare `-j`
(auto-detect online processors). `threads>=1`: `-jN`.

**Fix:**
```cpp
if (s.threads == 0) {
  add(args_, "-j");
} else if (s.threads > 0) {
  add(args_, "-j" + i2s(s.threads));
}
```
Update `Validate.h` to allow `threads == 0`. Ensure CLI/GUI default of -1 stays 'no flag' and
GUI 0 maps to bare `-j`.

**Evidence:** `reference_code/gifsicle/src/gifsicle.c:38`:
`const int GIFSICLE_DEFAULT_THREAD_COUNT = 8;` — bare `-j` sets this. `gifsicle.c:39`:
`int thread_count = 0;` — default is 0 (single-threaded). `gifsicle.c:1887-1893`: bare `-j`
sets `thread_count = GIFSICLE_DEFAULT_THREAD_COUNT (8)`. So `threads=0` in the GUI → no flag →
`thread_count` stays 0 → **single-threaded**. Real auto is bare `-j` → 8 threads.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-03`. Original report: ⬜ **OPEN** — confirmed by source read + execution. This is **U-03** in the
consolidated register. **B is right; Audit C's "benign" claim was wrong.**

---

### B-02 / BUG-02 [Medium] — Drag-and-drop filter accepts ANY existing file, not just GIFs

**File:** `src/qtui/MainWindow.cpp:426`

**Verified:** ✅ **SRC** — **identical to A:GS-013 / U-13**

**Finding:** `onFilesDropped` uses OR: `endsWith(".gif") || QFileInfo::exists(f)`. Any existing
file (`.exe`, `.txt`, `.jpg`) passes the filter and is added to the queue. The user only
discovers the problem at run-time when gifsicle fails.

**Reproduction:** Drag a file that exists on disk but is not a `.gif` (e.g. a `README.txt`) onto
the queue. It is added. Click 'Optimize GIF' → gifsicle errors on non-GIF input.

**Current:**
```cpp
if (f.endsWith(".gif", Qt::CaseInsensitive) || QFileInfo::exists(f)) gifs << f;
```

**Expected:** Only files that are GIFs should be added. Condition should require BOTH: extension
is `.gif` AND file exists (and perhaps also sniff the header for non-`.gif` extensions).

**Fix:**
```cpp
if (f.endsWith(".gif", Qt::CaseInsensitive) && QFileInfo::exists(f)) gifs << f;
```
Also consider adding a failed-to-add feedback for non-GIF drops.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-13`. Original report: ⬜ **OPEN** — confirmed by source read. **Identical to A:GS-013 / U-13.**

---

### B-03 / BUG-03 [Low] — runCommand: missing `return` after waitForStarted failure on non-batch path

**File:** `src/qtui/MainWindow.cpp:804-808`

**Verified:** ✅ **SRC**

**Finding:** When Merge/Auto/Explode engine fails to start, the code shows a critical message box
but does NOT `return;` afterwards (unlike the Batch path which does return at line 773). Function
falls through. Today this is harmless because no code follows the if-block, but any future code
added after line 808 would run in the error path.

**Reproduction:** Code inspection only. Trigger `waitForStarted` failure (e.g. rename engine
binary after launch), run Merge mode.

**Current:** After `QMessageBox::critical(...)` there's no `return;` — closing brace ends the
function.

**Expected:** Consistent early return like the batch branch.

**Fix:** Add `return;` after the `QMessageBox::critical` block (line ~808), mirroring line 772.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-28`. Original report: ⬜ **OPEN** — confirmed by source read. **Identical to C:U-28.**

---

### B-04 / BUG-04 [Medium] — Web UI defaults 'Scale %' to 50, desktop defaults to 100

**File:** `web/index.html:81`

**Verified:** ✅ **SRC** — **identical to C:U-25**

**Finding:** Desktop scale spinboxes default to 100% (= no scale factor change). Web scalePct
defaults to 50 (= half size). A user who selects 'Scale %' expects 'no change' (100%) as the
default, not 50%.

**Reproduction:** Open web UI, select resize 'Scale %'. The scale field shows 50, which would
halve the image on run.

**Current:**
```html
<input type="number" id="scalePct" min="1" max="1000" value="50" step="0.1" />
```
Desktop `SettingsPanel.cpp:183`: `scaleXSpin_->setValue(100.0);`

**Expected:** Default 100 (no-op scale) to match desktop.

**Fix:** Change `value` from 50 to 100 in `web/index.html:81`.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-25`. Original report: ⬜ **OPEN** — confirmed by source read. **Identical to C:U-25.**

---

### B-05 / BUG-05 [Low] — Web missing 'Touch' resize option in HTML dropdown

**File:** `web/index.html:70-77`

**Verified:** ✅ **SRC** — **identical to C:U-29**

**Finding:** Desktop Resize has 6 modes (None, Fit, Touch, Exact, Scale, Width, Height).
`command.mjs` implements the 'touch' case correctly (line 91), but the web `<select>` omits the
option, so users can't select it from the UI.

**Reproduction:** Inspect resize select in `web/index.html` — no 'touch' option.

**Current:** Options: none, fit, exact, width, height, scale. 'touch' is supported by
`command.mjs` but not exposed.

**Expected:** Include Touch option for parity.

**Fix:** Add
```html
<option value="touch">Touch W×H (resize to fit within WxH if larger)</option>
```
after the 'Fit' option.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-29`. Original report: ⬜ **OPEN** — confirmed by source read. **Identical to C:U-29.**

---

### B-06 / BUG-06 [Medium] — Web server doesn't verify that out.gif exists / is non-empty after engine exits 0

**File:** `web/server.mjs:169`

**Verified:** ✅ **SRC** — **identical to C:U-24**

**Finding:** The desktop GUI explicitly verifies after exit code 0 that the output file exists and
is non-empty before declaring success (`MainWindow.cpp:850-862`). The Node server skips this
check and goes straight to `readFile(outFile)`. If gifsicle exits 0 but writes no file (or zero
bytes), the server throws ENOENT which becomes a generic HTTP 500 rather than a helpful 422.

**Reproduction:** Craft settings that produce no output (e.g. info mode or gifsicle that exits 0
without writing). Server stack-traces to 500.

**Current:** After `result.code === 0`: immediately `const outBytes = await readFile(outFile);`
with no existence/size check.

**Expected:** Check `existsSync(outFile)` and size before reading; return structured 422 if
missing/empty.

**Fix:** Insert after `result.code` check:
```javascript
if (result.code !== 0) { ... existing 422 ... }
const st = await stat(outFile).catch(() => null);
if (!st || st.size === 0) {
  res.writeHead(422, ...);
  res.end(JSON.stringify({ok:false, error:'engine produced no output', ...}));
  return;
}
```

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-24`. Original report: ⬜ **OPEN** — confirmed by source read. **Identical to C:U-24.**

---

### B-07 / BUG-07 [Low] — Web server has no Validate-equivalent; out-of-range values passed straight to engine

**File:** `web/server.mjs:123-166`

**Verified:** ✅ **SRC** — **identical to C:U-30**

**Finding:** The desktop GUI runs `gs::validate()` and refuses to run with out-of-range values
(colors, lossy, disposal, delay, empty inputs, crop w/h of 0, etc.). The web server spreads user
settings directly into `buildArgs`, relying on `buildArgs`'s own range checks and the engine to
reject. Most invalid flags are silently dropped by `buildArgs` (e.g. `color_count=999` fails
`>=2 && <=256` and is skipped) but some (e.g. malformed dither method strings) are passed to
gifsicle which returns 422 via its stderr. This is acceptable but gives inconsistent UX vs.
desktop.

**Reproduction:** POST `/optimize` with `{"color_count": 999}` — flag is silently dropped. No
warning returned to user.

**Current:** No validation layer server-side.

**Expected:** Either port `Validate.h` to JS or document that engine's stderr is the source of
truth.

**Fix:** Add a JS equivalent of the `Validate.h` checks in `server.mjs` and return 422 with a
user-friendly list of issues before invoking the engine. This provides parity with the GUI's
pre-flight dialog.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-30`. Original report: ⬜ **OPEN** — confirmed by source read. **Identical to C:U-30.**

---

### B-08 / BUG-08 [Medium] — Explode mode never verifies that frame files were produced

**File:** `src/qtui/MainWindow.cpp:850-862`

**Verified:** ✅ **SRC** — **identical to A:GS-017 / U-17**

**Finding:** Non-explode modes verify the output file exists and is non-empty (pessimal check
against silent engine failure). Explode mode is explicitly skipped (`batchMode_ != gs::Mode::Explode`).
A failed explode (gifsicle exits 0 but writes zero frames — e.g. wrong permissions in target dir)
is reported as 'Optimization complete.' even though no frames were written.

**Reproduction:** Run Explode into a directory where you have no write permission (but the output
prefix points there). Gifsicle may exit non-zero (caught), but a scenario where exit=0 but no
frames exist (empty file set) is not verified.

**Current:** Only checks `pendingOutput_` for non-explode. Explode writes `<prefix>.000`,
`<prefix>.001`, etc.

**Expected:** For explode, verify that at least one frame file matching `<prefix>.*` exists.

**Fix:** After an Explode run completes with exit 0, `QDir` the parent and look for files starting
with the prefix+'.' — fail with the same 'No output produced' dialog if none exist.

**Status:** ✅ **FIXED (S11)** — see A:GS-017: `ExplodeVerify.h` snapshot-diff
verification in CLI + GUI, proven with a lying engine at every test layer.
**Identical to A:GS-017 / U-17.**

---

### B-09 / BUG-09 [Info] — Web POC only supports single-file Auto mode; no batch/merge/explode

**File:** `web/app.js:17`

**Verified:** ✅ **SRC** — **identical to C:U-41**

**Finding:** The web UI hardcodes `mode:'auto'` and only supports single-file upload. Multi-file
batch, merge, and explode modes that exist in the desktop app are not present in the web POC. This
is called out as a POC but is a major feature gap for a web port.

**Reproduction:** `web/app.js` `settings()` returns `mode: 'auto'` with no mode selector in HTML.

**Current:** Single-file optimization only.

**Expected:** At minimum document the limitation clearly, or add a mode selector.

**Fix:** Add a `<select id="mode">` to the HTML and read it in `settings()`. Batch mode requires
multiple file uploads; Merge needs an output filename. Document as roadmap.

**Status:** ✅ **FIXED (S11)** — the web UI grew a mode selector (Auto/Batch/
Merge/Explode + by-name), a multi-file queue with per-file removal, and a
results list with per-output downloads; the server grew `POST /run` (JSON
multi-file, per-mode semantics, output verification). Scoped first as P2-11.
**Identical to C:U-41.**

---

### B-10 / BUG-10 [Info] — Web uses a single scalePct for both axes; desktop has separate X and Y

**File:** `web/app.js:27-28`

**Verified:** ✅ **SRC** — **identical to C:U-42**

**Finding:** Desktop exposes independent X and Y scale percentages (allowing anamorphic scaling).
The web UI links both axes to one slider. Acceptable for POC but a parity gap.

**Reproduction:** Only one `scalePct` input in `web/index.html:81`.

**Current:** Uniform scaling only.

**Expected:** Two inputs for non-uniform scaling.

**Fix:** Add `scaleXPct` and `scaleYPct` inputs; default both to 100.

**Status:** ✅ **FIXED (S10)** — resolved; register §5 `U-42`. Original report: ⬜ **OPEN** — confirmed by source read. **Identical to C:U-42.**

---

### B-11 / BUG-11 [Low] — CLI compile via build.sh may fail on older g++ (no -lstdc++fs link)

**File:** `working_code/gifscythe/build.sh:58-60`

**Verified:** ⏸ **BLOCKED** (g++ 12 here)

**Finding:** `EngineLocator.h` uses `std::filesystem` which on g++ < 9 requires linking with
`-lstdc++fs`. `build.sh` does not add this link flag. On Debian 12 / g++ 12 it works, but older
distros (Ubuntu 18.04 / g++ 7) will fail at link time with undefined references to
`std::filesystem` symbols.

**Reproduction:** Build on a system with g++ 8 or earlier.

**Current:** No `-lstdc++fs` in link line.

**Expected:** Portable link.

**Fix:** Add a small autodetect in `build.sh` (try linking with and without `-lstdc++fs`). CMake
handles this via C++17 standard + proper compiler detection; the ad-hoc g++ line does not.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-31`. Original report: ⬜ **OPEN** — confirmed by source read. Cannot test (g++ 12 only).

---

### B-12 / BUG-12 [Low] — Posix ProcessRunner returns 1 on signal/kill instead of 128+signum convention

**File:** `src/core/ProcessRunner.h:117-120`

**Verified:** ✅ **SRC**

**Finding:** If child is killed by a signal, the POSIX branch returns generic 1 (printing a
message). Unix convention (and what bash returns) is 128 + signal number. This makes it impossible
for callers to distinguish crashes from user-requested kills (though the GUI uses `cancelling_`
flag).

**Reproduction:** Send SIGSEGV to child process.

**Current:**
```cpp
return 1;  // after WIFSIGNALED
```

**Expected:** Return `128 + WTERMSIG(status)`.

**Fix:** Replace `return 1;` with `return 128 + WTERMSIG(status);`. Update Windows branch for
consistency if desired.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-32`. Original report: ⬜ **OPEN** — confirmed by source read. **Identical to C:U-32.**

---

### B-13 / BUG-13 [Low] — SettingsIO: setting only position_x OR position_y auto-enables has_position

**File:** `src/core/SettingsIO.h:105-110`

**Verified:** ✅ **SRC**

**Finding:** When loading a conf, setting `position_x` alone (or `position_y` alone) sets
`has_position = true`, leaving the other coordinate at default 0. This may produce an unexpected
`-p X,0` flag. It's a corner case because valid configs always write both coordinates together
(as `save_settings` does), but a hand-written partial config will produce a partial position.

**Reproduction:** Write a config containing `position_x = 100` but no `position_y`. Loading sets
`has_position=true`, `position_y=0`.

**Current:** Each key independently sets `has_position = true`.

**Expected:** Position should only be enabled when BOTH coordinates are provided, or neither.
Otherwise it should warn and not set `has_position`.

**Fix:** Change loader so setting `position_x` or `position_y` doesn't immediately flip
`has_position`; instead, after parsing all fields, set `has_position = (position_x_was_set &&
position_y_was_set)`. Alternatively warn if exactly one was provided.

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-33`. Original report: ⬜ **OPEN** — confirmed by source read. **Identical to C:U-33.**

---

### B-14 / BUG-14 [Low] — Temp preview files older than seq-1 can accumulate during long sessions

**File:** `src/qtui/MainWindow.cpp:992-993`

**Verified:** ✅ **SRC** — **identical to D-03 / C:U-34**

**Finding:** The completion lambda only removes preview file `seq - 1`. If a preview is killed
before its lambda fires (stale preview, killed by a newer preview), the killed preview's partial
output file is never removed. Over a long session with many rapid setting changes this leaks
`preview_1.gif` … `preview_{n-2}.gif` in the temp directory. The whole directory is removed at app
exit, so it's only a per-session disk leak.

**Reproduction:** Change settings rapidly for several minutes, `ls` the temp preview dir: multiple
`preview_*.gif` exist.

**Current:** Only the immediately preceding seq file is removed on completion of seq. Killed/stale
previews don't clean up.

**Expected:** All old preview files should be cleaned up when a new run starts, regardless of
whether the previous run completed.

**Fix:** At the top of `startPreview()`, before launching a new process, remove all `preview_*.gif`
in `previewDir_` instead of just the predecessor. Or track all created preview paths in a set and
clean up in `killPreview` or when a new one starts.

**Status:** ✅ **FIXED (S10)** — resolved; register §5 `U-34`. Original report: ⬜ **OPEN** — confirmed by source read. **Identical to D-03 / C:U-34.**

---

### B-15 / BUG-15 [Info] — CLI prints warnings but still executes --run even with blocking issues

**File:** `src/cli/main.cpp:131-174`

**Verified:** ✅ **EXEC** — **identical to C:U-40**

**Finding:** The GUI refuses to run when `validate()` returns non-empty warnings (after removing
the 'empty inputs' item). The CLI prints the warnings to stderr but proceeds with run anyway. This
is an intentional design difference (CLI is scriptable, warnings are advisory), but it means the
CLI can run with `colors=999` or `crop=0x0` etc. without blocking, which may surprise users coming
from the GUI.

**Reproduction:** Run `gifscythe-cli bad.conf --run`. Warnings are printed but execution continues.

**Current:** Non-zero exit code only from engine, not from validate warnings.

**Expected:** Add a `--strict` flag (or document the behavior).

**Fix:** Document the behavior in README. Optionally add `--strict` that treats warnings as fatal.

**Status:** ✅ **FIXED (S10)** — resolved; register §5 `U-40`. Original report: ⬜ **OPEN** — confirmed by execution. **Identical to C:U-40.**

---

### B-16 / BUG-16 [Low] — GUI setBusy(false) re-enables Run button without re-checking engine existence

**File:** `src/qtui/MainWindow.cpp:667-684`

**Verified:** ✅ **SRC** — **identical to C:U-35**

**Finding:** `setBusy(true)` stops the preview and disables controls; `setBusy(false)` re-enables
the Run button with `!busy && !inputs_.isEmpty()`. It does not call `ensureEngine()` like
`appendInputs` does. If the engine binary is deleted while a run is in progress (unusual), Run is
re-enabled and clicking it will fail at `runCommand`'s `ensureEngine()` check, but the button is
misleadingly enabled.

**Reproduction:** Engine file deleted mid-run; after run ends Run is enabled despite engine missing.

**Current:** No engine check when clearing busy state.

**Expected:** Run button enabled only when engine is present AND queue non-empty.

**Fix:** After `setBusy(false)` call, use the same pattern as `appendInputs`:
```cpp
runButton_->setEnabled(!busy && !inputs_.isEmpty() && ensureEngine());
```
(`ensureEngine` re-probes and updates status).

**Status:** ✅ **FIXED (S10)** — resolved; register §5 `U-35`. Original report: ⬜ **OPEN** — confirmed by source read. **Identical to C:U-35.**

---

## 5. Consolidated master register — all 96 unique findings

Deduplicated across A/B/C/D/E/F. "Src" = which audit(s) raised it.

### Critical / release-blocking

| ID | Src | Finding | Verif | Status |
|----|-----|---------|-------|--------|
| **U-01** | A:GS-001 · C:F-01 · D:GS-101 | **Batch auto-naming overwrites other outputs *and* the source GIF.** Preflight only rejects templates with no `{name}`; targets are never computed as a set, never compared to inputs, never checked for existence. | ✅ **EXEC** | ✅ FIXED (S8) — `src/core/OutputPlan.h` + CLI/GUI planning |
| **U-02** | A:GS-002 | **Portable packager reports success for an incomplete release.** Engine missing → hard fail, but CLI missing → silently skipped, GUI missing → `Note:` + continue, `windeployqt` errors → `Note:` + continue, licenses guarded by `if [[ -f ]]`, no clean staging dir. | ✅ **EXEC** | ✅ FIXED (S8) — packager fails closed + `scripts/test_package.sh` |

### High

| ID | Src | Finding | Verif | Status |
|----|-----|---------|-------|--------|
| **U-03** | B:BUG-01 · C:F-02 correction | **Threads "Auto" runs single-threaded.** `threads=0` emits no flag; gifsicle default is single-threaded. Real auto is bare `-j` (→ 8 threads). | ✅ **EXEC**+SRC | ✅ FIXED (S8) — bare `-j` for Auto (C++ + JS parity) |
| **U-04** | A:GS-003 | **CLI `--run` without `output` corrupts its own stdout.** Status text and binary data share stdout. | ✅ **EXEC** | ✅ FIXED (S8) — `--run` commentary moved to stderr |
| **U-05** | A:GS-004 | **Documented PATH engine fallback is dead code.** `locate_engine()` returns bare `"gifsicle"`; `path_is_executable()` checks CWD, not PATH. | ✅ **EXEC** | ✅ FIXED (S8) — real `find_on_path()`; `""` when not found |
| **U-06** | A:GS-005 · D:GS-102 | **Web demo binds `0.0.0.0` with no auth, no concurrency cap, 64 MB bodies, 120 s engine runs.** Concurrent requests can race on temp dirs. | ✅ **EXEC** | ✅ FIXED (S23) — loopback bind (S8) + engine semaphore `GS_MAX_CONCURRENT`/`GS_MAX_QUEUED` + `GS_RATE_LIMIT_PER_MIN` window + configurable `GS_ENGINE_TIMEOUT_MS`; 429 past the cap, refusals cost no capacity |
| **U-07** | A:GS-006 | **Windows CLI execution is ANSI-only.** `CreateProcessA` + `std::string` cmdline ⇒ non-ASCII paths cannot be passed to the engine. | ✅ **EXEC** (wine 8, mingw 12) | ✅ FIXED (S11) — `CreateProcessW` + argv/env re-fetch + u8path boundaries; wine E2E: é paths rc=0 (old build rc=1), CJK reaches the child losslessly |
| **U-08** | A:GS-007 | **License set can ship incomplete, silently.** Root has `LICENSE` + `COPYING.gifsicle` but **no `COPYING`**; every license copy is `if [[ -f ]]`-guarded. | ✅ **EXEC**+SRC | ✅ FIXED (S19) — both packagers stage COPYING.lgplv3 + COPYING.gplv3 + GUI QT_NOTICE.txt; 36 packaging checks; CI manifest asserts the set |
| **U-09** | A:GS-008 | **Banked Windows snapshot is 5 commits behind the SHA its own notes claim.** Release body pins `d3544b1`; main is `8190c08`. | ✅ **EXEC** | ⬜ OPEN |
| **U-10** | A:GS-009 | **The "read-only, identical-to-upstream" vendored engine is neither.** Carries a handwritten `config.h` (Linux values), a functional patch, and an extra test. | ✅ **EXEC** (S11 re-clone; S13 relocation) | ◐ PARTIAL (S13) — provenance and product-config relocation verified; `reference_code/gifsicle/` is now upstream-only and native build stages `build_support/gifsicle/config.native.h`. MISSING: CI hash-pinning (workflows scope) |

### Medium

| ID | Src | Finding | Verif | Status |
|----|-----|---------|-------|--------|
| **U-11** | A:GS-011 | **Malformed booleans degrade silently.** `parse_bool` maps anything outside `1/true/yes/on` to `false` with no warning. | ✅ **EXEC** | ✅ FIXED (S8) — `parse_bool_strict` warns, leaves field unchanged |
| **U-12** | A:GS-012 | **"Fully async" GUI still blocks the UI thread in 5 places** — up to 5 s per run start. | ✅ **SRC** | ⬜ OPEN — scoped as P1-24 (S11); implementation deferred: the freeze is not reproducible offscreen, and the cancel rewrite would rewire T2/T9/T10 semantics with no executable proof of improvement |
| **U-13** | A:GS-013 · B:BUG-02 · D:GS-104 | **Drag-and-drop accepts any existing file.** Filter is `endsWith(".gif") \|\| exists(f)` — should be `&&`. Also: **empty comments emit `--comment` with no argument**, corrupting argv. | ✅ **SRC** | ✅ FIXED (S8) — drop filter `&&`; empty comments skipped (C++ + JS) |
| **U-14** | A:GS-014 | **Green CI does not enforce the claims used as release gates.** `verify_audit.sh` is never run in CI; package contents are never asserted. | ✅ **EXEC** | ◐ PARTIAL (S8) — packaging negatives + manifest assert + the doc gate run in CI; verify_audit itself stays out by design (job time) — S24: workflows scope is no longer the blocker |
| **U-15** | A:GS-015 | **CMake writes into the source tree.** `configure_file` targets `${CMAKE_SOURCE_DIR}/src/core/version.h`. | ✅ **EXEC** | ✅ FIXED (S11) — build-tree-only configure_file (`build_support/version.h.in`); generated-first includes; gate C9 + read-only-src repro flipped FAIL->PASS |
| **U-16** | A:GS-016 · C:F-06 | **Settings persistence is non-atomic** (Truncate + write). A crash mid-write leaves a truncated conf. | ✅ **SRC** | ✅ FIXED (S10) — `save_settings_file` is tmp+fsync+rename; GUI save is QSaveFile; unit test 32 + T19 no-stray check |
| **U-17** | A:GS-017 · B:BUG-08 | **Explode mode never verifies any frame was written.** Output verification is explicitly skipped for Explode. | ✅ **EXEC** | ✅ FIXED (S11) — `src/core/ExplodeVerify.h` snapshot-diff (CLI+GUI); lying engine (rc=0, 0 frames) refused: unit 33, smoke 9-11, harness T7, wine rc=1 |
| **U-18** | A:GS-018 | **The regression suite does not cover any of the failure classes above.** No test for target collisions, package completeness, stdout purity, PATH fallback, or thread flags. | ✅ **EXEC** | ✅ FIXED (S12) — unit planning/thread coverage, packaging negatives, strict CLI parsing, byte-pure stdout, PATH-only engine discovery, and unsafe-output refusal; smoke suite 21/21 |
| **U-19** | C:F-02 | **`readFrom()` is not the "exact inverse" of `writeInto()`.** Crop geometry, position and scale are serialized only when their parent toggle is on. | ✅ **EXEC** | ☑ CORRECTED (S8) — not reproducible with toggles on; wording fixed, pinned by unit test 26 |
| **U-20** | C:F-03 | **"The CLI reads GUI-saved files without warnings" is false.** A real GUI-saved file has no `input` key, so `validate()` warns. | ✅ **EXEC** | ☑ CORRECTED (S8) — doc claim reworded; the `input` warning is expected |
| **U-21** | C:F-04 · D:GS-101 | **Name-template sanitisation is POSIX-only** — no Windows invalid chars, no trailing dot/space trim, no reserved-name guard. | ✅ **EXEC** | ✅ FIXED (S8) — new `src/core/OutputName.h` (`NameRules` parameterised, so the Windows rule set is unit-tested on Linux); test 29, 30 assertions |
| **U-22** | C:F-05 | **`Validate.h` skips resize geometry**, so a conf can reach the engine with `--resize-fit 0x0`. | ✅ **EXEC** | ✅ FIXED (S8) — resize/scale geometry validated (rules probed off the engine) |
| **U-23** | A:GS-010 | **Unknown CLI arguments are silently ignored** — stronger than A's wording: nothing is printed at all, rc=0. | ✅ **EXEC** | ✅ FIXED (S8) — strict arg parser, rc=2 |
| **U-24** | B:BUG-06 · D:GS-102 | **Web server doesn't verify `out.gif` exists/non-empty after rc=0**, unlike the desktop. A zero-byte result throws ENOENT → generic **500** instead of 422. | ✅ **SRC** | ✅ FIXED (S8) — 422 when rc=0 produced no output |
| **U-25** | B:BUG-04 | **Web "Scale %" defaults to 50%, desktop to 100%.** A web user picking Scale gets half-size output by default. | ✅ **SRC** | ✅ FIXED (S8) — web Scale default 100 |

### Low

| ID | Src | Finding | Verif | Status |
|----|-----|---------|-------|--------|
| **U-26** | A:GS-019 · C:F-12 | Web engine discovery sorts versions lexicographically (`0.9.0` > `0.10.0`). | ✅ **SRC** | ✅ FIXED (S8) — numeric version compare |
| **U-27** | A:GS-020 | Current-state docs still listed a completed CI action as pending. **Fixed by S7.** | ✅ **SRC** | ✅ FIXED — S7 doc re-sync (§6 P3-2); the completed CI action is no longer listed as pending |
| **U-28** | B:BUG-03 | `runCommand()` non-batch path has **no `return`** after the "could not start engine" dialog (batch path does return). Harmless today; a landmine for the next edit. | ✅ **SRC** | ✅ FIXED (S8) — `return` added (CI-compiled) |
| **U-29** | B:BUG-05 | Web resize dropdown omits **Touch**, though `command.mjs:90-91` implements it and the desktop has 6 kinds. | ✅ **SRC** | ✅ FIXED (S8) — Touch option added |
| **U-30** | B:BUG-07 | Web server has no `validate()` equivalent; out-of-range values go straight to the engine. Inconsistent UX vs desktop. | ✅ **SRC** | ✅ FIXED (S8) — new `web/validate.mjs` mirrors `core/Validate.h`; 422 + issues; `web/test/validate.test.mjs` 19/19 parity vs the real CLI |
| **U-31** | B:BUG-11 | `build.sh` never links `-lstdc++fs`; `EngineLocator.h` uses `std::filesystem`, so g++ 7/8 hosts fail at link. | ✅ **EXEC** (g++ 12 here) | ✅ FIXED (S8) — `build.sh` link-probes `-lstdc++fs` (writes `build/.fs_probe.cpp`, tries with/without, cleans up) |
| **U-32** | B:BUG-12 | POSIX `run_argv` returns **1** on a signalled child instead of the `128+signum` convention. | ✅ **SRC** | ✅ FIXED (S8) — `run_argv` returns `128+WTERMSIG`; test 28: SIGTERM→143, SIGKILL→137, `exit 3`→3, missing binary→127 |
| **U-33** | B:BUG-13 | Setting only `position_x` **or** `position_y` in a conf sets `has_position = true`, yielding a half-specified `-p X,0`. | ✅ **SRC** | ✅ FIXED (S8) — `-p` needs both halves |
| **U-34** | B:BUG-14 · D:GS-103 | **Preview temp files leak within a session:** cleanup removes only `preview_{seq-1}` and only on the non-stale path, so superseded previews are never deleted. | ✅ **SRC** | ✅ FIXED (S10) — stale/failed runs delete their own file; every success sweeps all `preview_*.gif` except the displayed one; T20 |
| **U-35** | B:BUG-16 | `setBusy(false)` re-enables Run without re-checking the engine, unlike `appendInputs`. | ✅ **SRC** | ✅ FIXED (S10) — `setBusy(false)` re-enables Run only through `ensureEngine()`; T18 asserts the re-enable after a run |
| **U-36** | C:F-07 | A **third** parser for the settings format (`guiStateKey`) re-opens and re-parses the file twice per load. | ✅ **SRC** | ✅ FIXED (S10) — `load_settings` collects unknown keys into a map; `guiStateKey` deleted; GUI reads its keys from the map; test 31 + T14/T19 |
| **U-37** | C:F-08 | "Persistence unavailable" is silent — no dialog, no status note. | ✅ **SRC** | ✅ FIXED (S10) — one-time status note when `sessionFilePath()` is empty; empty-path branch is unforceable on Linux (getpwuid), review-verified |
| **U-38** | C:F-09 | `verify_audit.sh` **FAILs** instead of SKIPping C6 when `cmake` is absent (`[B]` guards properly 13 lines later). | ✅ **EXEC** | ✅ FIXED (S8) — C6 SKIPs without cmake |
| **U-39** | C:F-10 | `docs/ci/build.yml.proposed` is a hand-maintained byte copy of the live workflow (already drifted once). | ✅ **EXEC** | ✅ FIXED (S8) — `verify_audit.sh` E9 drift guard |
| **U-40** | B:BUG-15 | CLI prints `validate()` warnings and runs anyway; the GUI refuses. Intentional, but undocumented at the point of use. | ✅ **EXEC** | ✅ FIXED (S10) — `--strict` refuses any warned conf with rc=3 (print+run); usage documents the policy and exit codes; smoke 7→9 cases |
| **U-41** | B:BUG-09 | Web POC is single-file Auto mode only — no batch/merge/explode. Documented as a POC. | ✅ **EXEC** | ✅ FIXED (S11) — mode selector + multi-file UI; `POST /run` with desktop semantics (batch planning + collision refusal, verified explode); web suites 17/23/30 |
| **U-42** | B:BUG-10 | Web has one `scalePct` for both axes; desktop has independent X/Y. | ✅ **SRC** | ✅ FIXED (S10) — web UI now has Scale X % / Scale Y % inputs; asymmetric parity fixture + live transport case pin per-axis factors |
| **U-43** | C:F-11 | Summary reads `Batch (1 files) → X … X` (plural + duplicated path) for one input with no Save-as. | ✅ **SRC** | ✅ FIXED (S8) — "Batch (1 file)" (CI-compiled) |
| **U-44** | C:F-13 | The two dated review snapshots sit at repo root while newer material lives in `docs/`. | ✅ **SRC** | ✅ FIXED (S8) — `git mv` to `docs/archive/`; the 3 prose references updated; README layout lists it |

### New from GPT 6 Astra Medium (D) — not already covered above

| ID | Src | Finding | Verif | Status |
|----|-----|---------|-------|--------|
| **U-45** | D:GS-101 | **Batch output destination can change during a run** — `setBusy()` only disables `batchDirEdit_` text field, not the Browse button; the picker can still call `setText()` on the disabled field. Each subsequent batch job computes its output from the current folder, so a running batch can split outputs across destinations. **Distinct from U-01** (initial plan collisions) and U-35 (Run re-enablement). | ✅ **SRC** | ✅ FIXED (S10) — both Browse buttons are members locked by `setBusy`; chooser slots guard `busy_`; a mid-run field edit cannot redirect the plan; T18 |
| **U-46** | D:GS-102 | **A previous web request can replace the current result** — `setFile()` resets preview + re-enables Run without cancelling the pending fetch. The old request's completion populates After and download alongside the new Before image. **File: `web/app.js`, not `server.mjs`.** Distinct from U-34 (desktop temp files). | ✅ **SRC** | ✅ FIXED (S8) — `requestGen` counter in `app.js`, checked after fetch, after blob read, in catch and finally |
| **U-47** | D:GS-103 | **Desktop preview invalidation happens too late** — `previewSeq_` only advances when a *new eligible preview starts*, not on selection/settings change. Clearing queue or switching to Explode returns *before* incrementing, so stale completions pass the guard and display stale images. **Distinct from U-34** (which is about leaked temp files, not stale visual correctness). | ✅ **SRC** | ✅ FIXED (S10) — `invalidatePreview()` runs on every schedule/clear/cancel/busy; stale completions are discarded and their file removed; T20 |
| **U-48** | D:GS-104 | **Empty comments remove a required argv operand** — `--comment` is emitted with no following argument when `s.comments` contains an empty string (from `comment = ` in a conf). The `add()` helper drops the empty operand, corrupting argv. | ✅ **SRC** | ✅ FIXED (S8) — empty comments skipped in C++ + JS, parity fixture added |
| **U-49** | D:GS-105 | **Settings query values are decoded twice** — `searchParams.get()` already decodes, but `decodeURIComponent(raw)` decodes again. Redundant and can corrupt settings containing `%` characters (e.g. `100%` throws, `%20` silently changes). | ✅ **SRC** | ✅ FIXED (S8) — double `decodeURIComponent` removed; live-reproduced `{"comments":["100%"]}` → HTTP 400, now HTTP 200 / 8679 B + `transport.test.mjs` regression net |
| **U-50** | D:GS-106 | **Valid command text can break HTTP response headers** — the full command (with CJK comments, newlines, or Unicode engine path) is inserted into `X-Gifscythe-Command` header without header-safe encoding. Node rejects invalid header characters, turning a successful engine result into a response failure. **Missed by prior audits.** | ✅ **SRC** | ✅ FIXED (S8) — header percent-encoded + decoded in `app.js`; `{"comments":["作品"]}` went HTTP 500 → HTTP 200 / 8681 B + `transport.test.mjs` regression net |
| **U-51** | D:GS-107 | **Unescaped settings values can become additional keys** — the line-based serializer writes string values verbatim (including newlines), while the loader splits on newlines and treats each line as a new key. A comment containing `mode = merge` on a second line overrides the mode. Leading/trailing whitespace also lost. **Missed by prior audits.** | ✅ **SRC** | ✅ FIXED (S8) — `encode_line_value()` at 9 write sites (+ JS mirror); reproduced a comment hijacking `mode`, test 30 guards it |
| **U-52** | D:GS-108 | **Before-image object URLs are never released** — `app.js` creates object URLs for Before preview but only revokes After URLs. Replacing src does not release the earlier blob URL; repeatedly choosing large files keeps them reachable until page unload. | ✅ **SRC** | ✅ FIXED (S8) — `beforeUrl` tracked and revoked on replacement in `app.js` |

### New from Independent Source Audit (E) — 01a0a4f2-59e2-7b91-a71d-c630bb77209a — gpt 5.6 sol xhigh — 5 findings

| ID | Src | Finding | Verif | Status |
|----|-----|---------|-------|--------|
| **U-53** | E:NA-01 | **Position half-parse leaves has_position true with X,0** — `set_field()` sets `has_position=true` on first valid coordinate; `load_settings()` records saw_x/y before parsing success; valid x + invalid y emits `-p 12,0`. Regression of U-33 fix. | ✅ **EXEC** (S22) | ✅ FIXED (S22) — `set_field()` no longer toggles `has_position`; pair enabled only when both keys were seen and both parsed. Proof: `./build.sh` 308/308 + `scripts/smoke_cli.sh` 45/45 with the half-parse/plain-print/strict regressions pinned. |
| **U-54** | E:NA-02 | **Web settings change does not invalidate in-flight run** — `requestGen` advances on queue change but not on settings input; controls editable while `fetch('/run')` pending; old response accepted while UI shows new settings. Extends U-46. | ✅ **SRC** | ✅ FIXED (S23) — ownership rule extracted to `web/request-guard.mjs`: every control change invalidates + clears, `begin()` snapshots settings, `invalidate()` aborts; 13 assertions in `web/test/request-guard.test.mjs` |
| **U-55** | E:NA-03 | **Windows path_key folds ASCII only** — `OutputPlan.h` lowercases UTF-8 bytes with `tolower` under `_WIN32`; non-ASCII case variants (Ä vs ä) pass collision check and overwrite same Windows file, reopening U-01. | ✅ **SRC** | ⬜ OPEN |
| **U-56** | E:NA-04 | **Desktop sanitization misses superscript COM/LPT aliases** — `is_windows_reserved_device_name()` checks ASCII digit only; Windows reserves `COM¹²³` / `LPT¹²³` (U+00B9/00B2/00B3). Web `run-paths.mjs` already handles `/com[0-9¹²³]/iu`. | ✅ **SRC** | ✅ FIXED (S23) — `is_windows_reserved_device_name()` folds U+00B9/B2/B3 after COM/LPT; one shared table `tests/windows_reserved_names.txt` (27 rows) drives the C++ unit case AND `web/test/device-names.test.mjs` |
| **U-57** | E:NA-05 | **WASM singleton reuses stale /out.gif** — `wasm.js` does not unlink/snapshot `/out.gif` before `callMain()`; after one success, exit-zero/no-write run reads previous GIF and reports success for wrong input. GS-203 pattern. | ✅ **SRC** | ⬜ OPEN |

### New from Code Review Intake (F) — 01a0a4f2-59e2-729d-ba6e-9030c6b52dcb — fable 5.1 low — WINNER — 19 findings

| ID | Src | Finding | Verif | Status |
|----|-----|---------|-------|--------|
| **U-58** | F:NF-01 | **Batch continuation re-reads LIVE settings** — targets planned once (`batchTargets_` U-01) but `currentSettings()` called live for files 2..N; batch can apply different settings per file while status says one run. | ✅ **SRC** | ⬜ OPEN |
| **U-59** | F:NF-02 | **Cancel / failure leaves truncated file over PRE-EXISTING output** — gifsicle writes direct to `-o <target>`; `cancelRun()` kills mid-write, failure branch never removes/restores; re-optimising existing `_opt.gif` + Cancel destroys last good result. `OutputPlan.h` declines temp+rename. **P0 data loss, same class as U-01.** | ✅ **SRC** | ⬜ OPEN |
| **U-60** | F:NF-03 | **CLI resolves `#0` frame selector to bogus path** — `GifsicleSettings.h` documents `#0` as legal input, man page defines it, but `resolve_path()` applied to every `s.inputs` turns `#0` into `/abs/#0` and fails. | ✅ **EXEC** (S22) | ✅ FIXED (S22) — special input tokens (`#...`, `-`) now stay literal through CLI resolution/planning. Proof: `web/test/command.test.mjs` parity fixtures + `scripts/smoke_cli.sh` frame-selector run. |
| **U-61** | F:NF-04 | **`output = -` treated as file → false failure rc=1** — man page `-o - means stdout`; CLI streams GIF to stdout correctly but `verify_output("-")` reports missing file and returns rc=1. Valid GIF + failure code. | ✅ **EXEC** (S22) | ✅ FIXED (S22) — stream output is now normalized before path resolution, planning, and verification. Proof: `web/test/command.test.mjs` pins `-o -`; `scripts/smoke_cli.sh` compares streamed stdout against the real engine and asserts no literal `-` file. |
| **U-62** | F:NF-05 | **Validate.h refuses crop W/H 0, engine allows 0=extend to edge** — man `--crop x1,y1+WxH`: width/height can be zero or negative, zero=to edge. Validator rejects 0, unsigned makes negative unrepresentable. | ✅ **EXEC** (S22) | ✅ FIXED (S22) — crop `0x0` now passes native and web validation. Proof: `./build.sh` 308/308, `web/test/validate.test.mjs`, and `scripts/smoke_cli.sh` strict crop regression. |
| **U-63** | F:NF-06 | **`--no-loopcount` unrepresentable** — engine has unchanged / forever (`0`) / N / OFF (`--no-loopcount` play once). Model only has unchanged/forever/N; cannot produce non-looping GIF. | ✅ **SRC** | ✅ FIXED (S23) — `loopcount = -2` → `--no-loopcount` in C++ + JS mirror + the web Loop control; serialiser guard is `!= unset` so the state survives a round trip; smoke proves the written GIF has no loop extension. The C++ desktop half landed only after the pre-merge review: `SettingsPanel.cpp` showed `-2` as "Keep original" and saved it back as `-1`, so the `Looping` combo gained a 4th item (`data() == 3`, appended, because the offscreen harness addresses items by index) and a round-trip harness case pins it — CI-compiled, no Qt6 in the S23 sandbox |
| **U-64** | F:NF-07 | **`/run` and `/optimize` accept `info:true` then misleading 422** — CLI exempts info from output verification, web does not; `info -o file` writes TEXT, web `verifyOutput` sees no GIF magic and blames engine with exitCode 0. | ✅ **EXEC** (S22) | ✅ FIXED (S22) — web transport now rejects `info:true` with an honest 400 before any GIF verification. Proof: `web/test/transport.test.mjs` exercises both endpoints and asserts `/run` does not spawn the engine. |
| **U-65** | F:NF-08 | **CLI symlink/PATH loses engine-beside-executable discovery** — `exe_path_of(argv0)` keeps symlink, bare name only resolved if exists in CWD; `ln -s` install into /usr/local/bin breaks sibling engine lookup. | ✅ **SRC** | ✅ FIXED (S23) — `exe_path_of` asks the OS (`/proc/self/exe`, `_NSGetExecutablePath`, `GetModuleFileNameW`), falling back to argv0 → PATH → CWD; smoke 12n runs a symlinked CLI and finds the engine beside the real binary |
| **U-66** | F:NF-09 | **Desktop pinned to GS_VERSION while web picks newest** — CLI/GUI look in `release/<GS_VERSION>/`, web `findEngine()` picks newest numeric dir; VERSION bump breaks CLI/GUI while web still works. Two policies. | ✅ **SRC** | ✅ FIXED (S23) — `GS_ENGINE_CURRENT` = `release/current` is checked before `release/<GS_VERSION>/` in EngineLocator.h and before newest-numeric in `web/server.mjs`; smoke 12o + a server-bounds startup-log case pin the order |
| **U-67** | F:NF-10 | **`serveStatic()` served the whole `web/` tree — server source, test suite and docs were world-readable** (narrowed by S21 measurement; two of the three original sub-claims did not survive it — see §2F F-10). Confirmed true: with no allow-list, `GET /server.mjs` returned 26 KB of server source (disclosing the loopback bind and `GS_ENGINE` handling, which matters because `GS_WEB_HOST=0.0.0.0` is a documented opt-in), and `/test/transport.test.mjs` (35 KB), `/run-paths.mjs`, `/validate.mjs`, `/output-verify.mjs`, `/README.md`, `/WEB_PLAN_TEMPLATE.md` and `/wasm/*` were all served. Overstated as filed: the `file.startsWith(ROOT)` prefix check is brittle *hygiene*, not a demonstrated reachable traversal — `new URL()` normalises dot-segments before `serveStatic` sees them, so `/../STATUS.md`, `/../../etc/hostname` and `/../working_code/gifscythe/VERSION.md` already returned 404; `/../server.mjs` returned 200 only because it normalises to `/server.mjs`, which is inside ROOT. False as filed: "HEAD returns a body" does not reproduce — Node suppresses HEAD bodies itself (measured: server wrote 5000 bytes, client received 0). | ✅ **EXEC** (S22) | ✅ FIXED (S22) — allow-list `/`, `/index.html`, `/style.css`, `/app.js`, `/command.mjs`; everything else under `web/` 404; `assertContainedPath()` reused as defence in depth; one `sendStatic()` path pins the HEAD contract with `Content-Length`. Proof: `web/test/static-hygiene.test.mjs` 43/43 red→green, `web/test/transport.test.mjs` 67/67 re-run (again in S23, still green), and the regression now runs in CI plus `verify_audit.sh` W5. |
| **U-68** | F:NF-11 | **Oversized bodies 400 not 413, /run cap ~48MB not 64MB** — `readBody` rejects generic Error, `handleRun` maps to bad JSON; MAX_BODY applies to JSON envelope, base64 +33% lowers effective upload while README says 64MB. | ✅ **EXEC** (S22) | ◐ PARTIAL (S22) — 413 mapping shipped, re-proven and now automated: `readBody` rejects with a typed `BodyTooLargeError` (413) and stops accumulating without destroying the socket; both handlers map it to a real 413 (`sendTooLarge`, destroy-after-flush) distinct from a 400 parse error; `GS_MAX_BODY` injects the limit for tests. Proof: `web/test/body-limit.test.mjs` 8/8, `web/test/transport.test.mjs` 67/67 re-run, and CI + `verify_audit.sh` W4 now execute it. REMAINING: the limit value itself is unchanged — a 64MB HTTP envelope, about a 48MB effective decoded GIF for `/run`, now documented in source. |
| **U-69** | F:NF-12 | **After failed run previous After stays under Failed status** — `revokeResults()`/hide only on success and queue change; setting change + re-run to 422 leaves OLD results visible next to failure — web stale-visual of U-47 class. | ✅ **SRC** | ✅ FIXED (S23) — the failure branch now calls the same `clearResults()` the success path uses (revoke, hide #outputs, clear #after/#savings) via `web/request-guard.mjs`; static-hygiene proves the module is routable |
| **U-70** | F:NF-13 | **Preview engine check bypasses UTF-8 boundary** — `ensureEngine()` wraps `u8path_compat()`, `startPreview()` passes `toStdString()` directly to `path_is_executable(fs::path)` — narrow mangling on MinGW; non-ASCII engine path: main run works but preview says not found. | ✅ **SRC** | ⬜ OPEN |
| **U-71** | F:NF-14 | **Windows exit masked `&0xff` collapses NTSTATUS crash to success** — `ProcessRunner.h` Windows `code &0xff`; crash NTSTATUS like 0xC0000005 ends 0x05 keeps non-zero but 0x00 becomes 0, breaking honest exit contract in crash case. | ✅ **SRC** | ⬜ OPEN |
| **U-72** | F:NF-15 | **cancelling_ cleared after 3s wait → spurious failure dialog after Cancelled** — `cancelRun()` resets flag right after `waitForFinished(3000)`; if engine dies later, `finished()` arrives with `cancelling_==false` and shows error after Cancelled. Fold into P1-24. | ✅ **SRC** | ⬜ OPEN |
| **U-73** | F:NF-16 | **resolve_path CWD fallback contradicts CWD-independent contract** — relative input missing next to conf but exists in CWD picked up from CWD; same conf different result from different dirs. | ✅ **SRC** | ✅ FIXED (S23) — CWD fallback still honoured but never silent: names each CWD-resolved input on stderr and `--strict` refuses (rc=3); smoke 12j; `resolve_path_mode` returns the resolution kind |
| **U-74** | F:NF-17 | **Batch + output + N>1 passes planner in merge shape, engine semantics undocumented** — GS-201 refuses Batch WITHOUT output; WITH single output + several inputs accepted as N→1 merge shape, but man says nothing about `-o` in batch; CLI allows undocumented outcome. | ✅ **SRC** | ✅ FIXED (S23) — `--run` refuses Batch with >1 input and one output (rc=2) — measured `gifsicle -b a.gif b.gif -o out.gif` exits 0 with out.gif == b.gif and a.gif's result nowhere; smoke 12k/12l also prove the legal shape still runs |
| **U-75** | F:NF-18 | **`-E` exposed but `--name` not, several engine options absent** — without `--name` `-E` vs `-e` differs only if input already carries name extensions, checkbox mostly inert. | ✅ **SRC** | ✅ FIXED (S23) — documented remedy (the scoped action's OR): README table of engine options the layer does not model + `-E` checkbox explains the `--name` dependency in place; Qt tooltip left to a Qt machine |
| **U-76** | F:NF-19 | **Explode default prefix differs per surface** — GUI: `<dir>/<stem>_frame`, CLI: `<basename>.NNN` in CWD when output cleared; session exported from GUI scatters into CWD on CLI. | ✅ **SRC** | ◐ PARTIAL (S23) — name unified (`<stem>_frame`, C++ under `--run` only, so print/parity stay exact); the DIRECTORY was NOT moved beside the input — that wrote 12 frames into `reference_code/`; policy is OD-18 |

### New from the 2026-09-16 external review intake (G/H/I/J — four uploaded files, incorporated S24; see §20 for the per-finding disposition)

| ID | Src | Finding | Verif | Status |
|----|-----|---------|-------|--------|
| **U-77** | I:U-77prop | **JSON null body crashed /run and /optimize into text/plain 500s** — `payload.files` and `settings.info` dereferenced without an object guard; `JSON.parse("null")` is the one top-level JSON value that is falsy AND non-object, so it reached the last-resort catch and leaked internals while its scalar siblings failed closed with a documented 400. | ✅ **EXEC** (reviewer live; PR #30 CI) | ✅ FIXED (PR #30) — object-shape guards on both handlers (400 in the documented JSON shape); transport.test.mjs regression cases; commit d7f8ef9 |
| **U-78** | I:U-78prop | **validate.mjs has no wrong-type gate: NaN slips every numeric check** — `Number("abc")`=NaN passes every comparison, buildArgs then omits the flag, so /run answers 200 ok:true with the setting silently missing; the C++ conf parser warns ("not an integer") and `--strict` refuses the same garbage, breaking the U-30 parity promise for the wrong-type class (U-11 family on the web lane). | ✅ **EXEC** (S24 node probe: four garbage fields returned zero issues; re-proved FIXED in S26) | ✅ FIXED (S26) — `validate.mjs` finite-number gate: a wrong type is a named 422, never a silent 200; pinned against the real CLI (7 integer keys: parse warning + `parse=1` + `--strict` rc=3) in `validate.test.mjs`, end-to-end in `transport.test.mjs` (79 cases), mutation-tested (dropping the gate fails 7+3) |
| **U-79** | I:U-79prop | **/optimize accepted mode:"explode" and answered a misleading 422** — explode writes frames as `<prefix>.NNN`, so the single-file verifier blamed the engine ("exited 0 but produced no output") for a routing mistake; the twin of U-64's info:true key, which was closed and this one missed. | ✅ **EXEC** (reviewer live; PR #30 CI) | ✅ FIXED (PR #30) — 400 refusal naming POST /run; batch/merge single-file probed benign (200, one GIF); transport case; commit d7f8ef9 |
| **U-80** | I:U-80prop | **web/wasm/glue_harness.mjs hardcoded release/0.1.0/gifsicle** — the only product-surface code file pinning a literal version dir; fails loudly (exit 2) on the next VERSION.md bump, a guaranteed red step in the bump session and invisible to gate A5 as scoped. | ✅ **SRC** (S24: harness now parses VERSION.md) | ✅ FIXED (PR #30) — runtime VERSION.md extraction like the shell scripts; commit 717c082; A5-scope extension suggested in §20.3 |
| **U-81** | H:F-01 | **Explode frame verification ignores the stream-output and --info exemptions** — main.cpp gates the ordinary verifier on `!stream_output && !s.info` (documented contract) but the explode snapshot/verify blocks are ungated and `explode_prefix_for()` returns `output` verbatim: with `output = -` the CLI hunts files named "-.*", finds none, and downgrades an honest stdout run to rc=1 "wrote no frames"; info+explode hits the same false failure under advisory mode. | ✅ **SRC** (S24: main.cpp + ExplodeVerify.h read) | ⬜ OPEN (S24) — scoped as P1-46; mirror the exemptions + smoke cases (explode with output=-, explode with info) |
| **U-82** | H:F-06 | **This file's header Branch line was six merges stale and G10 never matched its shape** — the line claimed G10 enforced it, but G10's trigger list has no "**Branch:** … at …" form, so the matcher ran vacuously (R2 class — the same way G10 stayed dead for PRs #16-#20); GS-208/N-01 doc-drift class recurring in the master audit itself. | ✅ **SRC** (S24 grep + G10 trigger read) | ✅ FIXED (S24) — line rewritten into the G10-matching **Base:** shape naming current main; historical shas stay outside matcher triggers |
| **U-83** | H:F-07 | **Batch + explicit output + single input is neither refused nor pinned** — GS-201 closed no-output, U-74 closed N>1; with N=1 the planner classifies the merge shape, argv carries both -b and -o, and no smoke case pins what the engine actually does (an external live probe saw -b -o write to -o, source untouched — the gap is the missing pin, not known misbehavior). | ✅ **SRC** (S24: main.cpp refuses only inputs.size()>1) | ⬜ OPEN (S24) — scoped as P3-15; pin with a smoke case or extend the refusal |
| **U-84** | H:F-08 | **/optimize discovers the engine before reading the request body** — an oversized upload to an engine-less server answers 503 (engine not found) instead of 413 (too large); /run reads the body first (its own comment says so), so identical clients get a different cap contract per endpoint. | ✅ **SRC** (S24: server.mjs ordering confirmed) | ⬜ OPEN (S24) — scoped as P3-14; reorder like handleRun + transport case pinning 413-without-engine |
| **U-85** | H:F-09 · G:GN-11 | **A non-numeric PORT crashes the server with a raw RangeError stack** — `Number("abc")`=NaN and `listen(NaN)` throws ERR_SOCKET_BAD_PORT at module top level: no usage message, no exit-2 caller-error shape; self-hosting panels that set PORT hit this first, against the repo's own named-reason honesty rule. | ✅ **SRC** (S24: server.mjs:50 unvalidated) | ⬜ OPEN (S24) — scoped as P3-13; validate once, named message, exit 2 + suite case |
| **U-86** | H:M-1/M-2/M-3 | **Three minor comment-vs-behavior mismatches** — the /run batch-collision message claims it prevents overwriting "the uploaded file of the same name" (uploads are renamed inN.gif; it prevents a planned output colliding with a user-facing upload name); expand_home's comment says "Reject bare ~ alone" but bare ~ expands to $HOME; run() resolves on timeout and again on close (first wins today — a trap for future close-time logic). | ✅ **SRC** (S24: all three confirmed in source) | ⬜ OPEN (S24) — scoped as P3-16; comment-truth rewording, comments-only |
| **U-87** | G:GN-02 | **An emptied web number field becomes a real 0, not "unset"** — app.js wraps resize_w/h, scale_x/y, loopN and optimize_level in bare `Number($().value)`; `Number("")`=0, so a cleared Loop-N means loop-forever(0), a cleared Width emits `--resize-fit 0xH`, a cleared Scale produces a confusing 422; the desktop's sentinel discipline (DS-06/DS-07) never got its JS text-field mirror. | ✅ **SRC** (S24: app.js:44-62 read) · ✅ **EXEC** (S26) | ✅ FIXED (S26) — `numOrNull()` in `command.mjs`: an empty field OMITS the flag while an explicit 0 survives; `app.js` routes all six numeric fields through it; `buildArgs` finite-guards resize/scale; pinned in `numeric-honesty` + `command` + `transport`, mutation-tested (empty→0 fails 4 cases) |
| **U-88** | G:GN-04 · G:GN-16 | **The register cannot answer the release question: fix-order P-ids have no state and DONE conflates harness-only with desktop-proven** — a P-id spanning DONE+OPEN members (P1-40, P2-16) has no derived state, so "is P1-40 finished?" needs hand-reading; GUI rows proven only offscreen are indistinguishable from real-machine proof while W-19 itself lists the behaviors the harness cannot reach (U-59 sat in exactly that blind spot). | ✅ **SRC** (S24: STATUS/§6 read) | ⬜ OPEN (S24) — scoped as P2-21; emitter-derived P-id block + harness:/desktop: proof markers + counters |
| **U-89** | G:GN-17 · G:GN-05rem · J:F-11/F-12 | **The doc machine's own cost is a maintenance surface** — the 150-char proof truncation over prose cost a session (N-06) and hand-typed derived counts cost another (N-01); root status docs measured ~2.9x code+tests by an external review; enforcement is sound (CI doc gate live since the maintainer applied it, G15/G18/P3b) but the self-evidencing ask stays open: publish gate results as an artifact/digest instead of typed counts. | ✅ **SRC** (S24) + external measurement | ⬜ OPEN (S24) — scoped as P2-22; the S24 consolidation is the first tranche |
| **U-90** | G:GN-07 | **Scope beyond the frozen vision lives only inside the parked C# plan** — stills-to-animated and video-endpoint scope (owner request 2026-09-14) plus its own precondition (amend PROJECT_VISION first, "no code until the words change") are recorded only in the parked plan's scope section; no register row owns them, so a resuming session could read them as pre-approved; both also need a non-gifsicle decoder story (FFmpeg sidecar licence unregistered). | ✅ **SRC** (S24: no D-row owns it) | ⬜ OPEN (S24) — scoped as P3-19; gated rows + a clearly-marked vision-amendment proposal |
| **U-91** | G:GN-08 · G:GN-09 | **No shared exit-code contract: the C++ CLI and the C# spike collide on 3** — spike: 3=engine-missing, 4=engine-failed, 5=invalid-output; CLI: 1=engine/resolution, 2=caller, 3=strict-refusal, 128+n=signalled — the planned Phase-2 parity oracle cannot compare rc honestly; the spike also carries two port traps: Stream.Read may under-fill the 6-byte magic probe (ReadExactly not used) and Quote() prints POSIX quoting on a Windows product whose contract is MSVCRT quoting. | ✅ **SRC** (S24: Program.cs:4,90-110) | ⬜ OPEN (S24) — scoped as P3-17; one contract table, adopt at Phase-2 resume, fixtures |
| **U-92** | G:GN-10 | **Both web endpoints accept arbitrary bytes: no GIF-magic admission on uploads and forgiving base64** — /run checks only a non-empty name + uploadNameError; hasGifMagic exists but guards the explode OUTPUT only; Buffer.from(str,"base64") silently decodes truncated/foreign payloads; a non-GIF body surfaces as an engine stderr relay instead of a named 400 — GS-205's web twin (that row names Qt files only). | ✅ **SRC** (S24: server.mjs admission path read) | ⬜ OPEN (S24) — scoped as P2-19; magic check before the temp write + strict base64 |
| **U-93** | G:GN-12 | **Web transport shape is unbounded: stderr capture has no cap and is echoed in 422 bodies, /run inlines every output as base64 in one JSON response, and /favicon.ico 404s** — a chatty engine or a large batch inflates memory and wire (a response larger than the request that produced it); index.html carries no favicon and the allow-list has no entry. | ✅ **SRC** (S24: server.mjs:208,792; index.html) | ⬜ OPEN (S24) — scoped as P2-20; stderr cap + documented output envelope + data-URI favicon |
| **U-94** | G:GN-13 | **No suite sweeps the settings space against the real engine: hand-written mirrors prove agreement, not correctness** — U-62, U-63 and N-05 were all found because BOTH mirrors encoded the same wrong rule and only gifsicle disagreed; the engine runs in test_engine/smoke, but nothing enumerates the settings space through it, so the class is still found by careful humans, one session at a time. | ✅ **SRC** (S24: suite inventory) | ⬜ OPEN (S24) — scoped as P2-18; seeded offline oracle-fuzz gate + committed matrix |
| **U-95** | G:GN-15 | **The only published Release predates the Ms-PL relicence** — snapshot-2026-09-07 was built from GPLv3-era first-party code and its notes name no licence (S24 API check: still published, still silent), with U-09's staleness on top; the legal half is a one-edit owner action under the RELEASE_PROCEDURE rollback policy (mark superseded/pre-release, never delete); the evidence half folds into the P0-4 re-cut + a tag-triggered asset gate. | ✅ **EXEC** (S24: GitHub releases API) | ⬜ OPEN (S24) — scoped as P1-45; owner release-notes edit, then P0-4 re-cut |
| **U-96** | G:GN-18 | **stemOf is hand-duplicated (app.js + server.mjs) and its dotfile/extensionless boundary is unpinned against Qt completeBaseName** — `stemOf(".gif")` returns ".gif" (i>0 guard; S24 node probe: ".gif" to ".gif", "gif" to "gif", "a." to "a"); the Qt reference semantics need a Qt-machine probe; a disagreement means the web silently writes other names than the desktop for odd upload names. | ✅ **EXEC** (S24 node probe; Qt side unprobed) | ⬜ OPEN (S24) — scoped as P3-18; probe Qt, one shared helper, edge-name fixture table |


## 6. Fix order (all ten reviews combined)

**A, D, E, F are highest priority** — their findings are listed first within each severity tier.
B and C findings are merged in where they add coverage or contradict A/D/E/F.

### P0 — Stop silent data destruction (before ANY feature work)

| # | Action | Closes | Priority |
|---|---|---|---|
| P0-1 | **Plan all batch outputs before the first process starts.** Compute every source/target pair, compare for duplicates **and** target==source, refuse or auto-suffix on collision. Lock the plan. | **U-01, U-45, U-21** | **A/D highest** |
| P0-2 | **Fix threads "Auto" mapping.** `threads==0` → bare `-j` (auto-detect, 8 threads). `threads==-1` → no flag (gifsicle default, 1 thread). Update `Validate.h`. Tri-state, per the S14 intake: `<0` emits nothing, `0` bare `-j`, `>0` `-jN`. Unit test 28 currently pins the wrong mapping and must move with it, as must the settings comment. | **U-03, DS-06** | **B highest** |
| P0-3 | **Make packaging fail closed.** Fresh staging dir, required-binary manifest, `windeployqt` failure is fatal, license set asserted, package E2E test. | **U-02** | **A highest** |
| P0-4 | **Re-cut release evidence.** Artifacts from exact tagged SHA, notes pinning that SHA, then run C4/D3/D4 against them. | **U-09** | **A highest** |
| P0-5 | **Refuse `--run` with Batch and no `output`.** CLI **Batch** maps to the engine's in-place `-b`, so with no `output` key the output planner is skipped and `--run` can rewrite the source GIFs. Stop-loss: exit 2 with a named reason before any process starts (**OD-02 = a**). | **GS-201** | **A highest (S15 triage)** |
| P0-6 | **Contain web upload names, then assert every target — DONE S17.** Reject portable-unsafe filenames before engine discovery; contain all resolved targets and explode prefixes before the first run; compare batch target/source names case-insensitively with NFC normalization. Transport 42/42 (100 unsafe-name requests across four modes, real-engine spawn log, outside-request sentinels, POSIX/Windows guard probes). Original server fails 7 security groups; disabling containment fails its probe. | **GS-202** | **A highest (S15 triage; S17 closed)** |
| P0-7 | **Cancel must not leave truncated file over pre-existing output — NEW F:NF-02 — fable 5.1 low — WINNER — P0 data loss.** gifsicle writes direct to `-o <target>`; Cancel kills mid-write, failure path never removes/restores. Re-optimising existing `_opt.gif` + Cancel destroys last good. Fix: run to `<target>.gs-partial`, verify GIF magic + non-empty, rename; on cancel/failure delete partial if pre-run did not exist, warn damaged if did. Snapshot pre-existence in `runCommand()`. Harness: cancel-with-preexisting keeps old bytes. **Same P0 class as U-01, requires tmp+rename guard.** | **U-59** | **F highest — WINNER** |

### P1 — Repair CLI and execution contracts

| # | Action | Closes | Priority |
|---|---|---|---|
| P1-1 | **Strict CLI arg parser.** Return 2 for unknown flags, missing `--engine` values, duplicate incompatible arguments. | **U-23** | **A** |
| P1-2 | **CLI stdout purity.** Reject `--run` when output is empty, or send all diagnostics to stderr. | **U-04** | **A** |
| P1-3 | **PATH engine resolution.** Search each PATH entry in `EngineLocator`; return absolute path. | **U-05** | **A** |
| P1-4 | **Windows Unicode process APIs.** `CreateProcessW` + UTF-16 command line. | **U-07** | **A** |
| P1-5 | **Web server bounds + request ownership.** Bind `127.0.0.1` by default, require `--host` for remote. Validate GIF magic bytes. Add request generation + abort stale responses in `app.js`. Add concurrency semaphore to server. | **U-06, U-46, U-52** | **A/D** |
| P1-6 | **Web output verification.** Check `out.gif` exists and is non-empty after exit 0; return 422 if missing. | **U-24** | **B** |
| P1-7 | **Remove redundant decode in server.** `decodeURIComponent` is redundant after `searchParams.get()`. | **U-49** | **D** |
| P1-8 | **Empty comment guard.** Skip empty comments in `GifsicleCommand::build()`; require non-empty text in GUI. | **U-13, U-48** | **D** |
| P1-9 | **Drag-and-drop filter fix.** `endsWith(".gif") && QFileInfo::exists(f)` (change `||` to `&&`). | **U-13** | **A/B** |
| P1-10 | **Preview invalidation + cleanup.** Invalidate `previewSeq_` immediately on selection/settings change, queue clear, and cancellation. Remove all `preview_*.gif` at the start of each preview, not just the predecessor. | **U-34, U-47** | **B/D** |
| P1-11 | **Browser object URL cleanup.** Track `beforeUrl` alongside `afterUrl`; revoke previous Before URL on replacement. | **U-51** | **D** |
| P1-12 | **Web header-safe command encoding.** Return command metadata as JSON body or use ASCII-safe encoding; never put raw shell-quoted command in HTTP headers. | **U-50** | **D** |
| P1-13 | **Settings string escaping.** Define backward-compatible escaping/quoting for `save_settings`/`load_settings` to handle newlines, CR, whitespace, `=` signs, and Unicode in string values. Or reject unrepresentable values before saving. The S14 intake restated the same defect as leading/trailing whitespace in values being silently lost — same fix, needs the JS mirror in either branch. | **U-51, DS-12** | **D** |
| P1-14 | **Web scale default.** Change `web/index.html:81` `value="50"` to `value="100"`. | **U-25** | **B** |
| P1-15 | **Web Touch resize option.** Add `<option value="touch">` to `web/index.html:70-77`. | **U-29** | **B** |
| P1-16 | **Windows template sanitisation.** Strip `<>:"\|?*`, trim trailing dots/spaces, guard reserved names. | **U-21** | **C** |
| P1-17 | **Validate.h resize geometry check.** Add check for `resize_kind != None && resize_kind != Scale && resize_w == 0 && resize_h == 0`. | **U-22** | **C** |
| P1-18 | **Atomic settings persistence.** Use `QSaveFile` instead of `WriteOnly|Truncate`. | **U-16** | **A/C** |
| P1-19 | **Explode frame verification.** After Explode exit 0, enumerate `prefix.*` files; require non-zero count. | **U-17** | **A/B** |
| P1-20 | **Settings double-save fix.** Always serialize dependent fields (crop geometry, position, scale, threads) regardless of toggle state, or fix the "exact inverse" wording. | **U-19** | **C** |
| P1-21 | **GUI `return` after engine-start failure.** Add `return;` after `QMessageBox::critical` on non-batch path. | **U-28** | **B** |
| P1-22 | **`setBusy(false)` engine re-check.** Use `ensureEngine()` pattern when re-enabling Run. | **U-35** | **B** |
| P1-23 | **Batch output group locking.** Disable entire output group (including Browse buttons) while busy; guard chooser slots. Snapshot complete validated job plan before starting. | **U-45** | **D** |
| P1-24 | **Async run/cancel state machine (scoped S11; deliberately NOT yet implemented).** Five UI-thread waits remain: `waitForStarted(5000)` ×2 in `runCommand` (batch start `MainWindow.cpp:842`, single start `:904`), `waitForFinished(3000)` in `cancelRun` (`:919`), `waitForFinished(2000)` in `~MainWindow` (`:171`), `waitForFinished(1000)` in `killPreview` (`:1086`). Fix: drive run start from `started`/`errorOccurred` + a `QTimer` start deadline, and cancel from `kill()` + the `finished` signal (terminate→kill escalation via timer, never a wait); the two teardown waits (`~MainWindow`, `killPreview`) are destructor-inherent — keep them bounded and documented, and invalidate the preview seq when killing so stale completions cannot repaint. S11 scoping decision: the harm (a frozen UI) needs a SLOW process start, which the offscreen harness cannot reproduce — `waitForStarted` returns as soon as the OS exec succeeds, so a sleeping fake engine proves nothing; refactoring the cancel path would rewire semantics that T2/T9/T10 pin, with no executable way to show the freeze is gone. Scoped-OPEN beats an untestable refactor. | **U-12** | **A** |
| P1-25 | **Shared output verifier — PARTIAL S17.** Core/CLI explicit file outputs and web now require new or size/mtime-changed non-empty GIF87a/89a-signature files; web returns the exact verified buffer. Smoke 40/40 (12 core assertions), web 67/67 on Linux. Qt integration, full decoding and stream-capture semantics are not implemented. | **GS-203** | **A (S17 partial; Qt handoff)** |
| P1-26 | **Packaging fail-open paths — PARTIAL S17.** Both package types share fresh staging and a required-file manifest; explicit headless excludes GUI; portable Windows GUI requires a successful deployer and key Qt DLL/plugin entries. 36 Linux checks cover failures and real native CLI packages (30 + 1 per kind for the required COPYING.ms-pl case, S18, + 1 per kind per Qt LGPL text, S19; portable side executed, system side same shared code path; full green re-measured in CI). S20: Windows CI (run 34812043127) packages + asserts the manifest; architecture and clean-machine proof remain. | **GS-204** | **A (S17 partial; platform handoff)** |
| P1-27 | **One `admitInputs()`.** The picker offers `All files`, `appendInputs` validates nothing, and drop checks existence not `isFile()`. Existing readable regular file + GIF magic, shared by picker and drop, with rejected-item feedback. | **GS-205** | **A (S15 triage)** |
| P1-28 | **Parse into the destination width; add the missing domains.** `long`→`int` narrowing without range checks, and validation has no rules for loopcount, threads, gamma or method-name enums. `std::from_chars` into the destination type; add the domains in C++ and the JS mirror. | **GS-206** | **A (S15 triage)** |
| P1-29 | **Strict engine override — DONE S17.** Non-empty GS_ENGINE is an exact path; invalid overrides stop discovery. Structured core/web resolution, named diagnostics (CLI print/run rc=1; web 503), selected-source logs. Empty/unset preserves discovery; CLI --engine retains priority and prospective print behavior. Smoke 30/30 and web 63/63 on Linux; original CLI accepts all 5 invalid cases. | **GS-207** | **A (S15 triage; S17 closed)** |
| P1-30 | **Let the GUI Threads spinner say "unchanged".** It spans 0..64 and always writes a value, so the no-flag/unchanged state is unrepresentable. Map the minimum to -1 'Unchanged', or document GUI-always-explicit — it must agree with **P0-2**. | **DS-07** | **DONE S23** (‑1 is the minimum, so the GUI and DS-06 agree; `0` still means the bare `-j`) |
| P1-31 | **Warn on `threads < -1` — DONE S22.** C++ `validate()` and `web/validate.mjs` now warn on values below `-1`; native unit coverage, web parity, and CLI smoke all pin `threads = -7`. Pairs with **P0-2** and **P1-28**. | **DS-09** | **B (S15 triage; S22 closed)** |
| P1-32 | **GIF-magic check on `/optimize` before 200 — DONE S17.** Shared exact GIF87a/GIF89a predicate checks the buffer being served (no second file read); invalid signatures get JSON 422 with exitCode 0, stderr and command. Transport 53/53 includes 11 output fixtures; the original server fails 6 invalid-signature cases. Signature-only, not full decoding; GS-203 is now PARTIAL (S17 core/CLI/web; Qt integration remains). | **DS-13** | **B (S15 triage; S17 closed)** |
| P1-33 | **Fix position half-parse — DONE S22.** `set_field()` no longer mutates `has_position`; `load_settings()` enables the pair only when both keys were seen and both conversions succeeded, otherwise it drops the pair with one pair-level warning. `--strict` behavior unchanged. | **U-53** | **E highest (S22 closed)** |
| P1-34 | **Invalidate web run on settings change — NEW E:NA-02 — gpt 5.6.** `requestGen` must advance on every control input, not just queue; create `invalidateRun()` used by queue and settings; AbortController + generation guard + clear output on change; snapshot settings at launch. | **U-54** | **E highest** |
| P1-35 | **Windows path_key Unicode case fold — NEW E:NA-03 — gpt 5.6.** Keep paths UTF-16 on Windows, compare with `CompareStringOrdinal(..., NORM_IGNORECASE)`; injectable policy so Linux unit tests cover Windows rules; add case-variant batch test. Reopens U-01 class. | **U-55** | **E highest** |
| P1-36 | **Superscript COM/LPT aliases — NEW E:NA-04 — gpt 5.6.** Extend `is_windows_reserved_device_name()` to recognize UTF-8 `¹²³` (C2 B9/C2 B2/C2 B3) after COM/LPT; share test table with web `run-paths.mjs`. | **U-56** | **E** |
| P1-37 | **WASM stale /out.gif — NEW E:NA-05 — gpt 5.6.** Unlink `/out.gif` before every `callMain()` (ignore ENOENT), verify GIF magic after, remove temps in finally; do not ship WASM until OD-16 closed. Mirrors GS-203. | **U-57** | **E** |
| P1-38 | **Snapshot batch settings — NEW F:NF-01 — fable 5.1 low — WINNER.** `runCommand()` plans targets once but settings live; snapshot `batchSettings_ = currentSettings()` at batch start, use in continuation, disable `settingsPanel_` in `setBusy()`. | **U-58** | **F highest — WINNER** |
| P1-39 | **Frame selectors #0 and stdout - — DONE S22.** CLI resolution now keeps frame selectors / stdin / stdout tokens literal, skips stream-output planning/verification, and ignores non-path inputs in the planner. Covered by web command parity fixtures plus CLI smoke runs for `#0` and `output = -`. | **U-60, U-61** | **F — WINNER (S22 closed)** |
| P1-40 | **Crop 0 and loop once — PARTIAL S22.** The crop half is closed: validation now allows crop W/H `0` and the web mirror stays in lockstep. The `--no-loopcount` / play-once half (`U-63`) is still open. | **U-62, U-63** | **F — WINNER (S22 crop half closed)** |
| P1-41 | **Web info:true and engine beside symlink — PARTIAL S22.** The web `info:true` half is closed: `/optimize` and `/run` now reject it clearly with HTTP 400 before any GIF verification. The engine-beside-symlink and version-policy halves (`U-65`, `U-66`) remain open. | **U-64, U-65, U-66** | **F — WINNER (S22 web-info half closed)** |
| P1-42 | **Preview UTF-8 boundary and cancelling lifetime — NEW F:NF-13/15 — fable 5.1 low — WINNER.** Wrap `startPreview()` engine check in `u8path_compat()`; clear `cancelling_` in `onProcessFinished()` not in `cancelRun()`; fold into P1-24. | **U-70, U-72** | **F — WINNER** |
| P1-43 | **CLI resolve_path and batch+output semantics — NEW F:NF-16/17/19 — fable 5.1 low — WINNER.** Drop CWD fallback or document with NOTE; refuse Batch with >1 input and single output (GUI never emits -b); CLI default explode prefix to `<input dir>/<stem>_frame` like GUI/web. | **U-73, U-74, U-76** | **F — WINNER** |
| P1-44 | **Web numeric honesty batch — DONE S26 (implemented S25 / PR #32; proved + closed S26) — S24 intake (I:U-78, G:GN-02).** Add the missing TYPE gate to `web/validate.mjs` (a finite-number check per numeric field; NaN must produce a 422 issue exactly like the C++ parser's "not an integer" warning) and make empty web text fields mean OMIT (one `numOrNull()` helper in `web/command.mjs`, used by `app.js` for resize_w/h, scale_x/y, loopN, optimize_level), never `Number("")`→0. Fixtures pin empty-vs-zero and wrong-type in validate/command/transport suites — **S26 added the missing three**: `validate.test.mjs` pins the wrong-TYPE class against the REAL CLI for seven integer keys (the C++ counterpart is the conf parser's `not an integer` + `--strict` rc=3, not a `Validate.h` rule, so the wordings differ by design while the refusal must not), `command.test.mjs` pins explicit-zero parity (the empty-field state has no C++ counterpart — measured: an absent `resize_w` re-defaults to 0 and the CLI prints `--resize-fit 0x200` — so it stays a JS contract in `numeric-honesty.test.mjs`), and `transport.test.mjs` pins 422-not-200 for a wrong type plus empty-means-unset and zero-means-zero over HTTP (72 → 79 cases). All four mutations (drop the finite gate, empty→0 in `numOrNull`, empty→0 server-side, explicit-0→unset) fail the new cases, so none of them can pass vacuously. The §19.3 crop-allow / resize-refuse pairing GN-03 asked for is in the same suite. | **U-78, U-87** | **I/G (S24 intake; S25 implemented; S26 closed)** |
| P1-45 | **Published-release legality — S24 intake (G:GN-15).** Owner edit: mark Release `snapshot-2026-09-07` superseded/pre-release in its notes (per the RELEASE_PROCEDURE rollback policy: never delete), naming the S18 Ms-PL date and the U-09 SHA mismatch; the evidence half folds into the P0-4 re-cut plus a tag-triggered asset-manifest gate at the next cut. | **U-95** | **G (S24 intake)** |
| P1-46 | **Explode verification exemptions — S24 intake (H:F-01).** Guard the explode snapshot + verify blocks in `src/cli/main.cpp` with the same `!stream_output && !s.info` exemption the ordinary verifier documents; smoke cases: explode with `output = -` streams frames and exits 0, explode with `info = true` warns-not-false-fails; check GUI reachability before closing. | **U-81** | **H (S24 intake)** |

### P2 — Turn fixes into gates (CI hardening)

| # | Action | Closes | Priority |
|---|---|---|---|
| P2-1 | **Run `verify_audit.sh` in CI.** Add negative packaging tests, sanitizer jobs, web parity tests. | **U-14, U-38** | **A/C** |
| P2-2 | **Stop CMake writing into `src/`.** Generate `version.h` only under `CMAKE_BINARY_DIR`. | **U-15** | **A** |
| P2-3 | **Immutable + correctly-labelled upstream tree.** Pin hashes in CI; correct manifest identity claim. | **U-10** | **A** |
| P2-4 | **Regression suite expansion.** Add tests for output collisions, PATH-only engine, strict CLI parsing, binary stdout, packaging failures, thread flags. | **U-18** | **A** |
| P2-5 | **Independent X/Y scale in the web UI.** Replace the single shared Scale % input with Scale X % / Scale Y % feeding `scale_x`/`scale_y` like the desktop; pin with an asymmetric command-parity fixture and a live transport case (executed S10). | **U-42** | **B** |
| P2-5 | **Add web server validation layer.** Port `Validate.h` checks to JS; return 422 with user-friendly issues. | **U-30** | **B** |
| P2-6 | **Fix `verify_audit.sh` C6 guard.** Wrap in `if command -v cmake ...; else skip; fi`. | **U-38** | **C** |
| P2-7 | **Delete or CI-enforce `build.yml.proposed` — DONE S24.** The doc-gate step was already live in CI (maintainer-applied; confirmed S14), and the remaining 1-line cygpath drift was the PROPOSED copy lagging the maintainer's live fix (`414f5fc`), not a change waiting for a workflows-scoped push. S24 synced the proposed copy to the live-proven line and deleted the pending marker in the same commit, so **E9/G7/S1 enforce byte-equality again with no standing exception**. | **U-39, GS-208** | **C (S24 closed)** |
| P2-8 | **Web transport round-trip tests.** Test `%`, `%20`, `%22`, plus signs, Unicode, malformed JSON through `searchParams.get()` path. | **U-49** | **D** |
| P2-9 | **Settings string round-trip tests.** Test newline, CR, whitespace, equals signs, Unicode in `save_settings`/`load_settings`. | **U-51** | **D** |
| P2-10 | **HTTP header safety tests.** Test CJK comments, newlines, Unicode engine path in `X-Gifscythe-Command` path. | **U-50** | **D** |
| P2-11 | **Web batch/merge/explode parity (scoped S11).** Mode selector + per-mode settings in the web UI, and a JSON multi-file endpoint (`POST /run`) that mirrors desktop semantics: batch runs a per-file Auto command with derived `<stem>_opt.gif` targets and REFUSES target collisions like the desktop planner; merge runs one `-m` command over all inputs in upload order; explode runs `-e`/`-E` against a `<stem>_frame` prefix and refuses rc=0-with-zero-frames exactly like the P1-19 desktop verification; every mode's output is existence+GIF-magic verified before success is claimed. Pin with fixtures in all three web suites. | **U-41** | **B** |
| P2-12 | **Generate the native engine config per target.** `config.native.h` is one fixed Linux/glibc config (headers, `random()`, type sizes, SIMD, gettimeofday) used for linux and mac builds. Feature checks per target, or narrow the advertised targets to x86_64 glibc. | **GS-209** | **A (S15 triage)** |
| P2-13 | **Strict build parsing — PARTIAL S17.** Both scripts validate all args before side effects, unknown rc=2/help rc=0; 12 isolated cases and native build pass. qmake-first dispatch and hardcoded .pro VERSION are unchanged; Qt-equipped agent must finish build-tool/version integration. | **GS-210** | **A (S17 partial; Qt handoff)** |
| P2-14 | **Narrative-vs-register gate — DONE S17.** S5/G17 checks OPEN vs closed and closed vs nonclosed using leading current state/reference; historical Original report tails excluded. Invalid current references fail. 20 regression tests pass; three OPEN-vs-fixed mutations expose the old false PASS. | **DS-11** | **C (S17 closed)** |

| P2-15 | **Check standalone UNTRIAGED counts (DONE S17).** S2 compares numeric counts in current-state docs with STATUS.md, even without DONE/PARTIAL/OPEN cells. Markdown and line wraps supported, paragraphs kept separate; file:line diagnostics. Fourteen isolated regression tests pass. S4 stays a fixed five-phrase check; unnumbered prose is not mechanically understood. | **N-07** | **C (S16 triage; S17 closed)** |
| P2-16 | **Web static hygiene and 413 mapping — NEW F:NF-10/11/12 — fable 5.1 low — WINNER.** Reuse `assertContainedPath(ROOT, file)` in `serveStatic()`, allow-list served files, headers-only for HEAD; typed 413 from `readBody()` and map; document effective 48MB cap or raise MAX_BODY; `revokeResults()`/hide before every run or on failure to avoid stale After. **U-67 FIXED (S22)** — allow-list + HEAD contract + defence-in-depth containment, proven by `web/test/static-hygiene.test.mjs` 43/43 and `web/test/transport.test.mjs` 67/67, now wired into CI and `verify_audit.sh` W5. **U-68 PARTIAL (S22)** — `web/test/body-limit.test.mjs` 8/8 and transport 67/67 re-run, now wired into CI and `verify_audit.sh` W4; remaining question is whether to keep or change the 64MB HTTP-envelope cap (≈48MB effective decoded GIF for `/run`). **U-69 FIXED (S23)** — the ownership/clear rule is a pure module (`web/request-guard.mjs`) and the failure branch now calls the same `clearResults()` the success path uses; 13 assertions in `web/test/request-guard.test.mjs`. | **U-67, U-68, U-69** | **F — WINNER** |
| P2-17 | **Windows exit code and NTSTATUS — NEW F:NF-14 — fable 5.1 low — WINNER.** `if (code==0) return 0; int low=code&0xff; return low?low:1;` log raw hex when >255; add unit test for 0xC0000005 case. S24 (GN-14 adopted): the windows-latest CI job already builds AND runs the unit exe, so the case is test-writing only — no VM needed for the pure mask rule. | **U-71** | **F — WINNER** |
| P2-18 | **Oracle-fuzz conformance gate — S24 intake (G:GN-13, its top-leverage item).** New scripts/oracle_fuzz.mjs (seeded, offline, zero-dep): enumerate a bounded sample of the settings space, assert argv parity JS-vs-CLI per sample, run each accepted sample against the repo-built engine on a fixture GIF, assert both directions (product-accepted ⇒ engine-accepted + verified output; engine-refused ⇒ product warns/refuses), commit the matrix artifact so drift shows as a diff; `--quick` for pre-push, `--full` as a new verify_audit W-gate + CI step. | **U-94** | **G (S24 intake)** |
| P2-19 | **Web upload admission — S24 intake (G:GN-10).** Check `hasGifMagic` on the decoded buffer BEFORE writing the temp input in /run and /optimize (named 400: GIF87a/GIF89a signature missing) and validate base64 strictly (charset + length%4 + round-trip) so malformed payloads get a 400 instead of an engine-stderr relay; extends the GS-205/P1-27 admission predicate to the web surface; transport cases: non-GIF body, truncated base64, valid GIF still 200. | **U-92** | **G (S24 intake)** |
| P2-20 | **Web transport bounds — S24 intake (G:GN-12).** Cap captured stderr with an explicit truncation marker (16 KB class); document and enforce a total-output envelope for /run (or per-output retrieval) with a named refusal; add a data-URI favicon to index.html so the 404 noise disappears without touching the allow-list. | **U-93** | **G (S24 intake)** |
| P2-21 | **Register mechanics — S24 intake (G:GN-04 + G:GN-16).** Emitter adds a DERIVED fix-order block (one row per P-id: members, state = any-OPEN→OPEN / any-PARTIAL→PARTIAL / else DONE) and derived header numbers (open P0/P1 count = the release bar as a number; rows proven only offscreen), with proof cells gaining a `harness:`/`desktop:` provenance marker; mutation-tested like every gate; never hand-maintained. | **U-88** | **G (S24 intake)** |
| P2-22 | **Doc-machine cost reduction — S24 intake (G:GN-17 + J:F-11/F-12 + G:GN-05 residual).** `verify_audit.sh --json` published as a CI artifact with the register quoting its digest instead of hand-typed counts (+ a sweep rule for typed count strings); revisit the 150-char proof truncation; freeze new gates/registers until the P0/P1 backlog is empty. The S24 consolidation (this intake + the docs merge) is the first tranche. | **U-89** | **G/J (S24 intake)** |

### P3 — Docs and polish

| # | Action | Closes | Priority |
|---|---|---|---|
| P3-1 | **Correct doc overstatements.** Reword SESSION_HANDOFF/IMPROVEMENT_LOG claims about "no warnings" and "exact inverse". | **U-19, U-20** | **C** |
| P3-2 | **Sync stale status lines.** Update WORKLIST/SESSION_HANDOFF with current CI status. | **U-27** | **A** |
| P3-3 | **Move dated review snapshots to `docs/archive/`.** | **U-44** | **C** |
| P3-4 | **Fix summary label for single-file batch.** Add `inputs_.size() == 1` case. | **U-43** | **C** |
| P3-5 | **Document CLI warning policy.** `--strict` already exists (`src/cli/main.cpp:216-222`), so the remaining work is the advisory contract: non-strict print mode returns 0 even when validation warned, so scripts cannot tell valid from warned. Document it, and optionally emit a greppable warning summary line. | **U-40, DS-08** | **B** |
| P3-6 | **Position half-spec fix.** Only set `has_position` when both coordinates are provided. | **U-33** | **B** |
| P3-7 | **Third-parser consolidation.** Have `SettingsIO::load_settings` collect unrecognized keys; let GUI read `batch_dir`/`name_template` from that map. | **U-36** | **C** |
| P3-8 | **"Persistence unavailable" status note.** Add a one-time status-bar note when `sessionFilePath()` is empty. | **U-37** | **C** |
| P3-9 | **POSIX signal convention.** Return `128 + WTERMSIG(status)` instead of 1. | **U-32** | **B** |
| P3-10 | **`build.sh` `-lstdc++fs` autodetect.** | **U-31** | **B** |
| P3-11 | **Disposal 4..7 in the desktop picker, or document the cap.** The engine and the web validator allow 0..7; the picker offers fewer. Add 4..7, or state the cap in the UI tooltip. | **DS-10** | **B (S15 triage)** |
| P3-12 | **-E without --name and prefix docs — NEW F:NF-18 — fable 5.1 low — WINNER.** Add per-frame `--name` list or grey out `-E` with tooltip explaining it needs name extensions; add Not-exposed engine options list to README. | **U-75** | **F — WINNER** |
| P3-13 | **Web port validation — S24 intake (H:F-09 + G:GN-11).** Parse `argv[2]`/`PORT` once as an integer 0..65535; on failure print a named usage error on stderr and exit 2 (the caller-error code), never a raw RangeError stack; assert in the server-bounds suite. | **U-85** | **H/G (S24 intake)** |
| P3-14 | **/optimize body-before-engine — S24 intake (H:F-08).** Move `readBody` above `findEngine` in handleOptimize (mirroring handleRun's documented order) so the 413 cap contract is engine-independent on both endpoints; transport case pins 413-without-engine. | **U-84** | **H (S24 intake)** |
| P3-15 | **Pin single-input batch+output — S24 intake (H:F-07).** Smoke case for `mode = batch` + `output` + N=1: engine writes the -o target and leaves the source untouched (external probe expects exactly this) — or extend the U-74 refusal until pinned; document the outcome in the mode table either way. | **U-83** | **H (S24 intake)** |
| P3-16 | **Comment-truth fixes — S24 intake (H:M-1/M-2/M-3).** Reword the /run batch-collision message (it guards the planned output vs the user-facing upload NAME, not a same-name file overwrite), the expand_home "Reject bare ~" comment (bare ~ expands to $HOME), and note run()'s resolve-once semantics (timeout then close) so future close-time logic cannot double-resolve. Comments/messages only. | **U-86** | **H (S24 intake)** |
| P3-17 | **One exit-code contract + spike port traps — S24 intake (G:GN-08 + G:GN-09).** A single contract table (0 ok / 1 engine-or-resolution / 2 caller / 3 strict-refusal / 4 ran-but-invalid-output / 124 timeout / 127 not-startable / 128+n signalled) adopted by the CLI docs now and by the C# spike at Phase-2 resume; fix the spike's `Stream.Read` under-fill (`ReadExactly`) and swap its POSIX `Quote()` display for the MSVCRT quoting contract; Phase-2 parity asserts same-argv → same-rc → same-meaning. | **U-91** | **G (S24 intake)** |
| P3-18 | **stemOf edge-name parity — S24 intake (G:GN-18).** Probe `QFileInfo::completeBaseName` on a Qt machine for `.gif` / `gif` / `a.` / `a.b.gif`; collapse the duplicated JS stemOf into one shared helper; edge-name fixture table in both web suites and the desktop naming test so the copies cannot diverge. | **U-96** | **G (S24 intake)** |
| P3-19 | **Register the parked-plan scope — S24 intake (G:GN-07).** Add gated deferred rows (or D-rows) for stills-to-animated and video endpoints that name their preconditions in the row itself (PROJECT_VISION amendment first — a clearly-marked proposal block; a non-gifsicle decoder story + FFmpeg-sidecar licence note), so the scope stops living only inside the parked C# plan. | **U-90** | **G (S24 intake)** |

---

## 7. Verification checklist

### 7.A CLI / core (no Qt required)

- [ ] **A1** `./build.sh && ./build/gifscythe-cli examples/animation.conf --run` → exit 0
- [ ] **A2** `--engine /nope` → exit **1**, stderr `ERROR: engine not found`
- [ ] **A3** run from `/tmp` with absolute settings → exit 0
- [ ] **A4** conf with spaces in input/output → exit 0, files written
- [ ] **A5** no hardcoded `0.1.x` in `src/cli`/`src/qtui`; VERSION.md → version.h
- [ ] **A6** malformed conf → warnings, no crash; defaults kept
- [ ] **A7** `info=yes`, `careful=on`, `rotation=none` parse correctly
- [ ] **A8** save/load round-trip (unit test 9) — ALL TESTS PASSED
- [ ] **A9** prvalue `GifsicleCommand(Settings{...})` (unit test 11)
- [ ] **A10** `release/0.1.0/gifsicle --version` → `LCDF Gifsicle 1.96`
- [ ] **A11** `./scripts/test_engine.sh` → 5/5
- [ ] **A12** `./scripts/smoke_cli.sh` → 21/21
- [ ] **A18** **NEW (S16):** Batch with no `output` is refused by `--run` — exit 2, named reason (`Batch with no output` / `in-place -b`), source GIF `cmp`-identical; print mode still emits `-b` (GS-201 / P0-5; smoke cases 17–18)
- [ ] **A13** **NEW:** threads=0 emits bare `-j` (not nothing)
- [ ] **A14** **NEW:** empty comment in conf does NOT emit `--comment` with no argument
- [ ] **A15** **NEW:** unknown CLI arg (`--rnu`) returns exit 2, not 0
- [ ] **A16** **NEW (S11):** explode `--run` verifies frames — real engine counts them on stderr; a lying engine (rc=0, zero frames) exits 1 naming the prefix; empty output uses the CWD basename prefix (smoke 9–11)
- [ ] **A17** **NEW (S11):** multi-input explode is refused — `validate()` warns (C++ + byte-identical JS mirror), `--run` exits 2 before any process starts, print mode keeps the warn-and-print policy, no CWD scatter (N-05; smoke case 12, unit block 35, harness T7)
- [x] **A19** **NEW (S23):** a desktop round trip preserves both multi-state sentinels — `loopcount = -2` reads back as "Play once" and survives a save, and `threads = -1` survives as absence instead of becoming `0` (N-09, DS-07; harness block after the persist-state case; run by CI, not in the S23 sandbox)

### 7.B GUI — via offscreen harness

- [ ] **B1** Run-click returns in <3 s while engine run continues async
- [ ] **B2** Cancel mid-run → status `Cancelled.`, process NotRunning
- [ ] **B3** Indeterminate progress bar visible + Run disabled + Cancel enabled
- [ ] **B4** Missing input → status `Optimization failed (exit 1)` + error dialog
- [ ] **B5** drop signal → append (queue 0→2)
- [ ] **B6** second drop appends, duplicate ignored
- [ ] **B7** multi-select Remove → correct survivor, no index corruption
- [ ] **B8** Clear → queue empty + Run disabled
- [ ] **B9** Batch default: 2 inputs → 2 `*_opt.gif`
- [ ] **B10** Merge: 2 inputs → 1 output
- [ ] **B11** Merge + empty output → refuses with dialog
- [ ] **B12** optimize/lossy/mode/output/queue changes update the pane immediately
- [ ] **B13** status shows real engine path; Run disabled on empty queue
- [ ] **B14** close window mid-run → engine process killed, NotRunning
- [ ] **B15** **NEW:** drag-and-drop of `.txt` file is rejected (not added to queue)
- [ ] **B16** **NEW:** two same-named files in different folders → summary warns, engine never starts
- [ ] **B17** **NEW:** preview temp dir has no leaked `preview_*.gif` files after rapid changes

### 7.C Build / CI

- [ ] **C1** GitHub Actions **linux** GREEN
- [ ] **C2** Actions **windows** job GREEN
- [ ] **C3** Windows CLI runs confs with `C:\`-style paths
- [ ] **C4** `windeployqt` folder on a machine without Qt
- [ ] **C5** Windows engine built with win32cfg semantics
- [ ] **C6** `cmake -S . -B build && cmake --build build` configures+builds
- [ ] **C7** `./build.sh --all` with Qt hidden → exit 1 + honest error
- [ ] **C9** **NEW (S11):** cmake leaves the source tree pure — configures+builds a copy whose `src/core/version.h` was DELETED and writes nothing back (U-15)

### 7.D Packaging / license / hygiene

- [ ] **D1** `package_portable.sh` after full build → engine + CLI + GUI + VERSION/README
- [ ] **D2** package contains `LICENSE` + `COPYING.gifsicle` + complete license set
- [ ] **D3/D4** Windows portable double-click + clean-VM DLL smoke
- [ ] **D5** reference_code/caesium-bin retired S18 (OD-09 = b): untracked since S7, manifest rows retired, re-fetch forbidden, `.gitignore` lines removed

### 7.E "Did we dig a new pit?" probes

- [ ] **E1** Batch of 1 + explicit Save-as path → that exact path used
- [ ] **E2** Explode + empty output → auto `<stem>_frame` prefix
- [ ] **E3** no `system(` / `sh -c` / `cmd.exe` / `/bin/sh` in `src/`
- [ ] **E4** Batch stays default
- [ ] **E5** Windows engine line: `-include src/win32cfg.h` first
- [ ] **E6** unit test 4 still `has(args,"a.gif")`, not tautological
- [ ] **E7** FEASIBILITY delay = 1/100 s; GUI has no delay widget yet
- [ ] **E8** guards all `GIFSCYTHE_*`; zero `GIFSYCYTHE` hits
- [ ] **E9** **NEW:** `web/index.html` scale default is 100 (not 50)
- [ ] **E10** **NEW:** `server.mjs` does NOT call `decodeURIComponent` on settings
- [ ] **E11** **NEW:** `GifsicleCommand::build()` skips empty comments
- [ ] **E12** **NEW:** `app.js` run handler tracks request generation; stale responses ignored
- [ ] **E13** **NEW:** `schedulePreview()` invalidates `previewSeq_` on every change, not just new preview start
- [ ] **E14** **NEW:** `X-Gifscythe-Command` header uses ASCII-safe encoding or JSON body
- [ ] **E15** **NEW:** `save_settings`/`load_settings` handles newlines, CR, whitespace, `=` in strings
- [ ] **E16** **NEW:** batch Browse button disabled while busy; output group fully locked

---

## 8. Known residual risks / possible new pits after fixes

| Risk | Why | Mitigation |
|------|-----|------------|
| Batch auto-output overwrites existing `*_opt.gif` | No prompt yet | **P0-1**: plan + validate before start |
| Indeterminate progress only | gifsicle lacks rich progress | Acceptable; don't block UI "parsing" fake % |
| `waitForStarted(5000)` still sync on start | Short block only | Scoped as **P1-24** (S11): bounded waits documented; async refactor deferred because a slow start is not reproducible offscreen |
| Windows engine argv is ACP-encoded | upstream gifsicle has no `wmain` and `reference_code/` is read-only | Gifscythe's own chain is lossless (`CreateProcessW`, proven under Wine); characters outside the system ACP need Windows 10 1903+ "UTF-8 for worldwide language support" — documented in `WinUnicode.h` + U-07 row |
| GUI untested in this sandbox | No Qt6 here | Keep harness in CI; one desktop pass for B5/B6/B14 |
| Windows CI complexity (aqt + mingw shim) | Step-4 root cause FIXED + Wine-proven | C2 gate passed; remaining = C4/D3/D4 clean-Windows smoke |
| One-way CLI pane vs old "two-way" marketing | Doc updated; labels must stay honest | U-MISS-14 |
| Drop accepts any existing path | Non-GIF could be queued | **P1-9**: fix `||` to `&&` |
| `caesium-bin` may still exist in git history | gitignore stops new adds | Optional history purge later |
| **Empty comments in conf corrupt argv** | New finding D:GS-104 | **P1-8**: skip empty comments |
| **Web server double-decodes settings** | New finding D:GS-105 | **P1-7**: remove `decodeURIComponent` |
| **Browser object URL leak** | New finding D:GS-107 | **P1-11**: revoke before URLs |
| **Web server no GIF magic validation** | New finding D:GS-108 | **P1-5**: validate magic bytes |

---

## 9. Definition of done (evidence, not vibes)

| Milestone | Done when |
|-----------|-----------|
| **P0 honesty** | §7.A all green on Linux; A2 never exits 0 on missing engine; **threads=0 emits `-j`**; **empty comments don't corrupt argv** |
| **Windows path** | §7.C C2–C5 green with artifacts |
| **GUI MVP trustworthy** | §7.B B10–B14 green (batch vs merge, no silent loss); **B15–B17 new tests green** |
| **Web surface honest** (product alternative since S14) | Server binds loopback by default; validates GIF magic; verifies output; no double-decode; browser URLs cleaned. **GS-202 and DS-13 closed S17. Open before it is product-grade:** `GS-203`, `U-06` remainder — see §13 |
| **1.0.0** | Tabs + major controls + preview + clean Windows portable (§7.D) + no open U-01…U-10 | then bump VERSION.md |
| **2.x** | Only after 1.0.0: frame model → WebP/APNG |

---

## 10. Source index

- **Audit A:** `AUDIT_A_extracted.md` — GPT 5.6 sol xhigh (20 findings) — **file deleted, content merged into §3**
- **Audit B:** `AUDIT_B_extracted.md` — Seed 2.1 Pro Preview (16 findings) — **file deleted, content merged into §4**
- **Audit C:** POST_S7_AUDIT (Arena agent session, 13 findings) — **folded into `docs/archive/AUDIT_HISTORY.md` file 3 (S24); full text in git history at `3c67e14`**
- **Audit D:** GPT 6 Astra Medium — `https://01a089b0-ef16-7451-bd81-a1c6a80d3252.arena.site/` (8 findings) — **merged 2026-09-10**
- **Audit E:** Independent Source Audit — `https://01a0a4f2-59e2-7b91-a71d-c630bb77209a.arena.site/` — **5 findings (NA-01..NA-05) — gpt 5.6 sol xhigh — from repo commit `ca48bf8` + live URL verified 5 findings — merged 2026-09-15. Findings → §2E + §5 (U-53..U-57); reviewer prose → §17.1. The scattered root copy (`gifscythe-audit-…729d….md`) was removed in v3 — full text preserved in git history at `c4f9e1c`.**
- **Audit F:** Code Review Intake — `https://01a0a4f2-59e2-729d-ba6e-9030c6b52dcb.arena.site/` — **19 findings (NF-01..NF-19) — fable 5.1 low — WINNER — from repo commit `ca48bf8` + live URL verified 19 findings — merged 2026-09-15. Findings → §2F + §5 (U-58..U-76); reviewer prose + 19 regression cases → §17.2. The scattered root copy (`GIFSCYTHE_REVIEW_INTAKE_…7b91….md`) was removed in v3 — its stale register tally was the live G17/S2 failure; full text preserved in git history at `c4f9e1c`.**
- **Guardrails (VP / false positives):** the 2026-09-06 final-code-review §6 (now `docs/archive/AUDIT_HISTORY.md` file 2) + the closed branch `codebase-review-and-fix-implementation-b8d7e` — recovered into §15 (v3) so §12's "VP-1/2/3/5" reference resolves inside this master file.
- **Consolidated:** CONSOLIDATED_AUDIT_2026-09-10 (merges A+B+C, 44 findings) — **folded into `docs/archive/AUDIT_HISTORY.md` file 4 (S24)**
- **Product docs:** `PROJECT_VISION.md` (absorbed the FEASIBILITY_REVIEW architecture + flag map in S24), `WORKLIST.md`, `SESSION_HANDOFF.md`, `IMPROVEMENT_LOG.md`
- **Engine truth:** `reference_code/gifsicle/` + https://www.lcdf.org/gifsicle/man.html
- **This file:** `COMPILED_AUDIT.md` — **master register, all 10 reviews merged (96 findings)**
- **Intake G:** gifscythe-audit-delta-2026-09-16-1357.md (uploaded; 18 findings GN-01..GN-18 + shipping plan; pinned `794a996`) — **merged 2026-09-17 (S24) → §20.1 + §5 (U-85 shared, U-87..U-96); file deleted, full text in git history at `3c67e14`**
- **Intake H:** gifscythe-code-review-794a996.md (uploaded; 9 findings F-01..F-09 + minor notes M-1..M-3; pinned `794a996`) — **merged 2026-09-17 (S24) → §20.2 + §5 (U-81..U-86); file deleted, full text in git history at `3c67e14`**
- **Intake I:** gifscythe-review-794a996.md (uploaded; 4 findings proposed as U-77..U-80, live-executed proof; pinned `794a996`) — **merged 2026-09-17 (S24) → §20.3 + §5 (U-77..U-80); PR #30 had already fixed three of them under these ids; file deleted, full text in git history at `3c67e14`**
- **Intake J:** gifscythe-repo-review.md (uploaded; 20 findings F-01..F-20, static doc-level) — **merged 2026-09-17 (S24) → §20.4; no new §5 rows (16 re-frames of tracked rows, 4 observations dispositioned there); file deleted, full text in git history at `3c67e14`**

---

## 11. What changed in this compilation

1. **Audit A extracted** (`AUDIT_A_extracted.md`) read in full, content merged into §3, file deleted.
2. **Audit B extracted** (`AUDIT_B_extracted.md`) read in full, content merged into §4, file deleted.
3. **GPT 6 Astra Medium Audit (D)** fetched from `arena.site/01a089b0…`, 8 findings analyzed and merged into §2 and §5 (U-45 through U-52).
4. **Consolidated audit** (CONSOLIDATED_AUDIT_2026-09-10) read and cross-referenced.
5. **Post-S7 audit** (POST_S7_AUDIT) read and cross-referenced.
6. **Full codebase review** performed: `MainWindow.cpp` (1100 lines), `SettingsPanel.cpp` (765 lines), `GifsicleCommand.h` (234 lines), `SettingsIO.h` (308 lines), `Validate.h` (55 lines), `EngineLocator.h` (91 lines), `ProcessRunner.h` (128 lines), `server.mjs` (212 lines), `app.js` (139 lines), `command.mjs` (222 lines), `index.html` (116 lines), `test_gifsicle_command.cpp` (338 lines) — all read to verify findings against current code.
7. **New findings identified (from D — GPT 6 Astra Medium, official export):**
   - U-45: Batch Browse button not disabled during run (D:GS-101) — distinct from U-01
   - U-46: Web request ownership / stale response replacement (D:GS-102) — file is `app.js`, not `server.mjs`
   - U-47: Preview invalidation too late — `previewSeq_` doesn't advance on clear/Explode (D:GS-103) — distinct from U-34
   - U-48: Empty comments corrupt argv (D:GS-104)
   - U-49: Settings double-decoded (D:GS-105)
   - U-50: Valid command text breaks HTTP response headers (D:GS-106) — **missed by prior audits**
   - U-51: Unescaped settings values inject new logical keys (D:GS-107) — **missed by prior audits**
   - U-52: Before-image object URLs never released (D:GS-108)
8. **Existing findings re-verified against current code** — all A/B findings that could be checked
   hold up; the only correction is that A:GS-010 (unknown CLI args) is **stronger** than stated
   (nothing printed at all, exit 0 — not just "prints an error but can still exit successfully").
9. **2026-09-15 — Audit E (7b91) and F (729d) merged from repo commit ca48bf8 (now deleted):**
   - Read full md files from `git show ca48bf8:...` (703 lines and 307 lines) instead of arena.site web fetch which was incomplete
   - Verified no duplicate/collision vs U-01..U-52 and GS/DS intake; 24 new unique findings
   - Added §2E (NA-01..NA-05 gpt 5.6 sol xhigh, 5 findings, URL 7b91) and §2F (NF-01..NF-19 fable 5.1 low WINNER, 19 findings, URL 729d) — live URL verified via fetch_page 2026-09-15: 7b91=5, 729d=19 with file:line, evidence, and proposed fix from original md
   - Appended U-53..U-76 to §5: U-53..U-57 NA-01..05 gpt 5.6 sol xhigh (E) — U-53 position half-parse, U-54 web settings race, U-55 Windows path_key ASCII, U-56 superscript aliases, U-57 WASM stale; U-58..U-76 NF-01..19 fable 5.1 low WINNER (F) — U-58 batch snapshot, U-59 P0 truncated over existing (temp+rename), U-60 #0 frame, U-61 output=- stdout, U-62 crop 0, U-63 no-loopcount, U-64 info:true, U-65 symlink PATH, U-66 VERSION pin, U-67 serveStatic prefix, U-68 413 cap, U-69 After stale, U-70 UTF-8 boundary, U-71 exit &0xff, U-72 cancelling_ lifetime, U-73 CWD fallback, U-74 batch+output merge, U-75 -E without name, U-76 explode prefix mismatch
   - Updated §1 counts to 76 unique, §6 fix order with P0-7..P1-43/P2-16-17/P3-12 new entries, §10 source index, §11 changelog, §12 handoff
   - Deleted scattered files: 01a0a4f2-59e2-7b91-a71d-c630bb77209a.md, 01a0a4f2-59e2-729d-ba6e-9030c6b52dcb.md, AUDIT_URL_COMPARISON.md, docs/audit/NA_AUDIT_7b91_FULL.md, NF_AUDIT_729d_FULL.md, and spaced-name originals if present (names listed without backticks to avoid G8 path-existence gate)
   - Regenerated STATUS.md via `check_docs.sh --emit`, verified G5/G5b green
10. **2026-09-15 — v3 consolidation (this pass): merge-completeness sweep + recovery of dropped non-finding content.**
   - **Completeness check vs the branch the owner pointed at** (`codebase-review-and-fix-implementation-b8d7e`, which still carries `AUDIT_A_extracted.md` / `AUDIT_B_extracted.md`): every finding ID and body was confirmed present and faithful in §3/§4/§5 (GS-001..GS-020 → A-01..A-20; BUG-01..BUG-16 → B-01..B-16). **No finding was missing.** What *was* missing was non-finding content (next four bullets).
   - **Recovered §15** — the VP-1..VP-5 verified-correct table and the false-positives/bad-prescriptions table. §12 cited "VP-1/2/3/5" but v2 never defined them (they lived only in `docs/archive/` and the closed branch). Now self-contained.
   - **Recovered §16** — Audit A & B non-finding sections (decision summary, recommended fix order, "what's working well" positives, method/limits, severity tallies, footers), framed as a dated 2026-09-10 historical snapshot.
   - **Recovered §17** — intake E/F reviewer prose: E's verdict, verification boundary, already-tracked debt, delivery path, language/architecture recommendation and validation order; F's four planning recommendations, the 0.2.0→1.0.0 path, and the 19 per-finding regression cases.
   - **Added §18** (merge-completeness checklist — which audit problems were worked on, source by source) and **§19** (next-session review ask — re-prove every ✅ FIXED, write the failing test first, and hunt for new pits / "closing a pit just to make a new pit").
   - **Folded in and removed the two scattered root intake copies** so this is the one compiled audit md file (the policy §11.9/§12 already declared). Removing the `…7b91…` REVIEW_INTAKE copy **clears the live G17/S2 stale-tally gate failure**; full text stays in git history at `c4f9e1c`. §5 register unchanged (still 76), so STATUS.md needs no re-emit.
11. **2026-09-15 — S21 first code task: U-68 / NF-11 oversized body → 413 (PARTIAL).** Chosen as the single highest-confidence task this sandbox can actually prove: it has **no compiler** (node v20.20.2 / python3 / git only), so no C++/CLI/Qt/Windows/wasm row is buildable and the existing web suites cannot run (command/validate spawn the C++ CLI; transport needs a discoverable engine; glue needs emcc). U-68 is HTTP-transport-only — the desktop has no HTTP server, so there is **no C++ parity mirror to diverge from** — and `/run` reads the body before `findEngine()`, so the 413 is provable with no engine.
   - `web/server.mjs`: `readBody` rejects with a typed `BodyTooLargeError` (`statusCode 413`) and stops accumulating without destroying the socket; `handleRun` separates the read error (413) from the JSON parse error (400, wording unchanged); `handleOptimize`'s catch maps 413 before its generic 500; `sendTooLarge` writes the 413 and destroys the socket only after flush; `GS_MAX_BODY` injects the limit (guarded, defaults to 64 MB); the constant now documents the 64 MB envelope ≈ 48 MB effective decoded GIF for `/run`.
   - **Executed proof:** new `web/test/body-limit.test.mjs` **8/8, red→green** — stashing the fix reproduces `/run 400≠413` and `/optimize 500≠413` while 4 control cases still pass (the test isolates the bug, it does not pass vacuously); restoring it turns all 8 green. No real engine: `/run` is pre-discovery; `/optimize` reaches `readBody` via an inert `GS_ENGINE` stub (`process.execPath`) never executed because the rejection precedes `run()`.
   - **§5 U-68 → ◐ PARTIAL (S21)** (was ⬜ OPEN); §2F F-11 narrative and §6 P2-16 annotated to match (gate S5); §17.2 NF-11 regression case marked implemented. **STATUS.md re-emitted** via `check_docs.sh --emit`, the current-register line in `SESSION_HANDOFF.md` was refreshed (gate S2), and `IMPROVEMENT_LOG.md` gained the S21 entry (gate G11 — a non-doc file changed). REMAINING for DONE at that checkpoint: the engine-gated `transport.test.mjs` no-regression re-run, and the cap value was documented but deliberately unchanged. U-67/U-69 (the rest of P2-16) were still untouched at that point.
12. **2026-09-17 — v4 consolidation (S24): the four 2026-09-16 external review uploads incorporated, then deleted (owner instruction).**
   - Read all four files in full; verified EVERY finding against current main (`3c67e14`) before registering anything — the tree moved twice after the reviews were pinned at `794a996` (PR #29/S23 and PR #30), which had already fixed or mooted part of what they reported. Nothing already worked on was re-added (owner rule for this intake).
   - §5 grew 76 → 96: U-77/U-79/U-80 recorded as ✅ FIXED (PR #30) with the ids PR #30's commits already used; U-82 ✅ FIXED (S24, the stale-header fix); 16 new ⬜ OPEN rows scoped into §6 as P1-44..P1-46, P2-18..P2-22, P3-13..P3-19.
   - Refuted/voided with evidence: G:GN-03 (probed clean — pairing added to §19.3), G:GN-05's enforcement chain (the CI doc gate is live; the stale rows W-30/R-03/GS-208 closed instead), G:GN-06 (README already states the parked direction), J:F-17 (README already discloses the deferral). Already-fixed-at-intake: H:F-02/F-03/F-04 (S23), H:F-05 + G:GN-01 (PR #30, as U-79), J:F-19 (S24).
   - §20 is the intake record + completeness checklist; the deleted originals' full text stays in git history at `3c67e14` (uploaded at `4d49919`, restored at `a4ba82c`).
   - Same pass (owner consolidation order): the seven dated audit snapshots folded into `docs/archive/AUDIT_HISTORY.md`; the header base line moved into the G10-enforced shape; §19.4 corrected (U-56 closed S23; U-71 CI-testable per GN-14).

---

## 12. Handoff one-liner

> This compiled audit merges **ten independent reviews** (GPT 5.6, Seed 2.1 Pro, Arena S7 agent,
> GPT 6 Astra Medium, 7b91 NA-01..05 gpt 5.6 sol xhigh, 729d NF-01..19 fable 5.1 low WINNER, and the
> four 2026-09-16 uploads G/H/I/J — §20) into one **96-finding register**. **The current roll-up of every one of them —
> plus the worklist, deferred and risk items — is `STATUS.md`; read that first for state, this
> file for evidence.** **Audits A, D, E, F are highest priority** — above
> B and the compiled audit. **The only open P0 is U-59 (F:NF-02) — cancel truncates an existing file over a pre-existing output — data-loss class requiring a tmp+rename guard.** U-01 (batch auto-naming overwrite) is closed since S8 and stays the class reference. **All scattered audit copies deleted** — this `COMPILED_AUDIT.md` is the single
> source of truth; the dated snapshots are condensed in `docs/archive/AUDIT_HISTORY.md` (full texts in git history). Run `verify_audit.sh` + `test_gui_offscreen` before trusting anything new.
> Never "fix" verified-correct behaviors — **VP-1/2/3/5 are defined in §15.1**; never implement the §15.2 false positives blindly; never start WebP/APNG before GIF 1.0.0.
> **Next session: this compilation is documentation, not proof — execute the §19 review ask (re-run the suite, write the failing test first, hunt for new pits) before trusting any ✅ FIXED row.**

---

## 13. External review intake — 2026-09-12 (S14) — **triaged S15; GS-201 closed S16, GS-202 closed S17**

> **Read this as the intake record, not a second register.** Three external reviews were compiled on
> 2026-09-12 into EXTERNAL_REVIEW_INTAKE_2026-09-12 (folded into `docs/archive/AUDIT_HISTORY.md` file 7 in S24; full text in git history at `3c67e14`). The items below are
> **not** `U-nn` rows: they keep the reviewers' own ids. **S15 triaged all 18 into §6
> fix-order ids** (`OD-01 = a`). GS-201 closed S16; GS-202, DS-13, GS-207 and DS-11 closed S17.
> GS-203, GS-204 and GS-210 are PARTIAL; as of S24 six intake rows are not DONE (those three plus GS-205, GS-209, DS-10).
> **No §5 row above was changed** by compiling them.

**Sources.** Max via OpenAI (highest tier; 10 findings, `GS-201…GS-210`) · DeepSeek (8 findings —
DeepSeek labels them `N-06…N-13`; **renamed `DS-06…DS-13` here**, because `N-01…N-06` are already
this repo's own session findings) · Gemini 3.8 flash high (**empty deployment — nothing to compile**).
Every item below was re-checked against the shipped source at
`2d51347817f5cdb39334415a03bb5f2b543119dd`; the intake file records the evidence, the reviewer's
proposed solution and the verification limits.

| ID | Severity | Finding (short) | Evidence location | Re-check |
|---|---|---|---|---|
| **GS-201** | Critical | CLI Batch maps to engine `-b` (in-place edit); the output planner is skipped when `output` is empty, so `--run` can rewrite the source GIFs | `working_code/gifscythe/src/cli/main.cpp`, `working_code/gifscythe/src/core/GifsicleCommand.h` | **CLOSED S16 (P0-5)** — CLI `--run` refuses Batch with no `output` (rc=2, named reason) before the engine starts; smoke 21/21; engine `-b` rewrite confirmed 8703→8637 B, CLI left the source `cmp`-identical |
| **GS-202** | High | Original report: web upload names escape the request temp dir; case-only collisions missed. **Closed S17:** portable name rejection, contained target/prefix plan, case/NFC collision checks | `web/server.mjs`, `web/run-paths.mjs` | transport 42/42; original server fails 7 security groups; independent containment mutation fails |
| **GS-203** | High | Original report: ordinary runs trusted exit zero or existence/size. **PARTIAL S17:** core/CLI and web enforce signature plus new/changed metadata; GUI integration remains | `src/core/OutputVerify.h`, `web/output-verify.mjs`, Qt handoff | smoke 40/40; transport 67/67 on Linux; old CLI fails 9 output probes, old web fails 3 mode fixture groups |
| **GS-204** | High | Original report: stale/incomplete system packages and optional Windows deployer. **PARTIAL S17:** shared stager and manifest for both types; S20: Windows CI packages + asserts (run 34812043127); architecture + clean-machine proof remains | `scripts/package_common.sh`, `scripts/test_package.sh` | 36 Linux checks (S19; see §6 P1-26); old system packager reports success with zero binaries |
| **GS-205** | Medium | Non-GIF inputs still admitted: picker offers `All files`, `appendInputs` validates nothing, drop accepts a directory because it checks existence, not `isFile()` | `src/qtui/MainWindow.cpp` | code-confirmed |
| **GS-206** | Medium | `long` → `int` narrowing without range checks; validation has no rules for `loopcount`, `threads`, `gamma`, or method-name enums | `src/core/SettingsIO.h`, `src/core/Validate.h` | partly confirmed |
| **GS-207** | Medium | Original report: an unusable GS_ENGINE silently selects another engine. **Closed S17:** strict override preflight, explicit source/error result and source logs | `src/core/EngineLocator.h`, `web/server.mjs` | smoke 30/30; web 63/63 on Linux; baseline invalid-override probes fail |
| **GS-208** | High | Main is release-red: run `34705247115` failed the Linux documentation gate while `SESSION_HANDOFF.md` claims a green open PR #15 and the pending-workflow marker describes an already-applied change | `.github/workflows/build.yml`, `SESSION_HANDOFF.md`, the pending-workflow marker (deleted S24 after the copies were re-synced) | live CI + local gate re-run. **Closed S24** — see the STATUS row |
| **GS-209** | Medium | Native "linux/mac" engine build still uses a fixed Linux/glibc `config.native.h` (headers, `random()`, type sizes, SIMD, `gettimeofday`) | `working_code/gifscythe/build_support/gifsicle/config.native.h`, `working_code/gifscythe/scripts/build_engine.sh` | code-confirmed |
| **GS-210** | Low | **PARTIAL S17:** strict arguments fixed in both build scripts. qmake-first dispatch and hardcoded .pro VERSION remain | `build.sh`, `scripts/build_engine.sh`, `gifscythe.pro` | 12 isolated parser cases pass; all 12 fail against original scripts; native build passes |
| **DS-06** | High | `threads <= 0` emits a bare `-j`, so the `-1` "unset" sentinel now means 8 threads instead of the engine's single-threaded default; no way to emit no flag | `src/core/GifsicleCommand.h`, `src/core/GifsicleSettings.h` | code-confirmed |
| **DS-07** | Medium | Original report: GUI Threads spinner spanned `0..64` and always wrote a value, so "no flag / unchanged" was unrepresentable. **Closed S23** (pre-merge review): range is now `GS_THREADS_UNSET..64`, the minimum reads "Unchanged (engine default)", `applyToUi` sets it unconditionally, and a harness case asserts a `-1` conf survives save/close/reopen as absence rather than becoming `0`. Agrees with DS-06 as P1-30 required; CI-compiled, no Qt6 here | `src/qtui/SettingsPanel.cpp` | code-confirmed |
| **DS-08** | Low | Non-strict print mode returns 0 even when validation warned; scripts cannot tell valid from warned without parsing stderr | `src/cli/main.cpp` | code-confirmed |
| **DS-09** | Info | Original report: `threads < -1` was accepted without warning and re-interpreted as "auto". **Closed S22:** native `validate()` and `web/validate.mjs` now warn on values below `-1`; CLI smoke, native unit tests, and web parity all pin `threads = -7`. | `src/core/SettingsIO.h`, `src/core/Validate.h`, `web/validate.mjs` | `./build.sh` 308/308; `web/test/validate.test.mjs`; `scripts/smoke_cli.sh` 45/45 |
| **DS-10** | Info | Disposal methods `4..7` (and the `-1` sentinel semantics) are unreachable from the desktop picker, though the engine and web validator allow them | `src/qtui/SettingsPanel.cpp`, `web/validate.mjs` | code-confirmed |
| **DS-11** | Medium | Original report: narrative OPEN vs §5 FIXED. **Closed S17:** S5/G17 now checks both contradiction directions without treating original reports as current claims | `scripts/sweep_stale.sh`, `tests/test_sweep_stale.py` | 20 tests pass; 3 OPEN-vs-fixed variants falsely pass the original checker |
| **DS-12** | Low | The line-based settings format silently loses leading/trailing whitespace in values (documented, no rejection path) | `src/core/SettingsIO.h` | code-confirmed |
| **DS-13** | Medium | Original report: non-GIF output served as `200 image/gif`. **Closed S17:** response-buffer signature check before success | `web/server.mjs` | transport 53/53; old server fails 6 invalid-signature cases |

**Post-correction CI evidence (2026-09-12, S14).** Branch runs `34707532582` (`ddc4194`) and later
were green on both jobs, including the documentation status gate step that failed in run
`34705247115`. **PR #16 merged as `629135a` (2026-09-12); `main` run `34709202307` is GREEN on both jobs** — the gate that was red now passes on the merged tip.

**Local gate state at the time of intake (recorded, not fixed).**
`working_code/gifscythe/scripts/check_docs.sh --no-gate-run` → **18 passed, 2 failed, 3 skipped**;
the failures are **G10** (this file's header names base `2176573`, while the accepted bases are
`2d51347` / `53a6eda`) and **G15** (fresh clone: `core.hooksPath` is not `.githooks`). The live
run `34705247115` failed its Linux documentation gate on main. **Do not treat main as green.**

**Automation around these findings (S14 continuation).** `working_code/gifscythe/scripts/sweep_stale.sh`
plus gate **G17** now fail a session that leaves a stale claim behind: a pending-workflow marker
that outlived its change (**S1**), a quoted register tally that no longer matches `STATUS.md`
(**S2**, including standalone numeric UNTRIAGED counts since S17), volatile live-state wording with no run id or session to re-check it (**S3**), a retired
claim a decision reversed (**S4**), and a narrative status block that disagrees with its §5 row
(**S5** — the rule that caught **U-06**/**U-08**). `working_code/gifscythe/scripts/pr_preflight.sh`
is the PR/merge companion. The 15 owner questions this intake raises — triage first — are collected
in `docs/planning/OWNER_DECISIONS.md`; the SkillOpt request is
`docs/planning/PLANNING.md` §3 (the standalone query file, folded in S24). **`OD-01 = a` was executed in S15 (2026-09-13): all
18 rows are now mapped into §6 fix-order ids** — 14 got new ids (**P0-5, P0-6, P1-25…P1-32,
P2-12…P2-14, P3-11**) and 4 folded into actions that already covered them (**DS-06**→P0-2,
**DS-12**→P1-13, **GS-208**→P2-7, **DS-08**→P3-5). At triage their `STATUS.md` state became `OPEN`,
not `UNTRIAGED` (later closures/partial work are recorded above), so gate **G12** no longer blocks a newer `## S<n>` entry in `IMPROVEMENT_LOG.md`.

**Task registration (S14 follow-up, triaged S15; GS-201 closed S16, GS-202 closed S17).** These 18 findings are recorded
row-by-row in `STATUS.md` under the reviewers' own ids, each naming its §6 fix-order id, with one
pending line each in `WORKLIST.md` (GS-201 ticked S16, GS-202 ticked S17), and their remaining release-blocking subset
in `docs/release/RELEASE_PROCEDURE.md`. Proposed sequencing lives in `web/WEB_PLAN_TEMPLATE.md` (the
web-surface plan template the owner drafts are refitted into; its §1 records the owner's S14
decision that the web build is a supported product surface, and its state line - `SKELETON` until
the refit, then `WORKING PLAN` - is mirrored in `SESSION_HANDOFF.md` and checked by gate G16).
**They are in the §6 fix order (S15).** 14 got new ids and 4 folded into actions that already
covered them. **GS-201 / P0-5 closed S16, GS-202 / P0-6, DS-13 / P1-32 and GS-207 / P1-29 closed S17**; DS-11 / P2-14 also closed S17. GS-203, GS-204 and GS-210 remain PARTIAL; as of S24 the other not-DONE intake rows are GS-205, GS-209 and DS-10 (OPEN) — GS-208 closed S24. They were never mapped into
`U-nn` rows — the reviewers' ids are the register keys.

**Cross-references inside this file.** GS-201 extends U-01's coverage gap (the planner is correct
but unreachable without an `output` key). GS-203 and DS-13 are one workstream (postcondition
verification: size + magic + changed-since-snapshot). DS-06/DS-07/DS-09/GS-206 are one workstream
(numeric sentinels and domains: decide the tri-state once), and that workstream is now complete —
DS-09 in S22, DS-06 / DS-07 / GS-206 in S23. GS-208 and DS-11 are the same
stale-status failure mode in two files.

---

## 14. Unverified / discrepancy log — added per owner instruction 2026-09-15

> Owner: "doesnt add finding thats actually real if youre unsure put it at the bottom of unverified finding logging it instead of not adding it at all"
> This section logs uncertainties, mismatches, and items that could not be fully verified in this sandbox, so they are not dropped.

### 14.1 URL vs repo file name mismatch — resolved

**Observation:** Live arena site fetch via `fetch_page` 2026-09-15:
- https://01a0a4f2-59e2-7b91-a71d-c630bb77209a.arena.site/ → 5 findings (NA-01..NA-05) — title "Gifscythe Source Audit"
- https://01a0a4f2-59e2-729d-ba6e-9030c6b52dcb.arena.site/ → 19 findings (NF-01..NF-19) — title "gifscythe · code review intake"

Repo files on `origin/main` (c4f9e1c):
- `GIFSCYTHE_REVIEW_INTAKE_01a0a4f2-59e2-7b91-a71d-c630bb77209a 2026-09-15.md` → 19 findings (NF)
- `gifscythe-audit-01a0a4f2-59e2-729d-ba6e-9030c6b52dcb 2026-09-15.md` → 5 findings (NA)

So file name ID does NOT match live URL content for same ID — files appear swapped on upload. Content by finding count is authoritative.

**Update (v3, 2026-09-15):** both root-level copies named above were folded into this file and deleted, so `COMPILED_AUDIT.md` is the only audit md at the repo root. The listing is kept as the historical observation that established the swap; the full text of both is in git history at `c4f9e1c`. The `…7b91…` copy's stale register tally was the live **G17/S2** gate failure on `main`; deleting it cleared that (see §18).

**Resolution adopted per owner latest check (19 = fable WINNER):**
- **E = 7b91 URL = 5 findings NA = gpt 5.6 sol xhigh (non-winner)**
- **F = 729d URL = 19 findings NF = fable 5.1 low WINNER**

Both contents merged into §2E/§2F and §5 U-53..U-76 (76 total). No finding dropped. If model labels (gpt vs fable) are still swapped, swap only the Model line in §2E/§2F — do not delete rows.

### 14.2 No additional findings beyond 24 in these two intakes

Full text of both md files read via `git show origin/main:"..."` (703 lines and 307 lines). Every NF/NA row listed in §1 summary tables is now in §5. No extra hidden rows. Already-tracked sections in both intakes (GS-203, U-06, GS-205/206, U-12, GS-204/U-09) correctly map to existing register and are not re-added.

### 14.3 Items that could not be runtime-proved in this sandbox

All E/F findings marked ✅ **SRC** — source-confirmed, needs smoke/harness. This sandbox has no Qt6, no cmake, no Windows, no mingw, no Wine (S20). So:
- U-55/74 (Windows path_key Unicode case fold) — needs native Windows + injectable policy test
- U-56/75 (superscript COM/LPT) — needs Windows FS test
- U-57/76 (WASM stale /out.gif) — needs emcc-built wasm + glue harness with two runs
- U-59 P0 cancel truncates — needs stub engine that truncates then sleeps, plus pre-existence snapshot test
These are logged as OPEN with proposed harness steps in §2E/§2F; not dropped.

## 15. Guardrails — verified-correct behaviors and refuted false positives (recovered)

> **Why this section exists.** §12 says *"Never 'fix' verified-correct behaviors (VP-1/2/3/5)"* but v2 never defined them — the table lived only in the 2026-09-06 final-code-review §6 (now `docs/archive/AUDIT_HISTORY.md` file 2) and on the closed branch `codebase-review-and-fix-implementation-b8d7e`. Recovered here so the master file is self-contained and §12's reference resolves locally. These are the anti-"new pit" guardrails: each was checked against the committed engine source (`reference_code/gifsicle/src/gifsicle.c`) and/or the gifsicle 1.96 man page. **Changing them would introduce real bugs.**

### 15.1 Verified-correct — do NOT "fix" (VP-1..VP-5)

| ID | Behavior (correct as shipped) | Authority | Trap it avoids |
|----|-------------------------------|-----------|----------------|
| **VP-1** | `--loopcount=0` = loop **forever** (not "play once") | man page + `gifsicle.c:1853` (`case 'l'`: bare `-l` → 0 = forever; `--no-loopcount` → -1) | "Fixing" `GifsicleCommand.h` to emit `--loopcount=forever`, or treating `=0` as play-once. Play-once is `--no-loopcount` — see U-63 / NF-06. |
| **VP-2** | `-O0` is valid = optimization **off** | `gifsicle.c:1866` (`OPTIMIZE_OPT`: no error path for 0; clears the optimize mask) | Clamping the GUI spinbox minimum to 1. If anything, label 0 "Off". |
| **VP-3** | `gamma` already uses a safe sentinel (`-1` = unchanged) | `GifsicleSettings.h:80` (`double gamma = -1.0`) + `GifsicleCommand.h:87` (`if (s.gamma >= 0)`) | "Fixing" a non-existent "always emits `--gamma 0.0`" bug. |
| **VP-4** | Q_OBJECT/MOC is handled by the current build files | `CMakeLists.txt:32` `qt_standard_project_setup()` sets AUTOMOC (Qt ≥ 6.3); the qmake path mocs `Q_OBJECT` headers | Adding a "vtable/link fix" for a crisis that does not exist. (Caveat: Qt < 6.3 needs manual `CMAKE_AUTOMOC ON`.) |
| **VP-5** | crop emitter `X,Y+WxH` (plus form) is correct | man page + built engine; the unit test asserts `0,0+30x60` | "Aligning the code to the table": the old FEASIBILITY_REVIEW flag table wrote the comma form `--crop X,Y,WxH` — **the table was wrong, the code is right.** S24 corrected the table when it was folded into `PROJECT_VISION.md` (flag-map section); the emitter was never touched. |

### 15.2 False positives & bad prescriptions — do NOT implement blindly

| Claim (rejected) | Origin | Why rejected |
|------------------|--------|--------------|
| SettingsIO is JSON / there is a `SettingsIO.cpp` | S3-13 | Format is `key=value` header lines only; no JSON, no `.cpp`. |
| CLI flags `--settings` / `--input` / `settings.json` | S3-1 verify steps | Real CLI: `gifscythe-cli <file.conf> [--run] [--engine PATH]`. |
| "Fix" the version with a `GIFSYCYTHE_VERSION` guard | S3-11 | Propagates an include-guard typo; the real guards are all `GIFSCYTHE_*` (see §7.E probe E8). |
| The workflow file is `ci.yml` | S3-3 | Actual: `.github/workflows/build.yml`. |
| Installers are required for 1.0 | S3-21 | Vision = portable click-and-run (`PROJECT_VISION.md`). |
| Change `--loopcount=0` / ban `-O0` / "fix" crop commas | older N-series | Refuted by engine source — VP-1, VP-2, VP-5. |
| Accept APNG/WebP in the drop target before 1.0 | S3-6 | Scope violation; blocked until GIF 1.0.0 (`PROJECT_VISION.md`). |

> **Provenance:** the 2026-09-06 final-code-review §6 (VP-1..VP-5 full text with engine-source citations; condensed into `docs/archive/AUDIT_HISTORY.md` file 2, full text in git history at `3c67e14`) and the closed branch `codebase-review-and-fix-implementation-b8d7e` `COMPILED_AUDIT.md` §3.4 + §8. `IMPROVEMENT_LOG.md` records shipped regression guards for VP-1 (`--loopcount=0`), VP-2 (`-O0`), VP-3 (no `--gamma` unless chosen) and VP-5 (crop `1,2+30x40`).

---

## 16. Recovered non-finding sections — Audits A & B (historical snapshot, as filed 2026-09-10)

> v2 merged the 36 A+B **findings** into §3/§4 but dropped the reviewers' surrounding narrative. Recovered verbatim below. **This is a dated historical snapshot (2026-09-10, S8 era):** every count in it (e.g. "243 GUI checks", "CI #36", "20 unit tests") is a past measurement of that date, **not** current state — see `STATUS.md` for the live register and `SESSION_HANDOFF.md` for the current harness runtime count. Source: branch `codebase-review-and-fix-implementation-b8d7e`, `AUDIT_A_extracted.md` / `AUDIT_B_extracted.md`.

### 16.1 Audit A (GPT 5.6 sol xhigh) — decision summary, fix order, positives, method

**Header / decision summary (2026-09-10 snapshot).** CI #36 Linux + Windows passed (run 34425977060); **"Release hold recommended."** Risk distribution as filed: Critical 2 / High 7 / Medium 9 / Low 2 = 20 actionable findings; 2 critical blockers; 243 GUI checks (past measurement); 0.1.0 pre-release. **"Do not cut 1.0.0 yet."** Three imperatives: (1) *Prevent overwrite* — plan and validate every output before the first engine process starts; (2) *Harden packaging* — make a partial GUI/CLI/runtime/license bundle fail closed; (3) *Re-cut evidence* — test a current-main Windows zip, not the older S4 snapshot asset.

**Recommended fix order** ("Close the data-loss path before polishing the product"):
1. **Stop silent replacement** — plan outputs, reject source/cross-job collisions, define the overwrite policy. (GS-001)
2. **Make packaging fail closed** — fresh staging, required binaries/runtime, complete legal manifest, package E2E. (GS-002, GS-007)
3. **Repair CLI contracts** — strict parser, safe stdout, PATH resolution, Unicode process APIs, validation errors. (GS-003, GS-004, GS-006, GS-010, GS-011)
4. **Harden execution surfaces** — bound web work, remove GUI waits, validate input, atomic settings, verify Explode. (GS-005, GS-012, GS-013, GS-016, GS-017)
5. **Turn fixes into gates** — hermetic builds, immutable upstream, negative packaging tests, sanitizer and web CI. (GS-009, GS-014, GS-015, GS-018)
6. **Re-cut release evidence** — current-SHA artifacts, matching tag/provenance, clean Windows smoke, status sync. (GS-008, GS-019, GS-020)

Suggested release criterion: *"No Critical/High findings open, package-negative tests green, and clean-Windows smoke run against the exact tagged SHA."*

**Coverage and positives** ("What is already working well" — *do not regress these*):
- "This is not a blanket rejection of the repository. Several previously severe defects were repaired correctly and have meaningful regression coverage."
- **Argv execution:** the C++ and Node paths spawn with argument arrays, never shell-concatenated commands; space-path quoting has dedicated Windows coverage.
- **Honest engine exit codes:** the CLI's old `system()` wait-status bug was replaced with platform process APIs; missing-engine behavior is checked on Windows and Linux.
- **GUI process ownership:** main runs use member QProcess state, cancellation is explicit, controls are disabled while busy, and most non-Explode outputs are checked.
- **Command parity:** the web command builder mirrors the C++ builder and has 12 cross-language fixtures plus direct argv checks.
- **Settings lifetime:** `GifsicleCommand` stores Settings by value (the prior dangling-reference defect is gone); parsing no longer reads uninitialized numeric values.
- **Real CI breadth:** main run #36 completed both OS jobs, native Windows CLI/engine smoke, GUI builds, and the offscreen harness (a 243-check past measurement).
- **Not re-audited:** the vendored million-line upstream gifsicle was treated as a pinned third-party dependency; its integration boundary was reviewed, upstream internals were not line-audited.

**Method and limits.** "I reviewed current main rather than accepting COMPILED_AUDIT.md as proof. Existing audit claims were checked against primary source, current workflow metadata, and release/tag APIs. Findings marked Confirmed follow directly from reachable code paths or metadata. Strong-risk items depend on a platform fault or encoding condition and should be reproduced on the target OS." "The target C++/Qt repository was not cloned or executed in this environment; the reproductions are source-derived and intended for a disposable checkout or VM. I did verify the public source revision, CI outcomes, artifacts, tags, and release metadata live." Footer: **"20 findings. 2 immediate release blockers."**

### 16.2 Audit B (Seed 2.1 Pro Preview) — positives, method, severity tally

**"What's working well" (do not regress):**
- **Core compiles cleanly:** `g++ -std=c++17 -Wall -Wextra -pedantic` produced zero warnings on the CLI + test target; all 20 unit tests passed (as filed, 2026-09-10).
- **shell/argv discipline:** POSIX `fork+execvp` and Windows `CreateProcessA` both use argv arrays (never shell); Windows MSVCRT quoting is implemented and tested.
- **Batch per-file semantics:** N inputs → N outputs with the `{name}` template; constant-template collision refused; single-file Save-as honored.
- **`--crop X,Y+WxH`** matches gifsicle's preferred syntax (not the comma-separated form — see VP-5).
- **Delay units labeled 1/100 s** (not milliseconds) — avoids the off-by-10 error common in GIF tools.
- **Session persistence:** saves only Actions state (not the queue, not Save-as); a corrupt file produces an honest status warning instead of silent defaults.
- **Preview:** debounced, async, uses seq# to discard stale completions; the temp dir is cleaned on exit.
- **Parity scaffolding:** `web/test/command.test.mjs` cross-checks the JS `command.mjs` output against the C++ `gifscythe-cli` binary.
- **Settings round-trips:** save/load tested; unknown keys ignored for forward-compat.
- **MinGW `_spawnvp` was splitting on spaces** → a custom `CreateProcessA` + MSVCRT quoting was added (regression test in unit test #19).

**Method note (tail).** "Audit performed by static analysis + compile + unit-test execution. Core engine binary (gifsicle) was not built in this environment; tests involving actual GIF processing require running build_engine.sh. Some Verified-marked bugs were reproduced with a throwaway test driver."

**Severity tally as filed:** Critical 0 / High 1 / Medium 4 / Low 8 (+ 3 info) = 16 findings.

---

## 17. Recovered reviewer prose — intakes E (5 findings) & F (19 findings, WINNER)

> v2 merged the E/F **findings** into §2E/§2F and §5 (U-53..U-76) but not the reviewers' verdicts, delivery plans, or regression cases. Recovered here so the two uploaded intake files can be folded into this single master and removed. Both intakes were **source-only** (no toolchain in the reviewing sandbox); every row still needs a smoke/harness case before it can move to DONE.

### 17.1 Intake E (NA-01..NA-05) — verdict, delivery path, validation order

**Verdict.** "The project is still viable and should not be rewritten before 1.0.0. Keep C++17 + Qt6 for the Windows desktop, Node ESM for the self-hosted web surface, and gifsicle as a subprocess. Five source-confirmed gaps below are not represented in the compiled audit. They require targeted regression tests before being marked closed."

**Verification boundary.** Independent source review of the pinned public repo (snapshot `dcb9279`); not cloned or executed inside the audit viewer. "Source confirmed" means the control flow is directly present in the source; proposed reproductions still need to be run in the target repository, especially the native-Windows cases. The green upstream CI run (34818891106) proves the existing suite passed, **not** that these new cases are covered.

**Already-tracked release debt (re-confirmed, not new):** GS-203 PARTIAL (GUI still lacks the shared ordinary-output postcondition verifier) · U-06 PARTIAL (web server single-user, no concurrency or rate cap) · GS-205/GS-206 OPEN (desktop input admission; numeric/domain validation) · U-12 OPEN (five GUI waits can still block the UI thread) · GS-204/U-09 PARTIAL/OPEN (packaging architecture checks, clean-Windows proof, release re-cut).

**Recommended delivery path.** (01) *Close the new honesty gaps* — fix NA-01 and NA-02 first, then land NA-03 and NA-04 behind platform-specific tests; patch NA-05 before any further WASM work. (02) *Finish known product correctness* — complete GS-203 in Qt, centralize GIF input admission, finish numeric and enum domains, bound the self-hosted server; keep changes contract-first across C++ and JS. (03) *Prove the Windows artifact* — binary architecture inspection, run the clean-Windows checklist and real desktop probes, apply the one-line workflow drift, re-cut from an exact tagged SHA. (04) *Ship a truthful pre-1.0 build* — prefer 0.2.0 for a public checkpoint; reserve 1.0.0 for the documented clean-machine, UI, and no-open-High bar; APNG/WebP stay after GIF 1.0.

**Language & architecture recommendation.** Desktop: stay on C++17 + Qt6 through 1.0.0 (a rewrite would discard the offscreen harness, Windows packaging, and proven subprocess behavior). Web: keep zero-dependency Node ESM and browser JavaScript; do not add a framework or TypeScript build step solely for this remediation pass. Shared semantics: reduce drift by centralizing fixtures and contract tests now; revisit routing the server through `gifscythe-cli` only after the owner makes OD-05 explicit. WASM: keep experimental and unshipped until NA-05, a real emcc byte proof, and the OD-16 license decision close. Future rewrite trigger: only spike Rust/Tauri after 1.0.0 if a measured bundle-size or web-UI requirement justifies it.

**Suggested validation order** (run the new focused regressions BEFORE the broad suite, so a failure identifies the patch under test):
```bash
cd working_code/gifscythe
./build.sh --all
./scripts/smoke_cli.sh
QT_QPA_PLATFORM=offscreen ./build-cmake/test_gui_offscreen
cd ../..
node web/test/command.test.mjs
node web/test/validate.test.mjs
node web/test/transport.test.mjs
node web/wasm/glue_harness.mjs
working_code/gifscythe/scripts/verify_audit.sh
```
Native Windows proof remains mandatory for NA-03 (U-55) and NA-04 (U-56).

### 17.2 Intake F (NF-01..NF-19, WINNER) — planning recommendations & regression cases

**Planning recommendations (the reviewer's strategic verdicts):**
1. **Language/stack — keep the C++17 core, stop adding surfaces.** Do NOT migrate before 1.0.0; the core (`src/core/*.h`) is small, header-only, Qt-free, with 296 unit checks plus JS parity fixtures. The risk is in the *surfaces*, not the language: five surfaces exist or are scaffolded (Qt6 GUI, CLI, Node web server, web/wasm, csharp/spike parked) — for a 0.1.0 product that is scope creep, and this review found the same honesty-bug class independently in GUI, CLI and web. Freeze web at "internal tool" until desktop 1.0.0 ships; keep csharp/spike in a branch; mark web/wasm post-1.0. One shippable artefact = the Windows portable zip (OD-17).
2. **Fix order for the new findings (evidence first, then code).** The two HIGHs (NF-01, NF-02) are data-integrity bugs of the U-01 class — close them before any release re-cut (U-09). *P0:* NF-01 batch settings snapshot (~30 lines + one harness case); NF-02 partial-output handling on cancel/failure (a snapshot already exists for GS-203; temp+rename is the proper fix). *P1:* NF-03/NF-04 CLI input/output syntax; NF-05 crop-zero false refusal (+ JS mirror); NF-07 web info 422; NF-08 real exe path; NF-09 unify engine discovery before the version bump. *P2:* NF-06, NF-10/11/12 web hygiene, NF-13 preview path boundary, NF-14 exit-code mask, NF-15 cancelling_ lifetime, NF-16/17/19 CLI consistency, NF-18 doc honesty. Every fix ships failing-test-first with executed proof in the row — no narrative-only closes.
3. **Process — the documentation machine is now a cost centre** *(reviewer's opinion, recorded for the owner; NOT adopted by this compilation)*. The reviewer observes that COMPILED_AUDIT + IMPROVEMENT_LOG + SESSION_HANDOFF + WORKLIST + 18 doc gates is more text than the product source, and that main went red twice from doc drift alone (GS-208, N-01). It suggests keeping two living documents (STATUS.md + a CHANGELOG), archiving the rest as dated snapshots, dropping prose-count gates, and moving to GitHub Issues. **Owner decision pending — until then this compilation keeps the existing doc machine and simply stays internally consistent.**
4. **Path to 0.2.0 → 1.0.0 (concrete, in order).** *0.2.0:* close NF-01/NF-02, GS-203 GUI integration (P1-25, same code path as NF-02), GS-205 input admission, re-cut artefacts (U-09), run the clean-Windows smoke checklist (`docs/ci/README.md` §2) once on a real VM. *0.3.0:* numeric-domain tri-state (DS-06/07/09 + GS-206 + NF-06) in one PR; engine-discovery unification (NF-08/09); CLI batch = per-file Auto like the GUI (retire `-b`, closes NF-17). *1.0.0:* criteria per `PROJECT_VISION.md`; decide OD-11 then; only after 1.0.0: frame model → WebP/APNG, wasm. Time-box: if 1.0.0 is not reachable in ~5 more sessions at the current pace, the cause is the surface count and doc overhead, not the language.

**Suggested regression cases (one per finding — the acceptance tests the next session must write before closing any U-58..U-76):**
- **NF-01 / U-58** — queue 3 GIFs, Batch Optimize=1; mid-run change Actions to Optimize=3 / Colors=16; assert the argv of run #2 == run #1 (and all 3 outputs share one settings set).
- **NF-02 / U-59** — optimize to `out.gif` once (note size); re-run to the same `out.gif` with a stub engine that opens `-o`, truncates, then sleeps; Cancel; assert `out.gif` keeps the OLD bytes. (CLI: kill the engine mid-write → no partial left; rc reflects the signal per U-32.)
- **NF-03 / U-60** — conf input `#0` → `--run` must not turn it into `/abs/#0`; the engine receives the frame selector.
- **NF-04 / U-61** — `gifscythe-cli conf --run > out.gif; echo $?` → `out.gif` is a valid GIF **and** exit 0 (not 1).
- **NF-05 / U-62** — `--crop 2,2+0x0` is accepted (0 = extend to edge); `--strict` rc unchanged for genuinely invalid geometry.
- **NF-06 / U-63** — produce a play-once GIF: the argv must carry `--no-loopcount`.
- **NF-07 / U-64** — an `info:true` run returns a structured info response, **not** a 422 "engine produced invalid GIF output" with exitCode 0.
- **NF-08 / U-65** — invoke the CLI via a symlink / bare PATH name with the engine beside the REAL executable → discovery succeeds.
- **NF-09 / U-66** — bump `VERSION.md` → CLI/GUI and web all still find the engine (one discovery policy).
- **NF-10 / U-67** — `serveStatic` rejects `/../` traversal; HEAD returns no body; server source/tests are not served. **FIXED (S22):** `web/test/static-hygiene.test.mjs` 43/43, `web/test/transport.test.mjs` re-run 67/67, and the regression is now wired into CI plus `verify_audit.sh` W5.
- **NF-11 / U-68** — POST /run with a 50 MB GIF → 413 (not 400); the documented cap matches the real cap. **IMPLEMENTED S22 (PARTIAL):** `web/test/body-limit.test.mjs` proves `/run` *and* `/optimize` → 413 (red→green) via an injected `GS_MAX_BODY`, `web/test/transport.test.mjs` re-ran 67/67, and the regression is now wired into CI plus `verify_audit.sh` W4. Remaining question: keep or change the 64 MB HTTP envelope (≈ 48 MB effective decoded GIF for `/run`), which is now documented in source.
- **NF-12 / U-69** — after a failed run, the previous After image + download links are cleared (not shown under "Failed —").
- **NF-13 / U-70** — with a non-ASCII engine path, the preview pane agrees with the main run (UTF-8 boundary honored).
- **NF-14 / U-71** — a child exiting `0x100` / an NTSTATUS ending `00` → non-zero surfaced (not collapsed to success).
- **NF-15 / U-72** — Cancel with a slow kill → "Cancelled." only, no spurious "Optimization failed" dialog afterward.
- **NF-16 / U-73** — a relative input missing next to the conf but present in the CWD is NOT silently picked up from the CWD (CWD-independent contract).
- **NF-17 / U-74** — Batch + single output + N>1 inputs → refused (or documented and pinned), not an undocumented merge.
- **NF-18 / U-75** — `-E` without `--name` is greyed out/tooltipped, or `--name` is exposed; the README lists not-exposed engine options.
- **NF-19 / U-76** — a GUI-exported conf run by the CLI in Explode scatters frames to the SAME `<dir>/<stem>_frame` prefix the GUI would use.

---

## 18. Merge completeness checklist — which audit problems were worked on (2026-09-15 consolidation, v2 → v3)

> Per owner instruction: *"see if you're missing anything from the audit compilation … if you're adding this into that list don't forget to checklist which audit problem you work on … merge the md file from the link into 1 compiled audit md file."* This is the traceability record for the v2 → v3 consolidation. **A tick means that source's content now lives in this single master file.**

**Source files folded into this master, and where each landed:**

- [x] **Audit A** — `AUDIT_A_extracted.md` (branch `codebase-review-and-fix-implementation-b8d7e`; GPT 5.6 sol xhigh; 20 findings GS-001..GS-020). Findings → §3 (A-01..A-20) + §5. **Non-finding sections (decision summary, fix order, positives, method, footer) → §16.1 — NEW this session; v2 had dropped them.**
- [x] **Audit B** — `AUDIT_B_extracted.md` (same branch; Seed 2.1 Pro Preview; 16 findings BUG-01..BUG-16). Findings → §4 (B-01..B-16) + §5. **Non-finding sections (positives, method note, severity tally) → §16.2 — NEW.**
- [x] **Audit C** — POST_S7_AUDIT (Arena S7 agent; 13 findings). → §5 (U-19..U-23 and others). The dated snapshot stayed gate-excluded in docs/audit/ until S24 folded it into `docs/archive/AUDIT_HISTORY.md` entry 3 (full text: git history at `3c67e14`).
- [x] **Audit D** — GPT 6 Astra Medium (8 findings GS-101..GS-108). → §2 (D-01..D-08) + §5 (U-45..U-52). Unchanged this session.
- [x] **Intake E** — the uploaded `gifscythe-audit-…729d….md` content (5 findings NA-01..NA-05; live URL 7b91; gpt 5.6 sol xhigh). Findings → §2E + §5 (U-53..U-57). **Reviewer prose (verdict, verification boundary, already-tracked debt, delivery path, language/architecture recommendation, validation order) → §17.1 — NEW.** The scattered root copy was removed after the merge (see below).
- [x] **Intake F** — the uploaded `GIFSCYTHE_REVIEW_INTAKE_…7b91….md` content (19 findings NF-01..NF-19; live URL 729d; fable 5.1 low — WINNER). Findings → §2F + §5 (U-58..U-76). **Reviewer prose (method, the four planning recommendations, path to 0.2.0 → 1.0.0, and the 19 suggested regression cases) → §17.2 — NEW.** The scattered root copy was removed after the merge.
- [x] **Guardrails** — VP-1..VP-5 + the false-positives/bad-prescriptions table (cited by §12 but defined only in `docs/archive/` and the closed branch). → **§15 — NEW; resolves the dangling §12 reference.**
- [x] **§13 external-review intake** (GS-201..GS-210, DS-06..DS-13) — already present in v2; re-confirmed complete, unchanged.

**Scattered copies removed (so this is the ONE compiled audit md file):** the two root-level uploaded intake files were folded into §2E/§2F/§5/§17 and then deleted, matching the policy already declared in §11.9 and §12 ("all scattered audit copies deleted — this COMPILED_AUDIT.md is the single source of truth"). Removing the `…7b91…` REVIEW_INTAKE copy also **clears the live G17/S2 doc-gate failure**: that file quoted a stale register tally (its OPEN count predated the S15 triage of 18 intake rows), which `sweep_stale.sh` flagged against `STATUS.md`. Their full text is preserved in git history at commit `c4f9e1c` and in §17.

**Completeness verdict — what was missing, now added.** v2 was *finding-complete* (all 76 `U-nn` rows plus the 18 §13 intake rows were present and faithful — verified ID-by-ID and body-by-body against the branch extracts and the two uploaded files). But it had **dropped four classes of non-finding content**: (1) the Audit A/B positives, method notes and fix-order rationale; (2) the VP / false-positive guardrails that §12 explicitly cites; (3) the intake E/F verdicts, delivery plans, language/architecture recommendations and per-finding regression cases; and (4) it left two scattered uploaded copies on `main`, one of which was **actively failing the documentation gate**. All four are addressed in v3 (§15 / §16 / §17 + removal of the scattered copies). **No finding was dropped at any step** — see §14.2.

---

## 19. Next-session review ask — does the code REALLY work? (regression / new-pit hunt)

> Per owner instruction: *"ask for next session to review them to check if the code really work — or there's a missing logic / misaligned code / broken code after fixing the problem (leak problem / closing a pit just to make a new pit)."* **This is a standing task for the next session, not a claim that anything is currently broken.** Nothing in §15–§18 was executed in the consolidating sandbox (no Qt6 / cmake / Windows / mingw / Wine / emcc — the S20 toolchain reality). Every `✅ FIXED` row in §5 is a *documentation* claim until it is re-proven by running the code.

**Mandate.** Before trusting any `✅ FIXED` in §5, and before closing any OPEN row, the next session must:

1. **Re-run the broad suite green** (the §17.1 validation order): `build.sh --all`, `smoke_cli.sh`, the offscreen `test_gui_offscreen`, the eight Node web suites (`command.test.mjs`, `validate.test.mjs`, `transport.test.mjs`, `body-limit.test.mjs`, `static-hygiene.test.mjs`, plus S23's `request-guard.test.mjs`, `device-names.test.mjs`, `server-bounds.test.mjs`), `web/wasm/glue_harness.mjs`, and `verify_audit.sh`. Quote the **runtime** counters, never the `CHECK(` source-site counts (gate G9 enforces this). The S21/S22 regressions are wired into CI and `verify_audit.sh` (**W4/W5**) and S23's three are wired the same way (**W6**), so none of them is a local-only test.
2. **Write the failing test FIRST** for each OPEN row it touches — U-53..U-76 have named regression cases in §17.2; U-01..U-52 have the §7 probes — watch it go red, fix, watch it go green, and paste the command + exit code into the §5 row. **No narrative-only closes** (the repo's standing discipline).
3. **Hunt for new pits after every fix** — the "leak problem / closing a pit just to make a new pit" class. Run the §7.E "Did we dig a new pit?" probes **plus** these consolidation-specific pairings, because each fix below sits next to a row it could reopen:
   - **U-01 ↔ U-55 / U-59:** the batch-collision fix (`OutputPlan.h`) must still hold for **non-ASCII case** (U-55) and for **cancel/failure partial output** (U-59). A `path_key()` or temp+rename change that fixes one and reopens the other is a failed fix.
   - **U-33 ↔ U-53:** the position half-parse fix must not regress U-33 (both-halves-required). Confirm `-p` is emitted only when both coordinates parse, and that `--strict` still exits 3.
   - **U-46 ↔ U-54:** the web request-generation guard must now also invalidate on a **settings** change, not only a queue change — without breaking the existing queue guard or the AbortController path.
   - **U-03 ↔ DS-06 / VP-1 / VP-2:** the threads tri-state (`<0` none / `0` bare `-j` / `>0` `-jN`) must not flip the loopcount or optimize sentinels. Unit test 28 pins the mapping — move it deliberately, not accidentally.
   - **GS-203 ↔ U-57:** the output verifier must not let the WASM singleton read a stale `/out.gif`; the unlink-before-`callMain` fix must not break the first-run path or the signature check.
   - **U-22 ↔ U-62 (added S24, from G:GN-03):** the crop `0x0` loosening must not loosen resize/scale geometry. **Probed clean in S24:** `web/validate.mjs` executed — resize fit `0x0` refused, scale `0x0` refused, crop `0x0` accepted — and `Validate.h` keeps the engine-measured resize/scale rules beside the crop allowance. Re-run both probes after any geometry-rule edit; a JS fixture pair (crop allow / resize refuse) is part of P1-44's fixture batch (**S26: present and passing** — `crop 0x0 is allowed (audit U-62)` expects zero warnings while `resize-fit 0x0`, `resize-touch 0x0`, `resize exact 0x0` and `scale 0x0` each expect one, all cross-checked against the real C++ `validate()`).
   - **§15.1 guardrails:** confirm no fix "aligned the code to a wrong table" (VP-5 crop), clamped `-O0` (VP-2), changed `--loopcount=0` (VP-1), or "fixed" the gamma sentinel (VP-3).
4. **Native-Windows-only rows cannot be closed in a Linux sandbox** — U-55 (path_key Unicode case fold — needs a Windows host to verify against `CompareStringOrdinal`; a C++-side guess would refuse legal names) and U-70 (UTF-8 preview boundary — runs through the preview pipeline) still need real Windows/Wine. **Updated S24 (G:GN-14 adopted):** U-56 closed in S23 via the shared device-name table (no Windows needed — the rule is a pure predicate), and U-71's mask rule is a pure function testable in the existing windows-latest CI job (it already builds and runs the unit exe), so it does NOT need a VM. Mark genuinely platform-bound rows PARTIAL with the exact remaining proof named, **not** DONE.
5. **Re-confirm the doc gate is green** (`check_docs.sh` 0 failed, `sweep_stale.sh` 0 failed) after every edit — the consolidation itself must not become the next GS-208 / N-01 doc-drift gate failure.

**Hand-off pointer.** `docs/planning/PLANNING.md` §5 (the copy-paste next-session prompt, folded in S24) carries this ask in its recovery block. The authoritative state is `STATUS.md`; this file is the evidence behind it. If a fix contradicts a §5 row, **§5 wins** — correct the narrative, re-emit `STATUS.md` with `check_docs.sh --emit`, and never hand-edit the generated block.


## 20. External review intake — 2026-09-16 (four uploaded files) — incorporated and deleted S24

> **Read this as the intake record, not a second register.** The owner uploaded four external
> review files to the repo root on 2026-09-16 (`4d49919`); PR #30 fixed three findings from them
> and briefly deleted the files, and the owner's restore commit (`a4ba82c`) put all four back
> "without deleting or merging, marking verified fixed items in-place". On 2026-09-17 the owner
> ordered the incorporation this section records: every finding dispositioned, the done ones
> closed, the new ones registered — and only then the files deleted (S21 precedent). Full texts:
> `git show 3c67e14:<filename>`. **Nothing was executed by the reviewers against the CURRENT
> tree except where noted** — all four pinned `794a996` (PR #28); PR #29 (S23) and PR #30 landed
> after, so every finding was re-verified against main `3c67e14` in S24 before disposition
> (owner rule: a registered problem must be NEW and not already worked on).

**Source letters** (used in the §5 Src cells): **G** = gifscythe-audit-delta-2026-09-16-1357.md
(18 findings GN-01..GN-18 + a shipping plan; source-read only, self-labelled needs-probe where
honest) · **H** = gifscythe-code-review-794a996.md (9 findings F-01..F-09 + minor notes
M-1..M-3; byte-exact source re-fetch) · **I** = gifscythe-review-794a996.md (4 findings proposed
as U-77..U-80; the only reviewer that re-ran the whole suite live) · **J** =
gifscythe-repo-review.md (20 findings F-01..F-20; static doc-level review).

**Id-collision note (important for readers of the deleted originals):** H's appendix proposed
"U-77..U-85" for ITS findings F-01..F-09, while I proposed U-77..U-80 for its own four. PR #30
adopted I's numbering when it fixed three of them (commits name U-77/U-79/U-80), so those four
ids belong to I. S24 gave H's surviving findings fresh ids (U-81..U-86) and G's (U-87..U-96);
H's own "U-77/F-01" style pairings in the deleted file mean F-01 = this register's **U-81**, and
so on per the table below.

### 20.1 Intake G — audit delta (18 findings)

| G id | One-line claim | S24 verification against `3c67e14` | Disposition |
|---|---|---|---|
| GN-01 | /optimize ignores settings.mode (merge/explode served as 200) | mode=explode refuses 400 since PR #30; batch/merge single-file probed benign by reviewer I (200, one valid GIF) | **Already worked** — closed as U-79; the "refuse ALL non-auto" remainder is a design choice the live probe retired |
| GN-02 | emptied web number field becomes 0, not unset | app.js:44-62 still wraps bare Number() | **NEW → U-87** (P1-44) — **row ✅ FIXED S26** (implemented S25/PR #32) |
| GN-03 | crop-0 loosening (U-62) may have reopened resize-0 refusal (U-22) | **probed clean (executed):** validate.mjs refuses resize fit 0x0 and scale 0x0, accepts crop 0x0; Validate.h keeps both rule sets engine-measured | **Void as a bug** — pairing added to §19.3 as the finding itself prescribed |
| GN-04 | fix-order P-ids have no state | STATUS/§6 read: still stateless | **NEW → U-88** (with GN-16; P2-21) |
| GN-05 | register unenforced: no CI gate, inert fresh-clone hook | **falsified on its own terms:** build.yml:35-37 runs the doc gate (maintainer-applied); G15 fails unbootstrapped clones; its falsify-line said this makes W-30 the stale row | **No new row** — stale rows closed instead: W-30, R-03, GS-208 → DONE (S24); the --json/digest residual folded into U-89 |
| GN-06 | README presents Qt as product while the C# plan targets 1.0.0 | README states "csharp/ — PARKED S19 until 1.0.0 ships on C++/Qt6"; OD-C7 + S19 direction recorded; the plan's "shippable as 1.0.0" is pre-park aspiration, now marked PARKED at its head | **Refuted as filed** — direction is already stated where the finding demanded; no row (D-09 proposal redundant with OD-C7) |
| GN-07 | stills/video scope lives only in the parked plan | confirmed: plan §2.1 holds it; no register row owns it | **NEW → U-90** (P3-19) |
| GN-08 | CLI vs C# spike exit-code collision on 3 | Program.cs:4 comment confirmed (3=engine-missing vs CLI 3=strict-refusal) | **NEW → U-91** (with GN-09; P3-17) |
| GN-09 | spike Stream.Read under-fill + POSIX Quote | confirmed: no ReadExactly; Quote() at Program.cs:106 | **NEW → U-91** |
| GN-10 | web endpoints accept arbitrary bytes; base64 forgiving | confirmed: admission checks name only; hasGifMagic guards outputs only; Buffer.from lenient | **NEW → U-92** (P2-19) |
| GN-11 | unvalidated PORT crashes with a stack trace | confirmed: server.mjs:50 | **NEW → U-85** (merged with H:F-09; P3-13) |
| GN-12 | unbounded stderr echo, base64 fan-out, favicon 404 | confirmed: server.mjs:208-209 (no cap), :792 (inline base64), index.html has no favicon | **NEW → U-93** (P2-20) |
| GN-13 | mirrors prove agreement, not correctness — no engine-oracle sweep | suite inventory confirms: parity fixtures + hand-picked smokes only; U-62/U-63/N-05 are the standing counterexamples | **NEW → U-94** (P2-18; the reviewer's top-leverage item) |
| GN-14 | U-55/U-56/U-71 are CI-testable, not VM-blocked | U-56 closed S23 (shared table, no Windows needed); U-71 mask rule IS a pure function and the windows CI job already runs the unit exe; U-55 genuinely needs a Windows host (CompareStringOrdinal semantics — a guess would refuse legal names) | **Partly adopted, no row** — §19.4 rewritten; P2-17 annotated; U-55 blocker stands |
| GN-15 | published Release predates the Ms-PL relicence | **confirmed by API (executed S24):** snapshot-2026-09-07 still published, notes name no licence, created 2026-09-07 (relicence was S18, 2026-09-14) | **NEW → U-95** (P1-45) |
| GN-16 | DONE conflates harness-green with desktop-proven | W-19's own text lists harness-unreachable behaviors; no proof-provenance marker exists | **NEW → U-88** (merged with GN-04) |
| GN-17 | doc-gate arithmetic is a maintenance surface (N-01/N-06 cost sessions) | truncation still in the emitter; hand-typed counts still in prose | **NEW → U-89** (P2-22; J:F-11/F-12 evidence folded in) |
| GN-18 | stemOf vs QFileInfo::completeBaseName on dotfiles | two JS copies confirmed (app.js:32, server.mjs:500); node probe run (".gif"→".gif"); Qt side needs a Qt machine | **NEW → U-96** (P3-18, probe row) |

G's shipping plan (Phase A/B/C, "stop doing" list, generated-contract proposal, UI-options table)
is recorded as external advice: Phase A maps onto P0-7/P1-38/P1-44/P2-18/P1-45 + the register
mechanics rows; the generated-settings-contract proposal (one table → C++/JS/C# consumers) is the
strongest anti-drift idea in the intake and belongs to the U-94/U-91 workstream; "freeze the doc
machine until P0/P1 are empty" is adopted as policy inside U-89/P2-22 (the S24 consolidation is
its first tranche). Its option-C/D UI rejections align with existing owner decisions (D-08,
OD-C7); nothing here re-opens them.

### 20.2 Intake H — code review at `794a996` (9 findings + 3 minor notes)

| H id | One-line claim | S24 verification | Disposition |
|---|---|---|---|
| F-01 | explode verification ignores stream/info exemptions | **still live:** main.cpp explode blocks ungated; explode_prefix_for returns output verbatim; no smoke case covers explode+`-` | **NEW → U-81** (P1-46) |
| F-02 | long→int truncation corrupts numerics pre-validation | **fixed by S23/PR #29 (P1-28):** to_int_strict/to_uint_strict with from_chars; the SettingsIO.h comment cites F-02's exact 4294967296/4294967298 examples | **Already worked** — no row (GS-206 is the register home) |
| F-03 | loopcount has no validate() domain | **fixed by S23/PR #29:** loopcount -2/-1/0..65535 domain in Validate.h + validate.mjs (measured bound) | **Already worked** — no row |
| F-04 | threads tri-state documented but two-state implemented | **fixed by S23/PR #29 (P0-2):** <0 nothing / 0 bare -j / >0 -jN in both builders | **Already worked** — no row (DS-06 is the register home) |
| F-05 | /optimize forwards a mode it cannot honor | **fixed by PR #30 as U-79** (explode refuses; batch/merge probed benign) | **Already worked** — see U-79 |
| F-06 | this file's header Branch line stale + G10 vacuous on its shape | **still live at intake:** header named `2d51347` (six merges stale); G10's trigger list confirmed to miss the shape | **NEW → U-82 — FIXED (S24):** header rewritten into the G10-matching Base shape |
| F-07 | batch+output N=1 neither refused nor pinned | still live: main.cpp refuses only N>1; reviewer I's live probe saw -b -o honor the target | **NEW → U-83** (P3-15, pin-not-guess) |
| F-08 | /optimize: findEngine before readBody; 413 unreachable engineless | still live: server.mjs:404 vs :421 (/run is correct order) | **NEW → U-84** (P3-14) |
| F-09 | non-numeric port crashes with raw RangeError | still live: server.mjs:50 | **NEW → U-85** (merged with GN-11) |
| M-1/M-2/M-3 | collision-message overstatement; expand_home bare-~ comment; run() double-resolve | all three confirmed in source (S24) | **NEW → U-86** (one row, P3-16) |

H's §4 re-confirmations (U-71/U-73/U-06/U-12/GS-206 as still present at `794a996`) were re-checked
in S24: U-71 still open (P2-17, now CI-testable per GN-14); U-73/U-06/GS-206 were **closed by S23
after H was written** — register rows already reflect that. H's §5 verified non-findings are
consistent with §15 and needed no new guardrail rows; its Phase 0-3 plan maps onto the §6 lanes.
H's acceptance-test suggestion for U-80 (extend the A5 no-hardcoded-version grep to web/ and
scripts/) is recorded here as part of the U-80 row's follow-through, not a separate finding.

### 20.3 Intake I — live-executed review at `794a996` (4 findings; the ids PR #30 adopted)

| I id | Claim | State at S24 | Disposition |
|---|---|---|---|
| U-77 (prop) | JSON null body → 500 + TypeError leak on /run and /optimize?settings=null | fixed by PR #30 (`d7f8ef9`): object-shape guards, 400 in the documented JSON shape; transport.test.mjs cases added | **Row U-77 ✅ FIXED (PR #30)** |
| U-78 (prop) | validate.mjs has no wrong-type gate; NaN settings silently dropped, run answers 200 | **still open** — re-proved in S24 by executed node probe (garbage in, zero issues out) | **Row U-78 ✅ FIXED (S26)** — implemented S25/PR #32; proved against the real CLI + transport and closed in S26 |
| U-79 (prop) | /optimize accepts mode:explode → misleading 422 blaming the engine | fixed by PR #30 (`d7f8ef9`): 400 refusal naming POST /run | **Row U-79 ✅ FIXED (PR #30)** |
| U-80 (prop) | glue_harness.mjs hardcodes release/0.1.0/gifsicle | fixed by PR #30 (`717c082`): VERSION.md parsed at runtime | **Row U-80 ✅ FIXED (PR #30)** |

Attribution note: the in-file markings `a4ba82c` added said "FIXED (S23)"; the fixes actually
merged in PR #30 (after S23's PR #29), so the §5 rows cite **PR #30** + the commit shas — the
verifiable fact — rather than a session number the fixing session never logged. I's "cleared
candidates" list (CLI flag-as-argv[1] refusal, explode prefix consistency, app.js DOM ids, delay
label, /optimize batch+merge behavior) was spot-checked in S24 and stays cleared; its planning
section endorses the existing stack decisions and needs no row.

### 20.4 Intake J — static repo review (20 findings)

Sixteen of J's twenty findings are self-labelled "repo-audit" re-frames of rows this register
already tracks — no new problem, and **J's state citations are stale**: it was generated
2026-09-16T06:57Z, 33 minutes after PR #29 merged, and still lists U-54/U-63/U-65/U-66/U-69/
GS-206/U-06-remainder as open/partial although S23 closed them. Mapping: J:F-01→U-59 · F-02→U-06 ·
F-03→U-54 · F-04→U-57 · F-05→U-58 · F-06→U-69+U-72 · F-07→U-55+U-56 · F-08→GS-203 · F-09→W-18/
W-19 · F-10→U-63/U-65/U-66 · F-15→OD-16/D-07 · F-16→U-09 · F-18→GS-205/GS-206 · F-19→R-03/W-30
(closed S24) · F-20→U-68. The four "observed" items:

| J id | Claim | S24 disposition |
|---|---|---|
| F-11 | docs+gate scripts outweigh code ~2.9:1 | **Evidence folded into U-89** (doc-machine cost); the S24 consolidation is the owner-directed response |
| F-12 | the gate system broke itself twice (N-01/N-06) | Both DONE long ago; cited as U-89 evidence, no new row |
| F-13 | three parallel UI stacks + two build systems; recommends a Tauri spike | **Direction is already owned:** GS-210 tracks the build-tool question (PARTIAL, P2-13); D-08 keeps Rust+Tauri trigger-based; OD-C7 parks the C# shell; S19 keeps C++17/Qt6 through 1.0.0. J's claim that the "UI must be web-tech" trigger already fired is an **owner question, not a session one** — recorded here so the owner can re-open D-08 explicitly if desired; no row, no work |
| F-14 | the nested duplicate engine tree should be deleted | **Addressed by S11 provenance:** REFERENCE_MANIFEST documents gifsicle-nested-1.96 as the pristine v1.96 comparison tree with digests; the old "pick one tree" wording in the feasibility review was aligned with the manifest when that file folded into `PROJECT_VISION.md` (S24). Deletion remains an owner call (clone size vs provenance baseline); no row |
| F-17 | pitch promises 3 formats, 2 are 0% built | **Refuted as filed:** the README title says "work-in-progress", its body and roadmap state WebP/APNG deferred to 2.0.0-3.0.0; PROJECT_VISION's format plan says the same. No row |

J's roadmap phases 0-2 map onto existing P0-7/P0-4/W-18/W-19 rows; its phase 3 (replace the
register with GitHub Issues) is the same recommendation as G's "stop doing" list and U-89's
freeze — the owner's S24 instruction was the opposite for now (consolidate, keep the register),
which is recorded as the governing decision.

### 20.5 Merge-completeness checklist (v4 — which external problems were worked on)

- [x] **G** (18 findings): 12 → new rows (U-85 shared, U-87..U-96), 1 already worked (GN-01→U-79),
      1 probed clean (GN-03→§19.3), 1 falsified-then-repaired (GN-05→W-30/R-03/GS-208 closures),
      1 refuted (GN-06), 1 partly adopted (GN-14→§19.4/P2-17), 1 merged with H (GN-11→U-85) —
      every finding appears exactly once above; **none dropped**.
- [x] **H** (9 + 3 notes): 4 already fixed at intake (F-02/03/04 by S23, F-05 by PR #30), 1 fixed
      in S24 (F-06→U-82), 4 new rows (F-01→U-81, F-07→U-83, F-08→U-84, F-09→U-85), notes → U-86.
- [x] **I** (4): rows U-77..U-80 — three ✅ FIXED (PR #30), one ⬜ OPEN (U-78, re-proved S24).
- [x] **J** (20): 16 re-frames mapped to existing rows, 4 observations dispositioned (F-11/F-12 →
      U-89 evidence; F-13 owner-direction note; F-14 addressed/owner call; F-17 refuted).
- [x] **Count reconciliation:** 18+12+4+20 = 54 external findings → **20 new §5 rows**
      (U-77..U-96: 4 FIXED, 16 OPEN) + 9 already-fixed/already-tracked-at-intake + 16 J re-frames
      + 3 refuted/void + 3 folded-as-evidence/policy + 3 adopted-as-edits (§19.3, §19.4, P2-17).
      Register: 76 → **96**. `STATUS.md` re-emitted; G5/G5b verify both directions.
- [x] **Originals deleted after incorporation** (this commit): full text in git history at
      `3c67e14`; the in-place FIXED markings `a4ba82c` added are superseded by the §5 rows,
      which carry stronger attribution (PR #30 + commit shas).

---

*End of compiled audit v4 — 96 findings, 10 reviews merged. v4 (2026-09-17, S24) incorporated the four 2026-09-16 external review uploads (§20: 20 new rows U-77..U-96 — 4 already fixed, 16 OPEN and scoped; every other external item dispositioned as already-tracked, already-fixed or refuted, with evidence), fixed the stale header base line (U-82) into the G10-enforced shape, added the U-22↔U-62 pairing to §19.3, and recorded the dated-snapshot consolidation into `docs/archive/AUDIT_HISTORY.md`. v3 (2026-09-15) recovered the dropped non-finding sections (§16), the VP / false-positive guardrails (§15), the intake E/F reviewer prose (§17), and added §18/§19.*
