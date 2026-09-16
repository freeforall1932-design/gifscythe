# Gifscythe — Independent Code Review & Remediation Plan

> Generated from the review app on 2026-09-16.
> Repo: https://github.com/freeforall1932-design/gifscythe · HEAD `794a996` (Merge PR #28 "S22: wire web regression gates and fix CLI/web edge cases" (2026-09-15))
> Product version: 0.1.0 · Register state at review time: 94 tracked rows (76 compiled U-nn + 18 §13 intake GS/DS rows) per COMPILED_AUDIT v3
> Reviewed tree: 2026-09-15 (post-S22 tree)

**Purpose.** Find the missing logic, misaligned code and broken code that the compiled audit (76 U-nn rows + 18 §13 intake rows) has *not* yet recorded, propose a ready-to-use fix for each, and give the owner an updated delivery plan. Every finding below was confirmed by reading the cited source at the pinned commit; nothing is copied from the repo's own register.

---

## 1. Executive summary

**9 new findings** (High: 2 · Medium: 4 · Low: 3 · Info: 0). Of these, 8 are absent from the repo's 94-row register; 1 extends a known row's scope. The dominant class is unchanged from all six prior audits — silent false-success — but the specific mechanisms are new: a verification branch that forgot its own exemptions (F-01), an integer truncation that corrupts before validation runs (F-02), a sentinel with no domain check (F-03), a documented tri-state implemented as two states (F-04), and an endpoint that accepts a mode it cannot honor (F-05).

**Register health:** the compiled audit's own G10 branch line names a commit six merge-sessions stale (F-06) — the same doc-drift class that has turned `main` red twice (GS-208, N-01). Run `check_docs.sh` before the next PR.

**Strategic verdict:** stay on C++17 + Qt6 through 1.0.0. The risk is surface count and doc overhead, not the language. See §6–§7.

---

## 2. Method, scope and an important reproducibility caveat

**Files read line-by-line (11):**

- `working_code/gifscythe/src/core/GifsicleCommand.h` — argv builder — mode flags, whole-GIF, resize, animation sentinels, threads
- `working_code/gifscythe/src/core/GifsicleSettings.h` — Settings struct, sentinel defaults, equality for round-trip tests
- `working_code/gifscythe/src/core/SettingsIO.h` — conf parser, strict bools, atomic save, extra_keys channel
- `working_code/gifscythe/src/core/Validate.h` — pre-run domain warnings — colors/disposal/optimize/lossy/delay/threads/resize
- `working_code/gifscythe/src/core/OutputPlan.h` — U-01 fix: plan outputs before running, refuse source/duplicate targets
- `working_code/gifscythe/src/core/ExplodeVerify.h` — explode prefix derivation + new-or-changed GIF frame verification
- `working_code/gifscythe/src/core/EngineLocator.h` — GS_ENGINE override, bundled/release candidates, real PATH search
- `working_code/gifscythe/src/core/ProcessRunner.h` — fork/execvp + CreateProcessW, MSVCRT quoting, exit-code mapping
- `working_code/gifscythe/src/cli/main.cpp` — the whole --run driver: resolve, validate, plan, verify, execute
- `web/command.mjs` — JS mirror of the builder (live pane + server argv), saveSettingsLines
- `web/server.mjs` — zero-dep Node server: static allow-list, /optimize, /run (4 modes)

**Docs read for context:** `README.md` (root) and `COMPILED_AUDIT.md` v3 — §0 (usage rules), §1 (cross-audit summary), the §5 register structure, and §17–§19 in full (intake verdicts, regression cases, and the next-session mandate this review was run against).

**Not reviewed this pass (do not claim otherwise):** `src/qtui/*` (MainWindow.cpp, SettingsPanel.cpp), `scripts/*.sh`, `CMakeLists.txt`, `web/app.js`, `web/validate.mjs`, `web/run-paths.mjs`, `web/output-verify.mjs`, and the test suites. Where a finding may also reach the GUI it says so explicitly (F-01).

**Reproducibility caveat — read before re-auditing.** Raw-text fetches of the .h/.cpp sources swallow every < and > that looks like an HTML tag, which silently corrupts C++ comparisons (>=, <=, <256). All code quoted in this review was re-fetched in a byte-exact mode and re-checked before any finding was filed. Anyone reproducing this audit with a naive fetcher will see mangled operators and may file false positives (or miss real ones) because of it.

---

## 3. New findings (F-01 … F-09)

### F-01 — [High] Explode verification ignores the stream-output and --info exemptions the ordinary verifier honors — a successful run is reported as rc=1

| Field | Value |
|---|---|
| Severity | **High** |
| Surface | cli |
| Class | broken |
| Register status | NEW — not in the 94-row register |
| Confidence | source-confirmed |
| Effort | S — ~6 lines + 2 smoke cases + 1 GUI reachability check |
| Cross-ref | Adjacent to U-17/P1-19 (explode verification) and U-04 (stdout purity) — this is the exemption those fixes forgot to port to the Explode branch. |

**Files:**
- `src/cli/main.cpp` — the two Explode blocks below the verify_file definition (https://github.com/freeforall1932-design/gifscythe/blob/main/working_code/gifscythe/src/cli/main.cpp)
- `src/core/ExplodeVerify.h` — explode_prefix_for() (https://github.com/freeforall1932-design/gifscythe/blob/main/working_code/gifscythe/src/core/ExplodeVerify.h)

**Finding.** The ordinary output verifier is gated on "!stream_output && !s.info" with an explicit comment saying stdout streaming and --info "deliberately keep their existing contracts, so there is no output file to verify there". The Explode verification directly below that comment has NO such guard. explode_prefix_for() returns s.output verbatim, so with output = "-" the prefix becomes the literal string "-" and the verifier looks for files named "-.*" in the CWD — finds none — and the CLI downgrades a successful engine run to exit code 1 with "engine exited 0 but wrote no frames". The same false failure fires for an --info run in Explode mode, which the ordinary path explicitly exempts.

**Evidence (as read at 794a996):**

```cpp
// main.cpp — the ordinary path is correctly exempt:
const bool verify_file = !s.output.empty() && !stream_output
                      && s.mode != gs::Mode::Explode && !s.info;
// ...comment above it: "Streaming stdout (output = -) and --info deliberately
//    keep their existing contracts, so there is no output file to verify there."

// ...but the Explode path below has NO stream/info guard:
if (s.mode == gs::Mode::Explode) {
    explode_prefix = gs::explode_prefix_for(s);   // returns s.output VERBATIM
    explode_before = gs::snapshot_explode_candidates(explode_prefix);
}
// ...
if (rc == 0 && s.mode == gs::Mode::Explode) {
    const gs::ExplodeResult vr = gs::verify_explode_frames(explode_prefix, explode_before);
    if (!vr.ok) { /* ERROR: engine exited 0 but wrote no frames */ rc = 1; }
}

// ExplodeVerify.h — prefix is s.output with no "-" special case:
inline std::string explode_prefix_for(const Settings& s) {
  if (!s.output.empty()) return s.output;      // "-" arrives here verbatim
  ...
}
```

**Reproduction:**

1. Build the CLI: cd working_code/gifscythe && ./build.sh
2. Write a conf: mode = explode / input = anim.gif / output = - (the documented stdout contract)
3. Run: ./build/gifscythe-cli explode.conf --run > frames.gif; echo $?
4. Expected: exit 0 and frames.gif contains the concatenated exploded frames (gifsicle -e -o - writes them to fd 1).
5. Actual (per source): frames ARE written to stdout, but the CLI then fails prefix verification for "-" and exits 1 with "ERROR: engine exited 0 but wrote no frames" — a false failure after honest success.
6. Variant: mode = explode + info = true hits the same false failure (the ordinary verifier exempts --info; the explode verifier does not).

**Proposed fix.** Mirror the ordinary path's exemptions. Introduce a single guard used by both the snapshot and the verify call, and add the two regression cases (explode + output = -, explode + info = true) to the smoke suite before closing. Also check whether MainWindow.cpp reaches the same verify path from the GUI — that file was not reviewed this pass, so mark the GUI half PARTIAL until proven.

```cpp
// main.cpp — one guard, both blocks:
const bool verify_explode = s.mode == gs::Mode::Explode
                         && !stream_output && !s.info;
gs::ExplodeFileState... explode_before;
std::string explode_prefix;
if (verify_explode) {                            // was: if (mode == Explode)
    explode_prefix = gs::explode_prefix_for(s);
    explode_before = gs::snapshot_explode_candidates(explode_prefix);
}
// ...
if (rc == 0 && verify_explode) {                 // was: if (rc == 0 && mode == Explode)
    const gs::ExplodeResult vr =
        gs::verify_explode_frames(explode_prefix, explode_before);
    ...
}
// Regression (scripts/smoke_cli.sh):
//   explode + output=-  -> rc 0, non-empty stdout, no verification error
//   explode + info=true -> rc 0, no verification error
```

---

### F-02 — [High] long → int truncation silently corrupts numeric settings into valid-looking values (colors = 4294967298 becomes -k 2), and the web mirror warns where the desktop corrupts

| Field | Value |
|---|---|
| Severity | **High** |
| Surface | core |
| Class | broken |
| Register status | NEW — not in the 94-row register |
| Confidence | source-confirmed |
| Effort | S — ~25 lines + one unit test per key |
| Cross-ref | Same silent-failure class the whole register is organised around (U-11 booleans, U-13 argv cardinality). Not covered by GS-206's open numeric-domain work, which assumes the value survived parsing. |

**Files:**
- `src/core/SettingsIO.h` — the need_long-consuming branches (delay, disposal, loopcount, optimize, threads, colors, lossy) (https://github.com/freeforall1932-design/gifscythe/blob/main/working_code/gifscythe/src/core/SettingsIO.h)
- `src/core/Validate.h` — validate() — sees the already-truncated int, so the range check passes (https://github.com/freeforall1932-design/gifscythe/blob/main/working_code/gifscythe/src/core/Validate.h)

**Finding.** Every integer key is parsed into a long and then stored with static_cast<int>. On LP64 (Linux/macOS CI) long is 64-bit, so a value like 4294967298 parses cleanly, truncates to int 2 on the cast, passes validate()'s 2..256 range check, and is emitted as -k 2 with ZERO warnings. The user asked for a (nonsense) huge color count and silently got a 2-color GIF. The same mechanism flips delay, lossy, optimize, threads, disposal and loopcount — and loopcount = 4294967296 truncates to 0, which the builder emits as --loopcount=0 (loop FOREVER) instead of the requested count. The JS mirror does not truncate (JS numbers are doubles), so the identical settings file warns loudly on the web surface and corrupts silently on the desktop — a parity break in the shared honesty contract.

**Evidence (as read at 794a996):**

```cpp
// SettingsIO.h — parse to long, store as int, no range check on the cast:
else if (k == "colors") {
    long tmp = 0;
    if (need_long(&tmp)) s.color_count = static_cast<int>(tmp);  // 4294967298 -> 2
}
// (identical pattern for delay, disposal, loopcount, optimize, threads, lossy)

// Validate.h — checks the ALREADY-TRUNCATED value, so it passes:
if (s.color_count != -1 && (s.color_count < 2 || s.color_count > 256)) { ... }
// s.color_count == 2 here, so no warning is ever emitted.

// GifsicleCommand.h — emits the corrupted value:
if (s.color_count >= 2 && s.color_count <= 256) { add(args_, "-k"); add(args_, i2s(s.color_count)); }
```

**Reproduction:**

1. Write a conf containing: colors = 4294967298 and input = anim.gif
2. Run: ./build/gifscythe-cli conf.conf (print mode)
3. Expected: a WARNING (value out of int range) and no -k flag, or a refusal under --strict.
4. Actual: the printed argv contains -k 2 and stderr contains zero warnings — the requested value was silently replaced with a different, valid one.
5. Parity check: POST the same value to the web /run endpoint — web/validate.mjs sees the untruncated double and returns a 422 domain warning. Same settings, opposite behavior per surface.
6. Worst case: loopcount = 4294967296 truncates to 0 → builder emits --loopcount=0 → the animation is set to loop forever instead of the requested count.

**Proposed fix.** Range-check before the cast. Add a need_int helper that refuses anything outside [INT_MIN, INT_MAX] with the same LoadWarning channel the other parsers use, and use it for all seven integer keys. Pin it with a unit test that feeds 2^32 + k for each key and asserts a warning (and, under --strict, exit 3).

```cpp
// SettingsIO.h — new helper next to to_long:
inline bool to_int(const std::string& s, int* out) {
  long v = 0;
  if (!to_long(s, &v)) return false;
  if (v < INT_MIN || v > INT_MAX) return false;   // refuse, never truncate
  *out = static_cast<int>(v);
  return true;
}
// and in set_field's need_int lambda:
auto need_int = [&](int* dest) -> bool {
    int tmp = 0;
    if (!to_int(v, &tmp)) { warn("integer out of range (must fit in 32 bits)"); return false; }
    *dest = tmp;
    return true;
};
// Then replace static_cast<int>(tmp) in the delay/disposal/loopcount/
// optimize/threads/colors/lossy branches with need_int(&s.<field>).
```

---

### F-03 — [Medium] loopcount is the only sentinel integer with no validate() domain check — out-of-range values vanish silently

| Field | Value |
|---|---|
| Severity | **Medium** |
| Surface | core |
| Class | missing |
| Register status | NEW — not in the 94-row register |
| Confidence | source-confirmed |
| Effort | S — 5 lines in each of two files + 2 test cases |
| Cross-ref | Closes the one gap in GS-206's numeric-domain sweep; same class as U-11 (silent boolean swallow). |

**Files:**
- `src/core/Validate.h` — validate() — checks colors, disposal, optimize, lossy, delay, threads... never loopcount (https://github.com/freeforall1932-design/gifscythe/blob/main/working_code/gifscythe/src/core/Validate.h)
- `src/core/GifsicleCommand.h` — the loopcount emission (== 0 / > 0 only) (https://github.com/freeforall1932-design/gifscythe/blob/main/working_code/gifscythe/src/core/GifsicleCommand.h)

**Finding.** Every sibling sentinel warns when out of domain: colors (2..256), disposal (0..7), optimize (0..3), lossy (0..200), delay (>= 0), threads (>= -1). loopcount has no check at all, and the builder only emits for == 0 or > 0. So loopcount = -5 (or any negative other than the -1 'unset' sentinel) is parsed, stored, and then silently dropped: no --loopcount in the argv, no WARNING on stderr, rc = 0. The user believes they set a loop count; nothing happened.

**Evidence (as read at 794a996):**

```cpp
// Validate.h — the complete list of numeric checks; loopcount is absent:
if (s.color_count != -1 && (s.color_count < 2 || s.color_count > 256)) {...}
if (s.disposal   != -1 && (s.disposal   < 0  || s.disposal   > 7))    {...}
if (s.optimize_level != -1 && (s.optimize_level < 0 || s.optimize_level > 3)) {...}
if (s.lossy      != -1 && (s.lossy      < 0  || s.lossy      > 200))  {...}
if (s.delay_cs < -1) {...}
if (s.threads  < -1) {...}
// (no loopcount check anywhere in the file)

// GifsicleCommand.h — only two of the three states are representable:
if (s.loopcount == 0) { add(args_, "--loopcount=0"); }
else if (s.loopcount > 0) { add(args_, "--loopcount=" + i2s(s.loopcount)); }
// s.loopcount < -1  ->  nothing emitted, nothing warned
```

**Reproduction:**

1. Write a conf with: loopcount = -5 and input = anim.gif
2. Run: ./build/gifscythe-cli conf.conf
3. Expected: a WARNING about the loopcount domain (compare lossy = -5, which does warn).
4. Actual: the argv contains no --loopcount and stderr is empty.
5. Contrast: lossy = -5 in the same conf prints 'WARNING: lossy=-5: must be 0..200 (or unset)'.

**Proposed fix.** Add the missing domain check to validate() and mirror it in web/validate.mjs. -1 already means 'unset', so the legal domain is exactly -1 or >= 0.

```cpp
// Validate.h — after the threads check:
if (s.loopcount < -1) {
    add("loopcount", std::to_string(s.loopcount),
        "must be >= -1 (-1 = unset, 0 = forever, >0 = iteration count)");
}
// web/validate.mjs — the mirror:
if (s.loopcount < -1) {
    add("loopcount", String(s.loopcount),
        "must be >= -1 (-1 = unset, 0 = forever, >0 = iteration count)");
}
```

---

### F-04 — [Medium] Threads contract is a documented tri-state but a implemented two-state — 'engine default (no -j)' is unreachable and every argv carries -j

| Field | Value |
|---|---|
| Severity | **Medium** |
| Surface | core |
| Class | misaligned |
| Register status | NEW — not in the 94-row register |
| Confidence | source-confirmed |
| Effort | S — 4 lines across 2 builders + docs + a deliberate unit-test-28 move |
| Cross-ref | Directly reopens the U-03 ↔ DS-06 / VP-1 / VP-2 pairing named in §19; also touches U-03's original fix comment in GifsicleCommand.h. |

**Files:**
- `src/core/GifsicleCommand.h` — the threads emission at the end of build() (https://github.com/freeforall1932-design/gifscythe/blob/main/working_code/gifscythe/src/core/GifsicleCommand.h)
- `web/command.mjs` — buildArgs() — the mirrored two-state mapping (https://github.com/freeforall1932-design/gifscythe/blob/main/web/command.mjs)
- `COMPILED_AUDIT.md §19` — the U-03 ↔ DS-06 / VP-1 / VP-2 pairing guardrail (https://github.com/freeforall1932-design/gifscythe/blob/main/COMPILED_AUDIT.md)

**Finding.** The audit's own next-session mandate (§19) states the threads contract as a tri-state: '<0 none / 0 bare -j / >0 -jN', adding that 'Unit test 28 pins the mapping'. The code implements two states: threads > 0 emits -jN, everything else (including -1, the 'unset' default) emits a bare -j. Consequences: (a) the documented 'emit nothing' state is unreachable — the engine's own default of one thread cannot be requested; (b) every run, even one where the user never touched the control, spawns the engine with -j (GIFSICLE_DEFAULT_THREAD_COUNT = 8); (c) the 'live CLI pane' always shows -j for threads = -1, which misrepresents 'unset'; (d) Validate.h documents three states ('-1 = unset/default, 0 = auto, >0 = explicit') that the builder collapses into two. A reviewer following §19 would 'fix' either side and could accidentally flip the loopcount/optimize sentinels — the exact pairing §19 warns about.

**Evidence (as read at 794a996):**

```cpp
// GifsicleCommand.h (and identically web/command.mjs):
if (s.threads > 0) add(args_, "-j" + i2s(s.threads));
else add(args_, "-j");            // threads == -1 ("unset") ALSO lands here

// GifsicleSettings.h documents the sentinel:
int threads = -1;                 // -j; <=0 = auto

// Validate.h documents THREE states for the same field:
"must be >= -1 (-1 = unset/default, 0 = auto, >0 = explicit thread count)"

// COMPILED_AUDIT.md §19 (the guardrail):
"the threads tri-state (<0 none / 0 bare -j / >0 -jN) must not flip the
 loopcount or optimize sentinels. Unit test 28 pins the mapping"
```

**Reproduction:**

1. Write a conf with: input = anim.gif and NO threads key (so threads stays at the -1 default).
2. Run: ./build/gifscythe-cli conf.conf
3. Expected per §19: no -j in the argv (the '<0 none' state).
4. Actual: the argv ends with a bare -j — the 'unset' default and the explicit 'Auto' are indistinguishable, and the engine always runs with its 8-thread default.
5. Cross-check web/command.mjs buildArgs() — the identical two-state mapping, so the parity test cannot catch the divergence from the documented contract.

**Proposed fix.** Pick ONE contract and pin it. Given the project's 'engine truth' stance, implement the documented tri-state: -1 emits nothing (engine default), 0 emits bare -j, > 0 emits -jN — and make the GUI control three-state (Default / Auto / N threads). Alternatively keep two states but then correct §19, GifsicleSettings.h and Validate.h to say -1 and 0 are equivalent. Either way, move unit test 28 deliberately and re-run the §17.1 validation order.

```cpp
// GifsicleCommand.h — the tri-state §19 documents:
if (s.threads > 0)      add(args_, "-j" + i2s(s.threads));
else if (s.threads == 0) add(args_, "-j");
// s.threads < 0  ->  emit nothing (engine default: single thread)

// web/command.mjs — keep byte-identical parity:
if (s.threads > 0) add("-j" + i2s(s.threads));
else if (s.threads === 0) add("-j");

// SettingsIO.h save_settings() already writes every value >= 0, so the
// round trip survives unchanged; only the builder and the docs move.
```

---

### F-05 — [Medium] /optimize forwards a mode key it cannot honor — explode and batch produce misleading 422s that blame the engine

| Field | Value |
|---|---|
| Severity | **Medium** |
| Surface | web |
| Class | broken |
| Register status | NEW — not in the 94-row register |
| Confidence | source-confirmed |
| Effort | S — ~10 lines + 2 transport cases |
| Cross-ref | Same honesty class as U-64/NF-07 (info:true 422 fixed for this very endpoint) — the mode key was simply missed. |

**Files:**
- `web/server.mjs` — handleOptimize() — the settings spread into buildArgs (https://github.com/freeforall1932-design/gifscythe/blob/main/web/server.mjs)

**Finding.** POST /optimize is documented as "single-file Auto mode; kept for compatibility", and it already rejects info:true with a clean 400. But it spreads ...settings into buildArgs without forcing or rejecting mode. A client posting {"mode":"explode"} gets argv "-e in.gif -o out.gif" — gifsicle then writes out.gif.000, out.gif.001... while out.gif itself never appears, so verifyOutput() fails and the server answers 422 with exitCode 0 and an error blaming the engine, even though the engine did exactly what was asked. {"mode":"batch"} emits -b (in-place on the temp upload) and fails the same way. The frames written into the temp dir are then silently deleted by the finally block. The endpoint already has the right precedent one screen up: /run validates mode usage explicitly.

**Evidence (as read at 794a996):**

```js
// handleOptimize() — the mode key flows straight into the builder:
const issues = validate({ ...settings, inputs: ["<upload>"] });  // no mode rule
...
const s = { ...settings, inputs: [inFile], output: outFile };    // mode survives
const argv = [engine, ...buildArgs(s)];                          // -e / -b emitted
...
const verified = await verifyOutput(outFile, before);            // out.gif absent
if (verified.error) {
    sendJson(res, 422, { ok: false, exitCode: 0, stderr: verified.error, ... });
    // ^ a 422 that says the ENGINE produced invalid GIF output
```

**Reproduction:**

1. Start the server with an engine present: cd working_code/gifscythe && ./build.sh && node ../../web/server.mjs
2. curl -sS -X POST 'http://127.0.0.1:8000/optimize?settings=%7B%22mode%22%3A%22explode%22%7D' --data-binary @anim.gif -i
3. Expected: a clean 400 explaining /optimize is Auto-only (mirroring the info:true rejection), or the mode silently forced to auto.
4. Actual: HTTP 422 with ok:false, exitCode:0 and an error about invalid GIF output — the engine is blamed for a request the endpoint should never have accepted.
5. Variant: settings=%7B%22mode%22%3A%22batch%22%7D produces the same misleading 422 via the -b path.

**Proposed fix.** Reject non-auto modes up front with the same clean 400 used for info:true (or force mode to auto and say so). Add both cases to web/test/transport.test.mjs so the red-green proof exists.

```js
// handleOptimize(), right after the info rejection:
const optimizeMode = settings.mode === undefined || settings.mode === null
                   || settings.mode === "" ? "auto" : String(settings.mode);
if (optimizeMode !== "auto") {
    sendJson(res, 400, {
        ok: false,
        error: '"/optimize" is single-file Auto mode only; use POST /run for '
             + "batch, merge or explode",
    });
    return;
}
// transport.test.mjs:
//   POST /optimize?settings={"mode":"explode"} -> 400 (not 422)
//   POST /optimize?settings={"mode":"batch"}   -> 400 (not 422)
```

---

### F-06 — [Medium] COMPILED_AUDIT.md's G10 branch line still names 2d51347 while main is at 794a996 — the exact doc-drift class that turned main red twice

| Field | Value |
|---|---|
| Severity | **Medium** |
| Surface | docs |
| Class | misaligned |
| Register status | NEW — not in the 94-row register |
| Confidence | needs-runtime-proof |
| Effort | S — one line + a gate run |
| Cross-ref | Recurrence of GS-208 and N-01 (both 'main red from doc drift alone'). |

**Files:**
- `COMPILED_AUDIT.md` — the header 'Branch: main at 2d51347817f5cdb39334415a03bb5f2b543119dd (the PR #15 merge...)' (https://github.com/freeforall1932-design/gifscythe/blob/main/COMPILED_AUDIT.md)

**Finding.** The audit file's own header states that check_docs.sh gate G10 'fails if this line names anything else'. The line still names the PR #15 merge (2d51347, 2026-09-12), but main's HEAD is 794a996 (PR #28, S22, 2026-09-15) — six merge sessions of drift. Either the gate now auto-emits that line (in which case the file on main is stale and the gate is red), or the line is manual and nobody refreshed it through S16–S22 (in which case the gate is also red). Main has already gone red twice from documentation drift alone (GS-208, N-01), so this is the project's highest-frequency release blocker.

**Evidence (as read at 794a996):**

```cpp
// COMPILED_AUDIT.md header (as fetched from main @ 794a996):
"**Branch:** main at 2d51347817f5cdb39334415a03bb5f2b543119dd (the PR #15
 merge; re-confirm with gh api repos/freeforall1932-design/gifscythe/branches/main
 --jq .commit.sha; check_docs.sh gate **G10** fails if this line names anything else)"

// branches/main right now:
"sha": "794a9964550fde0b7006474170926a6382c835f3",
"message": "Merge pull request #28 ... S22: wire web regression gates and fix
            CLI/web edge cases" (2026-09-15T22:33:41Z)
```

**Reproduction:**

1. gh api repos/freeforall1932-design/gifscythe/branches/main --jq .commit.sha  → 794a996...
2. grep -n '2d51347817f5cdb39334415a03bb5f2b543119dd' COMPILED_AUDIT.md  → still present on main
3. cd working_code/gifscythe && ./scripts/check_docs.sh  → if gate G10 is still a manual equality check, it fails here
4. If it passes, the gate must now auto-emit the line — then the committed file is stale and the next --emit run will show a diff.

**Proposed fix.** Run check_docs.sh (and sweep_stale.sh) before the next PR; refresh the branch line (or land the auto-emit change that makes it self-maintaining) and re-emit STATUS.md. Longer term, this line is the third single-point-of-drift doc artifact that has broken main — see the Plan section for the doc-machine reduction that removes the class.

---

### F-07 — [Low] Batch + an explicit output key is neither refused nor pinned — the planner classifies -b -o as the merge shape

| Field | Value |
|---|---|
| Severity | **Low** |
| Surface | cli |
| Class | missing |
| Register status | extends a known row |
| Confidence | source-confirmed |
| Effort | S — 5 lines, or one pinned smoke test |
| Cross-ref | Extends U-74/NF-17 (batch + single output + N>1) to the single-input case; sibling of GS-201/P0-5. |

**Files:**
- `src/cli/main.cpp` — the plan_outputs call (always passes {s.output} regardless of mode) (https://github.com/freeforall1932-design/gifscythe/blob/main/working_code/gifscythe/src/cli/main.cpp)
- `src/core/Validate.h` — validate() — no batch+output warning (https://github.com/freeforall1932-design/gifscythe/blob/main/working_code/gifscythe/src/core/Validate.h)

**Finding.** GS-201 closed 'Batch with no output' (refused, exit 2) and U-74's repro covers 'Batch + single output + N>1 inputs'. But mode = batch + output = out.gif with exactly ONE input is not covered by either case, and validate() has no batch+output warning: the planner receives one input and one output, classifies it as the merge shape, and the argv carries both -b and -o out.gif. gifsicle's -b is documented as 'modify each GIF input in place', so the engine may rewrite the source and/or ignore -o, while the CLI reports the planned single output and its preflight as if -o were authoritative. The web surface dodges this entirely (its batch mode deliberately runs one Auto command per file, never -b) — only the CLI and the conf format can reach the combination. (Test suites were not read this pass — verify against smoke_cli.sh before relying on the absence of a pin.)

**Evidence (as read at 794a996):**

```cpp
// main.cpp — the planner is mode-blind about batch's in-place semantics:
if (!s.output.empty() && !stream_output && s.mode != gs::Mode::Explode) {
    ...
    const gs::OutputPlan plan = gs::plan_outputs(plan_inputs, {s.output});
    // mode == Batch lands here too: one input + one output == "merge shape",
    // so the plan validates -o while the argv also carries -b
}
// GS-201 only refuses the EMPTY-output case:
if (do_run && s.mode == gs::Mode::Batch && s.output.empty()) { ... return 2; }
```

**Reproduction:**

1. Write a conf: mode = batch / input = anim.gif / output = /tmp/out.gif
2. Run: ./build/gifscythe-cli conf.conf --run; echo $?
3. Observe what the bundled 1.96 actually does with -b anim.gif -o /tmp/out.gif (in-place rewrite of anim.gif? -o ignored? an error?) — no test in the repo pins this.
4. If the engine ignores -o under -b, the run rewrote the source while the CLI's preflight claimed the target was /tmp/out.gif.

**Proposed fix.** Cheapest and consistent with GS-201: refuse Batch whenever output is set too, until the engine behaviour is pinned by an executed test. If -b -o turns out to be legal and useful, pin it in smoke_cli.sh and document it in the mode table instead.

```cpp
// main.cpp — extend the GS-201 stop-loss until the engine behaviour is pinned:
if (do_run && s.mode == gs::Mode::Batch && !s.output.empty()) {
    std::fprintf(stderr,
                 "ERROR: refusing to run — Batch with an explicit output is not "
                 "pinned against the engine's in-place -b; use Auto for a single "
                 "target or run one file at a time.\n");
    return 2;
}
// (or, if the smoke test proves -b -o is honoured, document it and add
//  scripts/smoke_cli.sh case: batch + output + 1 input -> pinned rc + files)
```

---

### F-08 — [Low] /optimize discovers the engine before reading the request body — the 413 path is unreachable without an engine present

| Field | Value |
|---|---|
| Severity | **Low** |
| Surface | web |
| Class | broken |
| Register status | NEW — not in the 94-row register |
| Confidence | source-confirmed |
| Effort | S — ~10 lines reordered + 1 transport case |
| Cross-ref | Sibling of U-68/NF-11 (the 413 work that fixed /run's ordering). |

**Files:**
- `web/server.mjs` — handleOptimize() — findEngine() before readBody() (https://github.com/freeforall1932-design/gifscythe/blob/main/web/server.mjs)

**Finding.** In handleOptimize(), findEngine() runs before readBody(), so an oversized upload to a server with no engine answers 503 (engine not found) instead of 413. The code's own comment admits the asymmetry: 'This handler discovers the engine before reading the body, so the 413 is reachable only when an engine is present; /run reads the body first.' /run does it in the right order. The result is an inconsistent limit contract between the two endpoints for identical clients.

**Evidence (as read at 794a996):**

```js
// handleOptimize():
const resolution = await findEngine();      // runs FIRST
if (!resolution.path) { ...503... }
const dir = await mkdtemp(...);
try {
    const body = await readBody(req, MAX_BODY);   // 413 only reachable here
    ...
// handleRun() (correct order):
let raw;
try { raw = await readBody(req, MAX_BODY); } catch (err) {
    if (err && err.statusCode === 413) { sendTooLarge(res, req, MAX_BODY); return; }
    ...
}
```

**Reproduction:**

1. Unset GS_ENGINE and move the release/ tree aside so findEngine() fails.
2. GS_MAX_BODY=1000 node web/server.mjs
3. curl -sS -X POST 'http://127.0.0.1:8000/optimize?settings=%7B%7D' --data-binary @big.gif -i → 503 (engine), not 413 (size).
4. Same body to /run → 413, because that handler reads the body first.

**Proposed fix.** Move readBody above findEngine in handleOptimize, mirroring handleRun, and add a transport case that pins 413-without-engine for both endpoints.

```js
// handleOptimize() — read the body first, then discover the engine:
let body;
try {
    body = await readBody(req, MAX_BODY);
} catch (err) {
    if (err && err.statusCode === 413) { sendTooLarge(res, req, MAX_BODY); return; }
    sendJson(res, 400, { ok: false, error: "could not read request body" });
    return;
}
if (!body.length) { sendJson(res, 400, { ok: false, error: "empty upload" }); return; }
const resolution = await findEngine();
// ... then mkdtemp, writeFile, run
```

---

### F-09 — [Low] A non-numeric port argument crashes the server with an unhandled RangeError instead of a usage message

| Field | Value |
|---|---|
| Severity | **Low** |
| Surface | web |
| Class | broken |
| Register status | NEW — not in the 94-row register |
| Confidence | source-confirmed |
| Effort | S — 10 lines |

**Files:**
- `web/server.mjs` — the PORT constant (https://github.com/freeforall1932-design/gifscythe/blob/main/web/server.mjs)

**Finding.** Number(process.argv[2] || process.env.PORT || 8000) yields NaN for any non-numeric argument, and server.listen(NaN) throws an unhandled RangeError at the top level of an ESM module — the process dies with a raw stack trace instead of the "here is how to run me" message a tool whose brand is honesty should print.

**Evidence (as read at 794a996):**

```js
const PORT = Number(process.argv[2] || process.env.PORT || 8000);
// node server.mjs abc  ->  Number("abc") === NaN
// ...
server.listen(PORT, HOST, () => { ... });
// listen(NaN) -> RangeError [ERR_SOCKET_BAD_PORT]: Port should be > 0 and < 65536.
//                Unhandled at module top level -> raw stack, no usage hint.
```

**Reproduction:**

1. node web/server.mjs abc
2. Expected: a one-line usage error naming the valid range.
3. Actual: an unhandled RangeError with a stack trace.

**Proposed fix.** Validate the port once, before listen, and exit 2 with a usage line on failure.

```js
const PORT = (() => {
    const raw = process.argv[2] ?? process.env.PORT ?? "8000";
    const n = Number(raw);
    if (!Number.isInteger(n) || n < 1 || n > 65535) {
        console.error("usage: node web/server.mjs [port 1-65535] (got "
                    + JSON.stringify(raw) + ")");
        process.exit(2);
    }
    return n;
})();
```

---

### Minor notes (recorded, not filed as findings)

- **M-1** — The /run batch collision check's message says it prevents overwriting "the uploaded file of the same name", but uploads are decoded to neutral in<N>.gif names on disk, so no overwrite is possible — what it actually prevents is a planned output name colliding with a user-facing upload name (a response-shape problem, not a data one). Harmless, but the comment overstates the guarantee. (`web/server.mjs — the mode === "batch" collision loop`)
- **M-2** — expand_home()'s comment says "Reject bare ~ alone as a path", but the implementation expands a bare "~" to $HOME (only ~user is left unexpanded). The comment and the behaviour disagree about which case is "rejected". (`src/cli/main.cpp — expand_home()`)
- **M-3** — run() resolves its promise on timeout and then again on the later 'close' event; the first resolution wins so behaviour is correct, but the double-resolve is a trap for anyone who adds post-close logic inside that second callback later. (`web/server.mjs — run()`)

---

## 4. Known-open register rows re-confirmed in source

These are NOT new — they are already tracked. They are listed here because this pass re-confirmed each one is still present in the code at `794a996`, so the register's OPEN/PARTIAL states are accurate.

- **U-71 / NF-14** — ProcessRunner maps the Windows exit code with static_cast<int>(code) & 0xff — a child that exits with 0x100 (or any NTSTATUS whose low byte is 0) is collapsed to success. Confirmed still present in the current source.
  - Where: `src/core/ProcessRunner.h — run_argv(), Windows branch` · State: OPEN (needs a real Windows VM or the S11 Wine harness)
- **U-73 / NF-16** — resolve_path() falls back to fs::exists(path) from the CWD when the conf-anchored candidate is missing, so a relative input that is absent next to the conf but present in the CWD is silently picked up from the CWD — contradicting the CWD-independent contract. Confirmed still present.
  - Where: `src/cli/main.cpp — resolve_path()` · State: OPEN
- **U-06 (partial)** — The web server has no auth, no concurrency cap and no rate limit; only the loopback default and the body cap protect it. Confirmed — MAX_BODY and the 127.0.0.1 default are the only guards in the current source.
  - Where: `web/server.mjs — server.listen and the request handler` · State: PARTIAL (accepted as a single-user internal tool; GS_WEB_HOST opt-in documented)
- **U-12 / P1-24** — ProcessRunner is synchronous by design: POSIX waitpid and Windows WaitForSingleObject(INFINITE) both block the calling thread, so the GUI's five UI-thread waits remain. Confirmed at the core layer (GUI call sites not re-verified this pass).
  - Where: `src/core/ProcessRunner.h — run_argv()` · State: OPEN (deliberately deferred; the freeze is untestable offscreen)
- **GS-206 (partial)** — Numeric/domain validation exists for colors, disposal, optimize, lossy, delay and threads — but loopcount is missing entirely (F-03) and every integer key truncates before validation runs (F-02), so the sweep is not yet closed.
  - Where: `src/core/Validate.h + src/core/SettingsIO.h` · State: PARTIAL — F-02 and F-03 are the two concrete gaps

---

## 5. Verified non-findings (false-positive guardrails)

Each row below is a plausible-sounding bug that was checked against the source and is NOT a defect. Recorded so the next reviewer does not re-litigate them, and so this review's negatives are as auditable as its positives.

- **Claim:** color_count's <= 256 guard forces -k 256 on every run
  **Verdict:** False. The default is color_count = -1 (GifsicleSettings.h), so the 2..256 guard is only reachable when the user sets a count. No -k is emitted by default.
- **Claim:** Out-of-range numeric settings are silently dropped by the builder
  **Verdict:** False for every key validate() covers: colors/disposal/optimize/lossy/delay/threads all print a WARNING before the builder drops them, and --strict (U-40) turns that into exit 3. The CLI's 'warn and proceed' is the documented policy. Only loopcount escapes (F-03) and only pre-validation truncation corrupts (F-02).
- **Claim:** An empty comment still emits --comment with no operand
  **Verdict:** False. Both builders skip empty strings explicitly (the U-13/U-48 fix is present and commented in GifsicleCommand.h and mirrored in command.mjs).
- **Claim:** --loopcount=0 is a bug (should be --loopcount or nothing)
  **Verdict:** False. gifsicle's contract is that loopcount 0 means loop forever; the builder's == 0 branch is correct and matches the VP-1 guardrail in §15 of the compiled audit.
- **Claim:** The web batch mode wrongly emits a single -b run over all inputs
  **Verdict:** False. /run's batch branch deliberately runs one Auto command per file and never emits -b at all — the source documents this as the desktop parity decision.
- **Claim:** plan_outputs misses an output that overwrites a non-paired input in the merge shape
  **Verdict:** False. The planner builds input_keys from EVERY queued input and refuses any output matching any of them, not just the paired one; it also reports (not refuses) pre-existing targets, which is the documented re-run policy.
- **Claim:** win_quote_arg breaks on empty arguments or trailing backslashes
  **Verdict:** False. Empty arguments are wrapped in quotes (preserving argv cardinality) and trailing backslash runs are doubled per MSVCRT rules — both are covered by the unit suite.
- **Claim:** The documented PATH engine fallback is still dead code
  **Verdict:** False. find_on_path() is a real PATH walk now (the U-05 fix), and an invalid non-empty GS_ENGINE refuses fallback with a named error (the GS-207 fix) rather than silently selecting a different engine.
- **Claim:** The /optimize endpoint still double-decodes settings or crashes on non-ASCII headers
  **Verdict:** False. Both the U-49 fix (no extra decodeURIComponent) and the U-50 fix (encodeURIComponent on X-Gifscythe-Command) are present in the current source.

---

## 6. Language & architecture recommendation

**Stay on C++17 + Qt6 through 1.0.0. Do not rewrite, do not add a framework, do not switch languages to move faster.**

Why:

- The risk is concentrated in the SURFACES, not the language. Five surfaces exist or are scaffolded for a 0.1.0 product (Qt6 GUI, CLI, Node web, web/wasm, parked csharp/spike). This review independently found the same honesty-bug class in three of them — that is a scope problem, and a rewrite would replicate it in a new stack.
- The core is small and well-protected: eleven Qt-free header-only files, 296+ unit checks, JS parity fixtures, and an offscreen GUI harness at 300+ checks. A rewrite discards all of it and re-earns none.
- The Windows path is the actual hard part and it is already Wine-proven end-to-end (U-07: CreateProcessW + UTF-16 command line, é-path confs running rc=0 where the old build failed). That proof is worth more than any language preference.
- Both independent intake reviewers (E and F) reached the same verdict, and the repo's own OFFLINE_BUILD_REVIEW pinned the direction on 2026-09-09. A fourth opinion that agrees is signal, not noise.

What would actually make it faster to ship:

- Freeze the web surface at 'internal tool' (already decided in S14 — enforce it: no new server.mjs/app.js features until 1.0.0). This review's web findings (F-05, F-08, F-09) are all cheap, but the surface itself is the time sink.
- Cut the doc machine. COMPILED_AUDIT.md alone is ~214 KB — larger than the entire product source — and main has gone red twice from doc drift alone (GS-208, N-01), with F-06 showing the same class still live. Keep STATUS.md + a CHANGELOG as the only living documents, archive the rest as dated snapshots, and replace prose-count gates with a link checker. Move findings to GitHub Issues where they can be assigned and closed.
- One shippable artifact: the Windows portable zip (OD-17). Every other surface is a demo or a park until 1.0.0.
- Time-box rule (from intake F, endorsed here): if 1.0.0 is not reachable in ~5 more sessions at the current pace, the cause is surface count and doc overhead — cut those, not the language.

Rewrite trigger: Only after 1.0.0, and only if a measured requirement demands it: Rust + Tauri is the sole option that keeps gifsicle as an isolated subprocess while shrinking the binary and modernising the UI — but it costs the offscreen harness and the Wine-proven Windows path, so it needs a spike with a written go/no-go, not a vibe. A wasm-first future is license-blocked until OD-16 (GPLv2 distribution of a wasm build) is decided.

---

## 7. Phased plan to 0.2.0 → 1.0.0

### Phase 0 — Land this review (1 session)

**Goal.** Turn F-01..F-09 into register rows with executed proof, failing-test-first.

- File F-01..F-09 as U-77..U-85 in COMPILED_AUDIT.md §5 and re-emit STATUS.md (check_docs.sh --emit) — do not hand-edit the generated block.
- F-01, F-02, F-05 first (the three honesty bugs): write the red test, fix, paste command + exit code into the row.
- F-06: run check_docs.sh and sweep_stale.sh before anything else merges; refresh the branch line or land the auto-emit.
- Re-run the §17.1 validation order in full (build.sh --all, smoke_cli.sh, test_gui_offscreen, the five Node suites, glue_harness.mjs, verify_audit.sh).

**Exit criteria.** verify_audit.sh green, doc gates green, every new row carries an executed proof.

### Phase 1 → 0.2.0 (the truthful pre-release)

**Goal.** Close the remaining data-integrity and release blockers; re-cut the artifacts.

- U-58 (batch settings snapshot) and U-59 (partial-output handling on cancel/failure) — the two HIGHs from intake F.
- GS-203's GUI half: wire the shared ordinary-output postcondition verifier into MainWindow.
- GS-205: centralise GIF input admission before the engine sees a byte.
- U-09: re-cut the release from an exact tagged SHA; run docs/ci/CLEAN_WINDOWS_SMOKE.md once on a real VM (also the only way to close U-71, U-55, U-56, U-70).
- Owner decisions OD-11..OD-17 that block the cut (version number, two-way CLI, artifact set).

**Exit criteria.** 0.2.0 tagged, packaged, and proven on one clean Windows machine that is not the build machine.

### Phase 2 → 0.3.0 (one contract, every surface)

**Goal.** Kill the per-surface divergence this review kept tripping over.

- Numeric-domain tri-state in one place: DS-06/07/09 + GS-206 + F-02 + F-03 — one validation pass, run before any builder, mirrored in validate.mjs by contract test.
- F-04: pick and pin the threads contract (tri-state recommended), move unit test 28 deliberately.
- Engine-discovery unification (NF-08/NF-09): one discovery policy shared by CLI, GUI and web.
- CLI batch = per-file Auto like the GUI (retire -b from the conf surface; closes NF-17 and F-07 together).

**Exit criteria.** The same settings file produces byte-identical argv on desktop, CLI and web.

### Phase 3 → 1.0.0 (the finished GIF product)

**Goal.** The bar PROJECT_VISION.md already sets — do not lower it to ship sooner.

- The UI/UX task done (the README's own rule: never call it 1.0.0 until then).
- Zero open High findings in the register; every PARTIAL row either closed or explicitly accepted by the owner.
- U-12/P1-24 (UI-thread waits) resolved or formally waived with the reasoning recorded.

**Exit criteria.** 1.0.0. WebP/APNG (2.0.0) and the frame editor stay blocked until this ships.

---

## 8. Ready-to-run verification commands

Run the focused regressions BEFORE the broad suite so a failure names the patch under test, then the whole ladder:

```bash
# from the repo root
cd working_code/gifscythe && ./build.sh --all
#   engine + CLI + unit tests + Qt6 GUI (fails honestly if Qt is missing)
./scripts/smoke_cli.sh
#   CLI end-to-end against the real engine (add the F-01 cases here)
QT_QPA_PLATFORM=offscreen ./build-cmake/test_gui_offscreen
#   offscreen GUI harness (check F-01's GUI reachability here)
node web/test/command.test.mjs
#   JS⇄C++ builder parity (move unit test 28 deliberately when fixing F-04)
node web/test/validate.test.mjs
#   validate mirror parity (add the F-03 loopcount case)
node web/test/transport.test.mjs
#   server transport (add the F-05 and F-08 cases)
node web/test/body-limit.test.mjs && node web/test/static-hygiene.test.mjs
#   the S22 web gates
./scripts/check_docs.sh && ./scripts/sweep_stale.sh
#   documentation gates — run FIRST for F-06
./scripts/verify_audit.sh
#   the one-command suite (30 PASS / 0 FAIL / 3 SKIP at the last full checkpoint)
```

---

## Appendix A — suggested new register rows

Paste-ready rows in the repo's own §5 style (severity / file / one-line claim / fix). Numbering continues the U-nn sequence:

- **U-77 / F-01** [High] `src/cli/main.cpp` — Explode verification ignores the stream-output and --info exemptions the ordinary verifier honors — a successful run is reported as rc=1. Fix: Mirror the ordinary path's exemptions. (New — not in the register at review time.)
- **U-78 / F-02** [High] `src/core/SettingsIO.h` — long → int truncation silently corrupts numeric settings into valid-looking values (colors = 4294967298 becomes -k 2), and the web mirror warns where the desktop corrupts. Fix: Range-check before the cast. (New — not in the register at review time.)
- **U-79 / F-03** [Medium] `src/core/Validate.h` — loopcount is the only sentinel integer with no validate() domain check — out-of-range values vanish silently. Fix: Add the missing domain check to validate() and mirror it in web/validate.mjs. (New — not in the register at review time.)
- **U-80 / F-04** [Medium] `src/core/GifsicleCommand.h` — Threads contract is a documented tri-state but a implemented two-state — 'engine default (no -j)' is unreachable and every argv carries -j. Fix: Pick ONE contract and pin it. (New — not in the register at review time.)
- **U-81 / F-05** [Medium] `web/server.mjs` — /optimize forwards a mode key it cannot honor — explode and batch produce misleading 422s that blame the engine. Fix: Reject non-auto modes up front with the same clean 400 used for info:true (or force mode to auto and say so). (New — not in the register at review time.)
- **U-82 / F-06** [Medium] `COMPILED_AUDIT.md` — COMPILED_AUDIT.md's G10 branch line still names 2d51347 while main is at 794a996 — the exact doc-drift class that turned main red twice. Fix: Run check_docs.sh (and sweep_stale.sh) before the next PR; refresh the branch line (or land the auto-emit change that makes it self-maintaining) and re-emit STATUS.md. (New — not in the register at review time.)
- **U-83 / F-07** [Low] `src/cli/main.cpp` — Batch + an explicit output key is neither refused nor pinned — the planner classifies -b -o as the merge shape. Fix: Cheapest and consistent with GS-201: refuse Batch whenever output is set too, until the engine behaviour is pinned by an executed test. (Extends a known row.)
- **U-84 / F-08** [Low] `web/server.mjs` — /optimize discovers the engine before reading the request body — the 413 path is unreachable without an engine present. Fix: Move readBody above findEngine in handleOptimize, mirroring handleRun, and add a transport case that pins 413-without-engine for both endpoints.. (New — not in the register at review time.)
- **U-85 / F-09** [Low] `web/server.mjs` — A non-numeric port argument crashes the server with an unhandled RangeError instead of a usage message. Fix: Validate the port once, before listen, and exit 2 with a usage line on failure.. (New — not in the register at review time.)

## Appendix B — review metadata

- Repo: https://github.com/freeforall1932-design/gifscythe
- HEAD audited: `794a996` — Merge PR #28 "S22: wire web regression gates and fix CLI/web edge cases" (2026-09-15)
- Product version: 0.1.0 (per the README: do not bump to 1.0.0 until the UI/UX task is done)
- Register state: 94 tracked rows (76 compiled U-nn + 18 §13 intake GS/DS rows) per COMPILED_AUDIT v3
- Scope: Line-by-line review of the 11 files listed under Method. NOT reviewed this pass: src/qtui/* (MainWindow.cpp, SettingsPanel.cpp), scripts/*.sh, CMakeLists.txt, web/app.js, web/validate.mjs, web/run-paths.mjs, web/output-verify.mjs, and the test suites. Findings that may also reach the GUI are marked accordingly.
- Fetch caveat: Raw-text fetches of the .h/.cpp sources swallow every < and > that looks like an HTML tag, which silently corrupts C++ comparisons (>=, <=, <256). All code quoted in this review was re-fetched in a byte-exact mode and re-checked before any finding was filed. Anyone reproducing this audit with a naive fetcher will see mangled operators and may file false positives (or miss real ones) because of it.

*End of report — 9 findings, 3 minor notes, 5 re-confirmed open rows, 9 verified non-findings.*
