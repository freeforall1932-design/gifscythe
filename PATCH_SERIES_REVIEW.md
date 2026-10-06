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

**The three Qt6 hunks could not be compiled in this sandbox — and CI has now
compiled them.** There is no Qt6 here and no root for `apt-get install
qt6-base-dev`, so locally I could only type-check the two hunks that introduce a
new type or expression against the real Qt-independent headers, and read the
control-flow hunk (U-72) line by line.

PR CI run **37406097117** closed that gap: **all six jobs green**, and
specifically

| job | step | result |
|---|---|---|
| `linux` | Build engine, CLI, tests, and GUI | success |
| `linux` | GUI offscreen tests (COMPILED_AUDIT 6.B harness) | success |
| `windows` | Build GUI (CMake) + windeployqt | success |
| `windows` | GUI offscreen tests (Windows) | success |
| `linux` | Windows exit-code classifier proof (U-71 / P2-17) | success |
| `portability` | Windows exit-code classifier on a 4-byte-long target (U-71, LLP64) | success |
| `windows` | Windows exit-code classifier proof, native (U-71 / P2-17) | success |
| `linux` | WASM stale-output guard (U-57 / P1-37) | success |

So all four steps this review added to `build.yml` ran and passed on real
runners, and U-58/U-70/U-72 **compile on both platforms with both offscreen
harnesses green**.

**They stay PARTIAL anyway, and that is deliberate.** A green build plus an
unchanged harness proves the hunks compile and regress nothing; it does not
prove the fix, because no offscreen case exercises any of the three behaviours.
Contrast U-57 and U-71, whose proofs fail without the fix. The repo has form for
accepting build + offscreen as proof of a GUI row (U-59 was closed on run
36227237540) — but U-59 shipped T4/T8/T9 written *for it*. These three rows have
no such case, so each names the one that is still unwritten.

SESSION_HANDOFF.md lines 448-456 independently agrees with that split, and
names these exact rows: this sandbox is "g++ 12.2, node v22, gh, curl — no
cmake/Qt6/mingw/wine/emcc/dotnet", and the Qt rows "(U-59's half, U-12, U-58,
U-70/U-72, …)" plus "the Windows rows (U-55, U-71)" are "**CI-provable only**
(source edit here, proof on the runner)". Source edit here is what landed.

## Cross-check against the handoff's own constraints

* **U-70 is aligned with a real, documented rule.** SESSION_HANDOFF.md:763-764:
  "every std::string↔fs::path boundary goes through
  `u8path_compat`/`path_u8string` (`src/core/WinUnicode.h`)". `ensureEngine()`
  already obeyed it; `startPreview()` did not. The patch closes a genuine
  divergence from a written constraint.
  One citation nit: 0004's commit message attributes the rule to "PRODUCT
  CONSTRAINTS in SESSION_HANDOFF", but `grep -rn 'PRODUCT CONSTRAINTS'
  --include=*.md .` matches **nothing** — no such heading exists anywhere in
  the repo. The rule is real (line 763); only the label in the message is
  wrong.
* **The G10 failure is pre-existing and already documented as such.**
  SESSION_HANDOFF.md:466-467 records that on the re-creation repair "the docs'
  base shas became unresolvable in this clone (G10, the linux CI failure,
  reproduced exactly here)". So G10 red on `fe4f0a7` is a known property of
  this clone, not a consequence of applying the series — and it is the same
  root cause as the anchor mismatch in the table above.

## Open items — status after the follow-up work

Items 1 and 3 below are now **done**; item 4 was **withdrawn as wrong**. What
remains open is item 2 and the Qt proof.

1. ~~Neither new proof runs in existing CI.~~ **DONE.** Four steps added to
   `build.yml` and its byte mirror `docs/ci/build.yml.proposed` (gate G7 stays
   green): `linux` host `c++` for U-71; `linux` `u57-stale-output` as its own
   step so a red names U-57 rather than "web suite …"; `portability`
   `CXX="python3 -m ziglang c++ -target x86-linux-musl"`; `windows` `CXX=g++`
   (pinned because MinGW ships no `c++` alias, which is the script's default).
   The `portability` leg is the one that matters for this finding and it was
   **executed here** on the pinned `ziglang==0.16.0` that job installs —
   `x86-linux-musl` measures `sizeof(long)==4` against this host's 8, and the
   suite passes at both widths, so the table is proven at the LLP64 width class
   the real NTSTATUS path lives in, not only at the width it was written on.
   `node scripts/lint_workflow.mjs` → `actionlint: clean`, `--selftest` OK.
2. **Still open — the 0005 CI sentinel as written false-fails** on the patch's
   own comment (`MainWindow.cpp:1108` contains the literal `currentSettings()`),
   so a plain `grep -c currentSettings()` goes red. Needs a comment filter. Not
   wired into `build.yml` for that reason.
3. ~~Register/doc bookkeeping is outstanding.~~ **DONE.** U-57 and U-71 flipped
   OPEN → FIXED (S35) with their executed proof quoted; U-58, U-70 and U-72
   flipped OPEN → **PARTIAL** (S35), not FIXED, each naming the behavioural case
   that is still unwritten. `check_docs.sh --emit` regenerated STATUS.md:
   158/11/26/0 → **160/14/21/0 = 195**, which is exactly two OPEN→DONE and three
   OPEN→PARTIAL; P1-37 derived to DONE, P1-38 and P1-42 to PARTIAL, release bar
   10 → 9 open P0/P1 ids. SESSION_HANDOFF.md, WORKLIST.md and IMPROVEMENT_LOG.md
   all carry an S35 entry, so `review_change.sh` R5 now reports them `[touched]`
   instead of `[MISSING]`.

   **G10 was red on arrival and is green now.** This clone's `origin/main` is
   `4430a28`, one squashed upload commit, so the `fe4f0a7` base line was
   unresolvable here — the same event S27 recorded. Both base lines now name
   `4430a28` and keep `fe4f0a7` as a labelled historical record. The tree was
   checked against `fe4f0a7`'s description by markers rather than trust: zero
   image files in `4430a28` (what that commit's delete produced, N-36) and all
   five root license files present (U-97 survived).
4. **WITHDRAWN — do not add the `cancelling_ = false;` one-liner.** I offered it
   above as a hardening for U-72's residual. Working the interleaving through,
   it is wrong: clearing the flag at the top of `runCommand()` re-opens the exact
   race U-72 fixes, because the stale `finished()` from the killed engine then
   arrives with the flag already cleared and the spurious "Optimization failed"
   dialog comes back. And the harmful case is unreachable anyway — a new run
   cannot start until the old process has exited, and exit ⇒ `finished()` ⇒
   consumption. The real fix is U-12's state machine, still OPEN. This is
   recorded in the S35 hand-off block so nobody re-adds it.
5. `scripts/test_unit_32bit_long.sh` still SKIPs here (`no zig` on the system
   Python — PEP 668 blocks the install); it runs in the `portability` job, which
   installs `ziglang==0.16.0` into a venv.

## Salvage check on the three rejected patches

Measured, not assumed. Every `+` line the three git-rejected patches wanted to
add was extracted from the **original** files and checked against the applied
tree; every `-` line was checked for absence:

```
0001  ProcessRunner.h            35/35 added present · 1/1 removed gone
0001  test_u71_exit_codes.sh    120/120 added present · 1/1 removed gone
0004  MainWindow.cpp              7/7 added present · 2/2 removed gone
0005  MainWindow.cpp             12/12 added present · 1/2 removed gone (*)
0005  MainWindow.h                8/8 added present · 1/1 removed gone

TOTAL                           182/182 added lines salvaged
```

(*) Not a loss — a limit of a set-based check. `gs::Settings one = settings;`
still exists in the file, but only in `refreshCommand` (line 698) and
`runCommand`'s batch branch (line 851); the `onProcessFinished` copy the patch
removed is now `gs::Settings one = batchSettings_;` (line 1111). The removal
landed at the correct site.

So: **100% of the fix content in the three rejected patches is live in the
tree.** Only the three `@@` count fields were ever wrong; no code was
reconstructed, paraphrased or dropped.

## Wider regression run (added once the engine + CLI were built)

Building `./build.sh` (engine 1.96 + CLI + unit tests; the GUI needs Qt6 and was
not requested) unblocked the suites that had been skipping, and with them the
strongest available check on the U-71 change — `smoke_cli.sh` drives the real CLI
through `run_argv`, the exact function whose Windows return path was rewritten:

| suite | result | why it matters here |
|---|---|---|
| `scripts/smoke_cli.sh` | **63 passed, 0 failed** | exercises `run_argv` end to end; U-71's contract change (codes > 255 now deliver 255) does not disturb the POSIX exit-code path |
| `scripts/test_engine.sh` | 5 passed, 0 failed | engine pipeline unaffected |
| `scripts/test_output_verify.sh` | 25 assertions, 0 failures | output verifier unaffected |
| `scripts/oracle_fuzz.mjs --quick` | 24/24 deterministic cases | the pre-push hook's second stage |
| `scripts/test_unit_32bit_long.sh` | **396 checks, 0 failures** on `x86-linux-musl`, `sizeof(long)==4` | was SKIP (no zig); now runs, and confirms the N-30 width class is still green alongside U-71's new classifier |
| `scripts/verify_audit.sh` | **32 passed, 0 failed, 5 skipped** | includes **E9** (the `build.yml` byte mirror, so the CI edit is clean) and **F1** (doc gate green). Docs record 34/0/3 "with pip's cmake on PATH"; the two extra skips here are the no-cmake/no-Qt6 legs |
| `./build.sh` unit tests | 396 checks, 0 failures | matches the documented unit count |

The build writes only into gitignored paths (`build/`, `release/*/gifsicle`), so
the tree stayed clean throughout.

## Final gate state

`./scripts/check_docs.sh` → **23 passed, 0 failed, 3 skipped.** Both failures
present on arrival are gone: G10 (base re-anchored) and G15
(`scripts/bootstrap_hooks.sh`, so the pre-push doc gate is live). The 3 SKIPs
are the no-cmake/no-Qt6 measurements (G6, G9b) and G11's shallow-clone date
rule.

`./scripts/review_change.sh --range 4430a28..HEAD` → 4 passed, 0 failed,
1 skipped; R5 reports every doc obligation `[touched]`.

Both proof suites re-run green after the doc edits: U-71 S1/S2/S3, U-57 5/5.
