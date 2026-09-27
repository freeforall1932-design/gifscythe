I'll start by getting oriented and building the coverage ledger before reading any code for findings.

The workspace is empty. Let me clone the repo (read-only operation).

Direct git access is blocked. Let me try the fetch tool and check for a proxy.

Shell has no outbound network; the fetch tool does. Let me test both paths.

Shell has no network. Let me check whether a local proxy exists that the fetch tool uses.

No network in the shell. I'll work through the fetch tool. First, the file inventory for the ledger.

Full inventory obtained. Now orientation + the PR #5 diff.

PR list is empty — no open PR. Let me check recent commits and all PRs to locate the U-59 partial-write work.

Key facts established: **there is no open PR.** The U-59/P0-7 work merged via PR #3 (`4a8e353b` "Protect GUI outputs with verified partial promotion"), and PR #5 was the U-94 oracle. So Phase 1 = auditing the *merged* code. Starting there.

Note: the fetch pipeline strips `<...>` sequences (HTML-tag artifact) and collapses indentation — I will avoid quoting any line containing angle brackets. Now the GUI run path.

Now the core write-path files. Also testing whether GitHub's HTML view preserves line numbers (no local checkout is possible).

Testing GitHub's HTML blob view for line numbers (would let me cite exact lines without a local checkout).

Line numbers are not available from any endpoint — I will count them from the raw text (reliable for small files; I will cite without a line number where I cannot count confidently). Continuing with the C++ core.

Two important confirmations forming. Now the CLI path (Phase 1's "does the CLI have the same protection?") and the engine-exec signature.

Now the web surface (Phases 5–6 parity targets).

Strong signal in the JS mirrors. Now the client and the parity suites (Phase 6's vacuity question).

Now the two highest-stakes remaining questions: does the offscreen harness actually exercise the U-59 paths (Phase 1), and do the gates gate (Phase 7)?

Phase 1 is now answerable with evidence. Last fetch round: the gate suite (Phase 7) and the U-59 test fixture.

One more cheap fetch to settle a gate-integrity question.

I have enough verified evidence to report. Here is the audit.

---

# AUDIT REPORT — gifscythe @ `main` (`957c143`)

**Method note first, because it changes how you should read this.** This sandbox has **no network from the shell** (git/curl to github.com all time out) and **no local checkout**, so I could not clone, build, or run anything. Every file below was read through the GitHub raw endpoint in **this session**. Consequences, stated up front:

- Every finding below carries a **verbatim quote I copied from the fetched file**. Nothing is reconstructed from memory or from `COMPILED_AUDIT.md`.
- **Line numbers are only given where I counted them** (small files: `web/run-paths.mjs`, `web/output-verify.mjs`). For the large files (`MainWindow.cpp` 56 KB, `main.cpp` 31 KB, `test_gui_offscreen.cpp` 75 KB) **no endpoint returns line numbers and I refused to guess** — those findings cite a function anchor instead. That is deliberate, not an oversight.
- The fetch pipeline **strips `<...>` sequences** (HTML-tag artifact), so I quote only lines free of angle brackets.
- **I did not read `SESSION_HANDOFF.md`, `COMPILED_AUDIT.md`, or `WORKLIST.md`.** With no context-budget protocol in hand I chose the code over the narrative. Deduplication therefore ran against `STATUS.md` (all 169 rows) plus the U-nn items quoted inside the code itself.

---

## PHASE 0 — COVERAGE LEDGER

**62 in-scope files enumerated. 22 DEEP, 0 READ-only, 40 PENDING.**

| # | Path | Lines | State | Depth |
|---|---|---|---|---|
| **src/core/** | | | | |
| 1 | `OutputVerify.h` | ~110 | **DEEP** | U-59 guard, quoted |
| 2 | `OutputPlan.h` | ~205 | **DEEP** | planner, quoted |
| 3 | `ExplodeVerify.h` | ~185 | **DEEP** | explode verifier, quoted |
| 4 | `ProcessRunner.h` | ~150 | **DEEP** | exec path, quoted |
| 5 | `Validate.h` | ~175 | **DEEP** | rules, quoted |
| 6 | `WinUnicode.h` | ~195 | **DEEP** | u8path_compat, quoted |
| 7 | `EngineLocator.h` | ~170 | **DEEP** | engine discovery, quoted |
| 8 | `GifsicleCommand.h` | ~240 | PENDING | not reached |
| 9 | `GifsicleSettings.h` | ~200 | PENDING | not reached |
| 10 | `OutputName.h` | ~150 | PENDING | not reached |
| 11 | `SettingsIO.h` | ~640 | PENDING | not reached |
| 12 | `version.h` | 10 | PENDING | not reached |
| **src/qtui/** | | | | |
| 13 | `MainWindow.cpp` | ~1,400 | **DEEP** | full run/cancel/preview path |
| 14 | `MainWindow.h` | ~175 | **DEEP** | member + claim audit |
| 15 | `SettingsPanel.cpp` | ~980 | PENDING | not reached |
| 16 | `SettingsPanel.h` | ~120 | PENDING | not reached |
| 17 | `PreviewPanel.cpp` | ~160 | PENDING | not reached |
| 18 | `PreviewPanel.h` | ~50 | PENDING | not reached |
| 19 | `DropListWidget.h` | ~40 | PENDING | not reached |
| 20 | `qtui/main.cpp` | 12 | PENDING | not reached |
| **src/cli/** | | | | |
| 21 | `main.cpp` | ~730 | **DEEP** | full U-59 CLI path |
| **tests/** | | | | |
| 22 | `test_gui_offscreen.cpp` | ~1,700 | **PARTIAL** | T1–T14 read + quoted; **T15–T20 PENDING** (response truncated) |
| 23 | `fake_engine_partial_failure.cpp` | 28 | **DEEP** | U-59 fixture |
| 24 | `fake_engine_exit0.cpp` | 20 | PENDING | not reached |
| 25 | `test_gifsicle_command.cpp` | ~1,400 | PENDING | not reached |
| 26 | `test_output_verify.cpp` | ~110 | PENDING | not reached |
| 27 | `test_sweep_stale.py` | ~230 | PENDING | not reached |
| 28 | `test_build_options.py` | 50 | PENDING | not reached |
| 29 | `oracle_fuzz_matrix.json` | 32 KB | PENDING | not reached |
| 30 | `windows_reserved_names.txt` | 27 rows | PENDING | not reached |
| **scripts/** | | | | |
| 31 | `verify_audit.sh` | ~300 | **DEEP** | all gate matchers quoted |
| 32 | `check_docs.sh` | ~1,500 | **PENDING** | 57 KB — not read, **the single biggest gap in this audit** |
| 33 | `smoke_cli.sh` | ~1,100 | PENDING | not reached |
| 34 | `sweep_stale.sh` | ~450 | PENDING | not reached |
| 35 | `oracle_fuzz.mjs` | ~320 | PENDING | not reached |
| 36 | `pr_preflight.sh` | ~320 | PENDING | not reached |
| 37 | `review_change.sh` | ~400 | PENDING | not reached |
| 38 | `build_engine.sh` | ~140 | PENDING | not reached |
| 39 | `package_common.sh` | ~140 | PENDING | not reached |
| 40 | `package_portable.sh` / `package_system.sh` | 6 / 6 | PENDING | not reached |
| 41 | `test_engine.sh` / `test_output_verify.sh` / `test_package.sh` | 70/12/170 | PENDING | not reached |
| 42 | `bootstrap_hooks.sh` | 40 | PENDING | not reached |
| **root of product** | | | | |
| 43 | `CMakeLists.txt` | 140 | **DEEP** | target graph, quoted |
| 44 | `build.sh` | 190 | PENDING | not reached |
| 45 | `gifscythe.pro` | 35 | PENDING | not reached |
| 46 | `build_support/gifsicle/config.native.h` | 90 | PENDING | not reached |
| 47 | `build_support/version.h.in` | 10 | PENDING | not reached |
| **web/** | | | | |
| 48 | `run-paths.mjs` | **45** | **DEEP** | counted; containment quoted |
| 49 | `output-verify.mjs` | **29** | **DEEP** | counted; verifier quoted |
| 50 | `validate.mjs` | ~130 | **DEEP** | JS validator, quoted |
| 51 | `command.mjs` | ~250 | **DEEP** | argv builder, quoted |
| 52 | `request-guard.mjs` | ~79 | **DEEP** | ownership rule, quoted |
| 53 | `app.js` | ~360 | **DEEP** | client, quoted |
| 54 | `server.mjs` | ~1,050 | **PENDING** | **not read — the largest functional gap** |
| 55 | `index.html` | 160 | PENDING | not reached |
| 56 | `style.css` | 120 | PENDING | not reached |
| **web/test/** | | | | |
| 57 | `command.test.mjs` | ~190 | **DEEP** | parity suite, quoted |
| 58 | `validate.test.mjs` | ~330 | PENDING | not reached |
| 59 | `transport.test.mjs` | ~1,100 | PENDING | not reached |
| 60 | `server-bounds.test.mjs` | ~390 | PENDING | not reached |
| 61 | `body-limit` / `static-hygiene` / `request-guard` / `device-names` / `numeric-honesty` | 245/250/95/57/50 | PENDING | not reached |
| **root** | | | | |
| 62 | `.githooks/pre-push` | ~55 | PENDING | not reached |

**Explicitly OUT OF SCOPE** (as instructed, not silently skipped): `reference_code/gifsicle/**`, `reference_code/gifsicle-nested-1.96/**`, `web/wasm/**` (OD-16), `csharp/**`, all `*.md` except where used for orientation/claim checks, `LICENSE`/`COPYING.*`, `docs/screenshots/*.png`.

**Not in your list that I noticed:** `docs/ci/build.yml.proposed` (a byte-copy of the live workflow, guarded by gate E9) and `reference_code/REFERENCE_MANIFEST.md`. Neither is product code. **I did not audit them — tell me if you want `build.yml.proposed` treated as in scope**, since a byte-copy that has drifted once is a real gate target.

**Not machine-generated, nothing vendored-in found inside scope.** All 62 files are first-party.

---

## PHASE 1 — PR #5 / U-59 / P0-7 — **VERDICT: SHIP, WITH ONE REGISTERED HOLE**

**First, a factual correction to the brief: there is no open PR.** The GitHub API returns `[]` for open PRs. The history is:

- `4a8e353` "Protect GUI outputs with verified partial promotion" — the Qt half of U-59.
- `42306bb` — PR #3 merged (S28, "U-59/P0-7 + the node/CLI-provable lane").
- `957c143` — PR #5 merged: *"Add seeded real-engine settings oracle (U-94 / P2-18)"*. **That PR is not the U-59 work.**

So I audited the **merged** code, which is the stronger test anyway.

### What I verified as genuinely correct

I am reporting these as **positive results**, because the brief asked whether this ships:

- **The CLI is protected, not just the GUI.** `main.cpp`, in `main()`: the guard is computed, the single `-o` operand is rewritten, and everything else discards the partial.
- **Old output survives cancel.** The engine never opens the user's file: the argv is rewritten to the `.gs-partial` before `run_argv`, and the target is only reached by `promote_partial` after verification. The CLI's own comment states it: *"the previous bytes survive a cancel, a signal and a refusal."* This matches the P0-7 mechanism the brief asked about.
- **`redirect_output_operand` fails closed.** It requires the operand to occur exactly once; a doubled or absent `-o` is refused, not run unguarded.
- **The offscreen harness is NOT vacuous.** This is the direct answer to your question. T4 pre-writes 27 bytes of *"previous known-good output bytes"* to the target, runs, then asserts the target starts with `GIF8` and that no `.gs-partial` survives — that is a real promote-over-existing assertion. T8 uses a real fixture (`fake_engine_partial_failure.cpp` writes 32 bytes of *"corrupt partial from failing engine"* to `-o` and returns 7) and asserts the pre-existing target still reads back byte-identical. **T9 is the P0-7 mechanism test and it is real**: it builds a 4,800-frame GIF, sets `lossy=100`, asserts the engine is `Running` at ~0.6 s, clicks Cancel, and asserts the 39-byte known-good output is untouched and no partial is left. Those assertions are reachable and the cleanup runs.
- **The fixtures are correctly wired.** I suspected `verify_audit.sh` built `--target test_gui_offscreen fake_engine_exit0` but omitted the partial-failure fixture T8 requires. **`CMakeLists.txt` refutes it**: `add_dependencies(test_gui_offscreen fake_engine_exit0 fake_engine_partial_failure)`. Not a finding.

### PHASE 1 findings

---

**[HIGH] data-loss — `src/qtui/MainWindow.cpp` (`runCommand()`, non-batch branch) and `src/cli/main.cpp` (`main()`)**
**Status: NEW** (U-59 is `DONE` in `STATUS.md`; this is a hole the row does not claim to cover, so it must be triaged in this session per the repo's `UNTRIAGED` rule.)

```
// MainWindow.cpp — runCommand()
 pendingOutput_ = QString::fromStdString(settings.output);
 if (settings.mode != gs::Mode::Explode) {
 pendingPartial_ = QString::fromStdString(
 gs::partial_output_path(pendingOutput_.toStdString()));
 gs::discard_partial(pendingPartial_.toStdString()); // self-heal a prior hard kill
 partialSnapshot_ = gs::snapshot_output(pendingPartial_.toStdString());
 settings.output = pendingPartial_.toStdString();
 } else {
 pendingPartial_.clear();
 }
```
```
// main.cpp — main()
 const bool guard_output =
 !s.output.empty() && !stream_output && s.mode != gs::Mode::Explode;
```

**Mechanism.** `Explode` is exempted from the partial mechanism on **both** surfaces. The exemption is deliberate and the CLI comment states the reason: *"explode writes frames and is verified by ExplodeVerify.h instead."* But `ExplodeVerify.h` **only verifies — it never protects**. `verify_explode_frames` diffs a pre-run snapshot against a post-run directory; nothing in the write path redirects frame writes. gifsicle's `explode_filename()` opens each `prefix.NNN` with truncating semantics, so frame *k* of a re-run truncates the previous complete frame *k* in place.

**Scenario.** User explodes `a.gif` → 12 good frames `a_frame.000…011`. User changes a setting and re-runs Explode. The engine truncates `a_frame.000` and begins writing. The user presses Cancel. `cancelRun()` discards `pendingPartial_` — which is **empty** for Explode — and reports `"Cancelled."` No dialog, no error. The user's last good exploded frame set is now half-truncated on disk, silently. The identical scenario exists on the CLI (SIGTERM instead of Cancel; the partial is likewise never created). This is precisely the failure class all 11 prior reviews converged on, and the only registered P0 data-loss row (`U-59`) is closed.

**Why the harness does not catch it:** T7 runs Explode exactly once, in a fresh `QTemporaryDir`, and only asserts the happy path plus the lying-engine refusal. **No case re-runs Explode over an existing frame set, and no case cancels during Explode.** The gap is structural, not an oversight in the assertions.

**Fix** — guard the frame prefix the same way (see the pasteable block for the full diff; the short form is to make `guard_output` cover Explode, have the engine write to `prefix + ".gs-partial"`, then promote each rewritten frame after `verify_explode_frames` passes). Minimum viable interim fix, if the full promote is too large: refuse the run when the prefix directory already contains frames and the user has not confirmed, plus surface `"Cancelled."` as `"Cancelled — N frame(s) may be incomplete."`.

**Confidence: CONFIRMED** (read both files).

---

**[MEDIUM] data-loss — `src/qtui/MainWindow.cpp` (`onProcessFinished()`, promote-failure branch) + `onProcessFinished()` success branch + `cancelRun()`**
**Status: NEW**

```
 // Verify the isolated write before replacing the user's previous output.
 if (!pendingOutput_.isEmpty()) {
 const std::string verificationError =
 gs::verify_output(pendingPartial_.toStdString(), partialSnapshot_);
 ...
 const std::string promotionError =
 gs::promote_partial(pendingPartial_.toStdString(), pendingOutput_.toStdString());
 if (!promotionError.empty()) {
 gs::discard_partial(pendingPartial_.toStdString());
 pendingPartial_.clear();
```

**Mechanism.** The guard is correct in the common case, but every non-success exit **deletes the partial** — including the case where the partial is *fully written and fully verified* and the promotion alone failed. `promote_partial` is a single `std::filesystem::rename`; on Windows that is `MoveFileExW` under the hood and it fails for ordinary reasons (AV scanner holding a handle, the target open in another app, an antivirus filter driver). The result: a good, verified, renderable GIF is deleted, the old target is kept, and the user is told only *"Could not promote output."*

**Second, narrower path:** `cancelRun()` unconditionally does `gs::discard_partial(pendingPartial_.toStdString())` **before** checking whether a verified partial is awaiting promotion. If Cancel lands after the engine exited but before `finished()` is delivered, the verified partial is destroyed and the status reads `"Cancelled."` — a successful run reported as a cancellation, with its work deleted.

**Scenario.** (a) Target `out.gif` is open in Preview Pane/another viewer on Windows; run completes; rename fails; the new GIF is deleted. (b) Engine exits at t=0.4 s on a 4,800-frame GIF; the user hits Cancel in the sub-100 ms window before the event loop delivers `finished()`; verified work is discarded and reported as cancelled.

**Fix.** Never delete a *verified* partial. On promotion failure, rename it to a recovery name and tell the user where it is. On cancel, promote a verified partial instead of discarding it. Snippet in the pasteable block.

**Confidence: CONFIRMED** for the promote-failure deletion (read). **SUSPECTED** for the cancel race — the window is real but narrow, and I could not execute it here (no Qt6).

---

**[LOW] claim-vs-code — `src/qtui/MainWindow.h` header comment**
**Status: NEW (the comment); confirms U-12 `OPEN` (the blocking calls)**

```
// Preview pipeline: any control/queue/selection change restarts a 1200 ms
// debounce timer; on fire, a SEPARATE QProcess re-encodes the selected file
// to a temp dir. Never blocks the UI thread (S3-7 rule); main runs pause
// previewing and kill any in-flight preview process.
```

**Mechanism — U-12 is CONFIRMED, and I can name all five sites.** The same session's `STATUS.md` says U-12 is *"still blocks the UI thread in 5 places — up to 5 s per run start"*. That is exactly right, and the claim in the header is the mirror image of the code:

1. `MainWindow::~MainWindow()` — `process_->waitForFinished(2000)`
2. `runCommand()` batch branch — `if (!process_->waitForStarted(5000))`
3. `runCommand()` single branch — `if (!process_->waitForStarted(5000))`
4. `cancelRun()` — `process_->waitForFinished(3000)`
5. `killPreview()` — `previewProcess_->waitForFinished(1000)`, and `killPreview()` is called from `startPreview()` **and** from `setBusy(true)`, i.e. on the UI thread during the first frame of every run.

The header comment is therefore false in the file that declares the run semantics. It is the same "claim the code does not honour" class as U-19/U-20/U-12, and it is the comment a future maintainer will trust.

**Fix.** Reword the comment to the truth (bounded waits during *start* and *cancel*, not during the run), and add `P1-24` as the tracked fix for the blocking itself.

**Confidence: CONFIRMED** (all five sites read).

---

## PHASE 2 — Qt GUI claims sweep

**[MEDIUM] silent-setting-loss — `web/command.mjs` (`finite`, `buildArgs`) vs `web/validate.mjs` (`num`, `validate`)**
**Status: NEW — and it means the U-78 fix is INCOMPLETE**

```
// command.mjs
const finite = (value) => typeof value === "number" && Number.isFinite(value);
...
 if (finite(s.color_count) && s.color_count >= 2 && s.color_count <= 256) {
 add("-k"); add(i2s(s.color_count));
 }
```
```
// validate.mjs
 const num = (field, v, dflt) => {
 if (v === undefined || v === null || v === "") return dflt;
 const n = Number(v);
 if (Number.isFinite(n)) return n;
 add(field, v, "must be a finite number (or unset)");
 return dflt;
 };
```

**Mechanism.** The two modules disagree about the *type* of a numeric setting. `validate.mjs` **coerces** — `Number("5")` is finite, so the string `"5"` produces **no warning**. `command.mjs` **requires** `typeof value === "number"`, so the same string fails `finite()` and the flag is **silently omitted**. Validator says "clean"; builder drops the setting. The user gets HTTP 200, a rendered GIF, and a control they set that was never applied. That is the repo's stated worst failure class (false success) reached through the validator/builder seam.

Same silent-drop for `disposal`, `lossy`, `delay_cs`, `optimize_level`, `threads` (a string `"4"` works only by accident, via `>` coercion, while `=== 0` fails), and `loopcount` (strict `=== -2` / `=== 0` comparisons mean a string `"-2"` emits **no** loop flag at all).

**A second, independent instance of the fixed class:** `app.js`'s `settings()` never populates `gamma`, so it arrives `undefined` — fine. But `buildArgs` has
```
if (s.gamma_str) add("--gamma=" + s.gamma_str);
else if (s.gamma >= 0) add("--gamma=" + fmtDouble(s.gamma));
```
with **no** `finite()` guard. An empty gamma field is `""`, and `"" >= 0` is **`true`** in JavaScript, so an empty field emits `--gamma=0`. U-87 fixed exactly this "emptied field becomes a real 0" bug for six fields via `numOrNull`; **gamma was left out of that list**, and `Validate.h` documents that the engine *accepts* `--gamma=0` — so it passes validation and silently changes the image.

**Scope check, stated honestly: the shipped browser UI is not affected** — `app.js` routes its six numeric fields through `numOrNull`, so they arrive as real numbers. The exposure is any **direct `POST /run` client**. `GS_WEB_HOST=0.0.0.0` is a documented, owner-opted-in, unauthenticated surface, and `curl -d '{"settings":{"disposal":"5"}}'` is the trigger.

**⚠️ One check I could not do:** **`web/server.mjs` was not read** (1,050 lines, and I ran out of budget). If `server.mjs` normalises types before calling `buildArgs`, this is unreachable via HTTP. **That is a 2-minute grep and it is the first thing to check** — it decides whether this is MEDIUM or LOW.

**Fix.** One coercion point, used by both modules. Snippet in the pasteable block.

**Confidence: CONFIRMED** for the module-level mismatch (both quoted). **UNVERIFIED** for HTTP reachability pending `server.mjs`.

---

## PHASE 3 — VERIFICATION PASS over `DONE`/`OPEN` rows

I re-derived these from source rather than from the register.

**U-71 `OPEN` — CONFIRMED verbatim.** `src/core/ProcessRunner.h`, `run_argv`, Windows branch:
```
 if (!ok) return 1;
 return static_cast (code) & 0xff;
```
The `& 0xff` is exactly as filed. An NTSTATUS like `0xC0000005` (access violation) becomes `0x05` — **a crash reported as success**. On the GUI's Windows path this means a crashed engine can reach `verify_output`; on the CLI it can reach the promotion path. The existing snapshot-diff guard catches most of it, which is why this is `P2` and not `P0` — the register's severity is right.

**U-70 `OPEN` — CONFIRMED, first half only.** `src/core/EngineLocator.h` declares the boundary helper:
```
inline std::filesystem::path u8path_compat(const std::string& utf8) {
#ifdef _WIN32
 return std::filesystem::u8path(utf8);
```
and `MainWindow::ensureEngine()` honours it:
```
 if (gs::path_is_executable(gs::u8path_compat(enginePath_.toStdString()))) {
```
but `MainWindow::startPreview()` does not:
```
 if (!gs::path_is_executable(enginePath_.toStdString())) {
```
`path_is_executable` takes `const fs::path&`, so the second line converts the UTF-8 `std::string` through the toolchain's native narrow encoding — precisely the mangling `WinUnicode.h` exists to prevent (N-04). On MinGW a non-ASCII engine path previews as "engine not found" while `ensureEngine()` says Ready. **U-70's "preview engine check bypasses UTF-8 boundary" is correct.** I did not reach U-70's second half (cancelling lifetime).

**U-72 `OPEN` — CONFIRMED.**
```
 cancelling_ = true;
 if (process_ && process_->state() != QProcess::NotRunning) {
 process_->kill();
 process_->waitForFinished(3000);
 }
 cancelling_ = false;
```
If `waitForFinished` times out, `cancelling_` is `false` when `finished()` later arrives, and `onProcessFinished()` takes the failure branch and shows a dialog *after* `"Cancelled."`. Exactly as filed.

**U-55 `OPEN` — CONFIRMED.** `OutputPlan.h`, `path_key`:
```
 for (auto& c : s) {
 c = static_cast (std::tolower(static_cast (c)));
 if (c == '\\') c = '/';
 }
```
Bytewise `tolower` under `_WIN32`. Non-ASCII case collisions are missed on Windows. Register's `P1-35` is correctly scoped.

**[LOW] `NEW` — a test that cannot fail, pinned to an OPEN bug.** `tests/test_gui_offscreen.cpp`, T9:
```
 g_dialogs.clear(); // isolate the cancel window: no dialog may appear here
 x.cancel->click();
 CHECK_MSG(waitForStatus(w, QStringLiteral("Cancelled"), 10000), "status shows Cancelled");
 ...
 CHECK_MSG(g_dialogs.empty(), "cancel does not pop a spurious error dialog");
```
This is the natural test for U-72, and **it is structurally incapable of failing.** U-72 requires `waitForFinished(3000)` to time out. The harness always kills a cooperative, interruptible engine that dies in milliseconds, so the timeout never occurs and `cancelling_` is always still `true` when `finished()` arrives. The assertion is reachable, but it exercises the *pass* case only. **This is the Phase 3 pattern you asked for: a test that passes without exercising what it claims.** A test for U-72 needs an engine that ignores `SIGTERM` for >3 s.

**[LOW] `NEW` — a mis-specified assertion in T4.**
```
 QElapsedTimer clickTime;
 clickTime.start();
 x.run->click(); // async: must NOT block for the whole run
 const qint64 clickMs = clickTime.elapsed();
 CHECK_MSG(clickMs < 3000, "Run click returns fast (no waitForFinished block)");
```
`runCommand()` calls `waitForStarted(5000)`. The threshold (3,000 ms) is **tighter than the budget the code allows (5,000 ms)**, and the message names the wrong function — it is `waitForStarted`, not `waitForFinished`. So a correct-but-slow engine start fails the harness. That is a false-failure waiting to happen on a loaded CI runner, and it is also why U-12's real bound is invisible: the test asserts a bound the code never promised.

**[LOW] `NEW` — OutputPlan.h's header comment now contradicts shipped code.**
```
// Not implemented here, and why: temp-file + rename. It would only protect a
// PREVIOUS output from a crashed engine, and it perturbs the live command pane
// contract (the pane must show the command that actually runs). The two
// vectors that actually destroy data are closed by planning. See
// docs/audit/FIX_PICK_2026-09-10.md §1.4.
```
U-59 shipped exactly the "temp-file + rename" this paragraph says was rejected. Two problems: the design rationale is now stale in the file a reader opens first, and **`docs/audit/FIX_PICK_2026-09-10.md` does not exist** — the repo's `docs/` tree is `archive/ ci/ legal/ planning/ release/ screenshots/`. A decision record cited by the core planner's header is missing, so the "why not" reads as settled policy when the opposite shipped.

**[LOW] `NEW` — parity coverage gap.** `web/test/command.test.mjs` has 24 fixtures and **none sets `info: true`**, so `buildArgs`'s `if (s.info) add("--info");` has no cross-surface pin. One fixture closes it.

**`NEW` — the parity suite is NOT vacuous (positive result).** You asked whether it imports both sides or compares a fixture to a fixture. It executes the real binary and compares both directions:
```
 const conf = join(dir, "case.conf");
 writeFileSync(conf, saveSettingsLines(fx.s));
 const out = execFileSync(CLI, [conf, "--engine", ENGINE], { encoding: "utf8" });
 ...
 const cpp = cmdLine.slice(prefix.length);
 const js = toString(fx.s);
 if (cpp === js) {
```
`cpp` comes from the C++ parser reading a conf the **JS writer** produced; `js` comes from the JS builder on the original object. A field the writer drops makes the two diverge and the test **fails**. A guard rejects a mis-parsed line (`if (!cmdLine.startsWith(prefix))`). This is a genuine two-way check. The real weakness is *fixture coverage*, not vacuity — and the blank-`dither` divergence is invisible because `validate.mjs` documents, in a comment, that it deliberately has no rule where `Validate.h` has one.

---

## PHASE 5 — `run-paths.mjs` containment (the question you asked directly)

**Verdict: I could not bypass it, and the design is honest about its limits.** `assertContainedPath` is at **lines 31–39** (45-line file, counted):

```
export function assertContainedPath(dir, target, paths = path) {
 const root = paths.resolve(dir);
 const resolved = paths.resolve(target);
 const rel = paths.relative(root, resolved);
 if (!rel || rel === ".." || rel.startsWith(`..${paths.sep}`) || paths.isAbsolute(rel)) {
 throw new Error("refusing to run: output path is outside the request temporary directory");
 }
 return resolved;
}
```

Attempted bypasses and why each fails: `../x` → `rel` starts with `../`; an absolute path → rejected earlier by the `/` and `\` ban in `uploadNameError` (line 10); `C:foo` → `:` is in the banned class; `""`/`"."`/`".."` → rejected by name admission; a `~` expansion does not happen because the path is never shell-expanded; a trailing-dot Windows quirk is refused at line 13. The `!rel` clause correctly refuses `target === root`. `paths.isAbsolute(rel)` catches the Windows cross-drive case. The collision key is NFC + case-fold (line 24), which over-refuses on case-sensitive hosts but never under-refuses.

The one residual risk is real and **already disclosed in the file**: *"Lexical containment assumes our private mkdtemp and a trusted engine; it is not a sandbox for a malicious engine or a hostile local symlink writer."* That is the correct scope statement, and a hostile input *filename* cannot reach it. `GS-202`/`P0-6` is honestly closed.

---

## PHASE 7 — Do the gates gate?

**This is where I found the most actionable problems, and they are all the U-82 class you predicted.**

**[HIGH] gate-integrity — `scripts/verify_audit.sh` collapses 20 harness cases into one bit.**
```
 if GS_ENGINE="$ENGINE" GS_TEST_REF_DIR="$self/../../reference_code/gifsicle" \
 QT_QPA_PLATFORM=offscreen "$work/gui/test_gui_offscreen" 2>/dev/null | tail -1 | grep -q "ALL GUI TESTS PASSED"; then
 ok "B1-B15" "offscreen GUI harness green (batch/merge/explode/cancel/close/dedupe/live pane)"
```
The suite's only assertion is that the last stdout line is the success banner. **Nothing asserts that T1…T20 still exist, or that a minimum number of `CHECK`s ran.** A future session can delete T14 (persistence), T17 (batch planning) or T19 (atomic save) outright, the harness still prints `ALL GUI TESTS PASSED`, and `B1-B15` stays green in the register. `STATUS.md` cites `B1-B15` as the proof for `U-34`/`U-47`/`U-16`/`U-01`/`U-45` — so deleting a case silently invalidates five `DONE` rows at once. The harness already prints per-stage banners (`std::printf("== T1 defaults + tabs ==\n")`) and maintains `g_checks`/`g_failures`, so the fix is a two-line grep. **This is the highest-value single change in the repo.** Snippet in the pasteable block.

**[MEDIUM] gate-integrity — E3's shell check is defeated by a keyword.**
```
if ! grep -rn "system(\|/bin/sh\|cmd\.exe\|sh -c" src/ 2>/dev/null \
 | grep -v "not for system()\|no shell\|NEVER\|never" \
 | grep -vE ':[0-9]+:[[:space:]]*(//|\*|/\*)' | grep -q .; then
 ok "E3" "no shell execution in src/"
```
The safety of the whole gate rests on a substring allow-list. **Any line containing the word "never" or "NEVER" is exempt from the shell check** — including a line that really calls `system()` and carries an apologetic comment. `ProcessRunner.h` documents this about itself, so it is known, but it means E3 cannot fail for the most likely way someone would introduce a shell. Fix: strip `//` and `/* */` comments properly, then match code lines only.

**[MEDIUM] gate-integrity — E7 verifies a doc literal, not behaviour.**
```
if grep -q "1/100 s" ../../PROJECT_VISION.md; then ok "E7" "delay unit documented as 1/100 s (flag map folded into PROJECT_VISION.md, S24)"; else bad "E7" "delay unit doc"; fi
```
This passes iff a Markdown file contains a fixed string. The *behavioural* claim (the GUI never says "ms") is genuinely covered — by T11's `CHECK(delayLabel && delayLabel->text().contains("1/100"))` — but E7 as written cannot detect a regression in the label. A reader of the register sees "E7 delay unit" and assumes code coverage. Rename it to `E7-doc` and add a real check, or delete it.

**[LOW] gate-integrity — three smaller ones.**
- `A1` is emitted **twice** with the same id (once for `build.sh`, once for the example conf), so any per-id tally in the docs is ambiguous. Gate-id uniqueness has no test.
- `DOC_GATE_CHECKS=2` is a hand-maintained constant that `check_docs.sh` reads back to keep the quoted gate totals honest. Add a third `F`-check without bumping it and the doc totals drift silently — **this is exactly the N-01 class that already bit this repo once** ("every doc quoted a gate count that was already wrong"). Fix: have `check_docs.sh` count the `ok`/`bad`/`skip` lines from the run rather than trusting a hand-written number.
- E9 has a permanent escape hatch: creating `docs/ci/PENDING_WORKFLOW_CHANGE.md` downgrades workflow-copy drift to `SKIP` forever. The file is absent today, so E9 is enforced — but the hatch is author-creatable and undated.

**`NEW` — a low-severity asymmetry in the CLI's verifier.** In `main.cpp` the pre-run snapshot is taken of the **target** (`output_before = gs::snapshot_output(s.output)`) but `verify_output` is then called on the **partial** (`output_partial`). The GUI snapshots the partial, so `before.exists` is always `false` there and the "unchanged" clause cannot fire. On the CLI a new partial whose size *and* mtime coincidentally equal the old target's trips a **false refusal** (rc=1, work discarded). Conservative direction, so not data loss — but it is a real mismatch, `OutputVerify.h` documents the clause as *"Identical rewrites within filesystem timestamp granularity fail conservatively"*, and the fix is one line: snapshot the partial, not the target.

---

## Coverage statement (restated, no spin)

**22 of 62 in-scope files DEEP. 0 READ-only. 39 PENDING. 1 BLOCKED.**

Every PENDING file, with its reason: all reached no further than the budget in §Method — `server.mjs` and `check_docs.sh` (the two highest-value unread files, 1,050 and ~1,500 lines), `smoke_cli.sh`, `SettingsIO.h`, `GifsicleCommand.h`, `test_gifsicle_command.cpp`, the remaining `T15–T20` of the GUI harness, and the other eight web suites. **This is not a full-coverage audit.** Phase 5's HTTP-layer questions (batch collision refusal, base64 admission, concurrent state) are **unanswered** — I only reached `run-paths.mjs`. Phase 6 is answered for `command.mjs` and **not** for `validate.mjs`'s suite. Phase 3 sampled 6 tracked rows (U-12, U-55, U-70, U-71, U-72, U-59), not the 20 requested.

**Prioritized fix order**

- **Today** — (1) register the Explode U-59 hole (Finding 1) and decide ship/do-not-ship on it; (2) the B1-B15 check-count gate (Finding 12); (3) the 2-minute `server.mjs` type-normalisation grep that decides Finding 5's severity.
- **This week** — never discard a verified partial (Finding 2); the `finite`/`num` coercion seam + the `--gamma=0` empty-field bug (Finding 5); T4's `clickMs < 3000` threshold (Finding 20); `--info` parity fixture; the `--info` CLI asymmetry one-liner.
- **This month** — `check_docs.sh` in CI with a real time budget (U-14/`P2-1`); `DOC_GATE_CHECKS` derived not declared; E3 comment-stripping; E7 rename; the Explode frame-guard; the remaining 39 files above.

Now the deliverable you asked for — the new problems and the fixes, in a block you can copy straight into a file.

---

````markdown
# Gifscythe — NEW findings + proposed fixes
# Source: independent full-file re-read on 2026-09-27, branch main @ 957c143
# Method: no local checkout available (fetch-only). Quotes are verbatim from the
# files as served by the GitHub raw endpoint. Line numbers are given only for
# web/run-paths.mjs and web/output-verify.mjs, which I counted exactly; larger
# files are cited by function anchor because no endpoint returns line numbers
# and I will not guess. Confirm anchors with a local checkout before committing.
#
# Repo rule: every NEW item below is UNTRIAGED and MUST be given a row in
# STATUS.md (state UNTRIAGED) plus a pending line in WORKLIST.md **in the same
# session**, or gate G18/the UNTRIAGED policy fails.

================================================================================
## PRIORITY 1 — SILENT DATA LOSS (the repo's top-ranked failure class)
================================================================================

### N-01  [HIGH] Explode is exempt from the U-59 partial guard on BOTH surfaces
Status: NEW (U-59 is DONE; this hole is outside the row's stated scope)
Files: working_code/gifscythe/src/qtui/MainWindow.cpp (runCommand),
       working_code/gifscythe/src/cli/main.cpp (main),
       working_code/gifscythe/tests/test_gui_offscreen.cpp (T7)

Quote — MainWindow.cpp, runCommand():
    pendingOutput_ = QString::fromStdString(settings.output);
    if (settings.mode != gs::Mode::Explode) {
    pendingPartial_ = QString::fromStdString(
    gs::partial_output_path(pendingOutput_.toStdString()));
    gs::discard_partial(pendingPartial_.toStdString()); // self-heal a prior hard kill
    partialSnapshot_ = gs::snapshot_output(pendingPartial_.toStdString());
    settings.output = pendingPartial_.toStdString();
    } else {
    pendingPartial_.clear();
    }

Quote — main.cpp:
    const bool guard_output =
    !s.output.empty() && !stream_output && s.mode != gs::Mode::Explode;

Mechanism: Explode writes prefix.NNN directly and truncates any pre-existing
frame in place. ExplodeVerify.h only *verifies* (snapshot diff); it never
protects. cancelRun() discards pendingPartial_, which is EMPTY for Explode, so
a cancel leaves half-truncated frames on disk and reports "Cancelled.".

Scenario: explode a.gif -> 12 good frames. Change a setting, re-run Explode,
press Cancel. a_frame.000 is truncated; 11 stale frames remain; the UI says
"Cancelled." The previous complete frame set is destroyed with no dialog.
Identical on the CLI via SIGTERM. T7 never re-runs Explode over an existing
frame set and never cancels during Explode, so the harness cannot see it.

Fix (minimal, honest, shippable today — status text instead of full promote):
  In MainWindow::cancelRun(), replace
      updateStatus(QStringLiteral("Cancelled."));
  with
      updateStatus(batchMode_ == gs::Mode::Explode
      ? QStringLiteral("Cancelled — frames already written may be incomplete.")
      : QStringLiteral("Cancelled."));
  and in onProcessFinished()'s Explode failure branch, append the same warning
  to the dialog. This does not PREVENT the loss; it makes it visible. Register
  the prevention as the next U-59 sub-row.

Fix (full prevention, next milestone): make guard_output cover Explode too.
  1. In main.cpp change the guard to drop the Explode exemption:
         const bool guard_output = !s.output.empty() && !stream_output;
     and set the engine prefix to `s.output + ".gs-partial"`, keeping
     explode_prefix_for() pointed at the PARTIAL.
  2. After verify_explode_frames(partial_prefix, before) passes, rename every
     frame `partial_prefix.NNN` -> `prefix.NNN` with std::filesystem::rename
     (same directory, so it is one step each), then discard the partial set.
  3. On ANY other outcome, leave the existing frames alone and say so.
  Add harness cases: (a) re-run Explode over an existing frame set, cancel
  mid-run, assert the original frames still decode; (b) same with a failing
  engine. Both are currently impossible to write, which is the point.

Confidence: CONFIRMED (all three files read this session)

================================================================================
## PRIORITY 2 — GATE INTEGRITY (the U-82 class; highest automation value)
================================================================================

### N-02  [HIGH] verify_audit.sh B1-B15 can pass with the harness gutted
Status: NEW
File: working_code/gifscythe/scripts/verify_audit.sh

Quote:
    if GS_ENGINE="$ENGINE" GS_TEST_REF_DIR="$self/../../reference_code/gifsicle" \
    QT_QPA_PLATFORM=offscreen "$work/gui/test_gui_offscreen" 2>/dev/null | tail -1 | grep -q "ALL GUI TESTS PASSED"; then
    ok "B1-B15" "offscreen GUI harness green (batch/merge/explode/cancel/close/dedupe/live pane)"

Mechanism: the only assertion is the final banner. Nothing asserts T1..T20 still
exist or that a minimum number of CHECKs ran. Deleting T14 (persistence),
T17 (batch planning) or T19 (atomic save) leaves the suite green. STATUS.md
cites B1-B15 as the proof for U-34, U-47, U-16, U-01 and U-45, so one deleted
case silently invalidates five DONE rows at once.

Fix — replace the B1-B15 block with a version that counts (drop into
verify_audit.sh in place of the current one; the harness already prints
"== Tn ..." per stage and counts into g_checks):
    gui_out="$(cd "$self" && GS_ENGINE="$ENGINE" \
      GS_TEST_REF_DIR="$self/../../reference_code/gifsicle" \
      QT_QPA_PLATFORM=offscreen "$work/gui/test_gui_offscreen" 2>&1)"
    gui_n="$(grep -c '^== T' <<<"$gui_out")"
    gui_checks="$(grep -oE '[0-9]+ checks' <<<"$gui_out" | grep -oE '^[0-9]+' | tail -1)"
    if grep -q "ALL GUI TESTS PASSED" <<<"$gui_out" \
      && [[ "${gui_n:-0}" -ge 20 ]] \
      && [[ "${gui_checks:-0}" -ge 300 ]]; then
      ok "B1-B20" "offscreen GUI harness green (${gui_checks} checks, ${gui_n} test blocks)"
    else
      bad "B1-B20" "GUI harness: banner/banner-count mismatch (${gui_checks:-?} checks, ${gui_n:-?} blocks)"
    fi
  Pick the floor by running the harness ONCE on a known-good checkout and
  setting the number ~5% below what it reports; then ratchet it up. If the
  harness does not print a total, add one line to its summary:
      std::printf("ALL GUI TESTS PASSED (%d checks, %d failures)\n", g_checks, g_failures);
  and grep for the numeric banner instead. The floor is the anti-vacuity
  property; a banner-only gate can never have it.

Falsification test for the rule itself (required before trusting it):
  (a) delete one T-block, run the gate, it MUST go red;
  (b) restore it, the gate MUST go green.
  If (a) stays green the new gate is vacuous and must not ship.

Confidence: CONFIRMED (read verify_audit.sh and the harness's stage prints)

### N-03  [MEDIUM] Gate E3's shell check is defeated by the word "never"
Status: NEW
File: scripts/verify_audit.sh

Quote:
    if ! grep -rn "system(\|/bin/sh\|cmd\.exe\|sh -c" src/ 2>/dev/null \
    | grep -v "not for system()\|no shell\|NEVER\|never" \
    | grep -vE ':[0-9]+:[[:space:]]*(//|\*|/\*)' | grep -q .; then
    ok "E3" "no shell execution in src/"

Mechanism: the entire guarantee rests on a substring allow-list. Any line
containing "never" or "NEVER" is exempt — including a line that really calls
system() and carries an apologetic comment. ProcessRunner.h documents this
about itself, so it is known; it still means E3 cannot fail for the most
likely way a shell gets introduced.

Fix: strip C/C++ comments first, then match code lines only. Replace the
pipeline with:
    if ! grep -rn "system(\|/bin/sh\|cmd\.exe\|sh -c" src/ 2>/dev/null \
      | sed -E 's://.*::; s:/\*.*\*/::g' \
      | grep -q .; then
      ok "E3" "no shell execution in src/ (comments stripped)"
    else bad "E3" "shell execution pattern found in src/"; fi
  (sed is already assumed available: the script uses awk/grep/diff throughout.)
Falsification: add `std::system(cmd); // never do this` to a scratch file in
src/, the gate MUST go red; remove it, MUST go green.

Confidence: CONFIRMED

### N-04  [LOW] Gate E7 verifies a Markdown literal, not behaviour
Status: NEW
File: scripts/verify_audit.sh
Quote:
    if grep -q "1/100 s" ../../PROJECT_VISION.md; then ok "E7" "delay unit documented as 1/100 s (flag map folded into PROJECT_VISION.md, S24)"; else bad "E7" "delay unit doc"; fi
Mechanism: passes iff a doc contains a fixed string. The real property (the GUI
never says "ms") IS covered by the harness T11 check
`CHECK(delayLabel && delayLabel->text().contains("1/100"))`, but E7 cannot
detect a label regression, and the register reads as if it could.
Fix: rename to "E7-doc", and add a behavioural sibling in the harness:
    CHECK(delayLabel && !delayLabel->text().contains("ms", Qt::CaseInsensitive));
  (T11 already has this line — promote it to its own named check so the
  register can cite a behavioural id.)
Confidence: CONFIRMED

### N-05  [LOW] DOC_GATE_CHECKS is a hand-maintained invariant with no test
Status: NEW
File: scripts/verify_audit.sh
Quote:
    DOC_GATE_CHECKS=2
    if [[ "${GS_SKIP_DOC_GATE:-0}" == "1" ]]; then
     : # nested run from check_docs.sh — do not recurse
    else
     if ./scripts/check_docs.sh --from-verify-audit > /tmp/vs_docs.log 2>&1; then
Mechanism: check_docs.sh runs verify_audit.sh to learn the real gate totals,
and verify_audit.sh's F1 verdict comes from check_docs.sh. The count the docs
quote is a hand-written constant. Add a third F-check without bumping it and
the quoted totals drift silently — this is the N-01 class that already bit
this repo once ("every doc quoted a gate count that was already wrong").
Fix: in verify_audit.sh, emit the live counts the doc gate can parse instead
of the constant, and have check_docs.sh read those:
    # at the end of verify_audit.sh, replacing the current summary line:
    echo "==> Done. $PASS passed, $FAIL failed, $SKIP skipped."
    printf 'GATE_TOTALS passed=%d failed=%d skipped=%d doc_gate_checks=%d\n' \
      "$PASS" "$FAIL" "$SKIP" "$DOC_GATE_CHECKS"
  and in check_docs.sh, parse GATE_TOTALS rather than trusting
  DOC_GATE_CHECKS. Then assert in CI: the totals in STATUS.md equal the
  numbers from the last run (that assertion is the real anti-drift gate).
Confidence: CONFIRMED (mechanism), SUSPECTED (that check_docs.sh reads it
  exactly this way — check_docs.sh was NOT read this session, 57 KB)

### N-06  [LOW] Gate id "A1" is emitted twice
Status: NEW
File: scripts/verify_audit.sh
Quote: `ok "A1" "build.sh green (engine+CLI+unit tests)"` ... and later
       `ok "A1" "example conf --run -> /tmp/gifscythe_demo.gif ..."`
Mechanism: any per-id tally is ambiguous, and no test asserts gate-id
uniqueness. Fix: rename the second to "A1b", and add to a gate-meta test:
    ids=$(grep -oE ' (ok|bad|skip) "[A-Z][0-9A-Za-z-]+"' scripts/verify_audit.sh | sort | uniq -d)
    [[ -z "$ids" ]] || { echo "duplicate gate ids: $ids"; exit 1; }
Confidence: CONFIRMED

================================================================================
## PRIORITY 3 — SILENT SETTINGS LOSS (web; false success, no data loss)
================================================================================

### N-07  [MEDIUM] command.mjs requires typeof number; validate.mjs coerces
Status: NEW. THIS MEANS THE U-78 FIX IS INCOMPLETE.
Files: web/command.mjs, web/validate.mjs

Quote — command.mjs:
    const finite = (value) => typeof value === "number" && Number.isFinite(value);
    ...
     if (finite(s.color_count) && s.color_count >= 2 && s.color_count <= 256) {
     add("-k"); add(i2s(s.color_count));
     }
Quote — validate.mjs:
     const num = (field, v, dflt) => {
     if (v === undefined || v === null || v === "") return dflt;
     const n = Number(v);
     if (Number.isFinite(n)) return n;
     add(field, v, "must be a finite number (or unset)");
     return dflt;
     };

Mechanism: the validator coerces, the builder does not. `"5"` passes validation
with NO warning and is then silently dropped by the builder. HTTP 200, a real
GIF, and a setting the user set that was never applied. Same for disposal,
lossy, delay_cs, optimize_level, threads (string "4" works only by `>`
coercion; `=== 0` fails) and loopcount (strict `=== -2` means a string "-2"
emits NO loop flag at all).
The shipped browser UI is NOT affected (app.js routes its six numeric fields
through numOrNull). The exposure is any direct POST /run client.

>>> DO THIS FIRST (2 minutes, decides the severity) <<<
  grep -n "numOrNull\|Number(\|typeof" web/server.mjs | head -40
  and find where server.mjs hands settings to buildArgs. If it normalises
  types, this is LOW (unreachable via HTTP). If it does not, it is MEDIUM on
  the documented GS_WEB_HOST=0.0.0.0 unauthenticated surface.
  web/server.mjs was NOT read this session — do not skip this.

Fix (single coercion point, used by BOTH modules) — web/command.mjs:
    // one place decides what "a number setting" means; both buildArgs and
    // validate.mjs import it, so they cannot disagree again.
    export const numOrNull = (value) => {
      if (value === undefined || value === null || value === "") return null;
      const n = Number(value);
      return Number.isFinite(n) ? n : null;
    };
    const finite = (value) => {
      const n = numOrNull(value);
      return n !== null;
    };
  then in buildArgs, read the coerced value once per field:
     if (s.color_count !== undefined && s.color_count !== null && s.color_count !== "") {
       const k = numOrNull(s.color_count);
       if (k !== null && k >= 2 && k <= 256) { add("-k"); add(i2s(k)); }
     }
  and in validate.mjs's num(), keep the coercion but ALSO use numOrNull so a
  non-numeric string produces the same single named warning.
Add a parity fixture so the string form is pinned, e.g. in command.test.mjs:
    { name: "numeric string still reaches the engine (N-07)",
      s: { mode: "auto", inputs: ["/tmp/parity/in.gif"],
           disposal: "3", loopcount: "-2", optimize_level: "2" } },
  (Note: saveSettingsLines writes them as strings into the conf, the C++ parser
  reads them as integers, and the JS builder now coerces too — all three agree.
  Verify the C++ side actually parses those keys; if not, coerce before
  saveSettingsLines instead of in the fixture.)

Confidence: CONFIRMED for the module mismatch; reachability UNVERIFIED
  pending the server.mjs grep above

### N-08  [MEDIUM] An emptied gamma field emits --gamma=0 (U-87 was incomplete)
Status: NEW
File: web/command.mjs
Quote:
     if (s.gamma_str) add("--gamma=" + s.gamma_str);
     else if (s.gamma >= 0) add("--gamma=" + fmtDouble(s.gamma));
Mechanism: `"" >= 0` is TRUE in JavaScript, and there is no finite() guard on
this line, so an empty gamma field becomes --gamma=0. Validate.h documents
that the engine ACCEPTS --gamma=0, so it passes validation and silently
changes the image. U-87 fixed exactly this class for six fields via
numOrNull and did not include gamma.
Scenario: the API client (or a future web control) sends settings.gamma = "".
The run succeeds, the returned GIF is gamma-corrected at 0, and nothing warns.
Fix: use the N-07 coercion point —
     else { const g = numOrNull(s.gamma); if (g !== null && g >= 0) add("--gamma=" + fmtDouble(g)); }
  and add "gamma" to the six-field numOrNull list in app.js if a control is
  ever added.
Confidence: CONFIRMED

================================================================================
## PRIORITY 4 — WASTING VERIFIED WORK
================================================================================

### N-09  [MEDIUM] A verified partial is DELETED when promotion fails
Status: NEW
File: src/qtui/MainWindow.cpp, onProcessFinished()
Quote:
     const std::string promotionError =
     gs::promote_partial(pendingPartial_.toStdString(), pendingOutput_.toStdString());
     if (!promotionError.empty()) {
     gs::discard_partial(pendingPartial_.toStdString());
Mechanism: promote_partial is a single std::filesystem::rename. On Windows it
fails for ordinary reasons (AV holding a handle, the target open elsewhere).
The partial is fully written and fully verified at that point — the code
throws it away, keeps the stale target, and says "Could not promote output."
Scenario: out.gif is open in another viewer; run completes; rename fails; a
good GIF is deleted and the user is told nothing useful.
Fix: on promotion failure, RENAME the partial to a recovery name and tell the
user where it is. Never discard verified bytes.
     if (!promotionError.empty()) {
     const QString rescue = pendingOutput_ + QStringLiteral(".gs-keep");
     std::error_code ec;
     fs::rename(u8path_compat(pendingPartial_.toStdString()),
     u8path_compat(rescue.toStdString()), ec);
     pendingPartial_.clear();
     setBusy(false); ...
     updateStatus(QStringLiteral("Could not replace the destination — "
     "your new file was kept at %1").arg(rescue));
     QMessageBox::warning(this, QStringLiteral("Gifscythe"),
     QStringLiteral("The destination could not be replaced (%1).\n\n"
     "The verified result was NOT deleted — it is here:\n%2")
     .arg(promotionError.c_str(), rescue));
     return;
     }
  (Apply the same rule in main.cpp: on promote failure, keep the partial and
  name it on stderr instead of discard_partial.)
Add a harness case: make promotion fail deterministically. Easiest reliable
trick on Linux is to make the destination a read-only DIRECTORY entry —
rename(2) then fails with EISDIR/ENOTDIR while the partial is intact.
Confidence: CONFIRMED (read)

### N-10  [MEDIUM] cancelRun() deletes a verified partial awaiting promotion
Status: NEW
File: src/qtui/MainWindow.cpp, cancelRun()
Quote:
     cancelling_ = true;
     if (process_ && process_->state() != QProcess::NotRunning) {
     process_->kill();
     process_->waitForFinished(3000);
     }
     cancelling_ = false;
     gs::discard_partial(pendingPartial_.toStdString());
Mechanism: if Cancel lands after the engine exited but before finished() is
delivered, the verified partial is destroyed and the status reads "Cancelled."
— a successful run reported as a cancellation, with its work deleted.
Scenario: engine exits at t=0.4 s on a 4,800-frame GIF; the user hits Cancel
in the sub-100 ms window before the event loop delivers finished().
Fix: make the discard conditional on the run not having succeeded. Cheapest
correct version — have onProcessFinished() mark the partial verified before
promoting, and have cancelRun() promote-or-keep instead of discard:
     if (!pendingPartial_.isEmpty()) {
       // Verified? Then it is real work: never throw it away on a cancel.
       const auto e = gs::verify_output(pendingPartial_.toStdString(), partialSnapshot_);
       if (e.empty()) {
         const auto pe = gs::promote_partial(pendingPartial_.toStdString(),
         pendingOutput_.toStdString());
         if (pe.empty())
           updateStatus(QStringLiteral("Cancelled after the file was written — "
           "the result was kept."));
       }
       gs::discard_partial(pendingPartial_.toStdString());
     }
     pendingPartial_.clear();
Confidence: SUSPECTED (read; the window is real but narrow and I could not
  execute it — no Qt6 in this session). Verify with a harness case that
  clicks Cancel in a tight loop right after waitForStatus sees the engine
  stop.

================================================================================
## PRIORITY 5 — CLAIMS THE CODE DOES NOT HONOUR (U-19/U-20/U-12 class)
================================================================================

### N-11  [LOW] MainWindow.h says "Never blocks the UI thread" — five sites do
Status: NEW (the comment). CONFIRMS U-12 OPEN (the blocking calls).
File: src/qtui/MainWindow.h
Quote:
    // to a temp dir. Never blocks the UI thread (S3-7 rule); main runs pause
    // previewing and kill any in-flight preview process.
Mechanism: five blocking waits run on the UI thread, exactly as U-12 records:
  MainWindow::~MainWindow()            process_->waitForFinished(2000)
  runCommand() batch branch            process_->waitForStarted(5000)
  runCommand() single branch           process_->waitForStarted(5000)
  cancelRun()                          process_->waitForFinished(3000)
  killPreview()                        previewProcess_->waitForFinished(1000)
killPreview() is called from startPreview() AND from setBusy(true), i.e. on
the first frame of every run. The comment is the exact inverse of the code and
sits at the top of the file that declares the run semantics.
Fix: reword to the truth —
    // Bounded waits DO occur on the UI thread, during start (up to 5 s) and
    // cancel (up to 3 s); the run itself never blocks. Tracked as U-12/P1-24.
  Leave the blocking itself to the existing U-12 row; this is only the claim.
Confidence: CONFIRMED (all five sites read)

### N-12  [LOW] OutputPlan.h's header says tmp+rename was rejected; it shipped
Status: NEW
File: src/core/OutputPlan.h
Quote:
    // Not implemented here, and why: temp-file + rename. It would only protect a
    // PREVIOUS output from a crashed engine, and it perturbs the live command pane
    // contract (the pane must show the command that actually runs). The two
    // vectors that actually destroy data are closed by planning. See
    // docs/audit/FIX_PICK_2026-09-10.md §1.4.
Mechanism: U-59 shipped exactly the "temp-file + rename" this paragraph
rejects, in OutputVerify.h + main.cpp + MainWindow.cpp. Worse, the cited
decision record does not exist: docs/ contains only archive/, ci/, legal/,
planning/, release/, screenshots/ — there is no docs/audit/FIX_PICK_2026-09-10.md.
A reader opening the core planner first is told a rejected design is settled
policy, and is pointed at a file that is not in the repo.
Fix: replace the paragraph with the truth and a live reference:
    // Temp-file + rename DID ship, as U-59/P0-7: OutputVerify.h holds
    // partial_output_path/redirect_output_operand/promote_partial/
    // discard_partial, and both the CLI and the GUI route every real-file
    // output through a same-directory .gs-partial. Planning (above) and the
    // partial guard solve DIFFERENT problems: planning refuses an unsafe
    // target BEFORE the run; the guard preserves the previous bytes when a
    // run is cancelled or fails. Neither replaces the other.
    // Still open: the Explode prefix is exempt from the guard (see STATUS.md
    // U-59 remainder / the N-01 row in this audit).
  And either restore docs/audit/FIX_PICK_2026-09-10.md from git history or
  delete the citation. A gate that fails on a broken in-code doc reference
  would stop this class recurring (see the Semgrep list in the audit report).
Confidence: CONFIRMED (the comment contradicts shipped code; the repo tree
  from the GitHub API shows no docs/audit/)

### N-13  [LOW] The CLI snapshots the target but verifies the partial
Status: NEW
File: src/cli/main.cpp
Quote:
    if (verify_file) {
    output_before = gs::snapshot_output(s.output);
    ...
    if (rc == 0) {
    if (verify_file) output_error = gs::verify_output(output_partial, output_before);
Mechanism: `before` describes the pre-existing TARGET while verify_output
reads the PARTIAL. A new partial whose size AND mtime coincidentally match the
old target trips "output is unchanged since the pre-run snapshot" -> rc=1 and
the good partial is discarded. Conservative direction (never a false success),
so LOW — but the GUI does it correctly (it snapshots the partial after
discarding it, so before.exists is always false) and the two surfaces
disagree for no reason.
Fix: one line —
    if (verify_file) {
    output_before = gs::snapshot_output(output_partial);
  Guard the ordering: output_partial is computed later in main(); move the
  snapshot to just after `output_partial = gs::partial_output_path(s.output)`
  and just after `gs::discard_partial(output_partial)`, which is where the GUI
  takes it.
Confidence: CONFIRMED (read)

================================================================================
## PRIORITY 6 — TESTS THAT CANNOT FAIL
================================================================================

### N-14  [LOW] T9's "no spurious error dialog" cannot detect U-72
Status: NEW. Confirms U-72 OPEN.
File: tests/test_gui_offscreen.cpp, T9
Quote:
     g_dialogs.clear(); // isolate the cancel window: no dialog may appear here
     x.cancel->click();
     CHECK_MSG(waitForStatus(w, QStringLiteral("Cancelled"), 10000), "status shows Cancelled");
     ...
     CHECK_MSG(g_dialogs.empty(), "cancel does not pop a spurious error dialog");
Mechanism: U-72 requires cancelRun()'s `waitForFinished(3000)` to TIME OUT, so
that cancelling_ is already false when finished() arrives. The harness always
kills a cooperative engine that dies in milliseconds, so the timeout never
happens and the assertion only ever exercises the pass case. Reachable, but
structurally incapable of failing.
Fix: build a third fixture, tests/fake_engine_ignore_term.cpp:
    // Ignores SIGTERM for ~6 s, then exits 9. Makes cancelRun()'s 3 s
    // waitForFinished time out, which is the ONLY way to reach U-72.
    #include <csignal>
    #include <thread>
    #include <chrono>
    #include <cstdlib>
    int main(int argc, char** argv) {
      std::string output;
      for (int i = 1; i + 1 < argc; ++i)
        if (std::string(argv[i]) == "-o") { output = argv[i + 1]; break; }
      if (!output.empty()) { std::FILE* f = std::fopen(output.c_str(), "wb");
        if (f) { std::fputs("partial", f); std::fclose(f); } }
      std::signal(SIGTERM, SIG_IGN);
      std::this_thread::sleep_for(std::chrono::seconds(6));
      return 9;
    }
  Add it to CMakeLists.txt next to the other fixtures AND to
  add_dependencies(test_gui_offscreen ...). Add a T9 subcase that points
  GS_ENGINE at it, clicks Cancel, and asserts the status is "Cancelled" and
  that no failure dialog appears AFTER it.
  While there: also fix U-72 itself (the one-line cause):
     cancelling_ = false;   -> only clear it when the wait actually finished:
     const bool done = process_->waitForFinished(3000);
     if (done) cancelling_ = false;
  Confidence: CONFIRMED (the test cannot fail); the FIX is unexecuted here.

### N-15  [LOW] T4's run-start threshold is tighter than the code's own budget
Status: NEW
File: tests/test_gui_offscreen.cpp, T4
Quote:
     QElapsedTimer clickTime;
     clickTime.start();
     x.run->click(); // async: must NOT block for the whole run
     const qint64 clickMs = clickTime.elapsed();
     CHECK_MSG(clickMs < 3000, "Run click returns fast (no waitForFinished block)");
Mechanism: runCommand() calls waitForStarted(5000). The assertion caps at
3000 ms — tighter than the 5000 ms the code allows — and the message names
waitForFinished, which is not what blocks. A correct-but-slow engine start
fails the harness on a loaded runner, while the real U-12 bound stays
invisible.
Fix: measure liveness instead of a wall-clock guess —
     CHECK_MSG(clickMs < 6000, "Run click returns without a blocking wait "
     "(code allows waitForStarted(5000))");
  and, better, assert the event loop ticked during the click:
     int ticks = 0; QTimer ticker; ticker.setInterval(10);
     QObject::connect(&ticker, &QTimer::timeout, [&ticks]() { ++ticks; });
     ticker.start(); x.run->click(); ticker.stop();
     CHECK_MSG(ticks > 0 || clickMs < 50, "UI thread not blocked by the run start");
Confidence: CONFIRMED

### N-16  [LOW] --info has no cross-surface parity fixture
Status: NEW
File: web/test/command.test.mjs
Mechanism: 24 fixtures, none sets `info: true`, so
`if (s.info) add("--info");` in buildArgs has no C++ pin. One fixture closes
it:
     { name: "info mode emits --info (N-16)",
       s: { mode: "auto", info: true, inputs: ["/tmp/parity/in.gif"] } },
Confidence: CONFIRMED (all 24 fixtures read; none sets info)

================================================================================
## PRIORITY 7 — PARITY-SUITE BLIND SPOT (not vacuous, but incomplete)
================================================================================

### N-17  [LOW] Blank dither: C++ warns, JS does not — and the exclusion is invisible
Status: NEW
Files: src/core/Validate.h, web/validate.mjs
Quote — Validate.h:
    if (!s.dither_method.empty() && s.dither_method.find_first_not_of(" \t") == std::string::npos) {
     add("dither", s.dither_method, "dither method name is blank (the engine reports it and exits 0 anyway)");
    }
Quote — web/validate.mjs (a comment, not a rule):
    // A blank value never reaches the settings model either: the loader trims
    // it, so a rule for it would be dead on the C++ side and would break the
    // parity this file's test enforces.
Mechanism: the two mirrors legitimately differ here, and validate.test.mjs
presumably excludes the case — but the exclusion lives in a COMMENT, so nothing
asserts it stays excluded. Delete the C++ rule and the parity suite still
passes; the divergence is discovered by a user, not by a gate.
Fix: make the exclusion explicit and asserted. In web/test/validate.test.mjs,
add a named case that asserts the KNOWN divergence rather than omitting it:
     // KNOWN, ASSERTED DIVERGENCE (N-17): Validate.h warns on a blank
     // dither_method; validate.mjs deliberately does not, because the JS
     // settings model trims the value before it can be blank. If the C++
     // rule is removed, this assertion FAILS and the divergence is revisited.
     t.assertDeepEqual(validate({ mode: "auto", dither_method: "  ",
     inputs: ["/tmp/parity/in.gif"] }), []);
     // and pin the C++ side still warns by running the real CLI --strict on a
     // conf with `dither = "  "` and expecting exit 3.
Confidence: CONFIRMED

================================================================================
## WHAT I COULD NOT AUDIT — read this before treating the list as complete
================================================================================
* web/server.mjs (1,050 lines) — NOT READ. Phase 5's HTTP questions (batch
  collision refusal, base64 admission, concurrent state machine) are
  UNANSWERED. Check N-07's severity here first.
* scripts/check_docs.sh (~1,500 lines) — NOT READ. The biggest single gap.
* scripts/smoke_cli.sh, SettingsIO.h, GifsicleCommand.h, GifsicleSettings.h,
  OutputName.h, and the GUI harness T15-T20 — NOT READ.
* Eight of the nine web/test suites — NOT READ, so "no vacuous test" is
  claimed for command.test.mjs ONLY.
* Line numbers are anchors, not line numbers, everywhere except run-paths.mjs.
  Confirm each with: grep -n '<first words of the quote>' <file>
* STATUS.md was read in full (169 rows) for dedup. COMPILED_AUDIT.md §5 was
  NOT opened — a "NEW" here could in principle overlap a §5 row that STATUS.md
  summarises. Before filing, grep COMPILED_AUDIT.md for the mechanism.
````

---

**Two notes on my own work, since you asked for confidence-tiered output only.**

Everything in the pasteable block marked **CONFIRMED** rests on a quote I copied from a file I opened in this session. The two **SUSPECTED** items (N-10's cancel race, N-05's `check_docs.sh` read path) are labelled as such because I could not close them without executing code or reading a 57 KB file — do not treat them as actionable until the 2-minute greps named inline are run. Nothing in that block is UNVERIFIED-and-still-listed-as-a-finding; the one genuinely unverified claim (N-07's HTTP reachability) is presented as a question to answer first, not as a result.

And the single most important thing in this report is not a bug: **the U-59 fix itself is sound and the offscreen harness genuinely proves it** — the cancel test kills a real 4,800-frame encode and asserts the old bytes survive. The hole is that Explode was never inside the guard, and the one place where a test claims to cover a known-open bug (U-72) provably cannot fail.