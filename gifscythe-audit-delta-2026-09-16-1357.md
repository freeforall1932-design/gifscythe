# Gifscythe - independent audit delta and shipping plan

**Generated:** 2026-09-16-1357 (from this console - deterministic content, timestamped file name)
**Repository:** freeforall1932-design/gifscythe - https://github.com/freeforall1932-design/gifscythe
**Pinned review state:** tree SHA `794a9964550fde0b7006474170926a6382c835f3` (fetched from `main`; no clone, nothing executed)
**Focus:** Find missing logic, misaligned code, broken code that COMPILED_AUDIT.md v3 has not registered; then propose a shipping path (stack, order, and what to stop doing).

## 0. Context of the reviewed product

Product version 0.1.0 - pre-release. STATUS.md last regenerated S22 (register header: 96 DONE / 8 PARTIAL / 42 OPEN / 0 UNTRIAGED / 146 total). Everything below was derived from the repository state at the fetched tree SHA, not from a clone.

| fact | value | source |
|---|---|---|
| Sessions logged | S4 then S7-S22 (19 numbered sessions) for a 0.1.0 pre-release of an argv-builder over a subprocess | README status paragraphs, STATUS.md session column |
| Product source of truth | working_code/gifscythe/src/core/*.h - header-only, Qt-free, 296 -> 308 unit checks across the record | COMPILED_AUDIT §16, STATUS rows U-18/U-53/U-62 |
| Surfaces shipping or scaffolded | Qt6 GUI, C++ CLI, Node web server, web/wasm (scaffolded, unshippable), csharp/spike (parked) | README repo layout, D-07/D-08 rows, S18/S19 |
| Engine | gifsicle 1.96 as a separate GPLv2 subprocess, argv array, never a shell | README licence note, licence set in repo root |
| Release bar in the repo's own words | 'Never call it 1.0.0 until the UI/UX task is done'; 0.2.0 allowed first; APNG/WebP deferred past GIF 1.0.0 | README versions note, OD-11 = a, PROJECT_VISION |
| Artefact actually published | One banked Windows snapshot, Release snapshot-2026-09-07, 5+ commits behind its own claimed SHA, pre-dating the S18 Ms-PL relicence | README, STATUS U-09, PR #23 |
| Open release blockers from the register | P0-4 (release re-cut, U-09), P0-7 (cancel leaves a truncated file over a previous good output, U-59), P1-24 (five GUI UI-thread waits, U-12), P1-25 (verifier not integrated in Qt, GS-203), plus 38 more OPEN rows | STATUS.md rows U-09, U-59, U-12, GS-203 and the generated counts |
| Proof debt | W-18 needs a clean Windows VM (and is itself blocked by U-09); W-19 needs a physical desktop for three probes the offscreen harness cannot reach | STATUS rows W-18/W-19, W-20/W-25 harness proofs |
| Doc machine | ~524 KB of root status docs + ~199 KB of archive/intake snapshots, 18 doc gates, 2 red mains caused by doc drift alone, 3 sessions of documentation-machine work (S9, S14/S15, S17) | file sizes from the repo tree; N-01/N-02/N-06/N-07, GS-208, SW-01..SW-03 |

| status document | bytes |
|---|---:|
| `COMPILED_AUDIT.md` | 213,853 |
| `IMPROVEMENT_LOG.md` | 160,408 |
| `SESSION_HANDOFF.md` | 65,870 |
| `STATUS.md` | 42,359 |
| `WORKLIST.md` | 41,368 |
| `docs/archive/* (2 snapshots)` | 128,017 |
| `docs/audit/EXTERNAL_REVIEW_INTAKE_2026-09-12.md` | 46,905 |
| `.github/workflows/build.yml` | 20,979 |

---

## 1. Result summary

| severity | findings |
|---|---:|
| P0 - P0 - data loss / liar | 0 |
| P1 - P1 - wrong behaviour or blocked release | 12 |
| P2 - P2 - hardening | 4 |
| P3 - P3 - polish | 2 |
| **total** | **18** |

All 18 items are **new relative to COMPILED_AUDIT.md v3 / STATUS.md** - either not registered at all, or registered only as a *fragment* whose consequence was never stated, or a reclassification of an existing row. Each carries its own relationship line, and none of them assert a new C++ finding: the C++ angle-bracket content could not be re-read through the fetch path, so every C++-dependent item is labelled `needs-probe` with the exact command that settles it.

### 1.1 Recommended order (do not start at the top of the table - start here)

1. **GN-03 (probe, 10 minutes)** - settle whether U-62's crop loosening re-opened U-22's resize geometry refusal. One grep, one CLI run. This delta registers no P0 of its own: the P0 work is the register's outstanding U-59 (cancel leaves a truncated file) and U-09 (release re-cut), both of which Phase A closes.
2. **A1 / U-59 (P0-7)** - temp+rename so a cancel can never leave a truncated file over a previous good result. The only remaining registered data-loss vector.
3. **A2 / U-58 (P1-38)** - snapshot settings with the plan so a batch run cannot change argv mid-flight.
4. **A4 / GN-13 (P1)** - the oracle fuzz gate: it is the machine that finds every future U-62/U-63-class bug, and it is offline and dependency-free.
5. **A3 (GN-01, GN-02, GN-10, GN-11)** - four small web refusals that turn silent/wrong results into named errors.
6. **A5 (GN-14)** - three rows move out of a VM-shaped blocker with tests that run on the CI job that already exists.
7. **A6 (GN-15 + U-09)** - make the published artefact legally and evidentially self-describing, then re-cut it.
8. **A7 (GN-04, GN-05, GN-17)** - make the register answer the release question, then freeze the doc machine for the rest of 1.0.0.

---

## 2. Findings (full detail - evidence, repro, fix, acceptance, falsification)

## 1. GN-01 - web /optimize ignores settings.mode: any mode is accepted, so a merge/explode can be served as a 200 "optimized GIF"

| field | value |
|---|---|
| Severity | P1 - P1 - wrong behaviour or blocked release |
| Class | misaligned code/docs |
| Area | web, core |
| Evidence level | read in the real source file this session |
| Relationship to COMPILED_AUDIT v3 | NEW. Not present in COMPILED_AUDIT v3. Adjacent to GS-201 (CLI Batch stop-loss, DONE S16) but that stop-loss lives in src/cli/main.cpp only - the /optimize endpoint has no equivalent guard. |
| Effort | S - one helper, two call sites, three transport assertions. |

### Evidence

- web/server.mjs header contract: "POST /optimize?settings= ... (single-file Auto mode; kept for compatibility and the transport suite)".
- web/server.mjs handleOptimize: settings are parsed, info:true is refused, validate() runs, the engine is located, then "const s = { ...settings, inputs: [inFile], output: outFile }" - settings.mode is passed straight through to buildArgs() with NO mode check.
- web/server.mjs handleRun, by contrast, does check: if (!["auto", "batch", "merge", "explode"].includes(mode)) sendJson 400, plus per-mode usage rules (auto = exactly 1 file, explode = exactly 1 file).
- So the same settings object is mode-validated on /run and mode-unvalidated on /optimize.

### Why it matters

The endpoint advertises Auto semantics but will happily emit -m (merge) or -e (explode) argv, return HTTP 200 image/gif and body.ok = true. A caller that asks for an optimization of an animation can receive a flattened single-frame merge (or a frame set) and be told it succeeded - the exact silent-wrong-result class this whole audit exists to kill. Because /optimize writes into a fresh mkdtemp dir it is not a data-loss bug, but it is a correctness-lie bug on a shipped surface, and web/README.md documents the endpoint's Auto-only contract.

### Reproduction

```bash
# Build the engine so discovery works: cd working_code/gifscythe && ./build.sh && ./scripts/build_engine.sh (per README quick start).
# node web/server.mjs 8000
# curl -sS -X POST --data-binary @some.gif -H 'Content-Type: image/gif' 'http://127.0.0.1:8000/optimize?settings=%7B%22mode%22%3A%22merge%22%7D' -o out.bin -w '%{http_code}\n'
# Expected (contract): HTTP 400 "mode is not supported by this endpoint". Actual (source-read): HTTP 200, Content-Type image/gif, X-Gifscythe-Command carries a -m line.
# Cross-check the mirrored case on /run: the same mode is refused there, which proves the rule exists and was simply not applied to /optimize.
```

### Proposed solution

- Close the asymmetry by making the mode guard a single shared function used by BOTH endpoints, instead of an inline check in handleRun.
- Cheapest correct change: in handleOptimize, right after normalising mode, refuse anything that is not auto/""/null with HTTP 400 and the same wording /run uses. Do not silently coerce to auto - coercion is how the wrong result gets a success code.
- Better: extract the whole per-mode admission block (mode normalisation + usage rules + info:true) from handleRun into one helper such as admitMode(mode, fileCount) returning { mode, error }, and call it from both handlers. That also keeps /optimize's compatibility promise honest if Auto-only is the intent.
- Add the rule to docs: web/README.md should state that /optimize is Auto-only and 400s on any other mode.

```diff
+// web/mode-admit.mjs (new) - one admission rule for both endpoints
+export const MODES = ["auto", "batch", "merge", "explode"];
+export function admitMode(rawMode, fileCount) {
+  const mode =
+    rawMode === undefined || rawMode === null || rawMode === ""
+      ? "auto"
+      : String(rawMode);
+  if (!MODES.includes(mode))
+    return { error: `unknown mode "${mode}" (auto|batch|merge|explode)` };
+  if (mode === "auto" && fileCount !== 1) return { error: "Auto processes exactly one file" };
+  if (mode === "explode" && fileCount !== 1) return { error: "Explode processes exactly one file per run" };
+  return { mode };
+}
+
+// web/server.mjs handleOptimize - right after JSON.parse of settings
+const admitted = admitMode(settings.mode, 1);
+if (admitted.error) { sendJson(res, 400, { ok: false, error: admitted.error }); return; }
+const mode = admitted.mode;   // "auto" - anything else already returned
```

### Acceptance / regression cases

- web/test/transport.test.mjs: new case - POST /optimize with mode=merge -> 400, and the engine is never spawned (assert no X-Gifscythe-Command header and an empty temp dir).
- Same suite: mode=auto still 200 + image/gif (regression guard for the compatibility path).
- Add the mirrored negative for /run with mode=auto and 2 files -> 400 (already covered; keep it next to the new case so the shared rule is visibly shared).

**How to falsify this finding (do this first):** grep -n 'mode' working_code/gifscythe/web/server.mjs | sed -n '1,40p' - if handleOptimize does contain a mode check I missed because of the transport mangling, this finding is void.

---

## 2. GN-02 - web app.js: an emptied number field becomes a real 0 setting, not "unset" (Number("") === 0)

| field | value |
|---|---|
| Severity | P1 - P1 - wrong behaviour or blocked release |
| Class | broken code |
| Area | web |
| Evidence level | read in the real source file this session |
| Relationship to COMPILED_AUDIT v3 | NEW. The audit registers the same *class* twice in C++ (DS-06 threads sentinel, DS-07 GUI spinner cannot express unchanged) and GS-206 (narrowing without range checks), but no row covers the JS side, where the mechanism is different: an empty input string converts to 0 instead of to the sentinel. |
| Effort | S/M - one helper + ~6 call sites + 2 fixtures; needs the C++ side confirmed for the null case. |

### Evidence

- web/app.js settings(): scale_x: Number($("scalePctX").value) / 100, and scale_y: Number($("scalePctY").value) / 100 (added by U-42 / P1 web parity work).
- web/app.js settings(): resize_w: Number($("w").value), resize_h: Number($("h").value).
- web/app.js settings(): color_count: $("colorsOn").checked ? Number($("colors").value) : -1, and delay_cs: $("delayOn").checked ? Number($("delay").value) : -1.
- The desktop models "unchanged" with a sentinel (-1) for lossy/colors/delay/loopcount precisely because the field must be omittable; the web applies the sentinel to the checkbox case but not to the text case.
- U-62 / P1-40 (S22) has just closed by *allowing* 0x0 geometry in crop - so a 0 that used to be refused upstream is now a value the pipeline is more willing to pass through.

### Why it matters

Number("") is 0, not NaN and not null, so clearing the Width box (or the Scale X/Y box) turns "leave it unchanged" into "resize to 0" / "scale by 0". Depending on validate(), that is either (a) a 422 the user cannot explain, or (b) an argv the engine receives (--resize-fit 0x0, --scale 0) and answers with a raw engine error under a UI that looks configured. Both outcomes are the "label says one thing, argv says another" family. It is the same defect DS-06 describes for threads, on the surface with no type system and no unit suite behind the conversion.

### Reproduction

```bash
# node web/server.mjs 8000, open http://127.0.0.1:8000 with a GIF queued.
# Set Resize kind = Fit, then clear the Width input (leave it empty) and watch the live command pane.
# Source-read expectation: the pane shows --resize-fit 0x<height> because Number("") === 0. Confirm with the pane, then confirm the same in the POST /run JSON payload (DevTools Network).
# Repeat with Scale X cleared: scale_x becomes 0. Compare against the desktop, where an empty/None field omits the flag.
```

### Proposed solution

- Introduce one explicit conversion helper in web/command.mjs (shared by app.js and any future client) that distinguishes empty from zero, and use it for every numeric control:
- Then decide per field what "empty" means: for geometry (w/h/scale) empty must mean OMIT the flag (never 0); for colors/delay it already means -1; for loopcount N, empty is a user error and must be refused in the UI before submit.
- Mirror the rule in validate.mjs with a 'missing but required' issue kind so the server refuses an empty-as-zero payload rather than passing a 0 to the engine.

```diff
// web/command.mjs (shared)
export function numOrNull(v) {
  const s = String(v ?? "").trim();
  if (s === "") return null;                 // unset - NOT zero
  const n = Number(s);
  return Number.isFinite(n) ? n : null;      // "abc" is unset, not NaN
}

// web/app.js settings() - geometry and scale
resize_w: numOrNull($("w").value),           // null => buildArgs must omit --resize-fit / --resize
resize_h: numOrNull($("h").value),
scale_x:  numOrNull($("scalePctX").value) === null ? null : numOrNull($("scalePctX").value) / 100,
scale_y:  numOrNull($("scalePctY").value) === null ? null : numOrNull($("scalePctY").value) / 100,
```

### Acceptance / regression cases

- web/test/validate.test.mjs + command.test.mjs: an argv fixture where resize_w/resize_h/scale_x/scale_y are null must contain no --resize*/--scale token at all (parity vs the C++ CLI, which already omits them for sentinel values).
- A fixture with resize_w = 0 keeps the current behaviour and is asserted explicitly, so the "0 is legal for crop" rule (U-62) stays pinned and is not confused with "empty".

**How to falsify this finding (do this first):** Read web/command.mjs: if it already normalises "" to null/sentinel for these keys, the bug is neutralised one layer down and only the UI label is wrong. Check that file before writing any code.

---

## 3. GN-03 - Registered pairing gap: U-62 (crop 0x0 now allowed, S22) was never cross-checked against U-22 (resize geometry must be refused, S8)

| field | value |
|---|---|
| Severity | P1 - P1 - wrong behaviour or blocked release |
| Class | unverified / false-confidence |
| Area | core, web |
| Evidence level | probe required before acting - labelled honestly |
| Relationship to COMPILED_AUDIT v3 | NEW as a *pairing*. The audit's own §19.3 lists four fix-adjacency pairings to re-probe after every fix (U-01/U-55/U-59, U-33/U-53, U-46/U-54, U-03/DS-06, GS-203/U-57). U-22 <-> U-62 is missing from that list even though both edits touch the same geometry validation. |
| Effort | S - probe 10 minutes; the fix, if needed, is one predicate in two files. |

### Evidence

- STATUS.md U-22 (DONE, S8): "Validate.h skips resize geometry, so a conf can reach the engine with --resize-fit 0x0" -> proof "resize/scale geometry validated (rules probed off the engine)".
- STATUS.md U-62 (DONE, S22): "Validate.h refuses crop W/H 0, engine allows 0=extend to edge" -> proof "crop 0x0 now passes native and web validation".
- Both live in src/core/Validate.h + the byte-identical web/validate.mjs mirror, and P1-40 is described in U-63's row as "Crop 0 and loop once - PARTIAL S22", i.e. the geometry loosening shipped in the same session as the crop work.
- Nothing in the register or §19 asserts that the loosening was scoped to --crop; a field-agnostic 'allow 0' rule would re-open U-22 silently and would pass every existing test, because the old failing fixture was the only witness.

### Why it matters

This is exactly the 'closing a pit while digging a new one' failure mode the audit's §19 mandates be hunted. If the crop loosening is keyed on the crop field, nothing is wrong and this finding closes in one command. If it is not, --resize-fit 0x0 and --scale 0 are reachable again from both the CLI and the web - the C++ control layer regressing the S8 guarantee while every suite stays green. Severity is deliberately P1 and not P0: no data loss is reachable (the engine writes a new file or refuses), the harm is a silently re-opened S8 refusal guarantee plus a wasted session if it surfaces later as a mystery engine error.

### Reproduction

```bash
# grep -n -A4 -B4 'crop' working_code/gifscythe/src/core/Validate.h    # is the 0 allowance inside the crop branch only?
# grep -n -A4 -B4 'crop' working_code/gifscythe/web/validate.mjs       # is the mirror identical?
# printf 'resize_kind = fit\nresize_w = 0\nresize_h = 0\ninput = a.gif\n' > /tmp/resize0.conf
# ./working_code/gifscythe/build/gifscythe-cli /tmp/resize0.conf --run --strict ; echo rc=$?
# Expectation after U-22: refused (rc=3, warned and refused by --strict). If it prints and runs, U-22 has been re-opened by U-62.
# Repeat the same payload as a schema.json/no-settings-body POST to /run on the web build and check for a 422.
```

### Proposed solution

- Do not guess: run the three commands above first and record the output in the U-62 row. This finding's disposition is one command away.
- If it reproduces: make the rule explicit instead of field-agnostic - a helper such as geometry_ok(kind, w, h) whose crop branch allows zero and whose resize/scale branches require >= 1, called from both Validate.h and validate.mjs.
- Whatever the outcome, add the pairing - 'U-22 <-> U-62: geometry loosening for crop must not loosen resize/scale' - to COMPILED_AUDIT.md §19.3 with the exact commands, so the next session inherits the probe rather than this note.
- Add a JS fixture to web/test/validate.test.mjs for resize 0x0 (refuse) next to the crop 0x0 fixture (allow) so the two rules can never be edited as one.

### Acceptance / regression cases

- A red-then-green test pair in the same file: crop 0x0 -> accepted, resize 0x0 -> refused, scale 0 -> refused.
- Unit suite count in the S23 log quotes a *runtime* counter (gate G9 rule) for the new cases.

**How to falsify this finding (do this first):** The probe itself falsifies it: if --resize-fit 0x0 is refused (rc=3 under --strict, 422 on the web), the pairing is clean and the finding collapses to 'add the pairing to §19.3'.

---

## 4. GN-04 - Fix-order ids (P0-n..P3-n) have no state, so a partially closed fix-order item is invisible to the register counts

| field | value |
|---|---|
| Severity | P1 - P1 - wrong behaviour or blocked release |
| Class | process / machine |
| Area | docs, process |
| Evidence level | read in the real source file this session |
| Relationship to COMPILED_AUDIT v3 | NEW. COMPILED_AUDIT §6 is the fix order and each register row names its P-id, but no artefact states whether a P-id is open/partial/done, and the header counts only row states. |
| Effort | M - emitter change + one gate + 3 mutation tests; ~1 session, mostly awk/bash. |

### Evidence

- STATUS.md header: "Counts (generated - do not edit by hand): 96 DONE - 8 PARTIAL - 42 OPEN - 0 UNTRIAGED - 146 total".
- STATUS.md U-63 row: state OPEN, proof "not started", next action "P1-40: Crop 0 and loop once - PARTIAL S22." - the row's own next action contains a PARTIAL claim about its fix-order id.
- Fix-order ids are many-to-many with rows: P1-40 covers U-62 (DONE) + U-63 (OPEN); P1-41 covers U-65 (OPEN) + U-66 (OPEN); P2-16 covers U-68 (PARTIAL) + U-69 (OPEN).
- Therefore 'what is left in P0?' / 'is P1-40 finished?' cannot be answered from the register without reading several rows by hand - the exact question the register exists to answer in one line.

### Why it matters

The register's stated purpose is to answer "how much is done?" in one line, and the release gate is 'no open P0/P1'. With P-ids state-less, a session can close one of three member rows of a P-id and the release picture still reads the same; the owner has to reconstruct completion by hand. It also means the 42 OPEN count can move while the fix-order backlog does not, or vice versa - the numbers that a release decision reads are not the numbers the work is planned in.

### Reproduction

```bash
# Open STATUS.md and search for "P1-40", "P1-41", "P2-16". Note that each appears in the Next action cell of more than one row.
# Try to answer 'is P2-16 done?': U-68 is PARTIAL, U-69 is OPEN - the P-id has no state of its own.
# Try to answer 'how many P0 items remain?' from the header counts alone: impossible, because P0-7 spans U-59 and P0-4 spans U-09, neither of which is labelled P0 in the row.
```

### Proposed solution

- Extend the emitter (scripts/check_docs.sh --emit), which already parses §5 top-to-bottom, with a third register part: one row per fix-order id, columns P-id / members / derived state / next member action. Derived state = AND of members: any OPEN member -> OPEN, else any PARTIAL -> PARTIAL, else DONE.
- Add the derived P0/P1 counts to the generated header next to the row counts, so the release bar ('no open P0/P1') is literally a number in the status header.
- Do NOT hand-maintain it - the whole value is that P-id state cannot drift from its members. This also gives the gate something to check (G5-style): a P-id whose derived state disagrees with the emitted block is a hard failure.

```diff
# generated header, after the row counts
**Fix-order state (derived from the rows above):** P0 4/6 closed - P1 19/31 closed - P2 8/16 closed - P3 2/9 closed
**Release bar:** no open P0/P1 -> 3 P0 and 12 P1 still open
```

### Acceptance / regression cases

- check_docs.sh plain mode diffs the new P-section like the rest, and a mutated member row fails the gate (mutation-test it the way S8 did).
- The next session's handoff quotes the derived P0/P1 numbers, not just the four row states.

**How to falsify this finding (do this first):** If STATUS.md already contains a per-P-id table further down (the retrieved copy was truncated mid-file), this finding is void - check the whole file first.

---

## 5. GN-05 - The single-status-register guarantee is unenforced where it matters: not in CI (blocked token), and inert in a fresh clone

| field | value |
|---|---|
| Severity | P1 - P1 - wrong behaviour or blocked release |
| Class | process / machine |
| Area | docs, process |
| Evidence level | read in the real source file this session |
| Relationship to COMPILED_AUDIT v3 | NEW as a chain. The audit holds all three links separately (W-30 OPEN, R-03 OPEN, R-04 PARTIAL, U-14 PARTIAL, GS-208 OPEN) but never states the combined consequence: on GitHub, nothing enforces the register. |
| Effort | S for the owner commit; M for the --json + digest change. |

### Evidence

- STATUS.md W-30 (OPEN): "blocked: the CI token has no workflows scope ... The step lives in docs/ci/build.yml.proposed".
- STATUS.md R-03 (OPEN): push rejected - the workflows directory cannot be updated without a workflows-scoped token; "drift is tolerated only via docs/ci/PENDING_WORKFLOW_CHANGE.md".
- STATUS.md R-04 (PARTIAL): "Still missing: a fresh clone is unprotected until it runs build.sh once, and nothing forces that".
- COMPILED_AUDIT §1/§16 plus STATUS rows record that main went red twice from documentation drift alone (GS-208, N-01), and gate E9/G7 deliberately SKIP on the declared workflow drift.
- Net chain: CI cannot run check_docs.sh -> the pre-push hook is the only enforcement -> the hook is inert until build.sh bootstraps it -> a fresh clone or a web-UI commit is unprotected.

### Why it matters

The repo's most expensive asset is the claim that STATUS.md cannot drift from the audit. That claim is currently enforced by nothing on the platform that actually merges code. Every session then spends effort re-verifying prose (S9, S14, S15, S17 all contain documentation-machine work) instead of closing P0/P1 rows. Two full sessions of doc-drift repair are already on the record.

### Reproduction

```bash
# Fresh clone: git clone --depth 1 <repo> /tmp/gs && cd /tmp/gs && git config --get core.hooksPath   # empty -> the doc gate is inert
# Open .github/workflows/build.yml and grep for check_docs / check_docs.sh: this review did NOT read the workflow file, so treat 'no gate step in the live job' as the expectation implied by W-30/R-03, and let this grep settle it. The gate itself lives in docs/ci/build.yml.proposed, which is a hand-maintained copy nothing executes.
# Make any prose edit to COMPILED_AUDIT.md, commit, push to a branch: if the push succeeds and CI stays green, the gate is not wired into CI (the local pre-push hook exists only after build.sh has run).
```

### Proposed solution

- Owner action (unblocks 3 rows at once): apply docs/ci/build.yml.proposed with a workflows-scoped token, delete docs/ci/PENDING_WORKFLOW_CHANGE.md in the same commit, and close W-30/R-03/GS-208 together. One commit, three rows, and E9/G7 stop SKIPping.
- Make the failure loud while waiting: docs/audit/REMEDIATION and the README header should carry one line - 'register enforcement: local hooks only (CI step pending)' - so no future session treats a green CI as proof the register is true.
- Make the gate self-evidencing rather than prose-counting: verify_audit.sh --json writes gate results to a file CI can publish as an artefact, and the register quotes a SHA-256 of that JSON instead of a hand-typed count. That removes the count-drift class (N-01) mechanically instead of by discipline.
- Make bootstrap non-optional: have scripts/check_docs.sh itself fail with a one-line instruction when core.hooksPath is not .githooks (G15 partially does this - extend it to the case where build.sh has never run).

### Acceptance / regression cases

- A CI run on a doc-only commit fails when STATUS.md is stale (the negative case that proves the step is live).
- verify_audit.sh --json output is downloadable from the run and its digest appears in STATUS.md.

**How to falsify this finding (do this first):** If .github/workflows/build.yml does contain a documentation status gate step (the retrieved copy was truncated), then W-30's row is what is stale, not the gate - and the finding becomes 'W-30 is a doc-drift row'.

---

## 6. GN-06 - The public README still presents the Qt6 GUI as the product while the newest planning decision targets a C#/WPF shell (Qt archived at cutover)

| field | value |
|---|---|
| Severity | P1 - P1 - wrong behaviour or blocked release |
| Class | doc drift |
| Area | docs, qt, csharp |
| Evidence level | read in the real source file this session |
| Relationship to COMPILED_AUDIT v3 | NEW. The audit records the C# plan (S18/S19 PARKED, OD-C7) and the C++/Qt direction, but no row covers the fact that the two coexist in the README's first screen. |
| Effort | S - documentation only, but it must be one commit and it must be the owner's call. |

### Evidence

- README quick start: "./build.sh --all # also Qt6 GUI (fails honestly if Qt missing)" + three Qt screenshots + "Never call it 1.0.0 until the UI/UX task is done."
- README layout: "csharp/  C# shell - PARKED S19 until 1.0.0 ships on C++/Qt6".
- docs/planning/CSHARP_SHELL_PLAN.md: goal "a portable, click-and-run Windows .exe built in C#/WPF with a custom Gifscythe UI/UX, driving the unchanged gifsicle engine subprocess ... shippable as 1.0.0", pre-parked state GO on all five OD-C decisions (WPF, archive Qt at cutover), and the plan's own context: the owner forked ScreenToGif as "a faster path to a Windows exe".
- So the README's headline UI and the repo's chosen UI for 1.0.0 are different artefacts, with an unresolved pivot in between.

### Why it matters

This is the single decision with the largest schedule effect in the repo (it decides whether 324 offscreen harness checks and the Qt task list are an asset or a sunk cost), and the public entry point does not mention it. A contributor reading README first will invest in the Qt surface; a session reading the plan will invest in WPF. Flip-flopping planning is how S18-S21 spent time on docs instead of on U-58/U-59.

### Reproduction

```bash
# Read README.md top-to-bottom, then docs/planning/CSHARP_SHELL_PLAN.md, then docs/planning/OFFLINE_BUILD_REVIEW.md §4.
# Try to answer: 'which UI is the product's UI at 1.0.0?' from the README alone - you cannot.
# Try to answer it from STATUS.md alone - W-19/W-20/W-23 all describe Qt GUI work as the near-term plan, and no row says 'Qt is retired at cutover'.
```

### Proposed solution

- Add a 6-line 'UI direction' block at the top of README.md and at the top of STATUS.md: shipped UI today (Qt6 GUI, offscreen/CI-proven in-sandbox, W-19 probes outstanding) / decided direction for 1.0.0 (per OD-C7: parked; resume point = CSHARP_SHELL_PLAN) / what is frozen until that decision is re-taken.
- Register the pivot as a single owned row (e.g. D-09 'Desktop UI direction and cutover') instead of leaving it implicit across W-19/W-20/W-26 and a parked plan. One row, one owner, one date - that is the whole fix.
- Until the row exists, do not add Qt UI work to the near-term list; that is the honest reading of a parked pivot. If the owner wants Qt at 1.0.0, delete the parked-plan half of the README instead - but pick one in the same commit.

### Acceptance / regression cases

- A reader who opens only README.md and STATUS.md can state the UI direction for 1.0.0 without opening docs/planning/.
- The new D-09 row appears in the emitted register with a proof/blocker cell that names the date of the decision.

**How to falsify this finding (do this first):** If STATUS.md already contains an explicit UI-direction row (truncated in the retrieved copy), the finding is only about the README.

---

## 7. GN-07 - Scope beyond the frozen vision (stills -> animated, video endpoints, promoted APNG/WebP) exists only inside a parked plan

| field | value |
|---|---|
| Severity | P1 - P1 - wrong behaviour or blocked release |
| Class | process / machine |
| Area | docs, process |
| Evidence level | read in the real source file this session |
| Relationship to COMPILED_AUDIT v3 | NEW. PROJECT_VISION's hard constraints and the README roadmap do not contain these items, and the register has no row for the amendment; only the parked plan does. |
| Effort | S - registration only. |

### Evidence

- docs/planning/CSHARP_SHELL_PLAN.md §2.1: 'Still-image collections -> animated (ezgif-maker-class, owner request 2026-09-14) ... Video <-> animated-picture conversion ... Editing features (ezgif-class: crop/resize/rotate/reverse/text-style frame ops)'.
- Same section, verbatim: "Mission-amendment note: PROJECT_VISION.md currently says 'Not photos, not video.' Items 3-4 narrow that to 'photos and video only as conversion endpoints/inputs, never as the subject.' The vision doc must be amended (with this rationale) before any stills-import or video-endpoint work starts - no code until the words change."
- README versions roadmap: 0.1.0 -> 1.0.0-1.9.9 (finished GIF product) -> 2.0.0-3.0.0 (WebP + APNG) - the stills/video/editing scope has no version.
- The plan is PARKED (OD-C7), so the amendment request is parked with it, in a file a future session is told to resume from.

### Why it matters

The park decision makes the leak *more* likely, not less: the plan is the designated resume point, so its scope list will be read as pre-approved scope. Item 3 (stills import) also drags in a decoder question that gifsicle cannot answer (gifsicle reads GIF inputs only) - i.e. a new FFmpeg sidecar with its own licence question (LGPL/GPL depending on build). That is a multi-session workstream hiding inside a one-line bullet.

### Reproduction

```bash
# grep -n 'amendment' docs/planning/CSHARP_SHELL_PLAN.md - note the plan's own precondition.
# grep -n -i 'photo\|video\|stills' PROJECT_VISION.md - note the words the plan says are still the constraint.
# grep -n -i 'stills\|video endpoint' STATUS.md WORKLIST.md - no row owns it.
```

### Proposed solution

- Register the scope as rows now, unstarted and explicitly gated: D-09 stills->animated (needs decoder story + licence note), D-10 video conversion endpoints (needs FFmpeg licence + argv contract), D-11 vision amendment (blocks D-09/D-10). A row costs one line and prevents 'the plan said we could'.
- Add the vision amendment paragraph to PROJECT_VISION.md as a *proposal block* (clearly marked unapproved) so the constraint text and its proposed narrowing live in the same file and cannot drift.
- State the engine precondition in the row itself: stills/video require a decoder that is not gifsicle; do not let a session start item 3 with single-frame GIFs as an undocumented hack.

### Acceptance / regression cases

- Three new rows exist in the emitted register with state OPEN and a blocker naming the licence/decoder questions.
- PROJECT_VISION.md contains the proposed amendment and no code has landed against it.

**How to falsify this finding (do this first):** If the register already carries rows for the stills/video scope (truncated section), collapse this to the vision-amendment half.

---

## 8. GN-08 - Same exit-code numbers, different meanings: the C++ CLI and the C# spike both claim 0/2/3 - and there is no shared exit-code contract

| field | value |
|---|---|
| Severity | P1 - P1 - wrong behaviour or blocked release |
| Class | misaligned code/docs |
| Area | cli, csharp, core |
| Evidence level | read in the real source file this session |
| Relationship to COMPILED_AUDIT v3 | NEW. Exit-code work exists (U-32 signalled child, U-40 --strict 3, GS-201 rc=2, GS-207 rc=1) but nothing compares the C++ code space with the C# spike's, even though the plan makes the CLI the C# shell's parity oracle. |
| Effort | S - a table, two small files, one gate; the value is that it is cheap now and structural later. |

### Evidence

- csharp/spike/Program.cs, verbatim: "// Exit codes: 0 ok - 2 usage/caller error - 3 engine missing - 4 engine failed - 5 invalid output (rc=0 but bad/missing GIF)."
- C++ CLI, per STATUS rows: U-40 '--strict refuses any warned conf with rc=3'; GS-201 'Batch + empty output ... rc=2'; GS-207 'CLI print/run exit 1' for an invalid GS_ENGINE override; U-32 'run_argv returns 128+WTERMSIG' (so 143/137 as well).
- So: CLI 3 = validation refusal vs C# 3 = engine missing. CLI 1 = resolution/engine error vs C# 1 = unassigned. C# 4/5 have no C++ counterpart at all.
- CSHARP_SHELL_PLAN.md relies on that contract: 'The C++ CLI stays as parity oracle' and a Phase gating that parity-tests the shell against the CLI.

### Why it matters

The parity oracle only works if a script can compare two exit codes and mean the same thing. Today, a wrapper that reports 3 cannot be interpreted without knowing which client produced it - so the planned parity test cannot be written honestly, and any user script (or the desktop's 'honest exit code' promise) inherits the ambiguity. It is a *cheap* thing to fix now and an expensive one to fix after the WPF port lands.

### Reproduction

```bash
# grep -rn 'return 3\|rc=3\|rc 3' working_code/gifscythe/src/cli/main.cpp docs csharp/spike/Program.cs
# Compare the two comment blocks side by side; the collision on 2 and 3 is visible in grep output alone.
# Attempt to write the Phase-2 parity assertion 'same argv -> same rc' and observe that you must special-case 1/3 to make it meaningful.
```

### Proposed solution

- Freeze one contract file, docs/contract/EXIT_CODES.md, as the single source of truth: 0 ok / 1 engine or resolution failure / 2 caller error (usage, unknown arg, refused run) / 3 refused by validation (--strict) / 4 engine ran, output invalid (rc=0 but no verified GIF) / 124 timeout / 127 engine not startable / 128+n signalled child.
- Emit it as code so it cannot drift: a tiny src/core/ExitCodes.h (enum class + to_string) and csharp/Core/ExitCodes.cs generated from the same markdown table by a 30-line script, wired into the existing doc gates as 'generated file matches source table'.
- Update the C# spike's comment and returns to the contract, then add the parity case to the Phase-2 gate: same argv, same rc, same meaning, asserted for at least 'engine missing' and 'output invalid'.
- Rename the numeric reasoning in the plan: the C# side must not invent 4/5 semantics - it should adopt them from the contract.

```diff
# docs/contract/EXIT_CODES.md  (excerpt - single source of truth)
| code | meaning                        | who returns it                  |
|------|--------------------------------|---------------------------------|
| 0    | ok, output verified            | CLI / web / GUI / spike         |
| 1    | engine or resolution failure   | CLI (GS_ENGINE override, spawn) |
| 2    | caller error / refused run     | CLI (usage, Batch+no output)    |
| 3    | refused by validation (strict) | CLI --strict                    |
| 4    | engine ran, output invalid     | web/GUI/spike verify step       |
| 124  | engine timeout                 | web runner, GUI, spike          |
| 127  | engine not startable           | all clients                     |
| 128+n| signalled child (POSIX)        | core run_argv                   |
```

### Acceptance / regression cases

- The generated header/sharp file matches the table (gate asserts it).
- A smoke case asserts CLI '--strict warned conf' = 3 and 'engine missing' = 127, so 3 can never again mean 'engine missing'.
- The C# spike's README documents only codes from the table.

**How to falsify this finding (do this first):** If docs/contract/EXIT_CODES.md already exists, this finding is void - grep for it first.

---

## 9. GN-09 - C# spike: two port-time traps already visible - Stream.Read may under-fill the magic probe, and the shown command line is POSIX-quoted on a Windows product

| field | value |
|---|---|
| Severity | P2 - P2 - hardening |
| Class | broken code |
| Area | csharp, qt |
| Evidence level | read in the real source file this session |
| Relationship to COMPILED_AUDIT v3 | NEW. The spike is explicitly throwaway-allowed and inert, but the plan makes it the Phase-2 seed for the shell and the C++-CLI parity oracle, so its two wrong details are inherited forward. |
| Effort | S - two small edits plus two tests; do them when Phase 2 actually resumes, but record them now. |

### Evidence

- csharp/spike/Program.cs VerifyGif: "using var stream = File.OpenRead(path); head = new byte[6]; if (stream.Read(head, 0, 6) != 6) return \"produced an empty/short file\";" - Stream.Read is documented as returning *up to* count bytes; the correct call is ReadExactly (or a loop).
- csharp/spike/Program.cs Quote(): "if (a.Length > 0 && a.All(c => char.IsLetterOrDigit(c) || \"/._-+=:@%,\".Contains(c))) return a; return \"'\" + a.Replace(\"'\", \"'\\''\") + \"'\";" and it is printed as the 'command' line.
- The repo's own desktop convention is MSVCRT quoting (the audit notes a custom CreateProcessA + MSVCRT quoting was added after MinGW _spawnvp split on spaces) and the product promise is a live command pane that shows 'exactly what the engine runs'.

### Why it matters

Two small details with disproportionate consequence in Phase 2: (1) a partial header read makes a valid GIF fail verification with a *false* 'invalid output' - the opposite failure direction from the audit's usual silence, but still a lie about the engine; it is also a classic port bug that survives code review because it looks correct. (2) Copying a POSIX-quoted line as the pane text breaks the pane-as-truth contract the moment a path contains a space (single quotes are literal characters in cmd.exe), and the pane is explicitly a hard constraint in the C# plan §2.

### Reproduction

```bash
# dotnet run --project csharp/spike -- <engine> <input> <output> and read the printed command line; paste it into cmd.exe - the quoting is wrong on Windows.
# Grep the repo for the quoting rule the desktop uses (ProcessRunner / command pane) and diff it against Quote().
# For the read bug: it needs a stream that returns short reads (network share, or a wrapper); assert with a custom Stream in a 5-line test - FileStream on a local disk usually hides it, which is why it must be pinned by a test rather than by observation.
```

### Proposed solution

- Replace the manual read with a read-exactly helper and keep the six-byte probe: `using var fs = File.OpenRead(path); var head = new byte[6]; fs.ReadExactly(head);` (catch EndOfStreamException -> 'empty/short file').
- Move the display quote into a port of the existing quoting contract, e.g. csharp/Core/CommandLineQuote.cs implementing the same rules the C++ ProcessRunner prints (and the same rules cmd.exe/CommandLineToArgvW round-trip), and add a fixture: path with a space, path with an embedded double quote, CJK comment.
- Record both as Phase-2 acceptance criteria in CSHARP_SHELL_PLAN.md so the port cannot 'simplify' them away.

### Acceptance / regression cases

- A test with a short-reading stream yields 'verified' for a valid GIF (red before the fix).
- A quoting fixture round-trips: spawn the child with the printed string through cmd.exe and assert the child sees the original argv.

**How to falsify this finding (do this first):** Read the current Program.cs - if ReadExactly / an MSVCRT quoter is already there, the snippet I read is stale (the file may have changed after the snapshot).

---

## 10. GN-10 - The non-GIF admission fix (GS-205) is scoped to the desktop, so both web endpoints still accept arbitrary bytes - and base64 decoding is forgiving

| field | value |
|---|---|
| Severity | P2 - P2 - hardening |
| Class | missing logic |
| Area | web, core |
| Evidence level | read in the real source file this session |
| Relationship to COMPILED_AUDIT v3 | EXTENDS GS-205 (OPEN, S15): its file list names only src/qtui/MainWindow.cpp:358,405,415, so the web half of the same defect has no registered row. |
| Effort | S/M - the magic helper exists; the work is wiring plus three tests. |

### Evidence

- STATUS GS-205: 'Non-GIF inputs still admitted: picker offers All files, appendInputs validates nothing, drop checks existence not isFile()' - cited files are Qt only.
- web/server.mjs /run admission: for each file it checks only 'a non-empty name and base64 data' plus uploadNameError(f.name) - no content check before the engine is spawned.
- web/server.mjs already owns the primitive: isGifMagic() reads 6 bytes and hasGifMagic() comes from ./output-verify.mjs - it is used for the *explode output* verification, not for the *upload*.
- web/app.js drop filter is extension/MIME-only (/\.gif$/i or type === image/gif), which is the web twin of the desktop's existence-only check.
- No explicit base64 validation appears in the retrieval: a payload whose base64 is truncated or contains foreign characters is decoded by Node's forgiving Buffer.from(str, 'base64') into silently different bytes.

### Why it matters

Two surfaces, one defect: the web endpoint is the one that is *reachable by a stranger* when GS_WEB_HOST=0.0.0.0 is set, and it will happily write any bytes to a temp file, spawn the engine, and answer with an engine error string. The fix is already 90% written (hasGifMagic exists) - the missing half is admission on input. The forgiving base64 decode is the same class: a malformed request produces a confusing engine failure instead of a 400 that names the real problem.

### Reproduction

```bash
# printf 'PK\003\004notagif' > /tmp/notagif.bin
# curl -sS -X POST --data-binary @/tmp/notagif.bin 'http://127.0.0.1:8000/optimize?settings=%7B%7D' -i | head -20  # observe: no 400 'not a GIF'; the engine's own message is relayed
# POST /run with files:[{name:'x.gif', data:'AAAA==AAAA=='}] (trailing garbage in base64) and compare the decoded size to the announced one.
```

### Proposed solution

- Extend the GS-205 row to name the web surface, and implement one admission predicate used by desktop + web + CLI: existing, readable, regular file, starts with GIF87a/GIF89a, plus a base64 strictness check on the wire format.
- In /run and /optimize, verify hasGifMagic on the decoded buffer BEFORE writing the temp input file, and answer 400 'input is not a GIF (GIF87a/GIF89a signature missing)'.
- Validate base64 strictly: decode, then re-encode and compare, or check /^[A-Za-z0-9+/]*={0,2}$/ plus length % 4 and the decoded-size vs declared size; reject with 400 instead of letting engine stderr be the error message.

```diff
// web/run-paths.mjs (or a new web/admit.mjs) - one admission rule for both endpoints
export function admitGifBuffer(buf) {
  if (!buf || buf.length < 6) return "input is empty or too short to be a GIF";
  if (!hasGifMagic(buf.subarray(0, 6))) return "input is not a GIF (GIF87a/GIF89a signature missing)";
  return null;
}
export function decodeBase64Strict(data) {
  if (!/^[A-Za-z0-9+/]*={0,2}$/.test(data) || data.length % 4 !== 0) return { error: "invalid base64 payload" };
  const buf = Buffer.from(data, "base64");
  if (buf.toString("base64").replace(/=+$/, "") !== data.replace(/=+$/, ""))
    return { error: "invalid base64 payload (round-trip mismatch)" };
  return { buf };
}
```

### Acceptance / regression cases

- transport test: a non-GIF body to /optimize and to /run -> 400 with the GIF-signature message, and the engine is never spawned (assert on the temp dir staying empty).
- Body containing a valid GIF still 200 (no over-refusal).
- A truncated base64 payload -> 400, not an engine error string.

**How to falsify this finding (do this first):** grep -n 'hasGifMagic\|GIF87a' working_code/gifscythe/web/server.mjs - if an upload-side check is already present in the batch admission loop (partially lost in retrieval), only the base64 half stands.

---

## 11. GN-11 - web server: an unvalidated PORT value crashes the process with a stack trace instead of failing with a message

| field | value |
|---|---|
| Severity | P2 - P2 - hardening |
| Class | broken code |
| Area | web |
| Evidence level | read in the real source file this session |
| Relationship to COMPILED_AUDIT v3 | NEW. Not covered by U-06 (bind address / caps) or the static-hygiene rows. |
| Effort | XS - ten lines plus one smoke case. |

### Evidence

- web/server.mjs: "const PORT = Number(process.argv[2] || process.env.PORT || 8000);" then later "server.listen(PORT, HOST, () => {...})" with no try/catch and no range check.
- Number() on a non-numeric string is NaN; on a value like '8080/' or a PaaS-style 'tcp://...' it is NaN too, and Node's listen throws ERR_SOCKET_BAD_PORT synchronously at module scope.
- The server module has no top-level try/catch (only the http.createServer request handler is wrapped), so the throw escapes as an uncaught exception with a stack trace.

### Why it matters

A one-line startup failure becomes a stack trace - and startup failures are exactly what a self-hosted user hits first (hosting panels set PORT, sometimes to a socket path or with trailing whitespace). It also breaks the repo's own honesty rule for user-facing errors: every other refusal in this codebase names the reason.

### Reproduction

```bash
# PORT=abc node web/server.mjs
# Observe: RangeError [ERR_SOCKET_BAD_PORT]: options.port should be >= 0 and < 65536 ... with a stack trace, exit 1 - no mention of what the user typed.
# Also: node web/server.mjs 999999 (argv path) behaves the same.
```

### Proposed solution

- Validate once, near the top, and exit 2 with a named reason (matching the CLI's caller-error code from GN-08):
- Cap the accepted range to 1..65535 and state that 0 means 'any free port' if you want to allow it. Print the value that was rejected, never the raw stack.

```diff
+function parsePort(raw) {
+  const n = Number(String(raw).trim());
+  if (!Number.isInteger(n) || n < 0 || n > 65535)
+    return { error: `invalid port ${JSON.stringify(raw)} (expected an integer 0-65535; pass it as argv[2] or set PORT=)` };
+  return { port: n };
+}
+const parsed = parsePort(process.argv[2] ?? process.env.PORT ?? 8000);
+if (parsed.error) { console.error(`gifscythe web: ${parsed.error}`); process.exit(2); }
+const PORT = parsed.port;
```

### Acceptance / regression cases

- A test (or smoke script) asserts PORT=abc exits 2 with the message and no stack trace.
- PORT=8000 still starts and prints the loopback banner.

**How to falsify this finding (do this first):** If a recent commit added port validation, this is already closed - check the first 40 lines of web/server.mjs.

---

## 12. GN-12 - web transport robustness: unbounded stderr capture, whole-response base64 fan-out, and /favicon.ico noise

| field | value |
|---|---|
| Severity | P3 - P3 - polish |
| Class | missing logic |
| Area | web |
| Evidence level | read in the real source file this session |
| Relationship to COMPILED_AUDIT v3 | NEW (hygiene). U-06's PARTIAL row covers concurrency/rate limits; these are the memory-shape details of the same endpoint and are not registered. |
| Effort | S - three independent small changes; none blocks the release. |

### Evidence

- web/server.mjs engine runner: "let stderr = \"\"; child.stderr.on(\"data\", (d) => { stderr += d.toString(); });" - no cap, and the full string is echoed back in the 422 body.
- web/server.mjs /run: outputs are collected as { name, path, data: verified.data } and returned as base64 inside one JSON response, so a batch of N files inlines every output in memory and on the wire (and app.js then builds one Blob per output).
- STATIC_FILES has no /favicon.ico entry, so a browser request for it gets the 404 text/plain 'not found' body unless index.html points at a data-URI icon - index.html was not read in this review, so verify that half before acting.

### Why it matters

A chatty engine (warnings per frame, a huge malformed input) can make the server hold and echo arbitrarily large stderr; a 4-mode batch of large GIFs can produce a response larger than the request that produced it. Both are the ordinary shape of memory bugs in a self-hosted tool, and both are cheap to bound now, before the wasm/desktop-parity work makes the transport load-bearing.

### Reproduction

```bash
# Feed a corrupt large file that makes gifsicle emit many warnings: watch the 422 body grow with the stderr text (curl -sS ... | wc -c).
# POST /run with 8 large GIFs (batch) against a server started with a low --max-old-space-size and observe peak RSS vs the request size.
# Load the UI and watch the network panel: /favicon.ico -> 404.
```

### Proposed solution

- Cap stderr at a constant (e.g. 16 KB) with an explicit truncation marker ('... [stderr truncated at 16 KB]'), which also matches the C# spike's Trim(500) intent.
- Bound the /run response: either return one output per request for batch (outputs metadata + a download id), or document and enforce a total-output cap with a 413-style refusal naming the cap. At minimum, compute the projected base64 size before building the JSON and refuse with a clear error when it exceeds a documented envelope.
- Add a favicon data-URI link in index.html (or an allow-list entry), so the 404 disappears without adding a file to the allow-list.

### Acceptance / regression cases

- A stderr-flood test asserts the 422 body is bounded and contains the truncation marker.
- A test asserts the documented output envelope refusal fires with a named reason and a machine-readable cap.

**How to falsify this finding (do this first):** grep -n 'stderr' working_code/gifscythe/web/server.mjs - if a cap already exists (possibly lost in retrieval), only the fan-out and favicon halves stand.

---

## 13. GN-13 - Hand-written mirrors prove agreement, not correctness: no suite sweeps the settings space against the real engine, so the U-62/U-63 bug class is still found by humans

| field | value |
|---|---|
| Severity | P1 - P1 - wrong behaviour or blocked release |
| Class | unverified / false-confidence |
| Area | core, web, process |
| Evidence level | read in the real source file this session |
| Relationship to COMPILED_AUDIT v3 | NEW as a diagnosis. The audit describes the parity suites as an asset (296/308 unit checks, five Node web suites, byte-identical mirror); it does not register that parity between two hand-written copies cannot detect a shared misunderstanding of gifsicle. |
| Effort | M/L - one session for a first bounded version; the highest long-term leverage item on this list. |

### Evidence

- COMPILED_AUDIT/§16: 'web/test/command.test.mjs cross-checks the JS command.mjs output against the C++ gifscythe-cli binary' - i.e. JS is compared to the CLI, and the CLI's builder is the mirror image of command.mjs.
- server.mjs and app.js both import buildArgs from ./command.mjs; src/core/GifsicleCommand.h is the C++ twin - the repo's own comments call them mirrors of one another.
- The record already contains two counterexamples where BOTH mirrors were wrong together and only the real engine disagreed: U-62 (crop 0x0 - 'engine allows 0=extend to edge') and U-63 (--no-loopcount play-once, 'engine has unchanged / forever (0) / N / OFF').
- To be precise about what is missing: the engine IS executed - scripts/test_engine.sh (5/5) and the smoke suite's --run cases run the real binary. What no suite does is sweep the *settings space* through it: coverage is the hand-picked smoke cases plus the mirrors' fixtures, so a rule both mirrors encode wrongly has nothing to disagree with.
- The audit's own method note adds the other half: in several auditing environments the engine binary was not built at all, so engine-dependent behaviour was never exercised, and the suites that were run could not have contradicted the mirrors.

### Why it matters

This is the mechanism that keeps producing findings late: every rule that both mirrors encode wrongly is invisible to the entire suite, and each discovery costs a session (U-62, U-63, N-05 multi-input explode were all engine-semantics discoveries). A generated conformance run against the real gifsicle converts that class from 'discovered by a careful reviewer' into 'discovered by CI'. It needs no new dependency, no network, and it works with the engine the repo already builds.

### Reproduction

```bash
# Show the gap: ./build.sh && ./scripts/smoke_cli.sh (green), node web/test/command.test.mjs (green) - then run one argv the mirrors never generate: ./build/gifsicle -O3 --no-loopcount in.gif -o out.gif and watch the engine accept a flag no settings field can emit (U-63).
# Second probe: generate argv from a random settings object and run it; count the cases where exit code != 0 while the product's own validate() accepted the settings - each one is a latent 'user configured what the UI offered, engine refused' finding.
```

### Proposed solution

- Add scripts/oracle_fuzz.mjs (node, zero deps, offline): enumerate a bounded, seeded sample of the settings space (optimize level x lossy on/off x colors on/off x dither x resize kinds x scale x loop states x delay x mode), map each sample through command.mjs AND through the CLI's print mode, then execute the argv against the repo-built engine on a 3-frame fixture GIF.
- Assert two directions and report a table: (a) every settings object the product can emit must be engine-accepted and produce a verified GIF; (b) every engine refusal must correspond to a settings object the product refuses or warns about. Commit the generated table as an artefact so drift shows up as a diff.
- Wire it into verify_audit.sh as a new gate (W6) with a --quick mode (fixed seed, first N samples) for the pre-push hook and a --full mode for CI, so it is both fast locally and exhaustive in CI.
- Feed the table back into the register: each (settings -> engine) mismatch becomes a row automatically, which is the cheapest finding-generation machine this repo can own.

```diff
# scripts/oracle_fuzz.mjs (shape - zero deps, offline, uses the repo engine)
#   node scripts/oracle_fuzz.mjs --quick --engine build/gifsicle --fixture test/fixtures/anim3.gif
# outputs: docs/audit/ORACLE_MATRIX.md   (seeded, committed, diffed by a gate)
1. seeded sampler over gs::Settings (finite enumerations only; step the numerics coarsely)
2. argvA = command.mjs buildArgs(s)          # JS mirror
3. argvB = gifscythe-cli conf --print        # C++ mirror, same conf file
4. assert argvA === argvB                    # parity: catches mirror drift
5. run argvA against the engine -> rc, stderr, output magic
6. assert (rc === 0 && verified) for every sample the product would accept
7. write mismatches to ORACLE_MATRIX.md with the settings JSON and the engine stderr
```

### Acceptance / regression cases

- The gate is red on at least one known case before the fixes land (crop 0x0 pre-U-62 is the historical reference point - if nothing is red today, say so and keep the gate as a regression net).
- Command parity (step 4) is asserted for every sample, so the two mirrors can no longer drift silently.

**How to falsify this finding (do this first):** If web/test/command.test.mjs already executes a broad sampled matrix through the real engine (not just the CLI), this finding is a duplicate - read that suite before starting.

---

## 14. GN-14 - Three 'needs native Windows' rows are pure functions testable on the existing windows-latest CI job today

| field | value |
|---|---|
| Severity | P1 - P1 - wrong behaviour or blocked release |
| Class | process / machine |
| Area | core, process, cli |
| Evidence level | from repo-quoted fragments / register text (C++ angle brackets were lost in transit) |
| Relationship to COMPILED_AUDIT v3 | RECLASSIFIES rows the register blocks on a clean VM: U-55 (Windows path_key case folding), U-56 (superscript COM/LPT aliases), U-71 (exit mask &0xff collapses an NTSTATUS). U-70 (UTF-8 preview boundary) is left alone here - it runs through the preview pipeline rather than a pure rule. |
| Effort | S/M - test writing only; the toolchain and CI job already exist. |

### Evidence

- STATUS.md blockers: U-55/U-56/U-70/U-71 all say 'not started' with a Windows-native requirement; W-18 adds 'needs a clean Windows VM with no Qt/MinGW/dev tools'.
- The register itself proves the pattern is viable on Linux: U-21's proof is 'new src/core/OutputName.h (NameRules parameterised, so the Windows rule set is unit-tested on Linux)' and W-23's proof is 'unit tests 29/30 (Windows rule set tested on Linux)'.
- U-07 (the one case that truly needed CreateProcessW) was executed under Wine, proving a native-Windows runtime is not required for argv/path logic - only for ctypes that are OS calls.
- path_key() is a string normalisation over UTF-8 bytes; is_windows_reserved_device_name() is a name predicate. Neither spawns a process, opens a device, or links Qt - so both are reachable from the header-only unit suite, and a windows-latest job already exists (Windows CI green since PR #5). Unverified in this review: whether that job currently builds and runs the unit binary - if it only builds the engine + CLI, the fix is to add the unit step, which is still far cheaper than a VM.

### Why it matters

Three release-adjacent rows are parked behind a slow, unverifiable blocker when the correct test is a 20-line case in a suite that already runs on both platforms every push. Parked rows accumulate: W-18's own proof cell says it is *blocked by U-09*, so one stale artefact now also blocks the clean-machine gate. Moving U-55/U-56/U-71 into the unit suite removes three rows from the 'cannot be closed here' pile at the cost of one session of test writing.

### Reproduction

```bash
# grep -n 'Windows' .github/workflows/build.yml | head -20  # confirm the windows job exists and what it runs
# grep -rn 'unit' working_code/gifscythe/CMakeLists.txt build.sh | head  # confirm the unit binary is buildable on Windows CI
# Write one case for path_key('HERO.GIF') vs path_key('hero.gif') compiled with -D_WIN32, and one for is_windows_reserved_device_name('COM\u00b9') - run locally on Linux first, then in the windows job.
```

### Proposed solution

- Add the three Win32 rule cases to the header-only unit suite with the same parameterisation trick used for OutputName (rule set injected, so Linux runs the Win32 rules too).
- Run that suite in the existing windows-latest CI job (it already builds the engine and CLI there) as a second invocation, so the *native* semantics are asserted on real Windows - which is strictly stronger evidence than Wine and needs no VM.
- Re-word the three register rows: blocker becomes 'needs the Win32 rule cases (test-only)', and U-70 keeps the real desktop dependency. Update W-18's blocker so it no longer cites U-09 for the desktop-only probes that do not depend on the artifact.
- Keep Wine + a clean VM only for the two remaining genuine cases (drop a file on the window, kill the engine mid-run) - W-19 already exists and is honest.

### Acceptance / regression cases

- Three rows move out of 'blocked' with executed proof in the windows CI job log (the run URL goes in the proof cell).
- The Linux run of the same suite stays green with the Win32 rule set injected.

**How to falsify this finding (do this first):** If U-55/U-56/U-71's blockers are not the VM but something other than testability (e.g. a missing Windows fixture engine), the reclassification is wrong - read the three rows in full first.

---

## 15. GN-15 - The only banked artefact is pre-relicence: the published Release predates S18's GPLv3 -> Ms-PL switch on first-party code

| field | value |
|---|---|
| Severity | P1 - P1 - wrong behaviour or blocked release |
| Class | doc drift |
| Area | release, docs, process |
| Evidence level | read in the real source file this session |
| Relationship to COMPILED_AUDIT v3 | NEW. U-09 covers 'banked snapshot is 5 commits behind' as a release-evidence problem; U-08 covers licence-file completeness *inside* a package. Neither notices that the relicence changed the licence of the code the published binaries were built from. |
| Effort | S for the note; M for the re-cut + tag-triggered gate (already scoped as P0-4/U-09). |

### Evidence

- README: 'binaries banked on Release snapshot-2026-09-07' and 'Windows CI green and merged 2026-09-07 (PR #5 -> 0ad1ff5 ...)'.
- STATUS U-09 (OPEN): 'Banked Windows snapshot is 5 commits behind the SHA its own notes claim'.
- README/repo: 'S18: Ms-PL relicense + C# shell plan + Phase-1 spike' (PR #23, merged 2026-09-14) - first-party code relicensed GPLv3 -> Ms-PL; LICENSE now Ms-PL, with docs/legal/WHY_MSPL.md and docs/legal/COPYING_RULES.md added later.
- Therefore the 2026-09-07 asset was built from GPLv3-licensed first-party code and predates the Ms-PL licence files, the legal docs, and the S19 licence-completeness packaging work (U-08).

### Why it matters

A published release asset is the one artefact a third party actually takes away. Right now the downloadable ZIP's licence, the repo's licence, and the licence files bundled inside the ZIP are from three different eras. For a project whose stated discipline is 'never let the docs and the artefact disagree', this is the highest-consequence disagreement available, and it is invisible to every code gate because no gate looks at the release page.

### Reproduction

```bash
# Open the Release 'snapshot-2026-09-07', download the ZIP, and read LICENSE / the bundled COPYING.* files - compare against main's LICENSE (Ms-PL) and docs/legal/WHY_MSPL.md.
# Compare the asset's commit against the tagged SHA its notes claim (U-09).
# grep the asset for QT_NOTICE.txt / COPYING.gplv3 - the S19 additions should be absent, which dates the asset precisely.
```

### Proposed solution

- Do the cheap legal half immediately and the expensive half at the next cut: mark the existing Release as superseded in its release notes ('pre-relicence build; licence terms changed in S18 - do not redistribute; superseded by <next tag>'), or delete the asset. A note costs one edit and removes the ambiguity now.
- Land the U-09 re-cut from an exact tagged SHA, then add a release-asset gate: an asset is publishable only if (a) its commit == the tagged SHA, (b) its manifest lists the licence files present in the tree at that SHA (the S19 CI manifest assertion already checks the file set, so extend it to match the SHA's digest set), and (c) the release notes name the licence.
- Make the gate run on tag creation, not only on push - the current gates are push-shaped, so the release page is the one surface that can drift unnoticed.

### Acceptance / regression cases

- The Release page states the licence of its own build, or the asset is gone.
- A tag-triggered job fails when the asset manifest digests do not match the tagged tree.

**How to falsify this finding (do this first):** If the banked release was re-cut after 2026-09-14 (or the asset was withdrawn), the licence half is already closed - check the Releases page date against PR #23's merge date.

---

## 16. GN-16 - DONE conflates 'offscreen harness green' with 'shipped behaviour proven', and the register's own W-19 admits the harness cannot reach the behaviours in question

| field | value |
|---|---|
| Severity | P1 - P1 - wrong behaviour or blocked release |
| Class | unverified / false-confidence |
| Area | qt, process, docs |
| Evidence level | from repo-quoted fragments / register text (C++ angle brackets were lost in transit) |
| Relationship to COMPILED_AUDIT v3 | RESHAPES the vocabulary COMPILED_AUDIT §19 already demands ('every FIXED row is a documentation claim until re-proven'), applied to the state machine rather than to individual rows. |
| Effort | M - emitter + gate work, no code changes; but it must not become a new doc project (see the plan's 'freeze the doc machine' item). |

### Evidence

- STATUS vocabulary: 'DONE | closed, with executed proof named in the row | Proof column names a command, a test id or a diff.'
- Many GUI rows name the offscreen harness specifically (U-45 'T18', U-47 'T20', U-34 'T20', W-20 'harness T1', W-25 'harness T15') and are marked DONE.
- STATUS W-19 (OPEN), verbatim: 'needs a physical desktop: the offscreen harness cannot kill the engine mid-run, physically drop a file, or show the engine-missing dialog'.
- U-59 (P0-7, OPEN) is precisely the cancel/kill-mid-write case; U-13 (drop filter, DONE) is proven at the filter level, while the physical drag-and-drop behaviour W-19 names as unreachable by the harness is exactly the layer the filter sits in.
- R-01 records the toolchain reality (Qt6/cmake absent in most sandboxes), so most of these rows were also CI-compiled-only when closed.

### Why it matters

The state machine has one state where two kinds of evidence are indistinguishable. That is a correctness risk for the release gate ('no open P0/P1' reads as 'nothing known is broken'), and it is the reason U-59 could sit OPEN while rows next to it claim the same code path is closed. It also makes honest work look dishonest: a session closing a GUI row with T-cases is not lying, but the register cannot show what was actually exercised.

### Reproduction

```bash
# Pick three DONE GUI rows whose proof names only T-cases and whose behaviour is in W-19's list (drop, cancel, engine-missing).
# Try to answer 'has this behaviour been observed once on a real desktop?' from the register: the answer is not representable.
# grep -n 'offscreen' STATUS.md - the vocabulary has no term for it even though every GUI proof uses it.
```

### Proposed solution

- Split the proof vocabulary with two markers rather than two states: keep DONE, and require the Proof cell to start with either 'harness:' (offscreen/CI-compiled) or 'desktop:' (real-machine video/log, the B5/B6/B14 probe). Rows whose behaviour only exists in the second world cannot claim more than 'harness:'.
- Add one derived header number: 'rows proven only offscreen: N' next to the state counts - that is the honest size of the 1.0.0 UI debt, and it makes W-19 measurable instead of a note.
- Gate the 1.0.0 claim on it: release criteria must state '0 rows proven only offscreen among release-blocking rows', which finally gives W-19/W-18 a machine-checkable consequence.

### Acceptance / regression cases

- A mutation test: flipping a row's proof marker from desktop: to harness: changes the derived header number (proves the counter reads the cell, not a hardcoded list).
- The 1.0.0 criteria line in PROJECT_VISION/RELEASE_PROCEDURE references the counter.

**How to falsify this finding (do this first):** Read three of those rows in full - if their Proof cells already carry a real-desktop artefact, only the vocabulary half stands.

---

## 17. GN-17 - Doc-gate arithmetic is itself a maintenance surface: a 150-char truncation over a prose field needed a whole session (N-06), and stale counts cost another (N-01)

| field | value |
|---|---|
| Severity | P2 - P2 - hardening |
| Class | process / machine |
| Area | docs, process |
| Evidence level | read in the real source file this session |
| Relationship to COMPILED_AUDIT v3 | NEW as a cost claim. The audit records N-01 and N-06 individually and treats N-06 as fixed; nothing registers that both were caused by the gate's own design, not by repo state. |
| Effort | S - a deletion, a gate rule, and one memo. |

### Evidence

- STATUS N-01: the pending-workflow marker outlived its change, so 'eight doc locations quoting the wrong gate count' - a full session of correction.
- STATUS N-06: 'check_docs.sh emitter truncation was awk-locale-dependent: gawk counts CHARACTERS in length()/substr() under a UTF-8 locale, mawk counts BYTES ... so the committed STATUS.md passed G0 under gawk (S11 sandbox + CI) but FAILED under mawk (every prior sandbox).'
- The fix was 'export LC_ALL=C + reword the S11 §5 notes ASCII-only under the 150 limit' - i.e. prose was edited to suit the checker.
- The five root status docs total ~524 KB (COMPILED_AUDIT 214 KB, IMPROVEMENT_LOG 160 KB, SESSION_HANDOFF 66 KB, STATUS 42 KB, WORKLIST 41 KB), before docs/archive (~128 KB) and docs/audit (~71 KB); the CI workflow that enforces them is 21 KB.

### Why it matters

Effort is currently being spent making prose satisfy a truncation rule, and on repairing counts written by hand into prose. Both are self-inflicted and both scale with the doc machine. The fix is not 'more gates' - it is removing the two mechanisms: stop putting derived numbers in prose, and stop truncating a field the register does not need truncated.

### Reproduction

```bash
# grep -n '150' working_code/gifscythe/scripts/check_docs.sh  # the truncation that caused N-06
# grep -rn 'PASS / 0 FAIL\|30/0/3\|28 PASS' --include='*.md' . | wc -l  # hand-written counts in prose that N-01 broke
# Locale experiment: run check_docs.sh --emit under LC_ALL=C vs a UTF-8 locale and diff - the class is only de-fanged by LC_ALL=C, never removed.
```

### Proposed solution

- Delete the 150-char truncation: emit the full Proof cell and let the table wrap. Truncation exists to keep a table pretty; it cost a session and buys nothing machine-readable.
- Ban derived numbers from prose: gate counts and check counts are printed by verify_audit.sh --json and referenced by digest/artefact link, never typed into a sentence. Add a stale-sweep rule (S1-S5 style) that flags any hand-typed 'N PASS / M FAIL' string.
- Freeze the doc machine while the P0/P1 backlog is non-empty (see the plan view): no new gates, no new registers, no new audit merges until P0-7 and P1-38 are closed. The next audit intake becomes a GitHub Issue batch, not a new register part.

### Acceptance / regression cases

- check_docs.sh --emit is byte-identical under LC_ALL=C and UTF-8 with no ASCII-only rewording needed.
- The stale sweep fails a commit that hand-types a gate count.

**How to falsify this finding (do this first):** If the truncation was already removed after S11, only the 'derived numbers in prose' half stands.

---

## 18. GN-18 - Probe (not yet a claim): web stemOf and Qt completeBaseName may disagree on dotfile / extensionless names

| field | value |
|---|---|
| Severity | P3 - P3 - polish |
| Class | unverified / false-confidence |
| Area | web, core |
| Evidence level | probe required before acting - labelled honestly |
| Relationship to COMPILED_AUDIT v3 | NEW as a probe. Web/desktop naming parity is proven for ordinary names (U-01 planning, U-43 summary, W-23 templates) but the edge names are not pinned anywhere the register names. |
| Effort | XS - one helper, one fixture table. |

### Evidence

- web/server.mjs: "const stemOf = (name) => { const i = String(name).lastIndexOf('.'); return i > 0 ? String(name).slice(0, i) : String(name); };" with the comment 'QFileInfo::completeBaseName semantics (strip after the LAST dot), which is what the desktop naming uses for _opt.gif / _frame'.
- For a leading-dot name ('.gif') lastIndexOf('.') === 0, so i > 0 is false and the whole name is returned - the comment's claimed semantics are not obviously what the code does at that boundary; app.js has an identical copy of the function.
- Nothing in the suites appears to cover a dotfile, a name with no extension, or a name ending in a dot against the desktop's naming.

### Why it matters

It is one branch, and only reachable for odd input names - hence P3 - but upload names are user-controlled and the two implementations are hand-duplicated (the same drift class GN-13 describes). If they disagree, the web silently writes a different filename than the desktop would, which is exactly the parity claim the web exists to prove (GN-02's fixtures are the natural place to pin it).

### Reproduction

```bash
# node -e "const stemOf=(n)=>{const i=String(n).lastIndexOf('.');return i>0?String(n).slice(0,i):String(n)};console.log(JSON.stringify([stemOf('.gif'),stemOf('gif'),stemOf('a.b.gif'),stemOf('a.')]))"
# Compare with the desktop's naming for the same names (batch run with a name template)
# and with Qt: a 3-line qmake/Qt test printing QFileInfo('.gif').completeBaseName() and QFileInfo('gif').completeBaseName().
```

### Proposed solution

- Pin the rule from the desktop (Qt) first - it is the reference. Then either port the exact semantics into one shared JS helper (single copy, imported by app.js and server.mjs) or refuse the pathological names at upload (uploadNameError already exists and is the natural place).
- Add a fixture table of edge names to both suites so the two copies cannot diverge again.

### Acceptance / regression cases

- One shared stemOf in JS with a table of cases; the table includes '.gif', 'gif', 'a.b.gif', 'a.', 'A.GIF'.
- The desktop's own naming test prints the same table, and the two agree.

**How to falsify this finding (do this first):** Run the Qt one-liner: if completeBaseName('.gif') is also '.gif', the implementations agree and the comment alone is imprecise.

---

## 3. Negative space - read and found consistent (no finding)

- web/server.mjs static allow-list (/, /index.html, /style.css, /app.js, /command.mjs) plus assertContainedPath defence-in-depth, and the single sendStatic path that omits the body for HEAD while still pinning Content-Length - the U-67/NF-10 fix reads correct, including the deliberate decision not to route web/wasm.
- readBody + BodyTooLargeError + sendTooLarge: the 413 is typed, accumulation stops without destroying the socket, and the response is flushed before the request is destroyed (U-68/NF-11) - the logic matches its comment.
- handleOptimize removed the double decodeURIComponent (U-49) and percent-encodes X-Gifscythe-Command (U-50) - both fixes are present in the code as described, not just in the log.
- findEngine() structured resolution: an invalid non-empty GS_ENGINE fails closed with a named reason and never falls back, versions compare numerically (U-26), and the launch error path does not trigger a second discovery (GS-207) - the comment and the code agree.
- The engine is spawned with an argv array and no shell in all three clients (web spawn, C# ArgumentList, desktop ProcessRunner) - no shell-injection surface was found anywhere in the retrieved code.
- snapshotOutput/verifyOutput before-after verification on /optimize and /run, plus explode frame snapshot-diffing via hasGifMagic: the ordinary-output postcondition (GS-203/DONE half) is genuinely implemented for core/CLI/web, and the register is honest that the Qt integration is the missing part.
- Licence separation is structurally clean: GPLv2 engine as a separate subprocess, Ms-PL first-party UI/control layer, LGPLv3 Qt + companion GPLv3 text + generated QT_NOTICE.txt - no linking that would raise the in-process licence question (which is why OD-16 is about wasm, not about the desktop).
- The web UI revoke discipline (U-52/U-46) is complete in app.js: beforeUrl and every result URL are tracked and revoked, and requestGen is checked after the fetch, after the blob read, in the catch and in the finally.
- COMPILED_AUDIT v3 is internally finding-complete against its own claim (76 register rows, six audits, no row dropped between v2 and v3 per §18) - the merge-completeness checklist is a real artefact and is worth keeping even when the doc machine is frozen.

---

## 4. Method, limits and how to reproduce this review

- Retrieval limit (important): this review read the repository through the network fetch path, which strips anything that looks like an HTML tag. In C++ files that destroys everything inside angle brackets (std::vector<...>, #include <string>), and in Markdown it destroys quoted C++ fragments. Therefore EVERY claim about src/core/*.h, src/core/*.cpp or src/qtui/*.cpp in this report is marked 'documented-fragment': it is inherited from the repository's own quoted lines and register text, not freshly re-read. No new C++ bug is asserted anywhere in this report.
- Files read end-to-end this session: web/server.mjs, web/app.js, csharp/spike/Program.cs, docs/planning/CSHARP_SHELL_PLAN.md (partial, first ~2 sections), STATUS.md (partial - the fetch truncated inside the W/D/R rows), COMPILED_AUDIT.md v3 (partial - header, §1, §2D, §16-§19), README.md, the repository tree listing with file sizes.
- Files not read: src/core/Validate.h, src/core/SettingsIO.h, src/core/GifsicleCommand.h, src/core/OutputPlan.h (only its comment block came through intact), src/qtui/*, web/command.mjs, web/validate.mjs, web/run-paths.mjs, web/output-verify.mjs, scripts/*.sh, .github/workflows/build.yml, the test suites. Findings that depend on these are labelled needs-probe with the exact command to settle them.
- Nothing in this report was executed. Every 'expected' outcome is a source-read expectation, and every finding carries a falsify line that is deliberately mechanical (one grep, one command, one one-liner) so a false positive costs ten minutes, not a session.
- Register/doc-based findings (GN-04, GN-05, GN-07, GN-14, GN-15, GN-16, GN-17) can be voided by content I did not retrieve, because STATUS.md was truncated mid-file. Check the full file before acting on them.

### 4.1 Verification commands

```bash
# 0 - read-only triage: settle GN-03 (the P0 probe) in ~10 minutes
grep -n -A4 -B4 'crop'  working_code/gifscythe/src/core/Validate.h
grep -n -A4 -B4 'crop'  working_code/gifscythe/web/validate.mjs
printf 'resize_kind = fit\nresize_w = 0\nresize_h = 0\ninput = a.gif\n' > /tmp/resize0.conf
./working_code/gifscythe/build/gifscythe-cli /tmp/resize0.conf --run --strict ; echo rc=$?   # expect 3

# 1 - the web findings (GN-01, GN-10, GN-11) against a live server
cd working_code/gifscythe && node web/../server.mjs 2>/dev/null || (cd web && node server.mjs 8000 &)
printf 'PK\003\004notagif' > /tmp/notagif.bin
curl -sS -i -X POST --data-binary @/tmp/notagif.bin 'http://127.0.0.1:8000/optimize?settings=%7B%7D' | head -20   # GN-10
curl -sS -i -X POST --data-binary @some.gif 'http://127.0.0.1:8000/optimize?settings=%7B%22mode%22%3A%22merge%22%7D' | head -20   # GN-01
PORT=abc node web/server.mjs ; echo rc=$?   # GN-11 expect a named message, rc=2

# 2 - GN-02: empty number field -> 0, watch the live command pane or the /run payload
node -e "console.log(Number(''), Number(' '), Number('abc'))"   # 0 0 NaN - the mechanism

# 3 - the broad suite (quote RUNTIME counters, never CHECK( source sites - gate G9)
cd working_code/gifscythe && ./build.sh --all && ./scripts/test_engine.sh && ./scripts/smoke_cli.sh
QT_QPA_PLATFORM=offscreen ./build-cmake/test_gui_offscreen
node web/test/command.test.mjs && node web/test/validate.test.mjs && node web/test/transport.test.mjs \
  && node web/test/body-limit.test.mjs && node web/test/static-hygiene.test.mjs
./scripts/verify_audit.sh ; ./scripts/check_docs.sh

# 4 - release-asset check (GN-15)
gh release view snapshot-2026-09-07 --json tagName,createdAt,assets
```

### 4.2 How to register these findings without breaking the repo's own rules

1. Each finding that survives its own falsify check becomes a row in `COMPILED_AUDIT.md` §5 with the next free `U-nn` id - that file is the detail, and it is the only place state lives.
2. Run `working_code/gifscythe/scripts/check_docs.sh --emit` so `STATUS.md` is regenerated; never hand-edit the generated block (gate G0).
3. Record this document's path as the evidence source in each new row's Proof/Blocker cell, and name the session id.
4. Add the adjacency pairings this report introduces to §19.3: **U-22 <-> U-62** (geometry loosening must not loosen resize/scale) and **U-68/U-69 (shared P-id P2-16)** if GN-04 is accepted.
5. Anything found but not acted on goes in as OPEN (with its blocker) or UNTRIAGED for exactly one session, per the repo's own vocabulary.

---

## 5. Stack, language and shipping plan

### 5.1 Per-area verdicts

#### Engine (gifsicle 1.96, C)

- **Verdict:** Keep. Untouched subprocess, GPLv2, bundled as-is.
- **Why:** It is the only oracle in the project (GN-13) and the reason the licence separation is clean (GPLv2 subprocess + Ms-PL UI). Any in-process linking or rewritten optimizer would put both the licences and 19 sessions of proof at risk.
- **Do not:** Do not vendor, patch or 'modernise' the engine to remove a workaround - every existing workaround is documentation of real engine semantics (crop 0, loopcount, merge semantics).

#### Control layer (src/core, C++17 header-only, Qt-free)

- **Verdict:** Keep C++17. Do not rewrite, do not port to Rust for 1.0.0.
- **Why:** The bugs found in this code were contract bugs (argv cardinality, sentinel tri-states, path semantics, exit codes) - not language bugs. It already compiles on three toolchains, runs on Linux CI and Windows CI, and is provable without a GUI toolkit (the codebase's single best design decision). A rewrite would reset the proof clock on exactly the code class that took 19 sessions to get honest (U-07/N-04 path + Unicode + Windows argv).
- **Do not:** Do not add a second copy of it in another language unless it is generated from one contract (see the settings-contract recommendation below).

#### CLI (C++)

- **Verdict:** Keep, and promote it: it is the parity oracle and the only surface that can be claimed proven end-to-end today.
- **Why:** U-17/U-32/U-40/GS-201/GS-207 and the smoke suite 40/40 are all CLI proofs. The C# plan itself keeps the CLI as oracle. A CLI plus a config file is also the fastest thing a power user can put in a batch script, which is the actual job to be done for a GIF optimizer.
- **Do not:** Do not retire it at the WPF cutover, and do not let the shell become the only way to exercise the honesty rules.

#### Web (zero-dependency Node ESM + plain browser JS)

- **Verdict:** Keep as a shipped surface for 0.2.0; upgrade the type story only when it becomes the only UI.
- **Why:** It has executed proofs (transport 67/67, static hygiene 43/43, validate parity), it is the cheapest way to demo the product, and a local server + web UI is also the most portable 'no installer' story the project has. Its validation mirror (validate.mjs) is already cross-checked against the real C++ validate().
- **Do not:** Do not add TypeScript/a framework for its own sake (the repo's own planning says so, and this review agrees). DO add a typed contract if the web UI becomes the only UI - which is where the Number('')/sentinel class (GN-02) gets killed permanently.

#### Desktop UI - the real decision

- **Verdict:** Freeze the question for 0.2.0 (ship CLI+web), then take one dated decision with the register row to match (see the options table). Recommendation: WPF/C# per the already-answered OD-C series, with the Qt GUI's task list formally closed at that cutover - or, if the owner wants the smallest possible codebase, one web UI per shell - not both in parallel.
- **Why:** Every week the UI direction is ambiguous, a session is spent on prose instead of code (S18-S21), and the same defect class gets fixed twice, once per UI (the JS<->C++ parity rows: U-25, U-26, U-29, U-30, U-42, U-49, U-54, U-64, DS-13, GS-203's GUI half). The engine cannot tell who spawned it, so the UI choice only affects how much honesty code has to be re-ported and re-tested.
- **Do not:** Do not run two UI workstreams. Do not start the WPF port before the P0 rows are closed in the surface that already works.

### 5.2 Desktop UI options (the one decision with a schedule-sized consequence)

| option | what it is | for | against | verdict |
|---|---|---|---|---|
| A. Keep Qt6 as the desktop UI (status quo) | Finish the 42 OPEN rows against the Qt app; W-18/W-19 close the proof debt. | Nothing is thrown away: 324 harness checks, the XNConvert tab retrofit, the settings panel, packaging scripts (windeployqt), all already exist and are CI-compiled. | Only offscreen/CI evidence (R-01, W-19). Packaging is the heaviest part of the release (DLL sets, deployer, clean-VM proof). The repo's own plan calls it an infrastructure tax, and the owner has already forked a WPF codebase as the faster route. | Acceptable only if the owner commits to it and deletes the C# plan in the same commit. Otherwise it is the option that keeps the pivot alive forever. |
| B. WPF/C# shell, gifsicle stays the subprocess (OD-C series, parked) | Port the honesty layer (OutputPlan, OutputName, Validate, OutputVerify, ExplodeVerify, exit codes) to a Gifscythe.Core class library; WPF window on top; Qt archived at cutover; CLI stays as oracle. | Fastest path to a portable Windows exe (single-file publish, real dialogs, async/await, settings, packaging for free). The fork already has export-flow UI to adapt. The port target is a few hundred lines of pure, already-specified rules. | One more client of the argv contract to keep honest - so every future contract change must be ported and parity-tested three ways. The port must be test-first or it re-opens the silent-failure class the audit spent 19 sessions closing. MS-PL obligations on the fork must be respected in writing. | Recommended for the desktop, with two hard conditions: (1) the port is fixture-first against the CLI, and (2) the Qt task list is closed/archived in the same commit as the cutover so two UI lists never coexist again. |
| C. One UI: web UI inside a native shell (WPF WebView2 / Tauri / Electron) | Ship web/ as the only UI and wrap it. Kills the multi-UI parity class permanently. | One settings model, one command builder, one validation mirror. The web UI already has all four modes, the live command pane, before/after preview and the transport tests. | WebView2 Evergreen requires a runtime on the machine; the fixed-version runtime is ~130 MB - both collide with the project's hard constraints ('portable, no-installer, no-admin'). Electron is worse; Tauri adds a Rust toolchain and re-opens the path/argv proof class (D-08's trigger-based spike). | Reject for 1.0.0 on the hard constraint. Log it as the leading post-1.0.0 option if the portable promise is ever renegotiated. |
| D. Rust everything (engine client + Tauri UI) | Full rewrite of the control layer and the UI. | Memory safety, single toolchain, small binaries. | Resets the entire proof record for zero product value; the risky logic is OS/path/argv/exit-code semantics that are already written and tested in C++ and mirrored in JS. D-08 already scopes it as trigger-based only. | Reject. Keep D-08 as-is (trigger based), do not spend a session on it before 1.0.0. |

### 5.3 Concrete drift-killer: generated settings contract

**Why:** The recurring findings are all 'the same rule, written three times, one copy wrong': sentinel/tri-state values (DS-06/07/09, GN-02), exit codes (GN-08), naming rules (U-21, GN-18), engine discovery (U-65/U-66), and the argv builder itself (GN-13). Fixtures catch drift after the fact; generation prevents it.

- docs/contract/settings.json (or .yaml) - one entry per setting: name, engine flag shape, type, domain, sentinel for 'unset', and the UI control kind.
- scripts/gen_contract.mjs -> src/core/SettingsFields.generated.h (names, flag templates, domains) + web/contract.generated.mjs (same data as JS) + csharp/Core/SettingsFields.generated.cs (when Phase 2 resumes).
- scripts/gen_contract.mjs --check is a gate: generated files must match the table, so a hand edit fails the build instead of drifting silently.
- validate.mjs keeps its own logic but reads field names/domains from the generated module, so U-30's parity-by-duplication becomes parity-by-construction.
- A short fixtures table (settings JSON -> expected argv) is authored once per setting and loaded by the C++ unit suite, the JS suites and the future C# suite. One row per finding; three consumers.

**Payoff:** This is the smallest change that retires the largest class in the register. It also makes the desktop decision cheap: whichever UI ships, it consumes the same generated table instead of a fourth mirror.

### 5.4 Phased plan

#### Phase A - 0.2.0 - a truthful checkpoint on the surfaces that are already proven (CLI + web)

**Goal:** Ship a version whose every claim is provable today: no UI work, no new surfaces, no new docs machinery. Target: the next 1-2 sessions. Everything here is small and test-first.

**Exit criteria:**

- No open P0 rows.
- CLI smoke + unit + web suites green, and the new oracle gate green or explicitly red-listed.
- One tagged 0.2.0 artefact cut from the tagged SHA, with its licence files matching the tree.
- STATUS.md counts still generated (not hand-edited) and the new rows recorded.

| ticket | title | why | proof | effort |
|---|---|---|---|---|
| A1 | P0-7 / U-59 - atomic output: engine writes to a temp file, then rename over the target | The only remaining registered data-loss vector: gifsicle writes straight to -o, so a cancel/failure leaves a truncated file where a good result used to be. OutputPlan.h explicitly decided against temp+rename; that rationale is now superseded by NF-02 and must be rewritten or the next session will re-adopt it. | Stub engine that opens -o, truncates, sleeps; cancel mid-run; assert the previous output's bytes are untouched (the exact acceptance case NF-02 specifies). | M - one core helper (temp + fsync + rename) + CLI/GUI/web call sites + the stale comment deleted. |
| A2 | P1-38 / U-58 - snapshot the settings with the plan at run start | Batch targets are planned once (U-01) but settings are read live per file, so editing a control mid-run makes run #2 use different argv than run #1 while the summary describes one job. | Queue 3 GIFs in Batch, change Optimize mid-run, assert argv(run 2) == argv(run 1) and all three outputs share one settings set. | S - ~30 lines plus one harness case (the audit already scoped it). |
| A3 | Web honesty batch: GN-01 (mode guard on /optimize), GN-02 (empty != 0), GN-10 (admission + strict base64), GN-11 (port validation) | Four small refusals that convert silent/wrong results into named errors on the surface that is reachable over the network. No new architecture - one shared admission helper, one numeric helper, one port check. | Four transport/validate cases: mode=merge on /optimize -> 400; cleared geometry field -> no flag; non-GIF upload -> 400 with a signature message; PORT=abc -> exit 2 with a message. | S/M - one session including the tests. |
| A4 | GN-13 - oracle fuzz gate: run the product's own argv space against the bundled gifsicle | The highest-leverage item in this report. It is the only mechanism that can catch a bug both mirrors share (the U-62/U-63 class) without a human reviewer reading the gifsicle man page. | scripts/oracle_fuzz.mjs --quick green in the pre-push hook; --full in CI; the generated matrix committed so drift shows as a diff. | M/L - one session for the bounded version; extend later. |
| A5 | GN-14 - move U-55/U-56/U-71 from 'needs a clean VM' to Win32 rule cases in the header-only suite, run in the existing windows CI job | Three blocked rows become closable at test-writing cost, using the parameterised-rules trick the repo already uses for OutputName (U-21/W-23). | Windows CI log with the three new cases; Linux run of the same rules stays green. | S/M - tests only. |
| A6 | GN-15 + U-09 - asset legality: mark the banked pre-relicence build superseded, then re-cut from a tagged SHA | The published ZIP is from before the Ms-PL relicence and 5+ commits behind its own claimed SHA. One release-notes edit fixes the ambiguity today; the re-cut fixes the evidence. | Release page states the licence and SHA of its own build; a tag-triggered job fails on a manifest/tree mismatch. | S for the note; M for the re-cut and the tag-triggered gate. |
| A7 | GN-04 + GN-05 + GN-17 - make the register answer the release question, then freeze it | Add derived fix-order (P-id) state and a 'proven only offscreen' counter, drop the 150-char truncation, ban hand-typed counts; owner applies the pending workflow change so CI enforces the gate instead of the local hook. After that: no new gates until 1.0.0. | Header shows derived P0/P1 numbers; mutation test flips them; check_docs.sh --emit is locale-independent without ASCII rewrites; CI runs the gate on a doc-only stale commit and fails. | M - one session of emitter work, mostly mechanical. |

#### Phase B - 0.3.0 - contract consolidation (one settings model, one naming rule, one exit-code space)

**Goal:** Remove the drift class instead of chasing it. This is where a generated contract pays for itself and where the desktop decision is executed or killed.

**Exit criteria:**

- The three mirrors (C++ settings, JS settings, web/CLI argv) are pinned by a generated or fixture-driven contract.
- Discovery is one policy across CLI/GUI/web (U-65/U-66 closed).
- The UI decision row is closed: either the WPF port has started (fixture-first) or the Qt path is the single committed one.

| ticket | title | why | proof | effort |
|---|---|---|---|---|
| B1 | Contract file: settings schema + exit codes generated into C++/JS/C# from one source | Kills the sentinel/tri-state class (DS-06/07/09, GN-02, GN-08) at the root: one table, generated consumers, a gate that diffs generated files. | Generated files match the table; the parity gate compares argv for a fixture set across CLI and web. | M - one session. |
| B2 | Engine discovery: one policy (U-65/U-66), version-independent and symlink-safe | A VERSION bump currently breaks CLI/GUI while the web keeps working; a symlinked CLI loses the engine-beside-executable rule. | Smoke case: invoke via symlink and via bare PATH name with the engine beside the real executable; bump VERSION.md and assert all three clients still find it. | S/M. |
| B3 | Numeric/domain tri-state completion (P0-2/DS-06/07/09 + GS-206) | 'Unchanged' must be representable per control in every UI, and out-of-range values must be refused at the boundary with std::from_chars-style parsing in the destination width. | Unit test 28 re-pinned deliberately (`<0` no flag, `0` bare -j, `>0` -jN) plus a GUI spinner that can express Unchanged. | M. |
| B4 | Web transport bounds: concurrency cap, per-client rate limit, engine-run budget (U-06 PARTIAL) | The row has been PARTIAL since S8; the loopback default is honest but the caps named in the finding are still missing. | A test that a second concurrent expensive run is refused (429/503) with a named reason, and the cap is documented in web/README.md. | S/M. |
| B5 | Desktop decision execution (see the options table): one commit, one direction | The UI ambiguity is the largest single schedule risk remaining. Whichever way it goes, it must also delete the losing task list so the register stops carrying two plans. | If WPF: Phase-2 fixtures green against the CLI before any window exists. If Qt: the C# plan is marked retired and the W-19 probes are scheduled on a real machine. | Owner decision + 1 session of alignment, then the port/close-out. |

#### Phase C - 1.0.0 - the release bar, on evidence a stranger can check

**Goal:** Convert an internally-proven product into an externally-verifiable one. The gate is not 'the suites are green' - it is 'a clean machine and a real desktop agree'.

**Exit criteria:**

- Zero open P0/P1; zero release-blocking rows proven only offscreen (the GN-16 counter).
- W-18 clean-Windows smoke and W-19 real-desktop probes executed on the published artefact, with recorded evidence.
- One artefact, one SHA, one licence statement, one command line in the release notes that reproduces the build.
- The register's derived numbers - not prose - state the above.

| ticket | title | why | proof | effort |
|---|---|---|---|---|
| C1 | Proof-vocabulary split + counter (GN-16), then close W-19/W-18 against the real artefact | 'DONE' currently cannot distinguish an offscreen harness pass from an observed desktop behaviour; W-19 names three behaviours the harness cannot reach, and U-59 lived in exactly that blind spot. | Header counter moves when a proof marker changes; the three desktop probes recorded against the published zip. | S (vocabulary) + 1 session on a real machine. |
| C2 | GS-203 GUI integration of the ordinary-output verifier (the PARTIAL half) | Core/CLI/web verify the ordinary-run postcondition; the Qt run lifecycle does not - so the surface with the most users is the one with the weakest check. | Failing-first cases for stale/bogus/cancel paths, then green. | M. |
| C3 | U-12 / P1-24 async state machine (the five UI-thread waits) | Deliberately unscoped since S11 because the freeze cannot be tested offscreen. Once C1 gives you a real-desktop test path, it is testable - so it belongs here, not earlier. | A desktop probe that the UI stays responsive (no >100 ms stall) across run start/cancel/engine-missing. | M/L - unchanged from the audit's own scoping. |
| C4 | Release re-cut + publish + post-publish verification as one scripted path | RELEASE_PROCEDURE.md exists; what is missing is a scripted, tag-triggered path so the release page cannot describe a different tree than it ships. | Tag -> build -> manifest assert -> publish -> download -> smoke, all in one job, recorded in the release notes. | M. |

### 5.5 After 1.0.0 (so the post-1.0 backlog stops living in conversation)

- APNG + WebP through the frame model (D-01..D-04 in the register) - the audit's hard constraint is that no APNG/WebP work starts before the GIF UI is stable, so this is genuinely a 2.x bucket.
- Frame editor, overlays, presets (D-05) and the recorder-class features the owner has repeatedly scoped out (OD-C3) - keep them out unless the vision doc is amended in writing.
- Stills -> animated and video-as-endpoint (GN-07): register them now as gated rows; they need a decoder story (gifsicle reads GIF only) and a licence note for any FFmpeg sidecar.
- web/wasm (D-07): keep unshipped until the OD-16 in-process licence question is answered, a real emcc byte proof exists, and the GN-13 oracle gate can be run against the wasm binary too.
- Tauri/Rust (D-08) only against a written trigger (measured bundle size or a web-UI requirement that the current shell cannot meet) - never as a rewrite impulse.
- Issue-tracker migration: once 1.0.0 ships, move the remaining OPEN rows into GitHub Issues (labels = severity, milestones = 2.x buckets) and keep STATUS.md as a one-page release ledger + CHANGELOG. The doc machine has already cost more than the code it describes.

### 5.6 Stop doing (this list is as valuable as the ticket list)

- No new doc gates, registers, or audit-compilation merges until the P0/P1 backlog is empty (GN-05/GN-17). Next intake = GitHub Issues.
- No new surfaces. Freeze web/wasm and csharp/spike; do not scaffold a fourth client (GN-20 class: every surface multiplies the same honesty fixes).
- No hand-typed derived numbers anywhere in prose: gate counts, check counts and file counts are emitted or linked, never written (N-01, N-06).
- No closing a row with narrative: failing test first, then green, then the command + exit code in the proof cell (the repo's own §19 discipline - keep it, it is the best process the repo has).
- No 'PARTIAL' state without naming the missing piece (already the rule - just enforce it at emit time, not by review).
- No release asset that does not state its own SHA and licence (GN-15).
- No mid-session scope pivot: if a session discovers new scope, it writes a row and leaves the code alone. Three of the last sessions' audit entries record exactly this failure mode (S15 triage, S18 pivot then S19 park, S20 toolchain reality).

### 5.7 Risks with signals

| id | risk | early signal | mitigation |
|---|---|---|---|
| R1 | Documentation yield exceeds code yield: 19 sessions, ~524 KB of status prose, 42 OPEN rows, 2 red mains from drift. | A session whose diff is >70% .md, or a session whose only executed proof is a doc gate. | Phase A's A7 freeze + a per-session rule: every session ships one code change with one executed proof, or it is not a session. |
| R2 | The UI pivot flip-flops again (Qt -> C# park -> resume), keeping two task lists and two honesty ports alive. | Two UI task lists with OPEN rows at the same time; a planning doc newer than the code that contradicts README. | One dated decision row (GN-06/GN-07); the losing list is archived in the same commit as the decision. |
| R3 | Windows proof debt accumulates behind a VM that the workflow cannot reach - and it is currently compounded by U-09. | Rows whose blocker text mentions Windows/clean machine while their behaviour is a pure function (GN-14). | Split native-Windows rows into (a) test-only -> windows-latest CI now; (b) genuinely physical -> scheduled desktop probe with a named date. |
| R4 | Hand-written mirrors keep re-learning engine semantics (U-62, U-63, N-05 were all engine-semantics discoveries). | A fix whose description begins 'the engine actually allows...'. | GN-13 oracle gate: any such discovery must be accompanied by a new matrix row, so the next one is found by CI. |
| R5 | The published artefact drifts from the tree in licence and evidence (pre-relicence ZIP, 5 commits behind). | A release page older than the last licence/robustness change. | GN-15 note now, tag-triggered manifest gate at the next cut. |

---

## 6. Appendix - machine-readable findings (identical content to §2)

```json
{
  "repository": "freeforall1932-design/gifscythe",
  "treeSha": "794a9964550fde0b7006474170926a6382c835f3",
  "generated": "2026-09-16-1357",
  "counts": {
    "P0": 0,
    "P1": 12,
    "P2": 4,
    "P3": 2
  },
  "findings": [
    {
      "id": "GN-01",
      "title": "web /optimize ignores settings.mode: any mode is accepted, so a merge/explode can be served as a 200 \"optimized GIF\"",
      "severity": "P1",
      "category": "MISALIGNED",
      "area": [
        "web",
        "core"
      ],
      "confidence": "source-read",
      "auditRelation": "NEW. Not present in COMPILED_AUDIT v3. Adjacent to GS-201 (CLI Batch stop-loss, DONE S16) but that stop-loss lives in src/cli/main.cpp only - the /optimize endpoint has no equivalent guard.",
      "evidence": [
        "web/server.mjs header contract: \"POST /optimize?settings= ... (single-file Auto mode; kept for compatibility and the transport suite)\".",
        "web/server.mjs handleOptimize: settings are parsed, info:true is refused, validate() runs, the engine is located, then \"const s = { ...settings, inputs: [inFile], output: outFile }\" - settings.mode is passed straight through to buildArgs() with NO mode check.",
        "web/server.mjs handleRun, by contrast, does check: if (![\"auto\", \"batch\", \"merge\", \"explode\"].includes(mode)) sendJson 400, plus per-mode usage rules (auto = exactly 1 file, explode = exactly 1 file).",
        "So the same settings object is mode-validated on /run and mode-unvalidated on /optimize."
      ],
      "why": "The endpoint advertises Auto semantics but will happily emit -m (merge) or -e (explode) argv, return HTTP 200 image/gif and body.ok = true. A caller that asks for an optimization of an animation can receive a flattened single-frame merge (or a frame set) and be told it succeeded - the exact silent-wrong-result class this whole audit exists to kill. Because /optimize writes into a fresh mkdtemp dir it is not a data-loss bug, but it is a correctness-lie bug on a shipped surface, and web/README.md documents the endpoint's Auto-only contract.",
      "repro": [
        "Build the engine so discovery works: cd working_code/gifscythe && ./build.sh && ./scripts/build_engine.sh (per README quick start).",
        "node web/server.mjs 8000",
        "curl -sS -X POST --data-binary @some.gif -H 'Content-Type: image/gif' 'http://127.0.0.1:8000/optimize?settings=%7B%22mode%22%3A%22merge%22%7D' -o out.bin -w '%{http_code}\\n'",
        "Expected (contract): HTTP 400 \"mode is not supported by this endpoint\". Actual (source-read): HTTP 200, Content-Type image/gif, X-Gifscythe-Command carries a -m line.",
        "Cross-check the mirrored case on /run: the same mode is refused there, which proves the rule exists and was simply not applied to /optimize."
      ],
      "fix": [
        "Close the asymmetry by making the mode guard a single shared function used by BOTH endpoints, instead of an inline check in handleRun.",
        "Cheapest correct change: in handleOptimize, right after normalising mode, refuse anything that is not auto/\"\"/null with HTTP 400 and the same wording /run uses. Do not silently coerce to auto - coercion is how the wrong result gets a success code.",
        "Better: extract the whole per-mode admission block (mode normalisation + usage rules + info:true) from handleRun into one helper such as admitMode(mode, fileCount) returning { mode, error }, and call it from both handlers. That also keeps /optimize's compatibility promise honest if Auto-only is the intent.",
        "Add the rule to docs: web/README.md should state that /optimize is Auto-only and 400s on any other mode."
      ],
      "patch": "+// web/mode-admit.mjs (new) - one admission rule for both endpoints\n+export const MODES = [\"auto\", \"batch\", \"merge\", \"explode\"];\n+export function admitMode(rawMode, fileCount) {\n+  const mode =\n+    rawMode === undefined || rawMode === null || rawMode === \"\"\n+      ? \"auto\"\n+      : String(rawMode);\n+  if (!MODES.includes(mode))\n+    return { error: `unknown mode \"${mode}\" (auto|batch|merge|explode)` };\n+  if (mode === \"auto\" && fileCount !== 1) return { error: \"Auto processes exactly one file\" };\n+  if (mode === \"explode\" && fileCount !== 1) return { error: \"Explode processes exactly one file per run\" };\n+  return { mode };\n+}\n+\n+// web/server.mjs handleOptimize - right after JSON.parse of settings\n+const admitted = admitMode(settings.mode, 1);\n+if (admitted.error) { sendJson(res, 400, { ok: false, error: admitted.error }); return; }\n+const mode = admitted.mode;   // \"auto\" - anything else already returned",
      "acceptance": [
        "web/test/transport.test.mjs: new case - POST /optimize with mode=merge -> 400, and the engine is never spawned (assert no X-Gifscythe-Command header and an empty temp dir).",
        "Same suite: mode=auto still 200 + image/gif (regression guard for the compatibility path).",
        "Add the mirrored negative for /run with mode=auto and 2 files -> 400 (already covered; keep it next to the new case so the shared rule is visibly shared)."
      ],
      "effort": "S - one helper, two call sites, three transport assertions.",
      "falsify": "grep -n 'mode' working_code/gifscythe/web/server.mjs | sed -n '1,40p' - if handleOptimize does contain a mode check I missed because of the transport mangling, this finding is void."
    },
    {
      "id": "GN-02",
      "title": "web app.js: an emptied number field becomes a real 0 setting, not \"unset\" (Number(\"\") === 0)",
      "severity": "P1",
      "category": "BROKEN",
      "area": [
        "web"
      ],
      "confidence": "source-read",
      "auditRelation": "NEW. The audit registers the same *class* twice in C++ (DS-06 threads sentinel, DS-07 GUI spinner cannot express unchanged) and GS-206 (narrowing without range checks), but no row covers the JS side, where the mechanism is different: an empty input string converts to 0 instead of to the sentinel.",
      "evidence": [
        "web/app.js settings(): scale_x: Number($(\"scalePctX\").value) / 100, and scale_y: Number($(\"scalePctY\").value) / 100 (added by U-42 / P1 web parity work).",
        "web/app.js settings(): resize_w: Number($(\"w\").value), resize_h: Number($(\"h\").value).",
        "web/app.js settings(): color_count: $(\"colorsOn\").checked ? Number($(\"colors\").value) : -1, and delay_cs: $(\"delayOn\").checked ? Number($(\"delay\").value) : -1.",
        "The desktop models \"unchanged\" with a sentinel (-1) for lossy/colors/delay/loopcount precisely because the field must be omittable; the web applies the sentinel to the checkbox case but not to the text case.",
        "U-62 / P1-40 (S22) has just closed by *allowing* 0x0 geometry in crop - so a 0 that used to be refused upstream is now a value the pipeline is more willing to pass through."
      ],
      "why": "Number(\"\") is 0, not NaN and not null, so clearing the Width box (or the Scale X/Y box) turns \"leave it unchanged\" into \"resize to 0\" / \"scale by 0\". Depending on validate(), that is either (a) a 422 the user cannot explain, or (b) an argv the engine receives (--resize-fit 0x0, --scale 0) and answers with a raw engine error under a UI that looks configured. Both outcomes are the \"label says one thing, argv says another\" family. It is the same defect DS-06 describes for threads, on the surface with no type system and no unit suite behind the conversion.",
      "repro": [
        "node web/server.mjs 8000, open http://127.0.0.1:8000 with a GIF queued.",
        "Set Resize kind = Fit, then clear the Width input (leave it empty) and watch the live command pane.",
        "Source-read expectation: the pane shows --resize-fit 0x<height> because Number(\"\") === 0. Confirm with the pane, then confirm the same in the POST /run JSON payload (DevTools Network).",
        "Repeat with Scale X cleared: scale_x becomes 0. Compare against the desktop, where an empty/None field omits the flag."
      ],
      "fix": [
        "Introduce one explicit conversion helper in web/command.mjs (shared by app.js and any future client) that distinguishes empty from zero, and use it for every numeric control:",
        "Then decide per field what \"empty\" means: for geometry (w/h/scale) empty must mean OMIT the flag (never 0); for colors/delay it already means -1; for loopcount N, empty is a user error and must be refused in the UI before submit.",
        "Mirror the rule in validate.mjs with a 'missing but required' issue kind so the server refuses an empty-as-zero payload rather than passing a 0 to the engine."
      ],
      "patch": "// web/command.mjs (shared)\nexport function numOrNull(v) {\n  const s = String(v ?? \"\").trim();\n  if (s === \"\") return null;                 // unset - NOT zero\n  const n = Number(s);\n  return Number.isFinite(n) ? n : null;      // \"abc\" is unset, not NaN\n}\n\n// web/app.js settings() - geometry and scale\nresize_w: numOrNull($(\"w\").value),           // null => buildArgs must omit --resize-fit / --resize\nresize_h: numOrNull($(\"h\").value),\nscale_x:  numOrNull($(\"scalePctX\").value) === null ? null : numOrNull($(\"scalePctX\").value) / 100,\nscale_y:  numOrNull($(\"scalePctY\").value) === null ? null : numOrNull($(\"scalePctY\").value) / 100,",
      "acceptance": [
        "web/test/validate.test.mjs + command.test.mjs: an argv fixture where resize_w/resize_h/scale_x/scale_y are null must contain no --resize*/--scale token at all (parity vs the C++ CLI, which already omits them for sentinel values).",
        "A fixture with resize_w = 0 keeps the current behaviour and is asserted explicitly, so the \"0 is legal for crop\" rule (U-62) stays pinned and is not confused with \"empty\"."
      ],
      "effort": "S/M - one helper + ~6 call sites + 2 fixtures; needs the C++ side confirmed for the null case.",
      "falsify": "Read web/command.mjs: if it already normalises \"\" to null/sentinel for these keys, the bug is neutralised one layer down and only the UI label is wrong. Check that file before writing any code."
    },
    {
      "id": "GN-03",
      "title": "Registered pairing gap: U-62 (crop 0x0 now allowed, S22) was never cross-checked against U-22 (resize geometry must be refused, S8)",
      "severity": "P1",
      "category": "VERIFY-GAP",
      "area": [
        "core",
        "web"
      ],
      "confidence": "needs-probe",
      "auditRelation": "NEW as a *pairing*. The audit's own §19.3 lists four fix-adjacency pairings to re-probe after every fix (U-01/U-55/U-59, U-33/U-53, U-46/U-54, U-03/DS-06, GS-203/U-57). U-22 <-> U-62 is missing from that list even though both edits touch the same geometry validation.",
      "evidence": [
        "STATUS.md U-22 (DONE, S8): \"Validate.h skips resize geometry, so a conf can reach the engine with --resize-fit 0x0\" -> proof \"resize/scale geometry validated (rules probed off the engine)\".",
        "STATUS.md U-62 (DONE, S22): \"Validate.h refuses crop W/H 0, engine allows 0=extend to edge\" -> proof \"crop 0x0 now passes native and web validation\".",
        "Both live in src/core/Validate.h + the byte-identical web/validate.mjs mirror, and P1-40 is described in U-63's row as \"Crop 0 and loop once - PARTIAL S22\", i.e. the geometry loosening shipped in the same session as the crop work.",
        "Nothing in the register or §19 asserts that the loosening was scoped to --crop; a field-agnostic 'allow 0' rule would re-open U-22 silently and would pass every existing test, because the old failing fixture was the only witness."
      ],
      "why": "This is exactly the 'closing a pit while digging a new one' failure mode the audit's §19 mandates be hunted. If the crop loosening is keyed on the crop field, nothing is wrong and this finding closes in one command. If it is not, --resize-fit 0x0 and --scale 0 are reachable again from both the CLI and the web - the C++ control layer regressing the S8 guarantee while every suite stays green. Severity is deliberately P1 and not P0: no data loss is reachable (the engine writes a new file or refuses), the harm is a silently re-opened S8 refusal guarantee plus a wasted session if it surfaces later as a mystery engine error.",
      "repro": [
        "grep -n -A4 -B4 'crop' working_code/gifscythe/src/core/Validate.h    # is the 0 allowance inside the crop branch only?",
        "grep -n -A4 -B4 'crop' working_code/gifscythe/web/validate.mjs       # is the mirror identical?",
        "printf 'resize_kind = fit\\nresize_w = 0\\nresize_h = 0\\ninput = a.gif\\n' > /tmp/resize0.conf",
        "./working_code/gifscythe/build/gifscythe-cli /tmp/resize0.conf --run --strict ; echo rc=$?",
        "Expectation after U-22: refused (rc=3, warned and refused by --strict). If it prints and runs, U-22 has been re-opened by U-62.",
        "Repeat the same payload as a schema.json/no-settings-body POST to /run on the web build and check for a 422."
      ],
      "fix": [
        "Do not guess: run the three commands above first and record the output in the U-62 row. This finding's disposition is one command away.",
        "If it reproduces: make the rule explicit instead of field-agnostic - a helper such as geometry_ok(kind, w, h) whose crop branch allows zero and whose resize/scale branches require >= 1, called from both Validate.h and validate.mjs.",
        "Whatever the outcome, add the pairing - 'U-22 <-> U-62: geometry loosening for crop must not loosen resize/scale' - to COMPILED_AUDIT.md §19.3 with the exact commands, so the next session inherits the probe rather than this note.",
        "Add a JS fixture to web/test/validate.test.mjs for resize 0x0 (refuse) next to the crop 0x0 fixture (allow) so the two rules can never be edited as one."
      ],
      "acceptance": [
        "A red-then-green test pair in the same file: crop 0x0 -> accepted, resize 0x0 -> refused, scale 0 -> refused.",
        "Unit suite count in the S23 log quotes a *runtime* counter (gate G9 rule) for the new cases."
      ],
      "effort": "S - probe 10 minutes; the fix, if needed, is one predicate in two files.",
      "falsify": "The probe itself falsifies it: if --resize-fit 0x0 is refused (rc=3 under --strict, 422 on the web), the pairing is clean and the finding collapses to 'add the pairing to §19.3'."
    },
    {
      "id": "GN-04",
      "title": "Fix-order ids (P0-n..P3-n) have no state, so a partially closed fix-order item is invisible to the register counts",
      "severity": "P1",
      "category": "PROCESS",
      "area": [
        "docs",
        "process"
      ],
      "confidence": "source-read",
      "auditRelation": "NEW. COMPILED_AUDIT §6 is the fix order and each register row names its P-id, but no artefact states whether a P-id is open/partial/done, and the header counts only row states.",
      "evidence": [
        "STATUS.md header: \"Counts (generated - do not edit by hand): 96 DONE - 8 PARTIAL - 42 OPEN - 0 UNTRIAGED - 146 total\".",
        "STATUS.md U-63 row: state OPEN, proof \"not started\", next action \"P1-40: Crop 0 and loop once - PARTIAL S22.\" - the row's own next action contains a PARTIAL claim about its fix-order id.",
        "Fix-order ids are many-to-many with rows: P1-40 covers U-62 (DONE) + U-63 (OPEN); P1-41 covers U-65 (OPEN) + U-66 (OPEN); P2-16 covers U-68 (PARTIAL) + U-69 (OPEN).",
        "Therefore 'what is left in P0?' / 'is P1-40 finished?' cannot be answered from the register without reading several rows by hand - the exact question the register exists to answer in one line."
      ],
      "why": "The register's stated purpose is to answer \"how much is done?\" in one line, and the release gate is 'no open P0/P1'. With P-ids state-less, a session can close one of three member rows of a P-id and the release picture still reads the same; the owner has to reconstruct completion by hand. It also means the 42 OPEN count can move while the fix-order backlog does not, or vice versa - the numbers that a release decision reads are not the numbers the work is planned in.",
      "repro": [
        "Open STATUS.md and search for \"P1-40\", \"P1-41\", \"P2-16\". Note that each appears in the Next action cell of more than one row.",
        "Try to answer 'is P2-16 done?': U-68 is PARTIAL, U-69 is OPEN - the P-id has no state of its own.",
        "Try to answer 'how many P0 items remain?' from the header counts alone: impossible, because P0-7 spans U-59 and P0-4 spans U-09, neither of which is labelled P0 in the row."
      ],
      "fix": [
        "Extend the emitter (scripts/check_docs.sh --emit), which already parses §5 top-to-bottom, with a third register part: one row per fix-order id, columns P-id / members / derived state / next member action. Derived state = AND of members: any OPEN member -> OPEN, else any PARTIAL -> PARTIAL, else DONE.",
        "Add the derived P0/P1 counts to the generated header next to the row counts, so the release bar ('no open P0/P1') is literally a number in the status header.",
        "Do NOT hand-maintain it - the whole value is that P-id state cannot drift from its members. This also gives the gate something to check (G5-style): a P-id whose derived state disagrees with the emitted block is a hard failure."
      ],
      "patch": "# generated header, after the row counts\n**Fix-order state (derived from the rows above):** P0 4/6 closed - P1 19/31 closed - P2 8/16 closed - P3 2/9 closed\n**Release bar:** no open P0/P1 -> 3 P0 and 12 P1 still open",
      "acceptance": [
        "check_docs.sh plain mode diffs the new P-section like the rest, and a mutated member row fails the gate (mutation-test it the way S8 did).",
        "The next session's handoff quotes the derived P0/P1 numbers, not just the four row states."
      ],
      "effort": "M - emitter change + one gate + 3 mutation tests; ~1 session, mostly awk/bash.",
      "falsify": "If STATUS.md already contains a per-P-id table further down (the retrieved copy was truncated mid-file), this finding is void - check the whole file first."
    },
    {
      "id": "GN-05",
      "title": "The single-status-register guarantee is unenforced where it matters: not in CI (blocked token), and inert in a fresh clone",
      "severity": "P1",
      "category": "PROCESS",
      "area": [
        "docs",
        "process"
      ],
      "confidence": "source-read",
      "auditRelation": "NEW as a chain. The audit holds all three links separately (W-30 OPEN, R-03 OPEN, R-04 PARTIAL, U-14 PARTIAL, GS-208 OPEN) but never states the combined consequence: on GitHub, nothing enforces the register.",
      "evidence": [
        "STATUS.md W-30 (OPEN): \"blocked: the CI token has no workflows scope ... The step lives in docs/ci/build.yml.proposed\".",
        "STATUS.md R-03 (OPEN): push rejected - the workflows directory cannot be updated without a workflows-scoped token; \"drift is tolerated only via docs/ci/PENDING_WORKFLOW_CHANGE.md\".",
        "STATUS.md R-04 (PARTIAL): \"Still missing: a fresh clone is unprotected until it runs build.sh once, and nothing forces that\".",
        "COMPILED_AUDIT §1/§16 plus STATUS rows record that main went red twice from documentation drift alone (GS-208, N-01), and gate E9/G7 deliberately SKIP on the declared workflow drift.",
        "Net chain: CI cannot run check_docs.sh -> the pre-push hook is the only enforcement -> the hook is inert until build.sh bootstraps it -> a fresh clone or a web-UI commit is unprotected."
      ],
      "why": "The repo's most expensive asset is the claim that STATUS.md cannot drift from the audit. That claim is currently enforced by nothing on the platform that actually merges code. Every session then spends effort re-verifying prose (S9, S14, S15, S17 all contain documentation-machine work) instead of closing P0/P1 rows. Two full sessions of doc-drift repair are already on the record.",
      "repro": [
        "Fresh clone: git clone --depth 1 <repo> /tmp/gs && cd /tmp/gs && git config --get core.hooksPath   # empty -> the doc gate is inert",
        "Open .github/workflows/build.yml and grep for check_docs / check_docs.sh: this review did NOT read the workflow file, so treat 'no gate step in the live job' as the expectation implied by W-30/R-03, and let this grep settle it. The gate itself lives in docs/ci/build.yml.proposed, which is a hand-maintained copy nothing executes.",
        "Make any prose edit to COMPILED_AUDIT.md, commit, push to a branch: if the push succeeds and CI stays green, the gate is not wired into CI (the local pre-push hook exists only after build.sh has run)."
      ],
      "fix": [
        "Owner action (unblocks 3 rows at once): apply docs/ci/build.yml.proposed with a workflows-scoped token, delete docs/ci/PENDING_WORKFLOW_CHANGE.md in the same commit, and close W-30/R-03/GS-208 together. One commit, three rows, and E9/G7 stop SKIPping.",
        "Make the failure loud while waiting: docs/audit/REMEDIATION and the README header should carry one line - 'register enforcement: local hooks only (CI step pending)' - so no future session treats a green CI as proof the register is true.",
        "Make the gate self-evidencing rather than prose-counting: verify_audit.sh --json writes gate results to a file CI can publish as an artefact, and the register quotes a SHA-256 of that JSON instead of a hand-typed count. That removes the count-drift class (N-01) mechanically instead of by discipline.",
        "Make bootstrap non-optional: have scripts/check_docs.sh itself fail with a one-line instruction when core.hooksPath is not .githooks (G15 partially does this - extend it to the case where build.sh has never run)."
      ],
      "acceptance": [
        "A CI run on a doc-only commit fails when STATUS.md is stale (the negative case that proves the step is live).",
        "verify_audit.sh --json output is downloadable from the run and its digest appears in STATUS.md."
      ],
      "effort": "S for the owner commit; M for the --json + digest change.",
      "falsify": "If .github/workflows/build.yml does contain a documentation status gate step (the retrieved copy was truncated), then W-30's row is what is stale, not the gate - and the finding becomes 'W-30 is a doc-drift row'."
    },
    {
      "id": "GN-06",
      "title": "The public README still presents the Qt6 GUI as the product while the newest planning decision targets a C#/WPF shell (Qt archived at cutover)",
      "severity": "P1",
      "category": "DOC-DRIFT",
      "area": [
        "docs",
        "qt",
        "csharp"
      ],
      "confidence": "source-read",
      "auditRelation": "NEW. The audit records the C# plan (S18/S19 PARKED, OD-C7) and the C++/Qt direction, but no row covers the fact that the two coexist in the README's first screen.",
      "evidence": [
        "README quick start: \"./build.sh --all # also Qt6 GUI (fails honestly if Qt missing)\" + three Qt screenshots + \"Never call it 1.0.0 until the UI/UX task is done.\"",
        "README layout: \"csharp/  C# shell - PARKED S19 until 1.0.0 ships on C++/Qt6\".",
        "docs/planning/CSHARP_SHELL_PLAN.md: goal \"a portable, click-and-run Windows .exe built in C#/WPF with a custom Gifscythe UI/UX, driving the unchanged gifsicle engine subprocess ... shippable as 1.0.0\", pre-parked state GO on all five OD-C decisions (WPF, archive Qt at cutover), and the plan's own context: the owner forked ScreenToGif as \"a faster path to a Windows exe\".",
        "So the README's headline UI and the repo's chosen UI for 1.0.0 are different artefacts, with an unresolved pivot in between."
      ],
      "why": "This is the single decision with the largest schedule effect in the repo (it decides whether 324 offscreen harness checks and the Qt task list are an asset or a sunk cost), and the public entry point does not mention it. A contributor reading README first will invest in the Qt surface; a session reading the plan will invest in WPF. Flip-flopping planning is how S18-S21 spent time on docs instead of on U-58/U-59.",
      "repro": [
        "Read README.md top-to-bottom, then docs/planning/CSHARP_SHELL_PLAN.md, then docs/planning/OFFLINE_BUILD_REVIEW.md §4.",
        "Try to answer: 'which UI is the product's UI at 1.0.0?' from the README alone - you cannot.",
        "Try to answer it from STATUS.md alone - W-19/W-20/W-23 all describe Qt GUI work as the near-term plan, and no row says 'Qt is retired at cutover'."
      ],
      "fix": [
        "Add a 6-line 'UI direction' block at the top of README.md and at the top of STATUS.md: shipped UI today (Qt6 GUI, offscreen/CI-proven in-sandbox, W-19 probes outstanding) / decided direction for 1.0.0 (per OD-C7: parked; resume point = CSHARP_SHELL_PLAN) / what is frozen until that decision is re-taken.",
        "Register the pivot as a single owned row (e.g. D-09 'Desktop UI direction and cutover') instead of leaving it implicit across W-19/W-20/W-26 and a parked plan. One row, one owner, one date - that is the whole fix.",
        "Until the row exists, do not add Qt UI work to the near-term list; that is the honest reading of a parked pivot. If the owner wants Qt at 1.0.0, delete the parked-plan half of the README instead - but pick one in the same commit."
      ],
      "acceptance": [
        "A reader who opens only README.md and STATUS.md can state the UI direction for 1.0.0 without opening docs/planning/.",
        "The new D-09 row appears in the emitted register with a proof/blocker cell that names the date of the decision."
      ],
      "effort": "S - documentation only, but it must be one commit and it must be the owner's call.",
      "falsify": "If STATUS.md already contains an explicit UI-direction row (truncated in the retrieved copy), the finding is only about the README."
    },
    {
      "id": "GN-07",
      "title": "Scope beyond the frozen vision (stills -> animated, video endpoints, promoted APNG/WebP) exists only inside a parked plan",
      "severity": "P1",
      "category": "PROCESS",
      "area": [
        "docs",
        "process"
      ],
      "confidence": "source-read",
      "auditRelation": "NEW. PROJECT_VISION's hard constraints and the README roadmap do not contain these items, and the register has no row for the amendment; only the parked plan does.",
      "evidence": [
        "docs/planning/CSHARP_SHELL_PLAN.md §2.1: 'Still-image collections -> animated (ezgif-maker-class, owner request 2026-09-14) ... Video <-> animated-picture conversion ... Editing features (ezgif-class: crop/resize/rotate/reverse/text-style frame ops)'.",
        "Same section, verbatim: \"Mission-amendment note: PROJECT_VISION.md currently says 'Not photos, not video.' Items 3-4 narrow that to 'photos and video only as conversion endpoints/inputs, never as the subject.' The vision doc must be amended (with this rationale) before any stills-import or video-endpoint work starts - no code until the words change.\"",
        "README versions roadmap: 0.1.0 -> 1.0.0-1.9.9 (finished GIF product) -> 2.0.0-3.0.0 (WebP + APNG) - the stills/video/editing scope has no version.",
        "The plan is PARKED (OD-C7), so the amendment request is parked with it, in a file a future session is told to resume from."
      ],
      "why": "The park decision makes the leak *more* likely, not less: the plan is the designated resume point, so its scope list will be read as pre-approved scope. Item 3 (stills import) also drags in a decoder question that gifsicle cannot answer (gifsicle reads GIF inputs only) - i.e. a new FFmpeg sidecar with its own licence question (LGPL/GPL depending on build). That is a multi-session workstream hiding inside a one-line bullet.",
      "repro": [
        "grep -n 'amendment' docs/planning/CSHARP_SHELL_PLAN.md - note the plan's own precondition.",
        "grep -n -i 'photo\\|video\\|stills' PROJECT_VISION.md - note the words the plan says are still the constraint.",
        "grep -n -i 'stills\\|video endpoint' STATUS.md WORKLIST.md - no row owns it."
      ],
      "fix": [
        "Register the scope as rows now, unstarted and explicitly gated: D-09 stills->animated (needs decoder story + licence note), D-10 video conversion endpoints (needs FFmpeg licence + argv contract), D-11 vision amendment (blocks D-09/D-10). A row costs one line and prevents 'the plan said we could'.",
        "Add the vision amendment paragraph to PROJECT_VISION.md as a *proposal block* (clearly marked unapproved) so the constraint text and its proposed narrowing live in the same file and cannot drift.",
        "State the engine precondition in the row itself: stills/video require a decoder that is not gifsicle; do not let a session start item 3 with single-frame GIFs as an undocumented hack."
      ],
      "acceptance": [
        "Three new rows exist in the emitted register with state OPEN and a blocker naming the licence/decoder questions.",
        "PROJECT_VISION.md contains the proposed amendment and no code has landed against it."
      ],
      "effort": "S - registration only.",
      "falsify": "If the register already carries rows for the stills/video scope (truncated section), collapse this to the vision-amendment half."
    },
    {
      "id": "GN-08",
      "title": "Same exit-code numbers, different meanings: the C++ CLI and the C# spike both claim 0/2/3 - and there is no shared exit-code contract",
      "severity": "P1",
      "category": "MISALIGNED",
      "area": [
        "cli",
        "csharp",
        "core"
      ],
      "confidence": "source-read",
      "auditRelation": "NEW. Exit-code work exists (U-32 signalled child, U-40 --strict 3, GS-201 rc=2, GS-207 rc=1) but nothing compares the C++ code space with the C# spike's, even though the plan makes the CLI the C# shell's parity oracle.",
      "evidence": [
        "csharp/spike/Program.cs, verbatim: \"// Exit codes: 0 ok - 2 usage/caller error - 3 engine missing - 4 engine failed - 5 invalid output (rc=0 but bad/missing GIF).\"",
        "C++ CLI, per STATUS rows: U-40 '--strict refuses any warned conf with rc=3'; GS-201 'Batch + empty output ... rc=2'; GS-207 'CLI print/run exit 1' for an invalid GS_ENGINE override; U-32 'run_argv returns 128+WTERMSIG' (so 143/137 as well).",
        "So: CLI 3 = validation refusal vs C# 3 = engine missing. CLI 1 = resolution/engine error vs C# 1 = unassigned. C# 4/5 have no C++ counterpart at all.",
        "CSHARP_SHELL_PLAN.md relies on that contract: 'The C++ CLI stays as parity oracle' and a Phase gating that parity-tests the shell against the CLI."
      ],
      "why": "The parity oracle only works if a script can compare two exit codes and mean the same thing. Today, a wrapper that reports 3 cannot be interpreted without knowing which client produced it - so the planned parity test cannot be written honestly, and any user script (or the desktop's 'honest exit code' promise) inherits the ambiguity. It is a *cheap* thing to fix now and an expensive one to fix after the WPF port lands.",
      "repro": [
        "grep -rn 'return 3\\|rc=3\\|rc 3' working_code/gifscythe/src/cli/main.cpp docs csharp/spike/Program.cs",
        "Compare the two comment blocks side by side; the collision on 2 and 3 is visible in grep output alone.",
        "Attempt to write the Phase-2 parity assertion 'same argv -> same rc' and observe that you must special-case 1/3 to make it meaningful."
      ],
      "fix": [
        "Freeze one contract file, docs/contract/EXIT_CODES.md, as the single source of truth: 0 ok / 1 engine or resolution failure / 2 caller error (usage, unknown arg, refused run) / 3 refused by validation (--strict) / 4 engine ran, output invalid (rc=0 but no verified GIF) / 124 timeout / 127 engine not startable / 128+n signalled child.",
        "Emit it as code so it cannot drift: a tiny src/core/ExitCodes.h (enum class + to_string) and csharp/Core/ExitCodes.cs generated from the same markdown table by a 30-line script, wired into the existing doc gates as 'generated file matches source table'.",
        "Update the C# spike's comment and returns to the contract, then add the parity case to the Phase-2 gate: same argv, same rc, same meaning, asserted for at least 'engine missing' and 'output invalid'.",
        "Rename the numeric reasoning in the plan: the C# side must not invent 4/5 semantics - it should adopt them from the contract."
      ],
      "patch": "# docs/contract/EXIT_CODES.md  (excerpt - single source of truth)\n| code | meaning                        | who returns it                  |\n|------|--------------------------------|---------------------------------|\n| 0    | ok, output verified            | CLI / web / GUI / spike         |\n| 1    | engine or resolution failure   | CLI (GS_ENGINE override, spawn) |\n| 2    | caller error / refused run     | CLI (usage, Batch+no output)    |\n| 3    | refused by validation (strict) | CLI --strict                    |\n| 4    | engine ran, output invalid     | web/GUI/spike verify step       |\n| 124  | engine timeout                 | web runner, GUI, spike          |\n| 127  | engine not startable           | all clients                     |\n| 128+n| signalled child (POSIX)        | core run_argv                   |",
      "acceptance": [
        "The generated header/sharp file matches the table (gate asserts it).",
        "A smoke case asserts CLI '--strict warned conf' = 3 and 'engine missing' = 127, so 3 can never again mean 'engine missing'.",
        "The C# spike's README documents only codes from the table."
      ],
      "effort": "S - a table, two small files, one gate; the value is that it is cheap now and structural later.",
      "falsify": "If docs/contract/EXIT_CODES.md already exists, this finding is void - grep for it first."
    },
    {
      "id": "GN-09",
      "title": "C# spike: two port-time traps already visible - Stream.Read may under-fill the magic probe, and the shown command line is POSIX-quoted on a Windows product",
      "severity": "P2",
      "category": "BROKEN",
      "area": [
        "csharp",
        "qt"
      ],
      "confidence": "source-read",
      "auditRelation": "NEW. The spike is explicitly throwaway-allowed and inert, but the plan makes it the Phase-2 seed for the shell and the C++-CLI parity oracle, so its two wrong details are inherited forward.",
      "evidence": [
        "csharp/spike/Program.cs VerifyGif: \"using var stream = File.OpenRead(path); head = new byte[6]; if (stream.Read(head, 0, 6) != 6) return \\\"produced an empty/short file\\\";\" - Stream.Read is documented as returning *up to* count bytes; the correct call is ReadExactly (or a loop).",
        "csharp/spike/Program.cs Quote(): \"if (a.Length > 0 && a.All(c => char.IsLetterOrDigit(c) || \\\"/._-+=:@%,\\\".Contains(c))) return a; return \\\"'\\\" + a.Replace(\\\"'\\\", \\\"'\\\\''\\\") + \\\"'\\\";\" and it is printed as the 'command' line.",
        "The repo's own desktop convention is MSVCRT quoting (the audit notes a custom CreateProcessA + MSVCRT quoting was added after MinGW _spawnvp split on spaces) and the product promise is a live command pane that shows 'exactly what the engine runs'."
      ],
      "why": "Two small details with disproportionate consequence in Phase 2: (1) a partial header read makes a valid GIF fail verification with a *false* 'invalid output' - the opposite failure direction from the audit's usual silence, but still a lie about the engine; it is also a classic port bug that survives code review because it looks correct. (2) Copying a POSIX-quoted line as the pane text breaks the pane-as-truth contract the moment a path contains a space (single quotes are literal characters in cmd.exe), and the pane is explicitly a hard constraint in the C# plan §2.",
      "repro": [
        "dotnet run --project csharp/spike -- <engine> <input> <output> and read the printed command line; paste it into cmd.exe - the quoting is wrong on Windows.",
        "Grep the repo for the quoting rule the desktop uses (ProcessRunner / command pane) and diff it against Quote().",
        "For the read bug: it needs a stream that returns short reads (network share, or a wrapper); assert with a custom Stream in a 5-line test - FileStream on a local disk usually hides it, which is why it must be pinned by a test rather than by observation."
      ],
      "fix": [
        "Replace the manual read with a read-exactly helper and keep the six-byte probe: `using var fs = File.OpenRead(path); var head = new byte[6]; fs.ReadExactly(head);` (catch EndOfStreamException -> 'empty/short file').",
        "Move the display quote into a port of the existing quoting contract, e.g. csharp/Core/CommandLineQuote.cs implementing the same rules the C++ ProcessRunner prints (and the same rules cmd.exe/CommandLineToArgvW round-trip), and add a fixture: path with a space, path with an embedded double quote, CJK comment.",
        "Record both as Phase-2 acceptance criteria in CSHARP_SHELL_PLAN.md so the port cannot 'simplify' them away."
      ],
      "acceptance": [
        "A test with a short-reading stream yields 'verified' for a valid GIF (red before the fix).",
        "A quoting fixture round-trips: spawn the child with the printed string through cmd.exe and assert the child sees the original argv."
      ],
      "effort": "S - two small edits plus two tests; do them when Phase 2 actually resumes, but record them now.",
      "falsify": "Read the current Program.cs - if ReadExactly / an MSVCRT quoter is already there, the snippet I read is stale (the file may have changed after the snapshot)."
    },
    {
      "id": "GN-10",
      "title": "The non-GIF admission fix (GS-205) is scoped to the desktop, so both web endpoints still accept arbitrary bytes - and base64 decoding is forgiving",
      "severity": "P2",
      "category": "MISSING-LOGIC",
      "area": [
        "web",
        "core"
      ],
      "confidence": "source-read",
      "auditRelation": "EXTENDS GS-205 (OPEN, S15): its file list names only src/qtui/MainWindow.cpp:358,405,415, so the web half of the same defect has no registered row.",
      "evidence": [
        "STATUS GS-205: 'Non-GIF inputs still admitted: picker offers All files, appendInputs validates nothing, drop checks existence not isFile()' - cited files are Qt only.",
        "web/server.mjs /run admission: for each file it checks only 'a non-empty name and base64 data' plus uploadNameError(f.name) - no content check before the engine is spawned.",
        "web/server.mjs already owns the primitive: isGifMagic() reads 6 bytes and hasGifMagic() comes from ./output-verify.mjs - it is used for the *explode output* verification, not for the *upload*.",
        "web/app.js drop filter is extension/MIME-only (/\\.gif$/i or type === image/gif), which is the web twin of the desktop's existence-only check.",
        "No explicit base64 validation appears in the retrieval: a payload whose base64 is truncated or contains foreign characters is decoded by Node's forgiving Buffer.from(str, 'base64') into silently different bytes."
      ],
      "why": "Two surfaces, one defect: the web endpoint is the one that is *reachable by a stranger* when GS_WEB_HOST=0.0.0.0 is set, and it will happily write any bytes to a temp file, spawn the engine, and answer with an engine error string. The fix is already 90% written (hasGifMagic exists) - the missing half is admission on input. The forgiving base64 decode is the same class: a malformed request produces a confusing engine failure instead of a 400 that names the real problem.",
      "repro": [
        "printf 'PK\\003\\004notagif' > /tmp/notagif.bin",
        "curl -sS -X POST --data-binary @/tmp/notagif.bin 'http://127.0.0.1:8000/optimize?settings=%7B%7D' -i | head -20  # observe: no 400 'not a GIF'; the engine's own message is relayed",
        "POST /run with files:[{name:'x.gif', data:'AAAA==AAAA=='}] (trailing garbage in base64) and compare the decoded size to the announced one."
      ],
      "fix": [
        "Extend the GS-205 row to name the web surface, and implement one admission predicate used by desktop + web + CLI: existing, readable, regular file, starts with GIF87a/GIF89a, plus a base64 strictness check on the wire format.",
        "In /run and /optimize, verify hasGifMagic on the decoded buffer BEFORE writing the temp input file, and answer 400 'input is not a GIF (GIF87a/GIF89a signature missing)'.",
        "Validate base64 strictly: decode, then re-encode and compare, or check /^[A-Za-z0-9+/]*={0,2}$/ plus length % 4 and the decoded-size vs declared size; reject with 400 instead of letting engine stderr be the error message."
      ],
      "patch": "// web/run-paths.mjs (or a new web/admit.mjs) - one admission rule for both endpoints\nexport function admitGifBuffer(buf) {\n  if (!buf || buf.length < 6) return \"input is empty or too short to be a GIF\";\n  if (!hasGifMagic(buf.subarray(0, 6))) return \"input is not a GIF (GIF87a/GIF89a signature missing)\";\n  return null;\n}\nexport function decodeBase64Strict(data) {\n  if (!/^[A-Za-z0-9+/]*={0,2}$/.test(data) || data.length % 4 !== 0) return { error: \"invalid base64 payload\" };\n  const buf = Buffer.from(data, \"base64\");\n  if (buf.toString(\"base64\").replace(/=+$/, \"\") !== data.replace(/=+$/, \"\"))\n    return { error: \"invalid base64 payload (round-trip mismatch)\" };\n  return { buf };\n}",
      "acceptance": [
        "transport test: a non-GIF body to /optimize and to /run -> 400 with the GIF-signature message, and the engine is never spawned (assert on the temp dir staying empty).",
        "Body containing a valid GIF still 200 (no over-refusal).",
        "A truncated base64 payload -> 400, not an engine error string."
      ],
      "effort": "S/M - the magic helper exists; the work is wiring plus three tests.",
      "falsify": "grep -n 'hasGifMagic\\|GIF87a' working_code/gifscythe/web/server.mjs - if an upload-side check is already present in the batch admission loop (partially lost in retrieval), only the base64 half stands."
    },
    {
      "id": "GN-11",
      "title": "web server: an unvalidated PORT value crashes the process with a stack trace instead of failing with a message",
      "severity": "P2",
      "category": "BROKEN",
      "area": [
        "web"
      ],
      "confidence": "source-read",
      "auditRelation": "NEW. Not covered by U-06 (bind address / caps) or the static-hygiene rows.",
      "evidence": [
        "web/server.mjs: \"const PORT = Number(process.argv[2] || process.env.PORT || 8000);\" then later \"server.listen(PORT, HOST, () => {...})\" with no try/catch and no range check.",
        "Number() on a non-numeric string is NaN; on a value like '8080/' or a PaaS-style 'tcp://...' it is NaN too, and Node's listen throws ERR_SOCKET_BAD_PORT synchronously at module scope.",
        "The server module has no top-level try/catch (only the http.createServer request handler is wrapped), so the throw escapes as an uncaught exception with a stack trace."
      ],
      "why": "A one-line startup failure becomes a stack trace - and startup failures are exactly what a self-hosted user hits first (hosting panels set PORT, sometimes to a socket path or with trailing whitespace). It also breaks the repo's own honesty rule for user-facing errors: every other refusal in this codebase names the reason.",
      "repro": [
        "PORT=abc node web/server.mjs",
        "Observe: RangeError [ERR_SOCKET_BAD_PORT]: options.port should be >= 0 and < 65536 ... with a stack trace, exit 1 - no mention of what the user typed.",
        "Also: node web/server.mjs 999999 (argv path) behaves the same."
      ],
      "fix": [
        "Validate once, near the top, and exit 2 with a named reason (matching the CLI's caller-error code from GN-08):",
        "Cap the accepted range to 1..65535 and state that 0 means 'any free port' if you want to allow it. Print the value that was rejected, never the raw stack."
      ],
      "patch": "+function parsePort(raw) {\n+  const n = Number(String(raw).trim());\n+  if (!Number.isInteger(n) || n < 0 || n > 65535)\n+    return { error: `invalid port ${JSON.stringify(raw)} (expected an integer 0-65535; pass it as argv[2] or set PORT=)` };\n+  return { port: n };\n+}\n+const parsed = parsePort(process.argv[2] ?? process.env.PORT ?? 8000);\n+if (parsed.error) { console.error(`gifscythe web: ${parsed.error}`); process.exit(2); }\n+const PORT = parsed.port;",
      "acceptance": [
        "A test (or smoke script) asserts PORT=abc exits 2 with the message and no stack trace.",
        "PORT=8000 still starts and prints the loopback banner."
      ],
      "effort": "XS - ten lines plus one smoke case.",
      "falsify": "If a recent commit added port validation, this is already closed - check the first 40 lines of web/server.mjs."
    },
    {
      "id": "GN-12",
      "title": "web transport robustness: unbounded stderr capture, whole-response base64 fan-out, and /favicon.ico noise",
      "severity": "P3",
      "category": "MISSING-LOGIC",
      "area": [
        "web"
      ],
      "confidence": "source-read",
      "auditRelation": "NEW (hygiene). U-06's PARTIAL row covers concurrency/rate limits; these are the memory-shape details of the same endpoint and are not registered.",
      "evidence": [
        "web/server.mjs engine runner: \"let stderr = \\\"\\\"; child.stderr.on(\\\"data\\\", (d) => { stderr += d.toString(); });\" - no cap, and the full string is echoed back in the 422 body.",
        "web/server.mjs /run: outputs are collected as { name, path, data: verified.data } and returned as base64 inside one JSON response, so a batch of N files inlines every output in memory and on the wire (and app.js then builds one Blob per output).",
        "STATIC_FILES has no /favicon.ico entry, so a browser request for it gets the 404 text/plain 'not found' body unless index.html points at a data-URI icon - index.html was not read in this review, so verify that half before acting."
      ],
      "why": "A chatty engine (warnings per frame, a huge malformed input) can make the server hold and echo arbitrarily large stderr; a 4-mode batch of large GIFs can produce a response larger than the request that produced it. Both are the ordinary shape of memory bugs in a self-hosted tool, and both are cheap to bound now, before the wasm/desktop-parity work makes the transport load-bearing.",
      "repro": [
        "Feed a corrupt large file that makes gifsicle emit many warnings: watch the 422 body grow with the stderr text (curl -sS ... | wc -c).",
        "POST /run with 8 large GIFs (batch) against a server started with a low --max-old-space-size and observe peak RSS vs the request size.",
        "Load the UI and watch the network panel: /favicon.ico -> 404."
      ],
      "fix": [
        "Cap stderr at a constant (e.g. 16 KB) with an explicit truncation marker ('... [stderr truncated at 16 KB]'), which also matches the C# spike's Trim(500) intent.",
        "Bound the /run response: either return one output per request for batch (outputs metadata + a download id), or document and enforce a total-output cap with a 413-style refusal naming the cap. At minimum, compute the projected base64 size before building the JSON and refuse with a clear error when it exceeds a documented envelope.",
        "Add a favicon data-URI link in index.html (or an allow-list entry), so the 404 disappears without adding a file to the allow-list."
      ],
      "acceptance": [
        "A stderr-flood test asserts the 422 body is bounded and contains the truncation marker.",
        "A test asserts the documented output envelope refusal fires with a named reason and a machine-readable cap."
      ],
      "effort": "S - three independent small changes; none blocks the release.",
      "falsify": "grep -n 'stderr' working_code/gifscythe/web/server.mjs - if a cap already exists (possibly lost in retrieval), only the fan-out and favicon halves stand."
    },
    {
      "id": "GN-13",
      "title": "Hand-written mirrors prove agreement, not correctness: no suite sweeps the settings space against the real engine, so the U-62/U-63 bug class is still found by humans",
      "severity": "P1",
      "category": "VERIFY-GAP",
      "area": [
        "core",
        "web",
        "process"
      ],
      "confidence": "source-read",
      "auditRelation": "NEW as a diagnosis. The audit describes the parity suites as an asset (296/308 unit checks, five Node web suites, byte-identical mirror); it does not register that parity between two hand-written copies cannot detect a shared misunderstanding of gifsicle.",
      "evidence": [
        "COMPILED_AUDIT/§16: 'web/test/command.test.mjs cross-checks the JS command.mjs output against the C++ gifscythe-cli binary' - i.e. JS is compared to the CLI, and the CLI's builder is the mirror image of command.mjs.",
        "server.mjs and app.js both import buildArgs from ./command.mjs; src/core/GifsicleCommand.h is the C++ twin - the repo's own comments call them mirrors of one another.",
        "The record already contains two counterexamples where BOTH mirrors were wrong together and only the real engine disagreed: U-62 (crop 0x0 - 'engine allows 0=extend to edge') and U-63 (--no-loopcount play-once, 'engine has unchanged / forever (0) / N / OFF').",
        "To be precise about what is missing: the engine IS executed - scripts/test_engine.sh (5/5) and the smoke suite's --run cases run the real binary. What no suite does is sweep the *settings space* through it: coverage is the hand-picked smoke cases plus the mirrors' fixtures, so a rule both mirrors encode wrongly has nothing to disagree with.",
        "The audit's own method note adds the other half: in several auditing environments the engine binary was not built at all, so engine-dependent behaviour was never exercised, and the suites that were run could not have contradicted the mirrors."
      ],
      "why": "This is the mechanism that keeps producing findings late: every rule that both mirrors encode wrongly is invisible to the entire suite, and each discovery costs a session (U-62, U-63, N-05 multi-input explode were all engine-semantics discoveries). A generated conformance run against the real gifsicle converts that class from 'discovered by a careful reviewer' into 'discovered by CI'. It needs no new dependency, no network, and it works with the engine the repo already builds.",
      "repro": [
        "Show the gap: ./build.sh && ./scripts/smoke_cli.sh (green), node web/test/command.test.mjs (green) - then run one argv the mirrors never generate: ./build/gifsicle -O3 --no-loopcount in.gif -o out.gif and watch the engine accept a flag no settings field can emit (U-63).",
        "Second probe: generate argv from a random settings object and run it; count the cases where exit code != 0 while the product's own validate() accepted the settings - each one is a latent 'user configured what the UI offered, engine refused' finding."
      ],
      "fix": [
        "Add scripts/oracle_fuzz.mjs (node, zero deps, offline): enumerate a bounded, seeded sample of the settings space (optimize level x lossy on/off x colors on/off x dither x resize kinds x scale x loop states x delay x mode), map each sample through command.mjs AND through the CLI's print mode, then execute the argv against the repo-built engine on a 3-frame fixture GIF.",
        "Assert two directions and report a table: (a) every settings object the product can emit must be engine-accepted and produce a verified GIF; (b) every engine refusal must correspond to a settings object the product refuses or warns about. Commit the generated table as an artefact so drift shows up as a diff.",
        "Wire it into verify_audit.sh as a new gate (W6) with a --quick mode (fixed seed, first N samples) for the pre-push hook and a --full mode for CI, so it is both fast locally and exhaustive in CI.",
        "Feed the table back into the register: each (settings -> engine) mismatch becomes a row automatically, which is the cheapest finding-generation machine this repo can own."
      ],
      "patch": "# scripts/oracle_fuzz.mjs (shape - zero deps, offline, uses the repo engine)\n#   node scripts/oracle_fuzz.mjs --quick --engine build/gifsicle --fixture test/fixtures/anim3.gif\n# outputs: docs/audit/ORACLE_MATRIX.md   (seeded, committed, diffed by a gate)\n1. seeded sampler over gs::Settings (finite enumerations only; step the numerics coarsely)\n2. argvA = command.mjs buildArgs(s)          # JS mirror\n3. argvB = gifscythe-cli conf --print        # C++ mirror, same conf file\n4. assert argvA === argvB                    # parity: catches mirror drift\n5. run argvA against the engine -> rc, stderr, output magic\n6. assert (rc === 0 && verified) for every sample the product would accept\n7. write mismatches to ORACLE_MATRIX.md with the settings JSON and the engine stderr",
      "acceptance": [
        "The gate is red on at least one known case before the fixes land (crop 0x0 pre-U-62 is the historical reference point - if nothing is red today, say so and keep the gate as a regression net).",
        "Command parity (step 4) is asserted for every sample, so the two mirrors can no longer drift silently."
      ],
      "effort": "M/L - one session for a first bounded version; the highest long-term leverage item on this list.",
      "falsify": "If web/test/command.test.mjs already executes a broad sampled matrix through the real engine (not just the CLI), this finding is a duplicate - read that suite before starting."
    },
    {
      "id": "GN-14",
      "title": "Three 'needs native Windows' rows are pure functions testable on the existing windows-latest CI job today",
      "severity": "P1",
      "category": "PROCESS",
      "area": [
        "core",
        "process",
        "cli"
      ],
      "confidence": "documented-fragment",
      "auditRelation": "RECLASSIFIES rows the register blocks on a clean VM: U-55 (Windows path_key case folding), U-56 (superscript COM/LPT aliases), U-71 (exit mask &0xff collapses an NTSTATUS). U-70 (UTF-8 preview boundary) is left alone here - it runs through the preview pipeline rather than a pure rule.",
      "evidence": [
        "STATUS.md blockers: U-55/U-56/U-70/U-71 all say 'not started' with a Windows-native requirement; W-18 adds 'needs a clean Windows VM with no Qt/MinGW/dev tools'.",
        "The register itself proves the pattern is viable on Linux: U-21's proof is 'new src/core/OutputName.h (NameRules parameterised, so the Windows rule set is unit-tested on Linux)' and W-23's proof is 'unit tests 29/30 (Windows rule set tested on Linux)'.",
        "U-07 (the one case that truly needed CreateProcessW) was executed under Wine, proving a native-Windows runtime is not required for argv/path logic - only for ctypes that are OS calls.",
        "path_key() is a string normalisation over UTF-8 bytes; is_windows_reserved_device_name() is a name predicate. Neither spawns a process, opens a device, or links Qt - so both are reachable from the header-only unit suite, and a windows-latest job already exists (Windows CI green since PR #5). Unverified in this review: whether that job currently builds and runs the unit binary - if it only builds the engine + CLI, the fix is to add the unit step, which is still far cheaper than a VM."
      ],
      "why": "Three release-adjacent rows are parked behind a slow, unverifiable blocker when the correct test is a 20-line case in a suite that already runs on both platforms every push. Parked rows accumulate: W-18's own proof cell says it is *blocked by U-09*, so one stale artefact now also blocks the clean-machine gate. Moving U-55/U-56/U-71 into the unit suite removes three rows from the 'cannot be closed here' pile at the cost of one session of test writing.",
      "repro": [
        "grep -n 'Windows' .github/workflows/build.yml | head -20  # confirm the windows job exists and what it runs",
        "grep -rn 'unit' working_code/gifscythe/CMakeLists.txt build.sh | head  # confirm the unit binary is buildable on Windows CI",
        "Write one case for path_key('HERO.GIF') vs path_key('hero.gif') compiled with -D_WIN32, and one for is_windows_reserved_device_name('COM\\u00b9') - run locally on Linux first, then in the windows job."
      ],
      "fix": [
        "Add the three Win32 rule cases to the header-only unit suite with the same parameterisation trick used for OutputName (rule set injected, so Linux runs the Win32 rules too).",
        "Run that suite in the existing windows-latest CI job (it already builds the engine and CLI there) as a second invocation, so the *native* semantics are asserted on real Windows - which is strictly stronger evidence than Wine and needs no VM.",
        "Re-word the three register rows: blocker becomes 'needs the Win32 rule cases (test-only)', and U-70 keeps the real desktop dependency. Update W-18's blocker so it no longer cites U-09 for the desktop-only probes that do not depend on the artifact.",
        "Keep Wine + a clean VM only for the two remaining genuine cases (drop a file on the window, kill the engine mid-run) - W-19 already exists and is honest."
      ],
      "acceptance": [
        "Three rows move out of 'blocked' with executed proof in the windows CI job log (the run URL goes in the proof cell).",
        "The Linux run of the same suite stays green with the Win32 rule set injected."
      ],
      "effort": "S/M - test writing only; the toolchain and CI job already exist.",
      "falsify": "If U-55/U-56/U-71's blockers are not the VM but something other than testability (e.g. a missing Windows fixture engine), the reclassification is wrong - read the three rows in full first."
    },
    {
      "id": "GN-15",
      "title": "The only banked artefact is pre-relicence: the published Release predates S18's GPLv3 -> Ms-PL switch on first-party code",
      "severity": "P1",
      "category": "DOC-DRIFT",
      "area": [
        "release",
        "docs",
        "process"
      ],
      "confidence": "source-read",
      "auditRelation": "NEW. U-09 covers 'banked snapshot is 5 commits behind' as a release-evidence problem; U-08 covers licence-file completeness *inside* a package. Neither notices that the relicence changed the licence of the code the published binaries were built from.",
      "evidence": [
        "README: 'binaries banked on Release snapshot-2026-09-07' and 'Windows CI green and merged 2026-09-07 (PR #5 -> 0ad1ff5 ...)'.",
        "STATUS U-09 (OPEN): 'Banked Windows snapshot is 5 commits behind the SHA its own notes claim'.",
        "README/repo: 'S18: Ms-PL relicense + C# shell plan + Phase-1 spike' (PR #23, merged 2026-09-14) - first-party code relicensed GPLv3 -> Ms-PL; LICENSE now Ms-PL, with docs/legal/WHY_MSPL.md and docs/legal/COPYING_RULES.md added later.",
        "Therefore the 2026-09-07 asset was built from GPLv3-licensed first-party code and predates the Ms-PL licence files, the legal docs, and the S19 licence-completeness packaging work (U-08)."
      ],
      "why": "A published release asset is the one artefact a third party actually takes away. Right now the downloadable ZIP's licence, the repo's licence, and the licence files bundled inside the ZIP are from three different eras. For a project whose stated discipline is 'never let the docs and the artefact disagree', this is the highest-consequence disagreement available, and it is invisible to every code gate because no gate looks at the release page.",
      "repro": [
        "Open the Release 'snapshot-2026-09-07', download the ZIP, and read LICENSE / the bundled COPYING.* files - compare against main's LICENSE (Ms-PL) and docs/legal/WHY_MSPL.md.",
        "Compare the asset's commit against the tagged SHA its notes claim (U-09).",
        "grep the asset for QT_NOTICE.txt / COPYING.gplv3 - the S19 additions should be absent, which dates the asset precisely."
      ],
      "fix": [
        "Do the cheap legal half immediately and the expensive half at the next cut: mark the existing Release as superseded in its release notes ('pre-relicence build; licence terms changed in S18 - do not redistribute; superseded by <next tag>'), or delete the asset. A note costs one edit and removes the ambiguity now.",
        "Land the U-09 re-cut from an exact tagged SHA, then add a release-asset gate: an asset is publishable only if (a) its commit == the tagged SHA, (b) its manifest lists the licence files present in the tree at that SHA (the S19 CI manifest assertion already checks the file set, so extend it to match the SHA's digest set), and (c) the release notes name the licence.",
        "Make the gate run on tag creation, not only on push - the current gates are push-shaped, so the release page is the one surface that can drift unnoticed."
      ],
      "acceptance": [
        "The Release page states the licence of its own build, or the asset is gone.",
        "A tag-triggered job fails when the asset manifest digests do not match the tagged tree."
      ],
      "effort": "S for the note; M for the re-cut + tag-triggered gate (already scoped as P0-4/U-09).",
      "falsify": "If the banked release was re-cut after 2026-09-14 (or the asset was withdrawn), the licence half is already closed - check the Releases page date against PR #23's merge date."
    },
    {
      "id": "GN-16",
      "title": "DONE conflates 'offscreen harness green' with 'shipped behaviour proven', and the register's own W-19 admits the harness cannot reach the behaviours in question",
      "severity": "P1",
      "category": "VERIFY-GAP",
      "area": [
        "qt",
        "process",
        "docs"
      ],
      "confidence": "documented-fragment",
      "auditRelation": "RESHAPES the vocabulary COMPILED_AUDIT §19 already demands ('every FIXED row is a documentation claim until re-proven'), applied to the state machine rather than to individual rows.",
      "evidence": [
        "STATUS vocabulary: 'DONE | closed, with executed proof named in the row | Proof column names a command, a test id or a diff.'",
        "Many GUI rows name the offscreen harness specifically (U-45 'T18', U-47 'T20', U-34 'T20', W-20 'harness T1', W-25 'harness T15') and are marked DONE.",
        "STATUS W-19 (OPEN), verbatim: 'needs a physical desktop: the offscreen harness cannot kill the engine mid-run, physically drop a file, or show the engine-missing dialog'.",
        "U-59 (P0-7, OPEN) is precisely the cancel/kill-mid-write case; U-13 (drop filter, DONE) is proven at the filter level, while the physical drag-and-drop behaviour W-19 names as unreachable by the harness is exactly the layer the filter sits in.",
        "R-01 records the toolchain reality (Qt6/cmake absent in most sandboxes), so most of these rows were also CI-compiled-only when closed."
      ],
      "why": "The state machine has one state where two kinds of evidence are indistinguishable. That is a correctness risk for the release gate ('no open P0/P1' reads as 'nothing known is broken'), and it is the reason U-59 could sit OPEN while rows next to it claim the same code path is closed. It also makes honest work look dishonest: a session closing a GUI row with T-cases is not lying, but the register cannot show what was actually exercised.",
      "repro": [
        "Pick three DONE GUI rows whose proof names only T-cases and whose behaviour is in W-19's list (drop, cancel, engine-missing).",
        "Try to answer 'has this behaviour been observed once on a real desktop?' from the register: the answer is not representable.",
        "grep -n 'offscreen' STATUS.md - the vocabulary has no term for it even though every GUI proof uses it."
      ],
      "fix": [
        "Split the proof vocabulary with two markers rather than two states: keep DONE, and require the Proof cell to start with either 'harness:' (offscreen/CI-compiled) or 'desktop:' (real-machine video/log, the B5/B6/B14 probe). Rows whose behaviour only exists in the second world cannot claim more than 'harness:'.",
        "Add one derived header number: 'rows proven only offscreen: N' next to the state counts - that is the honest size of the 1.0.0 UI debt, and it makes W-19 measurable instead of a note.",
        "Gate the 1.0.0 claim on it: release criteria must state '0 rows proven only offscreen among release-blocking rows', which finally gives W-19/W-18 a machine-checkable consequence."
      ],
      "acceptance": [
        "A mutation test: flipping a row's proof marker from desktop: to harness: changes the derived header number (proves the counter reads the cell, not a hardcoded list).",
        "The 1.0.0 criteria line in PROJECT_VISION/RELEASE_PROCEDURE references the counter."
      ],
      "effort": "M - emitter + gate work, no code changes; but it must not become a new doc project (see the plan's 'freeze the doc machine' item).",
      "falsify": "Read three of those rows in full - if their Proof cells already carry a real-desktop artefact, only the vocabulary half stands."
    },
    {
      "id": "GN-17",
      "title": "Doc-gate arithmetic is itself a maintenance surface: a 150-char truncation over a prose field needed a whole session (N-06), and stale counts cost another (N-01)",
      "severity": "P2",
      "category": "PROCESS",
      "area": [
        "docs",
        "process"
      ],
      "confidence": "source-read",
      "auditRelation": "NEW as a cost claim. The audit records N-01 and N-06 individually and treats N-06 as fixed; nothing registers that both were caused by the gate's own design, not by repo state.",
      "evidence": [
        "STATUS N-01: the pending-workflow marker outlived its change, so 'eight doc locations quoting the wrong gate count' - a full session of correction.",
        "STATUS N-06: 'check_docs.sh emitter truncation was awk-locale-dependent: gawk counts CHARACTERS in length()/substr() under a UTF-8 locale, mawk counts BYTES ... so the committed STATUS.md passed G0 under gawk (S11 sandbox + CI) but FAILED under mawk (every prior sandbox).'",
        "The fix was 'export LC_ALL=C + reword the S11 §5 notes ASCII-only under the 150 limit' - i.e. prose was edited to suit the checker.",
        "The five root status docs total ~524 KB (COMPILED_AUDIT 214 KB, IMPROVEMENT_LOG 160 KB, SESSION_HANDOFF 66 KB, STATUS 42 KB, WORKLIST 41 KB), before docs/archive (~128 KB) and docs/audit (~71 KB); the CI workflow that enforces them is 21 KB."
      ],
      "why": "Effort is currently being spent making prose satisfy a truncation rule, and on repairing counts written by hand into prose. Both are self-inflicted and both scale with the doc machine. The fix is not 'more gates' - it is removing the two mechanisms: stop putting derived numbers in prose, and stop truncating a field the register does not need truncated.",
      "repro": [
        "grep -n '150' working_code/gifscythe/scripts/check_docs.sh  # the truncation that caused N-06",
        "grep -rn 'PASS / 0 FAIL\\|30/0/3\\|28 PASS' --include='*.md' . | wc -l  # hand-written counts in prose that N-01 broke",
        "Locale experiment: run check_docs.sh --emit under LC_ALL=C vs a UTF-8 locale and diff - the class is only de-fanged by LC_ALL=C, never removed."
      ],
      "fix": [
        "Delete the 150-char truncation: emit the full Proof cell and let the table wrap. Truncation exists to keep a table pretty; it cost a session and buys nothing machine-readable.",
        "Ban derived numbers from prose: gate counts and check counts are printed by verify_audit.sh --json and referenced by digest/artefact link, never typed into a sentence. Add a stale-sweep rule (S1-S5 style) that flags any hand-typed 'N PASS / M FAIL' string.",
        "Freeze the doc machine while the P0/P1 backlog is non-empty (see the plan view): no new gates, no new registers, no new audit merges until P0-7 and P1-38 are closed. The next audit intake becomes a GitHub Issue batch, not a new register part."
      ],
      "acceptance": [
        "check_docs.sh --emit is byte-identical under LC_ALL=C and UTF-8 with no ASCII-only rewording needed.",
        "The stale sweep fails a commit that hand-types a gate count."
      ],
      "effort": "S - a deletion, a gate rule, and one memo.",
      "falsify": "If the truncation was already removed after S11, only the 'derived numbers in prose' half stands."
    },
    {
      "id": "GN-18",
      "title": "Probe (not yet a claim): web stemOf and Qt completeBaseName may disagree on dotfile / extensionless names",
      "severity": "P3",
      "category": "VERIFY-GAP",
      "area": [
        "web",
        "core"
      ],
      "confidence": "needs-probe",
      "auditRelation": "NEW as a probe. Web/desktop naming parity is proven for ordinary names (U-01 planning, U-43 summary, W-23 templates) but the edge names are not pinned anywhere the register names.",
      "evidence": [
        "web/server.mjs: \"const stemOf = (name) => { const i = String(name).lastIndexOf('.'); return i > 0 ? String(name).slice(0, i) : String(name); };\" with the comment 'QFileInfo::completeBaseName semantics (strip after the LAST dot), which is what the desktop naming uses for _opt.gif / _frame'.",
        "For a leading-dot name ('.gif') lastIndexOf('.') === 0, so i > 0 is false and the whole name is returned - the comment's claimed semantics are not obviously what the code does at that boundary; app.js has an identical copy of the function.",
        "Nothing in the suites appears to cover a dotfile, a name with no extension, or a name ending in a dot against the desktop's naming."
      ],
      "why": "It is one branch, and only reachable for odd input names - hence P3 - but upload names are user-controlled and the two implementations are hand-duplicated (the same drift class GN-13 describes). If they disagree, the web silently writes a different filename than the desktop would, which is exactly the parity claim the web exists to prove (GN-02's fixtures are the natural place to pin it).",
      "repro": [
        "node -e \"const stemOf=(n)=>{const i=String(n).lastIndexOf('.');return i>0?String(n).slice(0,i):String(n)};console.log(JSON.stringify([stemOf('.gif'),stemOf('gif'),stemOf('a.b.gif'),stemOf('a.')]))\"",
        "Compare with the desktop's naming for the same names (batch run with a name template)",
        "and with Qt: a 3-line qmake/Qt test printing QFileInfo('.gif').completeBaseName() and QFileInfo('gif').completeBaseName()."
      ],
      "fix": [
        "Pin the rule from the desktop (Qt) first - it is the reference. Then either port the exact semantics into one shared JS helper (single copy, imported by app.js and server.mjs) or refuse the pathological names at upload (uploadNameError already exists and is the natural place).",
        "Add a fixture table of edge names to both suites so the two copies cannot diverge again."
      ],
      "acceptance": [
        "One shared stemOf in JS with a table of cases; the table includes '.gif', 'gif', 'a.b.gif', 'a.', 'A.GIF'.",
        "The desktop's own naming test prints the same table, and the two agree."
      ],
      "effort": "XS - one helper, one fixture table.",
      "falsify": "Run the Qt one-liner: if completeBaseName('.gif') is also '.gif', the implementations agree and the comment alone is imprecise."
    }
  ],
  "consistentFindings": [
    "web/server.mjs static allow-list (/, /index.html, /style.css, /app.js, /command.mjs) plus assertContainedPath defence-in-depth, and the single sendStatic path that omits the body for HEAD while still pinning Content-Length - the U-67/NF-10 fix reads correct, including the deliberate decision not to route web/wasm.",
    "readBody + BodyTooLargeError + sendTooLarge: the 413 is typed, accumulation stops without destroying the socket, and the response is flushed before the request is destroyed (U-68/NF-11) - the logic matches its comment.",
    "handleOptimize removed the double decodeURIComponent (U-49) and percent-encodes X-Gifscythe-Command (U-50) - both fixes are present in the code as described, not just in the log.",
    "findEngine() structured resolution: an invalid non-empty GS_ENGINE fails closed with a named reason and never falls back, versions compare numerically (U-26), and the launch error path does not trigger a second discovery (GS-207) - the comment and the code agree.",
    "The engine is spawned with an argv array and no shell in all three clients (web spawn, C# ArgumentList, desktop ProcessRunner) - no shell-injection surface was found anywhere in the retrieved code.",
    "snapshotOutput/verifyOutput before-after verification on /optimize and /run, plus explode frame snapshot-diffing via hasGifMagic: the ordinary-output postcondition (GS-203/DONE half) is genuinely implemented for core/CLI/web, and the register is honest that the Qt integration is the missing part.",
    "Licence separation is structurally clean: GPLv2 engine as a separate subprocess, Ms-PL first-party UI/control layer, LGPLv3 Qt + companion GPLv3 text + generated QT_NOTICE.txt - no linking that would raise the in-process licence question (which is why OD-16 is about wasm, not about the desktop).",
    "The web UI revoke discipline (U-52/U-46) is complete in app.js: beforeUrl and every result URL are tracked and revoked, and requestGen is checked after the fetch, after the blob read, in the catch and in the finally.",
    "COMPILED_AUDIT v3 is internally finding-complete against its own claim (76 register rows, six audits, no row dropped between v2 and v3 per §18) - the merge-completeness checklist is a real artefact and is worth keeping even when the doc machine is frozen."
  ],
  "methodLimits": [
    "Retrieval limit (important): this review read the repository through the network fetch path, which strips anything that looks like an HTML tag. In C++ files that destroys everything inside angle brackets (std::vector<...>, #include <string>), and in Markdown it destroys quoted C++ fragments. Therefore EVERY claim about src/core/*.h, src/core/*.cpp or src/qtui/*.cpp in this report is marked 'documented-fragment': it is inherited from the repository's own quoted lines and register text, not freshly re-read. No new C++ bug is asserted anywhere in this report.",
    "Files read end-to-end this session: web/server.mjs, web/app.js, csharp/spike/Program.cs, docs/planning/CSHARP_SHELL_PLAN.md (partial, first ~2 sections), STATUS.md (partial - the fetch truncated inside the W/D/R rows), COMPILED_AUDIT.md v3 (partial - header, §1, §2D, §16-§19), README.md, the repository tree listing with file sizes.",
    "Files not read: src/core/Validate.h, src/core/SettingsIO.h, src/core/GifsicleCommand.h, src/core/OutputPlan.h (only its comment block came through intact), src/qtui/*, web/command.mjs, web/validate.mjs, web/run-paths.mjs, web/output-verify.mjs, scripts/*.sh, .github/workflows/build.yml, the test suites. Findings that depend on these are labelled needs-probe with the exact command to settle them.",
    "Nothing in this report was executed. Every 'expected' outcome is a source-read expectation, and every finding carries a falsify line that is deliberately mechanical (one grep, one command, one one-liner) so a false positive costs ten minutes, not a session.",
    "Register/doc-based findings (GN-04, GN-05, GN-07, GN-14, GN-15, GN-16, GN-17) can be voided by content I did not retrieve, because STATUS.md was truncated mid-file. Check the full file before acting on them."
  ]
}
```

---

*End of report. 18 findings (0 P0, 12 P1, 4 P2, 2 P3), each with a mechanical falsification path so a wrong item costs ten minutes instead of a session. Generated from the review console; the console is a static build and contains no repository code.*
