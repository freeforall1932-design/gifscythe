# Patch series review — 5 patches (U-71, U-57, U-72, U-70, U-58)

Reviewed and applied on branch `arena/9c0b8a7f-gifscythe`, base `4430a28`.
Every claim below was measured in this working tree; the command that produced
it is named.

## Verdict

All five patches are now applied as five commits and are **semantically
correct**. Three of the five were **structurally invalid as submitted** and
could not be applied at all until one count field in each was corrected. No
code was added, removed or reordered beyond those three count fields.

```
76405e2 P1-38 / U-58: batch continuation jobs must run the batch-start settings snapshot
f767246 P1-42 / U-70: preview engine probe must cross the u8path_compat boundary
926f810 P1-42 / U-72: latch the cancel flag — consume it in onProcessFinished
de769fb P1-37 / U-57: wasm page must never read a stale /out.gif
cce236a P2-17 / U-71: never mask the Windows exit code with 0xff
5565ed0 Fix malformed hunk headers in submitted patch series 0001/0004/0005
4430a28 Add files via upload
```

## What was broken and what I changed

`git apply --check` on the series as submitted:

| patch | as submitted | git's own words |
|---|---|---|
| 0001 | **REJECTED** | `patch fragment without header at line 93` |
| 0002 | OK (offset +14) | — |
| 0003 | OK (offsets +436 / +435) | — |
| 0004 | **REJECTED** | `corrupt patch at line 65` |
| 0005 | **REJECTED** | `corrupt patch at line 116` |

Cause in all three cases: the `@@ -old,n +new,m @@` count fields did not match
the number of lines the hunk actually carries. Three single-field corrections,
in commit `5565ed0`:

```
0001 line  66   @@ -66,6  +66,25 @@  ->  @@ -66,7  +66,26 @@
0004 line  49   @@ -617,7 +617,14 @@ ->  @@ -617,7 +617,13 @@
0005 line 100   @@ -164,6 +164,15 @@ ->  @@ -164,6 +164,14 @@
```

Counts recomputed by hand from the hunk bodies and confirmed by the applied
result:

* 0001 hunk 1 — 4 leading context (`out += '"';` … blank) + 19 added + 3
  trailing context = 7 old / 26 new. The header said 6/25, so git stopped one
  line early and tripped over the next `@@`.
* 0004 hunk 1 — 6 context + 1 deleted + 6 comment lines + 1 replaced line =
  7 old / 13 new. The header said 14 new.
* 0005 hunk 3 — 6 context + 8 added (7 comment + the member) = 6 old / 14 new.
  The header said 15 new.

Patch bodies are byte-identical to what was submitted (`diff` against the
originals shows only those three lines). Two cosmetic consequences remain and
are harmless: the `--stat` blocks in 0004/0005 still carry the pre-correction
counts (8 insertions claimed vs 7 landed; 21 vs 20), and 0002's `--stat`
overstates wasm.js by 2/2 (33 changed lines claimed, 29 landed) even though
its hunks were correct. Diffstats are informational; they do not affect
application.

## Anchor mismatch — read this before trusting any line number

The series is anchored on `main@fe4f0a7`. **That commit does not exist in this
repository** (`git cat-file -t fe4f0a7` → `fatal: Not a valid object name`);
this clone's whole history is the single squashed commit `4430a28`. Every hunk
therefore landed at an offset:

| hunk | anchored at | landed at | offset |
|---|---|---|---|
| 0002 wasm.js #2/#3 | 128 / 148 | 142 / 170 | +14 |
| 0003 MainWindow.cpp #1/#2 | 503 / 548 | 939 / 993 | +436 / +435 |
| 0004 MainWindow.cpp #1 | 617 | 1179 | +562 |
| 0005 MainWindow.cpp #1/#2 | 456 / 572 | 843 / 1086 | +387 / +508 |
| 0005 MainWindow.h #1 | 164 | 189 | +25 |

Two things about that table are worth naming. First, **0003's two hunks and
0005's two hunks land at different offsets within the same file** (+436 vs
+435, +387 vs +508), which is only possible if the anchors were reconstructed
rather than taken from a real file — consistent with 0005's own disclosure
about its reconstructed runCommand hunk. Second, `git apply` runs at fuzz 0, so
an offset alone cannot cause a silent mis-application: it requires 3 exact
context lines either side of every change. I verified each anchor site by
reading the file rather than trusting the offset — see below.

## Per-patch semantic review

### 0001 — U-71 Windows exit-code mask (ProcessRunner.h)

Anchor verified: `out += '"';` really is line 66, `return static_cast<int>(code)
& 0xff;` really is line 123, and the hunk's second `@@ -120,...` context
(`CloseHandle(pi.hProcess);`) really is line 120.

* `WinExitClass` / `classify_windows_exit_code` land **outside** the
  `#ifdef _WIN32` block, so the classifier compiles on Linux — the property the
  whole proof depends on. Confirmed by the probe actually building here.
* `std::fprintf` / `stderr` need `<cstdio>`; already included at line 27.
* `%08lX` matches the `unsigned long` argument on both LP64 and LLP64.
* Blast radius checked: `run_argv` has exactly **one** caller
  (`src/cli/main.cpp:666`), and it ends in `return rc; // honest 0..255` — no
  caller special-cases 255, so the new fixed deliverable cannot alias to
  anything meaningful. POSIX path untouched.
* Header contract "Returns the child's exit code 0..255" still holds.

### 0002 — U-57 stale `/out.gif` (web/wasm)

* `clearRunArtifacts(M.FS, ["/in.gif", "/out.gif"])` is placed **before**
  `M.FS.writeFile("/in.gif", …)` in the applied file — order is the whole fix,
  and it is right.
* Both original status strings survive verbatim; `reason === "missing"` maps to
  the old catch-branch message and everything else to the old not-a-GIF
  message, so the UI wording does not drift.
* `let outBytes;` … `outBytes = result.bytes;` keeps the later
  `new Blob([outBytes])` / `outBytes.length` uses intact.
* `web/wasm/.gitignore` only ignores `dist/` — the new `fs_run_guard.mjs` is
  not swallowed. `git ls-files -s` confirms both new files are tracked.

### 0003 — U-72 cancel latch (MainWindow.cpp)

* Latch armed inside `if (engineWasRunning)`, never cleared in `cancelRun()`;
  read-and-consumed at the top of `onProcessFinished()` before the verdict
  branch. Placement verified in the applied file at lines 957-964 and
  1001-1008.
* Checked the second delivery channel: `onProcessError` only acts on
  `FailedToStart` and explicitly comments "Crashes are also reported via
  finished()", so a kill cannot pop a dialog past the latch. The fix is
  sufficient on its own.
* Moving `cancelling_ = true` inside the state test removes no behaviour: the
  only reader is `onProcessFinished`, which cannot fire without a real process.
* **Residual risk, bounded and pre-existing.** If the latch is armed and
  `finished()` never arrives, it survives into the next run and swallows that
  run's genuine failure — the exact pit the patch set out to avoid. The only
  reachable route is: cancel → `waitForFinished(3000)` times out → user starts
  a new run while the engine is still dying. `QProcess::start()` on a running
  process is then a no-op, so that new run never starts regardless of the
  latch. That is U-12 (the async run/cancel state machine, still OPEN, called
  out in MainWindow.h lines 28-40), not a regression from this patch — before
  the patch the same sequence additionally produced the spurious dialog. A
  one-line `cancelling_ = false;` at the top of `runCommand()` would close even
  the theoretical case. **Not applied** — it is outside what the patch
  submitted; say the word and I will add it.

### 0004 — U-70 preview UTF-8 boundary (MainWindow.cpp)

* The bare `path_is_executable(enginePath_.toStdString())` really was still in
  `startPreview()` and `ensureEngine()` really does wrap `u8path_compat` at
  lines 343 and 350 — so this is a genuine one-rule-everywhere unification, not
  a cosmetic change.
* Type-checked the exact expression against the real headers:
  `path_is_executable(const fs::path&)` (EngineLocator.h:56) and
  `u8path_compat(const std::string&) -> fs::path` (WinUnicode.h:58) compose.
* Patch's own sentinel, re-run: `gs::u8path_compat(enginePath_.toStdString())`
  count **3**, bare `path_is_executable(enginePath_` count **0**. Both as
  specified.

### 0005 — U-58 batch settings snapshot (MainWindow.cpp/.h)

* The hunk the patch disclosed as reconstructed is in fact **correct in this
  tree**: line 844 is `partialSnapshot_ = gs::snapshot_output(…)`, line 845 was
  `gs::Settings one = settings;`, 4-space indent at that nesting. No manual
  fallback was needed.
* `settings` is in scope at the insertion point — declared at line 771 in
  `runCommand()`, and the batch branch is inside the same function.
* `gs::Settings batchSettings_;` needs a complete, default-constructible type:
  `core/GifsicleSettings.h` is included at MainWindow.h:71 and `struct
  Settings` (line 60) gives every member a default initializer. Verified by
  compiling the declaration and the copy against the real header.
* Write-before-read invariant holds: `batchIndex_ = 0` occurs at exactly one
  place (line 835, the batch branch), immediately after the new
  `batchSettings_ = settings;`, and the continuation is gated on
  `batchIndex_ >= 0`. Explode/single paths cannot reach the read.
* Sentinel, re-run properly: the only remaining `currentSettings()` call sites
  are the definition (659), `refreshCommand` (677), `runCommand` (771) and
  `startPreview` (1226). **None in `onProcessFinished`.**
  Caveat: a naive `grep -c currentSettings()` — the sentinel as written in the
  patch message — **false-fails**, because the patch's own new comment at line
  1108 contains the token. The CI grep needs a comment filter.

## Proof actually executed here

| proof | command | result |
|---|---|---|
| U-71 S1/S2/S3 | `bash scripts/test_u71_exit_codes.sh` | **PASS** — `S1 sentinel: old mask absent, classifier present` / `11/11 table rows; 0 aliases in 0xC0000000..0xC000FFFF` / `S3 mutation: old-mask mutant caught` |
| U-57 T1-T5 | `node web/test/u57-stale-output.test.mjs` | **PASS** — `# pass 5 # fail 0`, incl. the T1 RED leg proving the original bug |
| U-70 expression | `g++ -std=c++17 -Wall -Wextra` against real `EngineLocator.h` + `WinUnicode.h` | **compiles and runs clean**, no warnings |
| U-58 member/copy | `g++ -std=c++17 -Wall -Wextra` against real `GifsicleSettings.h` | **compiles and runs clean**, no warnings |
| JS syntax | `node --check` on wasm.js, fs_run_guard.mjs, the test | **PARSE OK**; the test's `../wasm/fs_run_guard.mjs` import resolves and exports both functions |
| Project change review | `./scripts/review_change.sh --range 4430a28..HEAD` | **4 passed, 0 failed, 1 skipped** |
| Project doc gate | `./scripts/check_docs.sh` | **21 passed, 2 failed, 3 skipped** |

The two `check_docs.sh` failures are **pre-existing and untouched by this
series** — the series changes no `.md` file (`git diff --stat` over the five
commits lists only `web/` and `working_code/` sources):

* `G10` — COMPILED_AUDIT.md / SESSION_HANDOFF.md name base `fe4f0a7`, which
  this clone does not contain. Same root cause as the anchor mismatch above.
* `G15` — `core.hooksPath` is not `.githooks` in this fresh clone
  (`scripts/bootstrap_hooks.sh` not run).

**Not verifiable here, and not verified:** the three Qt6 hunks were never
compiled. There is no Qt6 in this sandbox and no root for `apt-get install
qt6-base-dev`. What I did instead is type-check the two hunks that introduce a
new type or expression against the real Qt-independent headers, and read the
control-flow hunk (U-72) line by line. A `cmake` GUI build plus the offscreen
harness is still the outstanding proof for U-72/U-70/U-58, exactly as the
patches themselves say ("proof: Qt CI").

## Open items the series does not cover

1. **Neither new proof runs in existing CI.** `build.yml` names its scripts
   explicitly (test_engine.sh:113, test_package.sh:177,
   test_unit_32bit_long.sh:232) and hardcodes its web-suite list at line 135 —
   `test_u71_exit_codes.sh` and `u57-stale-output` appear in neither. The
   `audit-fix-proof.yml` workflow the console page offers as their runner is
   **not in this repository** (only `build.yml` is). Until one of those lands,
   both proofs run only by hand.
   Wiring them in means editing `build.yml` **and** its byte mirror
   `docs/ci/build.yml.proposed` together, or gate G7 goes red.
2. **The 0005 CI sentinel as written false-fails** on the patch's own comment.
3. **Register/doc bookkeeping is outstanding.** The patches each say "after
   green proof: §5 row → FIXED, then `check_docs.sh --emit`, then re-anchor the
   G10 base line". None of that is in the series, so STATUS.md and
   COMPILED_AUDIT.md still report all five findings OPEN while the code has
   them fixed. `review_change.sh` R5 names the same debt: SESSION_HANDOFF.md,
   IMPROVEMENT_LOG.md, WORKLIST.md and STATUS.md are all absent from the
   change. I did not touch the registers — flipping rows without the Qt CI
   proof would contradict the project's own "no patch without proof" rule.
4. `scripts/test_unit_32bit_long.sh` (the LLP64 leg the U-71 commit message
   leans on) SKIPs here: `SKIP: no zig (pip install ziglang)`, exit 3.
