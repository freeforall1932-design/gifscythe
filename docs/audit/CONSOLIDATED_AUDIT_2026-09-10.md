# Consolidated audit — three independent reviews of `8190c08` (post-PR #7)

**Date:** 2026-09-10 · **Commit:** `8190c0854f0b5427705e8afe47d43cd664b84179` (PR #7, S7) ·
**Product:** 0.1.0 · **Engine:** gifsicle 1.96

This file merges three independent audits of the same commit into one register and
records **what I could actually verify** for each claim.

| Source | Author / origin | Trust rank (owner-set) | Executed the code? |
|---|---|---|---|
| **A** | GPT 5.6 sol xhigh — `…d5c3-7e13…arena.site` "Gifscythe Codebase Audit", 20 findings (2 Critical / 7 High / 9 Medium / 2 Low) | **1 — highest** | **No** — A states: *"The target C++/Qt repository was not cloned or executed in this environment. The reproductions are source-derived."* |
| **B** | Seed 2.1 Pro Preview — `…d5c3-73a3…arena.site` "Gifscythe Code Audit Report", 16 findings (0 Critical / 1 High / 4 Medium / 8 Low / 3 info) | 2 | Not stated; some rows self-tagged "Verified" |
| **C** | This session (Arena agent) — `docs/audit/POST_S7_AUDIT.md`, 13 findings | 3 | **Yes** — engine, CLI, unit suite, web server, scripts all built and run |

**How the ranking is applied.** The owner's ordering (A > B > C) decides *priority and
framing*. It does not decide *fact*: where a claim can be tested, the test decides.
Every row below therefore carries a verification mark, and rows where A or B beat C are
called out in §2 — including two places where **my own earlier audit was wrong**.

**Verification marks**

| Mark | Meaning |
|---|---|
| ✅ **EXEC** | Confirmed by running the real code in this sandbox (command + output quoted) |
| ✅ **SRC** | Confirmed by reading the cited source; not executed (needs Qt/Windows/old toolchain) |
| ⚠️ **PART** | Real, but the finding is overstated or understated — corrected wording given |
| ❌ **NOT-REPRO** | Could not reproduce as stated |
| ⏸ **BLOCKED** | Cannot be checked in this sandbox (no Qt6/cmake/Windows; `apt` blocked) |

**Answer to "should I also provide the webdev zip?" — No.** §7 explains why.

---

## 1. Headline

All three audits agree on the same top item, and it is worse than any of us first
wrote it:

> **Batch auto-naming can silently destroy user data.** With the template `{name}.gif`
> and no batch folder, the computed output **is the input file**. Executed:
> `gifsicle self.gif -o self.gif` → **rc=0**, source replaced in place
> (8703 → 2246 bytes, md5 changed), and the GUI would report "Optimization complete".
> Two different inputs sharing a base name collide the same way.

Beyond that, the three audits are largely complementary rather than contradictory:
A found the packaging, CLI-contract and release-provenance class that B and C missed;
B found the threads/`Auto` bug that **both A and C missed**; C is the only one that ran
anything.

**Release posture: do not cut 1.0.0.** All three reach that conclusion independently.

---

## 2. Corrections to my own previous audit (`POST_S7_AUDIT.md`)

Two of my claims were wrong. Both are corrected here and in that file.

### C-err-1 — I called the `threads` round-trip "benign". It is a real bug. (B:BUG-01 is right)

I wrote: *"The `threads` case is benign (0 and −1 both mean 'Auto')"*. **False.**
Neither value emits a flag, and the engine's default is single-threaded:

```
reference_code/gifsicle/src/gifsicle.c:38   const int GIFSICLE_DEFAULT_THREAD_COUNT = 8;
reference_code/gifsicle/src/gifsicle.c:39   int thread_count = 0;              ← default
reference_code/gifsicle/src/gifsicle.c:1887 case THREADS_OPT:
                                     :1890   else if (clp->have_val) thread_count = clp->val.i;
                                     :1893   else thread_count = GIFSICLE_DEFAULT_THREAD_COUNT;
src/core/GifsicleCommand.h:210              if (s.threads > 0) add("-j" + i2s(s.threads));
```

So the GUI's spinner `0` is labelled **"Auto"** (`SettingsPanel.cpp:349`
`setSpecialValueText("Auto")`) and produces **no flag** → `thread_count` stays `0` →
**single-threaded**. Real "auto" is bare `-j` (→ 8). `GifsicleSettings.h:75` even
documents `-j; <=0 = auto`, which the builder does not implement. **B:BUG-01 confirmed;
my "benign" claim withdrawn.**

### C-err-2 — My F-01 missed the self-overwrite case. (A:GS-001 is more complete)

I described only the two-inputs-same-basename collision. A adds that a template such as
`{name}.gif` with an empty batch folder resolves the output to **the input itself**
(`MainWindow.cpp:527-531` builds `dir + "/" + renderedOutputName(fi)` where `dir` falls
back to `fi.absolutePath()`). Verified: the engine overwrites the source and returns 0.
**A:GS-001 confirmed and adopted as the canonical statement of the finding.**

---

## 3. Master register

Deduplicated across A/B/C. "Src" = which audit(s) raised it. 44 unique findings.

### Critical / release-blocking

| ID | Src | Finding | Verif. | Evidence |
|---|---|---|---|---|
| **U-01** | A:GS-001 · C:F-01 | **Batch auto-naming overwrites other outputs *and* the source GIF.** Preflight only rejects templates with no `{name}` (`MainWindow.cpp:730`); targets are never computed as a set, never compared to inputs, never checked for existence. | ✅ **EXEC** | `gifsicle self.gif -o self.gif` → rc=0, 8703→2246 B, md5 `5ccf9df7…`→`5bda7c28…`, still 12 frames. Collision loop at `:756`/`:872`; T16 uses `a.gif`/`b.gif` so it is untested |
| **U-02** | A:GS-002 | **Portable packager reports success for an incomplete release.** Engine missing → hard fail, but CLI missing → silently skipped, GUI missing → `Note:` + continue, `windeployqt` errors → `Note:` + continue, licenses guarded by `if [[ -f ]]`, no clean staging dir, "sanity" is `ls -la`. | ✅ **EXEC** | Ran it with no GUI built: `Note: Qt GUI not found; packaging engine and CLI only.` → `Portable package created` → **`SCRIPT_EXIT=0`**. Folder contains no `gifscythe` binary |

### High

| ID | Src | Finding | Verif. | Evidence |
|---|---|---|---|---|
| **U-03** | B:BUG-01 | **Threads "Auto" runs single-threaded.** See §2 C-err-1. | ✅ **EXEC**+SRC | `gifsicle.c:38,39,1887-1893` vs `GifsicleCommand.h:210` |
| **U-04** | A:GS-003 | **CLI `--run` without `output` corrupts its own stdout.** The engine writes GIF bytes to fd 1 while the CLI `printf`s the live-pane header, `# -> running` and `# -> exit code` into libc's buffered stdout. | ✅ **EXEC** | `gifscythe-cli noout.conf --run > mixed.bin` → 8859 B starting `GIF89a`, **trailer** `# Gifscythe 0.1.0 command (live CLI pane) … # -> exit code 0`; `gifsicle --info mixed.bin` → *"warning: trailing garbage after GIF ignored"* |
| **U-05** | A:GS-004 | **Documented PATH engine fallback is dead code.** Search-order step 5 returns bare `"gifsicle"`, then `path_is_executable()` does `fs::is_regular_file("gifsicle")` against **CWD**, never PATH. Same code guards the GUI's `ensureEngine()`. | ✅ **EXEC** | Isolated CLI + `gifsicle` on PATH (`command -v gifsicle` → `/home/user/.audit/fakebin/gifsicle`) → `ERROR: engine not found at gifsicle`, **rc=1** |
| **U-06** | A:GS-005 · C:F-12 | **Web demo binds `0.0.0.0` with no auth, no concurrency cap, 64 MB bodies, 120 s engine runs.** Anyone on the network can burn CPU. Documented as a POC, but nothing enforces "local only". | ✅ **EXEC** | `server.mjs:208` `listen(PORT, "0.0.0.0")`; served a real optimize from a non-loopback-capable bind |
| **U-07** | A:GS-006 | **Windows CLI execution is ANSI-only.** `CreateProcessA` + `STARTUPINFOA` + `std::vector<char>` cmdline ⇒ non-ASCII paths (e.g. `C:\Users\佐藤\in.gif`) cannot be passed to the engine. GUI uses QProcess and is unaffected. | ✅ **SRC** | `ProcessRunner.h:79` (`CreateProcessA`) |
| **U-08** | A:GS-007 | **License set can ship incomplete, silently.** Root has `LICENSE` (1133 B) and `COPYING.gifsicle` but **no `COPYING`**, so that copy branch never fires; every license copy is `if [[ -f ]]`-guarded. A GPL-v2 engine can be shipped with no license text and the script still exits 0. | ✅ **EXEC**+SRC | `package_portable.sh:51-59`; `ls LICENSE COPYING` → *No such file `COPYING`* |
| **U-09** | A:GS-008 | **Banked Windows snapshot is 5 commits behind the SHA its own notes claim.** Release body says *"Source of truth remains git main @ d3544b1"*; main is `8190c08`. The zip is the input to the clean-Windows smoke gate C4/D3/D4, so that gate would test pre-S7 code. | ✅ **EXEC** | `gh release view snapshot-2026-09-07` → body pins `d3544b1`; `git rev-parse main` → `8190c08…`; assets uploaded 2026-09-07 (pre-PR #7) |
| **U-10** | A:GS-009 | **The "read-only, identical-to-upstream" vendored engine is neither.** `REFERENCE_MANIFEST.md` says `gifsicle/` is *"Identical to upstream master"*; it carries a hand-written `config.h` (Linux values), a functional patch, and an extra test. Three folders the manifest documents don't exist in the checkout. | ✅ **EXEC** | `diff -rq` vs the pristine `gifsicle-nested-1.96/`: `gifsicle.h` adds `#define FRAME_SELECTION_MODE_MASK 0x1F`; `gifsicle.c` rewrites `frames_done` into a per-mode bitmap (`frames_done \|= 1 << mode`) + refactors `frame_argument` (23 diff lines); extra `test/012-framechange.testie`; `config.h` only in the patched tree. `gifsicle-upstream/`, `caesium-source/`, `caesium-bin/` → *No such file or directory* (gitignored at `.gitignore:22-24`) |

### Medium

| ID | Src | Finding | Verif. | Evidence |
|---|---|---|---|---|
| **U-11** | A:GS-011 | **Malformed booleans degrade silently.** `parse_bool` maps anything outside `1/true/yes/on` to `false` with no warning, unlike every numeric parser. | ✅ **EXEC** | conf with `unoptimize = maybe` + `careful = Trueish` → rc=0, **zero** warnings for them (only the two `validate()` warnings), and the built command has neither `-U` nor `--careful` |
| **U-12** | A:GS-012 · | **"Fully async" GUI still blocks the UI thread in 5 places** — up to 5 s per run start. | ✅ **SRC** | `MainWindow.cpp:188` `waitForFinished(2000)`, `:768` + `:804` `waitForStarted(5000)`, `:818` `waitForFinished(3000)`, `:936` `waitForFinished(1000)` |
| **U-13** | A:GS-013 · B:BUG-02 | **Drag-and-drop accepts any existing file.** The filter is `endsWith(".gif") **\|\|** exists(f)` — clearly meant `&&`. `.exe`, `.jpg`, `.txt` all queue and fail at run time. | ✅ **SRC** | `MainWindow.cpp:426` |
| **U-14** | A:GS-014 | **Green CI does not enforce the claims used as release gates.** `scripts/verify_audit.sh` is never run in CI; package contents are never asserted (no negative test). The "21 PASS / 0 FAIL" number is manual. | ✅ **EXEC** | Workflow steps: `build.sh --all`, `test_engine.sh`, `smoke_cli.sh`, harness, `package_portable.sh`, uploads — no `verify_audit.sh`; only `ls -la` at `:99,:144` |
| **U-15** | A:GS-015 | **CMake writes into the source tree.** `configure_file` targets `${CMAKE_SOURCE_DIR}/src/core/version.h`, so configuring mutates a committed file; the qmake path is a separate generator that must be kept byte-identical by hand. | ✅ **SRC** | `CMakeLists.txt:17-24` |
| **U-16** | A:GS-016 · C:F-06 | **Settings persistence is non-atomic** (Truncate + write). A crash mid-write leaves a truncated conf. | ✅ **SRC** | `MainWindow.cpp:1086-1096` |
| **U-17** | A:GS-017 · B:BUG-08 | **Explode mode never verifies any frame was written.** Output verification is explicitly skipped for Explode, so a zero-frame explode reports "Optimization complete." | ✅ **SRC** | `MainWindow.cpp:850` `if (batchMode_ != gs::Mode::Explode && …)` |
| **U-18** | A:GS-018 | **The regression suite does not cover any of the failure classes above.** No test for target collisions, package completeness, stdout purity, PATH fallback, or thread flags. | ✅ **EXEC** | `test_gui_offscreen.cpp` T16 uses distinct stems; unit suite has no `threads`/packaging/stdout case |
| **U-19** | C:F-02 | **`readFrom()` is not the "exact inverse" of `writeInto()`.** Crop geometry, position and scale are serialized only when their parent toggle is on. | ✅ **EXEC** | Real `save_settings`→`load_settings` round trip: `crop_w=200/crop_h=150`, `position=12x7` come back **0**, 0 load warnings |
| **U-20** | C:F-03 | **"The CLI reads GUI-saved files without warnings" is false.** A real GUI-saved file has no `input` key, so `validate()` warns. | ✅ **EXEC** | exit 0 + `WARNING: input=: at least one input file is required` |
| **U-21** | C:F-04 | **Name-template sanitisation is POSIX-only** — no Windows invalid chars (`< > : " \| ? *`), no trailing dot/space trim, no reserved-name guard. | ⏸ **BLOCKED** (no Windows) | `MainWindow.cpp:508-510` strips only `/` and `\`; `win32cfg.h` sets `PATHNAME_SEPARATOR='\\'` |
| **U-22** | C:F-05 | **`Validate.h` skips resize geometry**, so a conf can reach the engine with `--resize-fit 0x0`. | ✅ **EXEC** | `gifsicle --resize-fit 0x0` → rc=1 *"one of W and H must be positive"* (`40x0` is fine → 40x88) |
| **U-23** | A:GS-010 · | **Unknown CLI arguments are silently ignored** — stronger than A's wording: no error is printed at all. | ✅ **EXEC** | `gifscythe-cli conf --rnu` → **rc=0**, stderr empty apart from the validate warnings; `--engine` with no value likewise ignored |
| **U-24** | B:BUG-06 | **Web server doesn't verify `out.gif` exists/non-empty after rc=0**, unlike the desktop (`MainWindow.cpp:850`). A zero-byte result throws ENOENT → generic **500** instead of 422. | ✅ **SRC** | `server.mjs:169` `readFile(outFile)` inside the same try whose catch returns 500 |
| **U-25** | B:BUG-04 | **Web "Scale %" defaults to 50 %, desktop to 100 %.** A web user picking Scale gets half-size output by default. | ✅ **SRC** | `web/index.html:81` `value="50"` vs `SettingsPanel.cpp:183,189` `setValue(100.0)` |

### Low

| ID | Src | Finding | Verif. | Evidence |
|---|---|---|---|---|
| **U-26** | A:GS-019 · C:F-12 | Web engine discovery sorts versions lexicographically (`0.9.0` > `0.10.0`). | ✅ **SRC** | `server.mjs:52` |
| **U-27** | A:GS-020 | Current-state docs still listed a completed CI action as pending. **Fixed by this change.** | ✅ **SRC** | Was `8190c08:SESSION_HANDOFF.md:22` "push this branch → CI green on both OSes" (done: runs 34425977060 / 34427315414); `WORKLIST.md` item 2 also ticked |
| **U-28** | B:BUG-03 | `runCommand()` non-batch path has **no `return`** after the "could not start engine" dialog (batch path does return). Harmless today; a landmine for the next edit. | ✅ **SRC** | `MainWindow.cpp:804-808` vs `:768-773` |
| **U-29** | B:BUG-05 | Web resize dropdown omits **Touch**, though `command.mjs:90-91` implements it and the desktop has 6 kinds. | ✅ **SRC** | `web/index.html:70-77` — none/fit/exact/width/height/scale only |
| **U-30** | B:BUG-07 | Web server has no `validate()` equivalent; out-of-range values go straight to the engine. Inconsistent UX vs desktop. | ✅ **SRC** | `server.mjs:156` spreads settings into `buildArgs` |
| **U-31** | B:BUG-11 | `build.sh` never links `-lstdc++fs`; `EngineLocator.h` uses `std::filesystem`, so g++ 7/8 hosts fail at link. | ⏸ **BLOCKED** (g++ 12 here) | `build.sh:58,64` — flags are `-std=c++17 -Wall -Wextra -pedantic -O2 -I…` only |
| **U-32** | B:BUG-12 | POSIX `run_argv` returns **1** on a signalled child instead of the `128+signum` convention, so callers can't distinguish a crash from a kill. | ✅ **SRC** | `ProcessRunner.h:117-120` |
| **U-33** | B:BUG-13 | Setting only `position_x` **or** `position_y` in a conf sets `has_position = true`, yielding a half-specified `-p X,0`. | ✅ **SRC** | `SettingsIO.h:105-110` |
| **U-34** | B:BUG-14 | Preview temp files leak within a session: cleanup removes only `preview_{seq-1}` and only on the non-stale path, so superseded previews are never deleted. | ✅ **SRC** | `MainWindow.cpp:981` early return vs `:992-993` |
| **U-35** | B:BUG-16 | `setBusy(false)` re-enables Run without re-checking the engine, unlike `appendInputs`. | ✅ **SRC** | `MainWindow.cpp:668-670` |
| **U-36** | C:F-07 | A **third** parser for the settings format (`guiStateKey`) re-opens and re-parses the file twice per load. | ✅ **SRC** | `MainWindow.cpp:68-84`, called at `:1044` and `:1046` |
| **U-37** | C:F-08 | "Persistence unavailable" is silent — no dialog, no status note. | ✅ **SRC** | `MainWindow.cpp:1028`, `:1060` |
| **U-38** | C:F-09 | `verify_audit.sh` **FAILs** instead of SKIPping C6 when `cmake` is absent (`[B]` guards properly 13 lines later). | ✅ **EXEC** | `19 passed, 1 failed (C6), 3 skipped`, exit 1 — vs `IMPROVEMENT_LOG.md:100`'s 21/0/2 |
| **U-39** | C:F-10 | `docs/ci/build.yml.proposed` is a hand-maintained byte copy of the live workflow (already drifted once). | ✅ **EXEC** | `diff` currently clean; nothing keeps it that way |
| **U-40** | B:BUG-15 | CLI prints `validate()` warnings and runs anyway; the GUI refuses. Intentional, but undocumented at the point of use. | ✅ **EXEC** | `colors = 999` → warning + still runs |

### Info / POC gaps / nits

| ID | Src | Finding | Verif. |
|---|---|---|---|
| **U-41** | B:BUG-09 | Web POC is single-file Auto mode only — no batch/merge/explode. Documented as a POC. | ✅ **SRC** `web/app.js:17` |
| **U-42** | B:BUG-10 | Web has one `scalePct` for both axes; desktop has independent X/Y. | ✅ **SRC** `web/app.js:27-28` |
| **U-43** | C:F-11 | Summary reads `Batch (1 files) → X … X` (plural + duplicated path) for one input with no Save-as. | ✅ **SRC** `MainWindow.cpp:575` |
| **U-44** | C:F-13 | The two dated review snapshots sit at repo root while newer material lives in `docs/`. | ✅ **SRC** |

---

## 4. Where the audits disagree, and how it resolves

| Topic | A (GPT) | B (Seed) | C (this session) | Resolution |
|---|---|---|---|---|
| Threads `0` = "Auto" | not mentioned | **High bug** | called it **benign** (wrong) | **B is right.** Verified against `gifsicle.c:38/1887-1893`. → U-03 |
| Overwrite risk | Critical, includes self-target | not mentioned | High, same-basename only | **A is the most complete.** Verified rc=0 in-place replacement. → U-01 |
| Packaging | **Critical** | not mentioned | not mentioned | **A confirmed by execution** — exits 0 with no GUI in the folder. → U-02 |
| Unknown CLI args | "prints an error but can still exit successfully" | not mentioned | not mentioned | ⚠️ **A understated**: nothing is printed at all, rc=0. → U-23 |
| CLI/GUI warning policy | not mentioned | Info, "intentional difference" | used it as evidence for U-20 | Both hold: it is intentional (B) **and** the handoff's "no warnings" claim is false (C). → U-20, U-40 |
| `dither = none` round trip | not mentioned | not mentioned | checked, **not a bug** | Unchanged: `kDitherMethods` has no `"none"` entry and `GifsicleCommand.h:108-111` emits nothing either way |
| Vendored upstream | High: "already modified" | not mentioned | not mentioned | **A confirmed**, and the manifest's own wording (*"Identical to upstream master"*) is the defect. → U-10 |
| Harness / CI status | 243 checks corroborated via run #36 | 20 unit tests + argv discipline | 247 `CHECK` sites, not run locally | Consistent. CI runs 34427315414 passed linux+windows; log text not retrievable from this sandbox |

**Net:** every A finding that could be tested here held up — **2/2 Criticals confirmed
by execution**, **7/7 Highs confirmed** (GS-006 by source only; executing it needs
Windows), and one Medium (GS-010) turned out **stronger** than stated. All 16 B findings
held up too; B:BUG-11 is the only one that cannot be exercised here (needs g++ ≤ 8).
**Nothing in either external audit was found to be false** — the only inaccuracies were
understatements.

---

## 5. Fix order

A's ordering is sound and is adopted, with two changes from verification: the
threads bug is promoted into step 1 (it is a silent behaviour defect, not a polish
item), and the release-provenance re-cut moves up because it invalidates the
Windows smoke gate that everything else depends on.

1. **Stop silent destruction** — plan every output before the first process starts; reject duplicate targets and target-equals-source; define an overwrite policy; write to a temp sibling and rename after a non-empty GIF is produced. Fix the `-j` "Auto" mapping in the same pass. → **U-01, U-03**
2. **Make packaging fail closed** — fresh staging dir, required-binary manifest, `windeployqt` failure is fatal, license set asserted, package E2E test. → **U-02, U-08**
3. **Re-cut release evidence** — artifacts from the exact tagged SHA, notes pinning that SHA, then run C4/D3/D4 against them. → **U-09**
4. **Repair CLI contracts** — strict arg parser, stdout purity (or refuse `--run` with no `output`), PATH resolution, Unicode process APIs on Windows, validation errors that block. → **U-04, U-05, U-07, U-23, U-40**
5. **Harden execution surfaces** — web resource bounds, remove UI-thread waits, GIF-only drop filter, atomic settings, Explode frame verification, web output check, resize validation. → **U-06, U-12, U-13, U-16, U-17, U-22, U-24**
6. **Turn fixes into gates** — run `verify_audit.sh` in CI, add negative packaging tests, stop CMake writing into `src/`, immutable + correctly-labelled upstream tree, sanitizer job. → **U-10, U-14, U-15, U-18, U-38**
7. **Docs and polish** — correct the two overstatements, sync stale status lines, web parity items, nits. → **U-19, U-20, U-27, U-25, U-29, U-41…U-44**

**Suggested release criterion** (A's, adopted): no Critical/High open, package-negative
tests green, and a clean-Windows smoke run against the exact tagged SHA.

---

## 6. What is still unverified

* **U-21** (Windows template sanitisation) and **U-07** (ANSI process APIs) need a real
  Windows run — this sandbox has no Windows, no mingw, no Qt6, no cmake, and `apt`
  cannot reach `deb.debian.org`.
* **U-31** (`-lstdc++fs`) needs g++ ≤ 8; only g++ 12 is available here.
* **The Qt GUI was never compiled or run in this session.** All `MainWindow.cpp` /
  `SettingsPanel.cpp` rows marked ✅ SRC are source reads, not test runs. The
  offscreen harness was not executed here; CI run 34427315414 (linux 1m9s, windows
  2m48s, both pass) runs it under `set -euo pipefail` and it returns 1 on failure,
  which corroborates "243 checks, 0 failures" without me having seen the count.
* **U-10's provenance question is only half answered.** I can prove the vendored tree
  differs from the nested copy and carries a `config.h` upstream does not ship. I
  cannot prove whether `gifsicle.c`'s `FRAME_SELECTION_MODE_MASK` change is upstream
  master post-1.96 or a local edit, because the manifest's `gifsicle-upstream/`
  reference clone is gitignored and absent. Resolving it needs a fresh clone of
  `kohler/gifsicle` at the pinned `07f5c4c3`.

---

## 7. Do I need the webdev zip?

**No.** Everything the two external audits cite is in this repository and was checked
directly: `web/index.html`, `web/app.js`, `web/command.mjs`, `web/server.mjs`,
`web/test/command.test.mjs` are all present, and I ran the server and its test suite
against the built engine (`POST /optimize` → `200 image/gif`, 8703 → 2246 bytes,
`60x132 → 18x40`, `[64]` colours; client-supplied `inputs`/`output` ignored; five
path-traversal probes found no escape).

Send it only if one of these is true:

* the zip contains a **built/bundled** web app that differs from the committed source
  (e.g. a minified or transpiled bundle that is what actually ships);
* you want the **original audit artefacts** (A's "Download full report" export) kept in
  the repo for provenance — useful, but not needed for verification;
* the zip holds files that are **gitignored** here and therefore invisible to me —
  the ones I know are missing are `reference_code/gifsicle-upstream/`,
  `reference_code/caesium-source/`, `reference_code/caesium-bin/`
  (`.gitignore:22-24`). If the zip has those, it would let me close **U-10** fully.
