# Gifscythe — Compiled Audit (Master) — v2

> **Remediation status (2026-09-10, session S8): 31 findings closed outright
> (21 in batch 1 + 10 in batch 2), 3 closed in part (U-10/U-14/U-18), 15 still
> open, 2 register rows corrected (U-19/U-20) — see
> `docs/audit/REMEDIATION_2026-09-10.md` for the per-finding before/after
> evidence, the mutation-test record and the exact tally. Rows below carry a
> `✅ FIXED (S8)` / `◐ PARTIAL (S8)` / `☑ CORRECTED (S8)` marker in the Status
> column. CI on **PR #11** (run `34471563229`) is **green on linux and
> windows**, which is the first compilation of the S8 Qt edits.**

> **Current state (2026-09-12, S14) — main is RELEASE-RED.** Live check: `main` is `2d51347`, and
> run `34705247115` (the PR #15 merge) **failed the Linux documentation status gate**; every later
> Linux step was skipped and Windows passed. The S8 banner above is a dated snapshot of that
> session, **not** the current state. §13 is the external-review **intake inbox — not triaged**.
> Narrative `**Status:**` lines in §2/§3/§4 now name their §5 register row; where the original
> audit text disagreed with the register, the original wording is kept after *"Original report:"*
> and is superseded by the register.

**Compiled:** 2026-09-10 · **Verification sessions:** S4 (2026-09-07), S7, S8, S9, S10, S11, S12, S13 (2026-09-12), **S14 (2026-09-12 — external-review intake and status-truth corrections)**
**Branch:** `main` at `2d51347817f5cdb39334415a03bb5f2b543119dd` (the PR #15 merge; re-confirm with
`gh api repos/freeforall1932-design/gifscythe/branches/main --jq .commit.sha`;
`check_docs.sh` gate **G10** fails if this line names anything else)
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

This document merges **four independent audits**:

| ID | Source | What it is | Trust rank |
|----|--------|------------|------------|
| **A** | `AUDIT_A_extracted.md` — GPT 5.6 sol xhigh | 20 findings (2 Critical / 7 High / 9 Medium / 2 Low) | **1 — highest** |
| **B** | `AUDIT_B_extracted.md` — Seed 2.1 Pro Preview | 16 findings (0 Critical / 1 High / 4 Medium / 8 Low / 3 info) | **2** |
| **C** | `docs/audit/POST_S7_AUDIT.md` — Arena agent session | 13 findings (1 High / 4 Medium / 5 Low / 3 Nit) | **3** (executed code) |
| **D** | GPT 6 Astra Medium Audit (`arena.site/01a089b0…`) | 8 new findings (1 High / 6 Medium / 1 Low) — **new this round** | **1 — highest** (same rank as A) |

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
> have been incorporated into this compiled document and then deleted. This file is now the
> single source of truth for all findings from all four audits.

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

**All four audits converge on the same top failure class:**
**silent data loss / false success** — the tool reports success while destroying data,
writing nothing, or running single-threaded when the user asked for auto-threading.

| Metric | Count |
|--------|------:|
| Unique findings across all 4 audits | **52** |
| Confirmed BROKEN (wrong result / silent failure at runtime) | **14** |
| Confirmed MISALIGNED (code contradicts docs/labels) | **11** |
| Confirmed MISSING-logic (documented behavior that doesn't exist) | **16** |
| **New from GPT 6 Astra Medium (D)** | **8** (GS-101…GS-108) |
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

## 2. GPT 6 Astra Medium Audit (D) — 8 new findings (official export, corrected)

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

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-06`. Original report: ⬜ **OPEN** — confirmed by execution (C:U-06). `server.mjs:208`:
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

**Status:** ✅ **FIXED (S11)** — `CreateProcessW` + UTF-16 command line (strict
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

**Status:** ✅ **FIXED (S8)** — resolved; register §5 `U-08`. Original report: ⬜ **OPEN** — confirmed by execution (C:U-08). Root has `LICENSE` (1133 B) and
`COPYING.gifsicle` but **no `COPYING`**, so the `package_system` copy branch never fires.

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

**Status:** ◐ **PARTIAL (S13)** — provenance and configuration relocation are
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

**Status:** ✅ **FIXED (S11)** — the second `configure_file` is gone; the
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

**Status:** ✅ **FIXED** — S7 merged, CI green on both jobs (runs `34425977060` /
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

## 5. Consolidated master register — all 52 unique findings

Deduplicated across A/B/C/D. "Src" = which audit(s) raised it.

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
| **U-06** | A:GS-005 · D:GS-102 | **Web demo binds `0.0.0.0` with no auth, no concurrency cap, 64 MB bodies, 120 s engine runs.** Concurrent requests can race on temp dirs. | ✅ **EXEC** | ◐ PARTIAL (S8) — loopback bind + `GS_WEB_HOST` opt-in landed; the concurrency cap, per-client rate limit and engine-run bound named in the finding are still missing (see §13 intake) |
| **U-07** | A:GS-006 | **Windows CLI execution is ANSI-only.** `CreateProcessA` + `std::string` cmdline ⇒ non-ASCII paths cannot be passed to the engine. | ✅ **EXEC** (wine 8, mingw 12) | ✅ FIXED (S11) — `CreateProcessW` + argv/env re-fetch + u8path boundaries; wine E2E: é paths rc=0 (old build rc=1), CJK reaches the child losslessly |
| **U-08** | A:GS-007 | **License set can ship incomplete, silently.** Root has `LICENSE` + `COPYING.gifsicle` but **no `COPYING`**; every license copy is `if [[ -f ]]`-guarded. | ✅ **EXEC**+SRC | ◐ PARTIAL (S8) — silent-skip closed: both packagers now hard-require LICENSE + COPYING.gifsicle, with negative tests; the licence set itself is still incomplete (no full GPLv3 text, no Qt LGPL notices staged; owner decision pending — see §13 intake) |
| **U-09** | A:GS-008 | **Banked Windows snapshot is 5 commits behind the SHA its own notes claim.** Release body pins `d3544b1`; main is `8190c08`. | ✅ **EXEC** | ⬜ OPEN |
| **U-10** | A:GS-009 | **The "read-only, identical-to-upstream" vendored engine is neither.** Carries a handwritten `config.h` (Linux values), a functional patch, and an extra test. | ✅ **EXEC** (S11 re-clone; S13 relocation) | ◐ PARTIAL (S13) — provenance and product-config relocation verified; `reference_code/gifsicle/` is now upstream-only and native build stages `build_support/gifsicle/config.native.h`. MISSING: CI hash-pinning (workflows scope) |

### Medium

| ID | Src | Finding | Verif | Status |
|----|-----|---------|-------|--------|
| **U-11** | A:GS-011 | **Malformed booleans degrade silently.** `parse_bool` maps anything outside `1/true/yes/on` to `false` with no warning. | ✅ **EXEC** | ✅ FIXED (S8) — `parse_bool_strict` warns, leaves field unchanged |
| **U-12** | A:GS-012 | **"Fully async" GUI still blocks the UI thread in 5 places** — up to 5 s per run start. | ✅ **SRC** | ⬜ OPEN — scoped as P1-24 (S11); implementation deferred: the freeze is not reproducible offscreen, and the cancel rewrite would rewire T2/T9/T10 semantics with no executable proof of improvement |
| **U-13** | A:GS-013 · B:BUG-02 · D:GS-104 | **Drag-and-drop accepts any existing file.** Filter is `endsWith(".gif") \|\| exists(f)` — should be `&&`. Also: **empty comments emit `--comment` with no argument**, corrupting argv. | ✅ **SRC** | ✅ FIXED (S8) — drop filter `&&`; empty comments skipped (C++ + JS) |
| **U-14** | A:GS-014 | **Green CI does not enforce the claims used as release gates.** `verify_audit.sh` is never run in CI; package contents are never asserted. | ✅ **EXEC** | ◐ PARTIAL (S8) — negative packaging tests + manifest assertion in CI |
| **U-15** | A:GS-015 | **CMake writes into the source tree.** `configure_file` targets `${CMAKE_SOURCE_DIR}/src/core/version.h`. | ✅ **EXEC** | ✅ FIXED (S11) — build-tree-only configure_file (`build_support/version.h.in`); generated-first includes; gate C9 + read-only-src repro flipped FAIL->PASS |
| **U-16** | A:GS-016 · C:F-06 | **Settings persistence is non-atomic** (Truncate + write). A crash mid-write leaves a truncated conf. | ✅ **SRC** | ✅ FIXED (S10) — `save_settings_file` is tmp+fsync+rename; GUI save is QSaveFile; unit test 32 + T19 no-stray check |
| **U-17** | A:GS-017 · B:BUG-08 | **Explode mode never verifies any frame was written.** Output verification is explicitly skipped for Explode. | ✅ **EXEC** | ✅ FIXED (S11) — `src/core/ExplodeVerify.h` snapshot-diff (CLI+GUI); lying engine (rc=0, 0 frames) refused: unit 33, smoke 9-11, harness T7, wine rc=1 |
| **U-18** | A:GS-018 | **The regression suite does not cover any of the failure classes above.** No test for target collisions, package completeness, stdout purity, PATH fallback, or thread flags. | ✅ **EXEC** | ✅ FIXED (S12) — unit planning/thread coverage, packaging negatives, strict CLI parsing, byte-pure stdout, PATH-only engine discovery, and unsafe-output refusal; smoke suite 19/19 |
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

---

## 6. Fix order (all four audits combined)

**A and D are highest priority** — their findings are listed first within each severity tier.
B and C findings are merged in where they add coverage or contradict A/D.

### P0 — Stop silent data destruction (before ANY feature work)

| # | Action | Closes | Priority |
|---|---|---|---|
| P0-1 | **Plan all batch outputs before the first process starts.** Compute every source/target pair, compare for duplicates **and** target==source, refuse or auto-suffix on collision. Lock the plan. | **U-01, U-45, U-21** | **A/D highest** |
| P0-2 | **Fix threads "Auto" mapping.** `threads==0` → bare `-j` (auto-detect, 8 threads). `threads==-1` → no flag (gifsicle default, 1 thread). Update `Validate.h`. | **U-03** | **B highest** |
| P0-3 | **Make packaging fail closed.** Fresh staging dir, required-binary manifest, `windeployqt` failure is fatal, license set asserted, package E2E test. | **U-02** | **A highest** |
| P0-4 | **Re-cut release evidence.** Artifacts from exact tagged SHA, notes pinning that SHA, then run C4/D3/D4 against them. | **U-09** | **A highest** |

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
| P1-13 | **Settings string escaping.** Define backward-compatible escaping/quoting for `save_settings`/`load_settings` to handle newlines, CR, whitespace, `=` signs, and Unicode in string values. Or reject unrepresentable values before saving. | **U-51** | **D** |
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
| P2-7 | **Delete or CI-enforce `build.yml.proposed`.** | **U-39** | **C** |
| P2-8 | **Web transport round-trip tests.** Test `%`, `%20`, `%22`, plus signs, Unicode, malformed JSON through `searchParams.get()` path. | **U-49** | **D** |
| P2-9 | **Settings string round-trip tests.** Test newline, CR, whitespace, equals signs, Unicode in `save_settings`/`load_settings`. | **U-51** | **D** |
| P2-10 | **HTTP header safety tests.** Test CJK comments, newlines, Unicode engine path in `X-Gifscythe-Command` path. | **U-50** | **D** |
| P2-11 | **Web batch/merge/explode parity (scoped S11).** Mode selector + per-mode settings in the web UI, and a JSON multi-file endpoint (`POST /run`) that mirrors desktop semantics: batch runs a per-file Auto command with derived `<stem>_opt.gif` targets and REFUSES target collisions like the desktop planner; merge runs one `-m` command over all inputs in upload order; explode runs `-e`/`-E` against a `<stem>_frame` prefix and refuses rc=0-with-zero-frames exactly like the P1-19 desktop verification; every mode's output is existence+GIF-magic verified before success is claimed. Pin with fixtures in all three web suites. | **U-41** | **B** |

### P3 — Docs and polish

| # | Action | Closes | Priority |
|---|---|---|---|
| P3-1 | **Correct doc overstatements.** Reword SESSION_HANDOFF/IMPROVEMENT_LOG claims about "no warnings" and "exact inverse". | **U-19, U-20** | **C** |
| P3-2 | **Sync stale status lines.** Update WORKLIST/SESSION_HANDOFF with current CI status. | **U-27** | **A** |
| P3-3 | **Move dated review snapshots to `docs/archive/`.** | **U-44** | **C** |
| P3-4 | **Fix summary label for single-file batch.** Add `inputs_.size() == 1` case. | **U-43** | **C** |
| P3-5 | **Document CLI warning policy.** Add `--strict` flag or document the behavior. | **U-40** | **B** |
| P3-6 | **Position half-spec fix.** Only set `has_position` when both coordinates are provided. | **U-33** | **B** |
| P3-7 | **Third-parser consolidation.** Have `SettingsIO::load_settings` collect unrecognized keys; let GUI read `batch_dir`/`name_template` from that map. | **U-36** | **C** |
| P3-8 | **"Persistence unavailable" status note.** Add a one-time status-bar note when `sessionFilePath()` is empty. | **U-37** | **C** |
| P3-9 | **POSIX signal convention.** Return `128 + WTERMSIG(status)` instead of 1. | **U-32** | **B** |
| P3-10 | **`build.sh` `-lstdc++fs` autodetect.** | **U-31** | **B** |

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
- [ ] **A12** `./scripts/smoke_cli.sh` → 19/19
- [ ] **A13** **NEW:** threads=0 emits bare `-j` (not nothing)
- [ ] **A14** **NEW:** empty comment in conf does NOT emit `--comment` with no argument
- [ ] **A15** **NEW:** unknown CLI arg (`--rnu`) returns exit 2, not 0
- [ ] **A16** **NEW (S11):** explode `--run` verifies frames — real engine counts them on stderr; a lying engine (rc=0, zero frames) exits 1 naming the prefix; empty output uses the CWD basename prefix (smoke 9–11)
- [ ] **A17** **NEW (S11):** multi-input explode is refused — `validate()` warns (C++ + byte-identical JS mirror), `--run` exits 2 before any process starts, print mode keeps the warn-and-print policy, no CWD scatter (N-05; smoke case 12, unit block 35, harness T7)

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
- [ ] **D5** `reference_code/caesium-bin` untracked; `.gitignore` truthful

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
| **Web POC honest** | Server binds loopback by default; validates GIF magic; verifies output; no double-decode; browser URLs cleaned |
| **1.0.0** | Tabs + major controls + preview + clean Windows portable (§7.D) + no open U-01…U-10 | then bump VERSION.md |
| **2.x** | Only after 1.0.0: frame model → WebP/APNG |

---

## 10. Source index

- **Audit A:** `AUDIT_A_extracted.md` — GPT 5.6 sol xhigh (20 findings) — **file deleted, content merged into §3**
- **Audit B:** `AUDIT_B_extracted.md` — Seed 2.1 Pro Preview (16 findings) — **file deleted, content merged into §4**
- **Audit C:** `docs/audit/POST_S7_AUDIT.md` — Arena agent session (13 findings)
- **Audit D:** GPT 6 Astra Medium — `https://01a089b0-ef16-7451-bd81-a1c6a80d3252.arena.site/` (8 findings) — **new this round**
- **Consolidated:** `docs/audit/CONSOLIDATED_AUDIT_2026-09-10.md` — merges A+B+C (44 findings)
- **Product docs:** `PROJECT_VISION.md`, `WORKLIST.md`, `SESSION_HANDOFF.md`, `IMPROVEMENT_LOG.md`, `FEASIBILITY_REVIEW.md`
- **Engine truth:** `reference_code/gifsicle/` + https://www.lcdf.org/gifsicle/man.html
- **This file:** `COMPILED_AUDIT.md` — **master register, all 4 audits merged (52 findings)**

---

## 11. What changed in this compilation

1. **Audit A extracted** (`AUDIT_A_extracted.md`) read in full, content merged into §3, file deleted.
2. **Audit B extracted** (`AUDIT_B_extracted.md`) read in full, content merged into §4, file deleted.
3. **GPT 6 Astra Medium Audit (D)** fetched from `arena.site/01a089b0…`, 8 findings analyzed and merged into §2 and §5 (U-45 through U-52).
4. **Consolidated audit** (`docs/audit/CONSOLIDATED_AUDIT_2026-09-10.md`) read and cross-referenced.
5. **Post-S7 audit** (`docs/audit/POST_S7_AUDIT.md`) read and cross-referenced.
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

---

## 12. Handoff one-liner

> This compiled audit merges **four independent reviews** (GPT 5.6, Seed 2.1 Pro, Arena S7 agent,
> GPT 6 Astra Medium) into one 52-finding register. **The current roll-up of every one of them —
> plus the worklist, deferred and risk items — is `STATUS.md`; read that first for state, this
> file for evidence.** **Audits A and D are highest priority** — above
> B and the compiled audit. **A and D's 8 new findings (U-45…U-52) are fresh and not covered by
> prior audits.** The top item across all four audits is **U-01**: batch auto-naming silently
> overwrites other outputs **and** the source GIF. The threads bug (U-03) is the second-most-
> impactful finding and was missed by A and C but caught by B and re-confirmed by the S7 correction.
> **Audit A and B extracted files have been deleted** — this `COMPILED_AUDIT.md` is now the single
> source of truth. Run `verify_audit.sh` + `test_gui_offscreen` before trusting anything new.
> Never "fix" verified-correct behaviors (VP-1/2/3/5); never start WebP/APNG before GIF 1.0.0.

---

## 13. External review intake — 2026-09-12 (S14) — **NOT YET TRIAGED**

> **Read this as an inbox, not a register.** Three external reviews were compiled on
> 2026-09-12 into `docs/audit/EXTERNAL_REVIEW_INTAKE_2026-09-12.md`. The items below are
> **not** `U-nn` rows: they have not been triaged, scoped, or fixed, and **no §5 row above was
> changed** by compiling them. Nothing in this repository was remediated in that intake.

**Sources.** Max via OpenAI (highest tier; 10 findings, `GS-201…GS-210`) · DeepSeek (8 findings —
DeepSeek labels them `N-06…N-13`; **renamed `DS-06…DS-13` here**, because `N-01…N-06` are already
this repo's own session findings) · Gemini 3.8 flash high (**empty deployment — nothing to compile**).
Every item below was re-checked against the shipped source at
`2d51347817f5cdb39334415a03bb5f2b543119dd`; the intake file records the evidence, the reviewer's
proposed solution and the verification limits.

| ID | Severity | Finding (short) | Evidence location | Re-check |
|---|---|---|---|---|
| **GS-201** | Critical | CLI Batch maps to engine `-b` (in-place edit); the output planner is skipped when `output` is empty, so `--run` can rewrite the source GIFs | `working_code/gifscythe/src/cli/main.cpp`, `working_code/gifscythe/src/core/GifsicleCommand.h` | code-confirmed |
| **GS-202** | High | Web `/run` builds output paths and the explode prefix from client-supplied upload names; `../` escapes the request temp dir, collisions compared case-sensitively | `web/server.mjs` | code-confirmed |
| **GS-203** | High | Ordinary runs (Auto/Merge/Batch) claim success on exit 0 with no output verification; GUI and web accept stale/non-GIF files | `src/cli/main.cpp`, `src/qtui/MainWindow.cpp`, `web/server.mjs` | code-confirmed |
| **GS-204** | High | System packager never clears its destination, copies conditionally, always prints success; portable packager skips Qt deployment when the deployer is absent; negative tests cover portable only | `working_code/gifscythe/scripts/package_system.sh`, `working_code/gifscythe/scripts/package_portable.sh` | code-confirmed |
| **GS-205** | Medium | Non-GIF inputs still admitted: picker offers `All files`, `appendInputs` validates nothing, drop accepts a directory because it checks existence, not `isFile()` | `src/qtui/MainWindow.cpp` | code-confirmed |
| **GS-206** | Medium | `long` → `int` narrowing without range checks; validation has no rules for `loopcount`, `threads`, `gamma`, or method-name enums | `src/core/SettingsIO.h`, `src/core/Validate.h` | partly confirmed |
| **GS-207** | Medium | An unusable `GS_ENGINE` override is silently skipped and another engine runs (CLI and web) | `src/core/EngineLocator.h`, `web/server.mjs` | code-confirmed |
| **GS-208** | High | Main is release-red: run `34705247115` failed the Linux documentation gate while `SESSION_HANDOFF.md` claims a green open PR #15 and the pending-workflow marker describes an already-applied change | `.github/workflows/build.yml`, `SESSION_HANDOFF.md`, `docs/ci/PENDING_WORKFLOW_CHANGE.md` | live CI + local gate re-run |
| **GS-209** | Medium | Native "linux/mac" engine build still uses a fixed Linux/glibc `config.native.h` (headers, `random()`, type sizes, SIMD, `gettimeofday`) | `working_code/gifscythe/build_support/gifsicle/config.native.h`, `working_code/gifscythe/scripts/build_engine.sh` | code-confirmed |
| **GS-210** | Low | `build.sh` ignores unknown options, `build_engine.sh` treats any non-`--windows` argument as native, GUI dispatch tries qmake before CMake, `.pro` hardcodes the version | `build.sh`, `scripts/build_engine.sh`, `gifscythe.pro` | code-confirmed |
| **DS-06** | High | `threads <= 0` emits a bare `-j`, so the `-1` "unset" sentinel now means 8 threads instead of the engine's single-threaded default; no way to emit no flag | `src/core/GifsicleCommand.h`, `src/core/GifsicleSettings.h` | code-confirmed |
| **DS-07** | Medium | GUI Threads spinner spans `0..64` and always writes a value — "no flag / unchanged" is unrepresentable | `src/qtui/SettingsPanel.cpp` | code-confirmed |
| **DS-08** | Low | Non-strict print mode returns 0 even when validation warned; scripts cannot tell valid from warned without parsing stderr | `src/cli/main.cpp` | code-confirmed |
| **DS-09** | Info | `threads < -1` is accepted without warning and re-interpreted as "auto" | `src/core/SettingsIO.h`, `src/core/Validate.h` | code-confirmed |
| **DS-10** | Info | Disposal methods `4..7` (and the `-1` sentinel semantics) are unreachable from the desktop picker, though the engine and web validator allow them | `src/qtui/SettingsPanel.cpp`, `web/validate.mjs` | code-confirmed |
| **DS-11** | Medium | This file's own §3/§4 narrative still marks U-04/U-23/U-03/U-32-era items OPEN while §5 marks them FIXED — following §3 sends a reviewer after closed work | `COMPILED_AUDIT.md` | code-confirmed |
| **DS-12** | Low | The line-based settings format silently loses leading/trailing whitespace in values (documented, no rejection path) | `src/core/SettingsIO.h` | code-confirmed |
| **DS-13** | Medium | Web `/optimize` checks only non-empty output; no GIF magic check, so non-GIF bytes are served as `200 image/gif` | `web/server.mjs` | code-confirmed |

**Local gate state at the time of intake (recorded, not fixed).**
`working_code/gifscythe/scripts/check_docs.sh --no-gate-run` → **18 passed, 2 failed, 3 skipped**;
the failures are **G10** (this file's header names base `2176573`, while the accepted bases are
`2d51347` / `53a6eda`) and **G15** (fresh clone: `core.hooksPath` is not `.githooks`). The live
run `34705247115` failed its Linux documentation gate on main. **Do not treat main as green.**

**Cross-references inside this file.** GS-201 extends U-01's coverage gap (the planner is correct
but unreachable without an `output` key). GS-203 and DS-13 are one workstream (postcondition
verification: size + magic + changed-since-snapshot). DS-06/DS-07/DS-09/GS-206 are one workstream
(numeric sentinels and domains: decide the tri-state once). GS-208 and DS-11 are the same
stale-status failure mode in two files.

*End of compiled audit v2.*
