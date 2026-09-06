# GIFSCYTHE — Comprehensive Consolidated Code Review

**Repository:** `freeforall1932-design/gifscythe` (branch `main`, v0.1.0)
**Compiled:** 2026-09-06
**Purpose:** One complete, self-contained review document that merges every audit performed on this codebase — the original review, the external agent audit report, and the 4 published forensic review sites — with every claim re-verified against the gifsicle 1.96 man page **and the gifsicle 1.96 source committed in the repo itself**. Findings the secondary audit got wrong are explicitly refuted with evidence.

---

## 0. Sources consolidated into this document

| # | Source | What it contributed |
|---|--------|---------------------|
| S1 | Original review (this assistant, `gifscythe-review.md`) | Full source read of `working_code/gifscythe/`; man-page cross-checks; Windows CI Chocolatey package verification; license/packaging gaps |
| S2 | Attached agent audit report (`Qwen_markdown_20260906_as7ahubyq.md`) = Site 4 (`01a07621-…arena.site`) | "Full Code Audit Report": 7 pre-flagged confirmations (C1–C5, G3, M2) + 15 new findings (N1–N15) + 9 strategic recommendations |
| S3 | Site 1 (`01a075af-…arena.site`) — forensic audit, "23 findings filed / 6 critical reproduced" | 18 finding cards (C1–C5, M1–M7, G1–G6) with live reproduction transcripts; claim-vs-code table; fix order |
| S4 | Site 2 (`01a075d0-…arena.site`) — same forensic audit, second render ("20 findings / 5 critical") | Identical finding set to S3; confirms reproductions independently |
| S5 | Site 3 (`01a075d1-…arena.site`) — same forensic audit, third render ("21 findings / 6 critical") | Adds an explicit **"second verification pass"** reproduction log (C1, C2, C3, G3, M2 re-run on a fresh clone) |
| S6 | Repo context docs (read by every reviewer) | `PROJECT_VISION.md`, `WORKLIST.md`, `SESSION_HANDOFF.md`, `IMPROVEMENT_LOG.md`, `FEASIBILITY_REVIEW.md`, `VERSION.md`, READMEs |
| S7 | Independent verification performed for this compilation | Official gifsicle 1.96 man page (lcdf.org); `reference_code/gifsicle/src/gifsicle.c` + `clp.c` (committed in the repo, fetched and grep'd); Chocolatey package index |

> Sites 1–3 are three renderings of the **same** forensic audit (identical finding cards; the "23/20/21 findings" counters are cosmetic — the body contains 18 finding cards). Site 4 and the attached markdown are the **same** report. So this document consolidates **two independent audits + one original review**, then adjudicates conflicts.

---

## 1. Executive summary

**Verdict:** The architecture is sound and the foundation genuinely works — but the product currently has a class of failures worse than crashes: **silent false success**. In five separate places the tool reports success while doing nothing, doing the wrong thing, or destroying data. For a product whose mission is *"a normal person edits without surprises"* this is the worst possible failure class.

### Numbers (adjudicated)

| Metric | Count |
|---|---|
| Confirmed BROKEN findings (wrong results / silent failure at runtime) | **10** |
| Confirmed MISALIGNED findings (code contradicts its docs/labels) | **9** |
| Confirmed MISSING-logic findings (documented behavior that doesn't exist) | **13** |
| **Refuted false positives from the secondary audit** | **4** (N1, N2, N11, N14 — see §5) |
| Claims in project docs that fully hold | 3 of 7 checked |
| Claims that only partially hold | 3 of 7 |
| Claims that fail | 1 of 7 ("CLI driver + integration tests") |

### The five facts at the heart of the audits
*(Reproduced live by both audits with transcripts quoted below; re-verified statically by this compilation against the source. This compilation has no compiler in its sandbox, so it could not re-run them — see §5.1 evidence grading.)*

1. **`gifscythe-cli --run` exits 0 when the engine doesn't exist** (`system()` wait-status 32512 = 127«8 wraps to 0 through C's 8-bit exit truncation). CI and automation see green; nothing happened.
2. **The engine is only found from exactly one working directory** (repo root, absolute settings path). The README's own documented usage fails.
3. **The GUI welds the whole queue into one GIF** (`Mode::Merge` hardcoded). Reproduced: `gifsicle -m -O3 logo.gif logo1.gif -o out.gif` → **13 images** (12+1 concatenated) where the user expected two optimized files.
4. **An empty output field vaporizes the result**: gifsicle writes the optimized GIF to stdout, `MainWindow` never reads stdout, and the status bar still says *"Optimization complete."* Traced: 821 bytes discarded.
5. **The CMake build cannot configure at all**: `add_library(gifscythe_core STATIC)` with only a header source fails generation — reproduced verbatim with CMake 3.30.2: `Cannot determine link language for target "gifscythe_core"`. `build.sh` hides this with `>/dev/null 2>&1`.

Plus the Windows CI failure, now **diagnosed from the workflow file itself**: `choco install qt6-base` references a package that does not exist (the real package is `qt6-base-dev`), and even a correct install would not build the GUI because the scripts only look for `qmake6`/`qmake` (Windows ships `qmake.exe`) and there is no `windeployqt` step.

---

## 2. What actually works — verified, not assumed

The forensic audits compiled and ran everything before touching flaws; this compilation re-verified the flag semantics against upstream docs and the committed engine source. Keep this code; fix it, don't rewrite it.

| Verified working | Evidence |
|---|---|
| Core compiles clean: `g++ -std=c++17 -Wall -Wextra -pedantic`, zero warnings across `GifsicleSettings.h` / `GifsicleCommand.h` / `SettingsIO.h` | Forensic audit build (g++ 12.2) |
| **35/35 unit assertions pass** in `tests/test_gifsicle_command.cpp` (merge, resize-fit, explode, crop, parser case-insensitivity) — count independently confirmed: 35 `CHECK()` calls + 1 macro definition | Forensic audit test run |
| Engine builds from the reference source via the direct-gcc path → `release/0.1.0/gifsicle` | Forensic audit build |
| **5/5 engine pipeline tests pass**: `--info` (12 frames), `-O3` optimize, `--lossy=80`, `--resize-fit 30x66` (verified logical screen), explode → 12 frames | `scripts/test_engine.sh` live run |
| CLI produces a valid optimized GIF end-to-end (9,458-byte merged+optimized `/tmp/gifscythe_demo.gif`) — **but only from repo-root CWD** | Forensic audit |
| Architecture matches FEASIBILITY_REVIEW: Qt-independent header-only core shared by CLI+GUI, engine as subprocess (keeps GPL v2/v3 separation clean), WebP/APNG correctly deferred | Doc-vs-code check |
| Version discipline (still 0.1.0) and scope discipline (no premature APNG/WebP code) both hold perfectly | Claim-vs-code check |

### Option-mapping correctness (verified against the gifsicle 1.96 man page + committed source)

These generated flags were checked one-by-one and are **correct — no action needed**:

- `--crop X,Y+WxH` — the `+` form takes width/height per the man page (e.g. `--crop 0,0+30x60` is valid; `2,2+-2x-2` shaves 2px borders).
- `--loopcount=0` = loop forever — man page: *"--loopcount=0 is equivalent to --loopcount=forever"*; source: `case 'l': … loopcount = (clp->have_val ? clp->val.i : 0)` (see §5, refutation of N1).
- `-O3`, `-j4`, `--lossy=N` attached forms — correct (optional-value options need attached form in CLP).
- Mode flags (`-m`/`-b`/`-e`) placed before filenames — required by gifsicle, done correctly.
- `-o` placed after inputs — allowed (general option).
- `-k N` (colors 2–256), `-f` (dither), `-d` (delay in 1/100s), `-D 0..7` (disposal; man page allows 0–7 even though only 0–3 are meaningful), `-p X,Y`, `-E` (explode-by-name), `--flip-*`, `--rotate-*`, `-i`, `--careful`, `--no-comments/-names/-extensions` — all correct mappings.
- `scripts/build_gifsicle.sh` OBJS list matches upstream `gifsicle_SOURCES` exactly (clp, fmalloc, giffunc, gifread, gifsicle, gifunopt, gifwrite, kcolor, merge, optimize, quantize, support, xform).

---

## 3. Master findings list — BROKEN code

Severity: 🔴 CRITICAL (data loss / false success / security) · 🟠 MAJOR (feature broken) · IDs: unified `U-*`, with cross-references to every source that found it.

---

### U-B1 🔴 CLI `--run` exits 0 on total failure (false success)
**Found by:** S3/S4/S5 (C1) · S2 (C1) · S1 (A6) — **reproduced live, twice**
**File:** `src/cli/main.cpp:54–56, 142–144`

**Symptom.** When the engine binary can't be found, the shell still runs the command, fails with 127, and the CLI prints `# -> exit code 32512` while returning **0** to the OS. Automation/CI sees a green run.

**Root cause.** `run_with_system()` returns the raw `system()` wait status unmodified. Only the low 8 bits survive C's process-exit truncation: 32512 = 127«8 → 0. There is also no existence check on the engine before running.

**Reproduction (forensic audit, confirmed on fresh clone):**
```text
$ gifscythe-cli animation.conf --run
sh: 1: …/examples/working_code/gifscythe/release/0.1.0/gifsicle: not found
# -> exit code 32512
$ echo $?
0
```

**Fix.**
```cpp
// pre-flight:
if (!fs_probe::exists(engine_path)) {
  std::fprintf(stderr, "ERROR: engine not found at %s\n", engine_path.c_str());
  return 1;
}
// decode the wait status honestly:
int rc = std::system(cmd.c_str());
if (rc == -1) return 1;
if (WIFEXITED(rc)) return WEXITSTATUS(rc);   // 0..255, honest
return 1;                                     // killed by signal
```
Better still, stop using `system()` entirely (see U-B6).

---

### U-B2 🔴 Engine path resolution works only by accident (CWD-dependent)
**Found by:** S3 (C2) · S5 (C2, second pass) · S2 (C2) · S1 (B4) — **reproduced**
**File:** `src/cli/main.cpp:70, 90–113`

**Symptom.** The default engine path is `working_code/gifscythe/release/0.1.0/gifsicle` resolved against a "repo root" that is only discovered when (a) the settings path is absolute **and** contains the literal string `/working_code/gifscythe`, or (b) the CWD *is* the repo root. Run from `examples/`, or from `working_code/gifscythe` as the README documents, and the engine "isn't there" — silently (see U-B1).

**Root cause.** `repo_root` derived by string-sniffing (`find("/working_code/gifscythe")`) + three-levels-of-parent fallback + CWD-relative probing: three anchor points that coincide for exactly one working directory.

**Reproduction (second verification pass, Site 3):**
```text
# relative settings path → bogus nested engine path → fail, exit 0(!)
$ gifscythe-cli examples/animation.conf --run
sh: 1: examples/working_code/gifscythe/release/0.1.0/gifsicle: not found
# -> exit code 32512

# absolute settings path → works
$ gifscythe-cli $PWD/examples/animation.conf --run
# -> exit code 0   →  /tmp/gifscythe_demo.gif 9458 bytes ✓
```

**Fix.** Resolve the settings path to absolute (`realpath`) at startup; anchor engine search on the **executable location**, not CWD:
```cpp
const char* candidates[] = {
  exe_dir + "/gifsicle",                              // packaged layout
  exe_dir + "/../release/" + GS_VERSION + "/gifsicle" // dev layout
};
for (auto* c : candidates) if (fs_probe::exists(c)) { engine_path = c; break; }
// then: GS_ENGINE env var override, then PATH
```
(See U-M9 for the single-version-source that makes `GS_VERSION` possible.)

---

### U-B3 🔴 GUI welds the queue into one GIF (Merge semantics vs. batch intent)
**Found by:** S3 (C3) · S2 (C3) · S1 (B3) — **reproduced against the built engine**
**File:** `src/qtui/MainWindow.cpp:108–113` (`currentSettings()`)

**Symptom.** The UI presents an *"Animation queue"* with *"Add GIF files…"* — batch semantics — but every run hardcodes `s.mode = gs::Mode::Merge`. gifsicle `-m` concatenates all inputs frame-by-frame. Queue two GIFs to "optimize" and the tool fuses them into one animation. This is silent data mangling of the user's intent.

**Reproduction:**
```text
$ gifsicle --info logo.gif      → 12 images
$ gifsicle --info logo1.gif     → 1 image
# exactly what runCommand() builds for the 2-file queue:
$ gifsicle -m -O3 logo.gif logo1.gif -o out.gif
$ gifsicle --info out.gif
* out.gif 13 images     ← 12 + 1, concatenated; logical screen 60x132
```

**Root cause.** Merge was the CLI demo scenario; the queue concept needs batch semantics. Merging is a deliberate *Convert-branch* action (per FEASIBILITY_REVIEW), not a default.

**Fix.** Default the queue to `Mode::Batch` (per-file processing, in-place or `<name>_opt.gif` outputs), or loop one run per input. Keep Merge only behind an explicit "Merge into one animation" action. Add a mode selector either way (see U-MISS-9).

---

### U-B4 🔴 Empty output path vaporizes the result, then reports success
**Found by:** S3 (C4) · S2 (C4, "stdout deadlock") · S1 (B2) — **traced live**
**File:** `src/qtui/MainWindow.cpp:122–138` (`runCommand()`)

**Symptom.** The output field is optional. Left empty, gifsicle writes the optimized GIF to **stdout** — which QProcess never reads or redirects. The bytes evaporate and the status bar announces *"Optimization complete."* Guaranteed silent data loss for the most common first run (user adds a GIF, clicks Optimize, skips the output field).

**Evidence (forensic trace):**
```text
$ gifsicle -O3 logo1.gif | wc -c
821                          # engine provably writes the whole file to stdout

# MainWindow reads ONLY stderr (line 131); readAllStandardOutput() never called.
# → 821 bytes discarded, success branch taken because exitCode() == 0.
```

**Fix.**
```cpp
if (settings.output.empty()) {
  QMessageBox::warning(this, "Gifscythe",
    "Choose an output file (or enable auto <name>_opt.gif).");
  return;   // or auto-derive <input>_opt.gif in batch mode
}
// and AFTER a normal exit — verify, don't assume:
if (!QFileInfo::exists(outPath()) || QFileInfo(outPath()).size() == 0) {
  updateStatus("No output produced — check the command below.");
  return;
}
```

---

### U-B5 🔴 GUI run: `waitForFinished()` timeout ignored → zombie process + wrong status
**Found by:** S3 (G4) · S2 (N15) · S1 (A3)
**File:** `src/qtui/MainWindow.cpp:125–138`

**Symptom.** `proc.waitForFinished(60000)` blocks the UI thread up to 60 s; its **bool return is ignored**. On timeout the process is still running, but `exitStatus()`/`exitCode()` on a live QProcess default to `NormalExit`/`0`, so the UI can report **success for a process that hasn't finished**; the `QProcess` destructor then kills it (`QProcess: Destroyed while process is still running`). Large animations — the app's entire reason to exist — exceed 60 s routinely.

**Fix.** Check the bool; on timeout `proc.kill(); proc.waitForFinished(1000);` and report failure. Proper fix is async (see U-MISS-4): member `QProcess`, `finished`/`readyReadStandardError` signals, progress + cancel. This is the substrate WORKLIST task 4 stands on.

---

### U-B6 🔴 Shell execution of a concatenated command string (spaces + injection)
**Found by:** S3 (G3) · S2 (G3, critical) · S1 (A6) — **reproduced**
**Files:** `src/cli/main.cpp:54–56, ~182–190` · `src/core/GifsicleCommand.h` (`toString()`)

**Symptom.** `toString()` joins argv with bare spaces (no quoting), and the CLI executes exactly that string through `std::system()`. Any path with a space breaks; any settings value with shell metacharacters is *interpreted by the shell*. Power users are also invited to copy the unquoted line from the "show me the command" pane into a terminal, where it fails the same way.

**Reproduction (forensic):**
```text
# input = /tmp/my vacation/in.gif    output = /tmp/my vacation/out.gif
$ gifscythe-cli space.conf --run
…gifsicle -O3 /tmp/my vacation/in.gif -o /tmp/my vacation/out.gif
sh: 1: vacation/in.gif: not found
# -> exit code 32512   →  $? == 0   (see U-B1)

# adversarial: output = "out; rm -rf ~"  → executed by /bin/sh
```

**Fix — two separate surfaces, two fixes:**
```cpp
// EXECUTION: never through a shell. argv already exists:
int run_argv(const std::vector<std::string>& args);  // posix_spawn / fork+execvp
                                                      // (CreateProcess on Windows)
// DISPLAY: quote args containing shell specials for the live pane:
std::string shell_quote(const std::string& a);        // used only by toString()
```

---

### U-B7 🔴 CMake cannot configure: header-only lib declared STATIC
**Found by:** S3 (M2) · S2 (M2) · S1 (A2) — **reproduced with CMake 3.30.2**
**File:** `CMakeLists.txt:8–12`

**Reproduction:**
```text
-- Qt6 NOT found -> GUI skipped.
-- Configuring done (0.3s)
CMake Error: Cannot determine link language for target "gifscythe_core".
CMake Error: CMake can not determine linker language for target: gifscythe_core
-- Generating done (0.0s)
CMake Generate step failed. Build files cannot be regenerated correctly.
```

**Root cause.** A `STATIC` library needs ≥1 compiled translation unit. `target_sources(gifscythe_core PRIVATE src/core/SettingsIO.h)` contributes only a header → no link language. Linux CI passes only because `build.sh` prefers the qmake path; the cmake fallback swallows this error with `>/dev/null 2>&1` and prints nothing.

**Fix.**
```cmake
add_library(gifscythe_core INTERFACE)
target_include_directories(gifscythe_core INTERFACE src)
# remove the set_target_properties(... PUBLIC_HEADER ...) block
```
And stop silencing: when `--all` is requested and the GUI can't build, exit non-zero so CI fails honestly.

---

### U-B8 🟠 Windows CI: wrong package name + missing GUI/deploy steps — *the* CI failure
**Found by:** S1 (A1, verified against the Chocolatey index) · S2 (N13) · forensics (SESSION_HANDOFF open item, "diagnosable from the workflow file itself")
**File:** `.github/workflows/build.yml`

**Root causes (three stacked), each verified against `build.yml` / `build.sh` line-by-line:**
1. `choco install qt6-base` (build.yml:28) — **no such package exists**; the community package is **`qt6-base-dev`** (Qt 6 SDK, MinGW flavor; ~5.7k downloads). `choco install` exits non-zero → the job dies at install. This is the most likely proximate cause of the failing Windows job recorded in SESSION_HANDOFF. (Note the asymmetry is visible in the file itself: the Linux job installs the correctly-named `qt6-base-dev` apt package at build.yml:14.)
2. Even with Qt installed, the GUI would still never build: the Windows job runs **bare `./build.sh`** (build.yml:38), and bare `build.sh` never builds the GUI by design (U-B9.3: `want_gui=auto` → forced to 0, build.sh:34–36). Qt's bin dir is also never added to PATH, there is no CMake GUI step, and the cmake path is broken anyway (U-B7). The job installs Qt for nothing.
3. There is no `windeployqt` step anywhere and no artifact upload, and the engine gets built **twice** (once via `build_gifsicle.sh --windows` at build.yml:33, then again natively inside `./build.sh` step 1 at build.yml:38).

**Fix — recommended workflow (aqtinstall is deterministic; choco is moderation-fragile):**
```yaml
windows:
  runs-on: windows-latest
  defaults: { run: { shell: bash } }
  steps:
    - uses: actions/checkout@v4
    - name: Install Qt6 (MinGW) + toolchain
      run: |
        pip install aqtinstall
        aqt install-qt windows desktop win64_mingw -O "$RUNNER_TEMP/qt"
        echo "$RUNNER_TEMP/qt/6.*/mingw_64/bin" >> "$GITHUB_PATH"
        choco install mingw -y --no-progress    # or use Qt-bundled Tools/mingw*
    - name: Build engine ONCE (Windows)
      working-directory: working_code/gifscythe
      run: ./scripts/build_gifsicle.sh --windows
    - name: Verify engine
      working-directory: working_code/gifscythe
      run: ./scripts/test_engine.sh
    - name: Build CLI + tests
      working-directory: working_code/gifscythe
      run: ./build.sh
    - name: Build GUI (CMake) + deploy Qt runtime
      working-directory: working_code/gifscythe
      run: |
        cmake -S . -B build-win -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_PREFIX_PATH="$RUNNER_TEMP/qt/6.*/mingw_64"
        cmake --build build-win
        windeployqt --release --no-translations build-win/gifscythe.exe
    - uses: actions/upload-artifact@v4
      with: { name: gifscythe-windows, path: working_code/gifscythe/build-win }
```
(Alternative one-liner for Qt: `jurplel/install-qt-action@v3`.)

---

### U-B9 🟠 `build.sh` GUI dispatch: three logic errors
**Found by:** S1 (A8) · S3 (M5, M6 partial)
**File:** `build.sh`

1. **cmake→qmake misreport:** if `qmake*` is absent, the script builds via cmake and sets `qt_found=1`; the *next* block then tries `qmake6` anyway, fails, and prints **"GUI build failed"** — even though cmake just succeeded.
2. **`--gui` doesn't match its own header** ("build only the Qt6 GUI, skip engine/tests") — steps 1–3 always run.
3. **Plain `./build.sh` never builds the GUI** even where Qt6 exists (`want_gui=auto` is immediately forced to `0`), contradicting its header comment "+ GUI (only where Qt6 is installed)".

**Fix.** One honest GUI branch: try `qmake6` → `qmake` → cmake in order, report truthfully, and when a GUI was requested but can't build, **exit non-zero**.

---

### U-B10 🟠 Uninitialized reads on malformed settings (undefined behavior)
**Found by:** S3 (C5) · S2 (C5) · S1 (A7) — **reproduced**
**File:** `src/core/SettingsIO.h:29–34` (`to_long`/`to_double`)

**Symptom.** `long v; i >> v; return v;` — if extraction fails (`lossy = abc`), `v` is indeterminate → UB; the garbage flows into the option pipeline. On gcc 12.2 the stack happened to read 0 (observed live: `--lossy=0 -O0` silently emitted for garbage input) — other builds/compilers can yield any value, and an in-range garbage color count would pass `GifsicleCommand`'s range checks silently.

**Fix.**
```cpp
inline bool to_long(const std::string& s, long* out) {
  std::istringstream i(trim(s));
  long v = 0;
  if (!(i >> v)) return false;    // keep default; report the key as a load warning
  *out = v;
  return true;
}
```
Add a warning channel so corrupt config files can be surfaced (CLI prints; GUI shows field hints). Also clamp parsed values to legal ranges.

---

## 4. Master findings list — MISALIGNED code (code contradicts its own docs/labels)

### U-M1 🟠 Version "single source of truth" hardcoded in 7 places
**Found by:** S3 (M1) · S1 (C1)
`0.1.0` appears in: `VERSION.md:4` (the declared truth), `CMakeLists.txt:2`, `gifscythe.pro:16`, `qtui/main.cpp:9` (`setApplicationVersion`), `MainWindow.cpp:24` (window title), `cli/main.cpp:70` (default engine path), `scripts/test_engine.sh:13`. A version bump means editing 7 files; `test_engine.sh` would silently test the **previous** engine after a bump.
**Fix:** generate `version.h` from `VERSION.md` at configure time (`configure_file`), use `GS_VERSION` everywhere; scripts grep `VERSION.md` like the packaging scripts already do.

### U-M2 🟠 "Add GIF files…" replaces the queue instead of appending
**Found by:** S3 (M3) · S1 (note)
`chooseInputs()` does `inputs_ = files; inputList_->clear();` — the second selection throws away the first. Opposite of every batch queue (XnConvert, Caesium).
**Fix:** append + dedupe; add Remove-Selected and Clear-All (already on WORKLIST task 4).
```cpp
for (const auto& f : files) {
  if (inputs_.contains(f)) continue;
  inputs_.append(f);
  inputList_->addItem(QFileInfo(f).fileName());
}
```

### U-M3 🟠 The "live" command pane isn't live
**Found by:** S3 (M4) · S1 (A4)
The hint label promises *"The command preview is always kept in sync"*, but only the two spinboxes are connected to `refreshCommand()`. `outputEdit_` has no `textChanged` connection (only the Browse handler refreshes), and queue changes don't refresh either. The pane is the product's flagship power-user feature (FEASIBILITY_REVIEW core UX promise).
**Fix:** connect `outputEdit_::textChanged` and the list model's row changes — or better, one `settingsChanged()` signal owned by MainWindow that every control emits.

### U-M4 🟠 build.sh docs promise a GUI the default path never builds
**Found by:** S3 (M5) · S1 (A8)
Header comment vs. code disagree (see U-B9.3). *"The build script is this repo's contract with its next session"* — pick one story.

### U-M5 🟠 Dead config.h writer + swallowed Qt-build errors
**Found by:** S3 (M6) · S1 (C3)
`build_gifsicle.sh` contains an `if [[ ! -f config.h ]]` block that announces "writing a generated one" and **writes nothing** — misdirection about where the hand-written `reference_code/gifsicle/config.h` comes from. Meanwhile `build.sh` sends the entire Qt build to `/dev/null` (`>/dev/null 2>&1`), so the most fragile step leaves zero diagnostics, and its error message says "see Qt errors above" when no errors are visible.
**Fix:** delete the dead branch (state the real config.h source in one comment); log Qt build output to a file and `tail` it on failure.

### U-M6 🟠 Packagers look for the GUI where cmake doesn't put it
**Found by:** S3 (M7) · S1 (related)
The cmake path emits the GUI into `build/`, the qmake path into `build/gui/` — both packaging scripts only copy `build/gui/gifscythe`. Depending on which builder ran, **the "portable GUI release" silently ships without the GUI**. `windeployqt` (what makes Windows packages actually portable) is manual-only.
**Fix:** one build path (cmake), one output location; `package_portable.sh` runs `windeployqt` automatically on Windows; CI asserts expected files exist in the portable folder.

### U-M7 🟡 Engine identity vs. product identity conflated
**Found by:** S1 only (C2)
`build_gifsicle.sh` compiles with `-DVERSION="<product version>"`, so `gifsicle --version` reports *LCDF Gifsicle 0.1.0* instead of 1.96. Engine version ≠ product version; keeping 1.96 in the engine output preserves upstream traceability.
**Fix:** keep `-DVERSION=\"1.96\"` for the engine; use the product version only for the release directory name.

### U-M8 🟡 Settings parser inconsistencies
**Found by:** S1 only (C5)
- Bool parsing is inconsistent: `info` accepts `1/true/yes`, every other bool only `1/true`.
- `rotation` accepts `90/180/270` but no `none`.
- `(unsigned)to_long(v)` wraps negative inputs silently.
**Fix:** one `parse_bool()` helper; accept `none` for rotation; reject/clamp negatives.

### U-M9 🟡 Doc & repo hygiene drift
**Found by:** S1 only
- Root `README.md` layout block still says `gifsicle-1.96/` (stale folder name).
- `.gitignore` ignores `release/*/gifsicle` but **not** the packaged `release/<v>/Gifscythe*/` folders the package scripts create.
- Qt6 deprecations in use: `QStringList::toStdVector()` / `QStringList::fromStdVector()` (deprecated since Qt 6.0) — use range constructors.
- Two parallel build systems (`gifscythe.pro` + CMake) with drifting truth — standardize on CMake (required for Windows CI anyway).

---

## 5. Refuted false positives from the secondary audit (S2/Site 4)

The attached audit's N-findings are mostly valuable, but four of them contradict **primary sources** — the committed engine source, the official man page, or documented build-tool behavior (each item names its evidence; grading in §5.1). They should **not** be "fixed" — changing correct code to satisfy them would introduce real bugs.

### ❌ N1 — "loopcount=0 emits the wrong flag; change to `--loopcount=forever`"
**Refuted.** Official man page: *"Note that **--loopcount**=0 is equivalent to **--loopcount**=forever, not **--no-loopcount**."* Committed source (`gifsicle.c`):
```c
case 'l':
  MARK_CH(output, CH_LOOPCOUNT);
  if (clp->negated)
    def_output_data.loopcount = -1;                            // --no-loopcount
  else
    def_output_data.loopcount = (clp->have_val ? clp->val.i : 0); // bare -l → 0 = forever
```
The current code emitting `--loopcount=0` for "loop forever" is **exactly right**. (Both forms work; `=0` is canonical.)

### ❌ N2 — "`-O0` is invalid; clamp the spinbox to 1"
**Refuted.** Committed source (`gifsicle.c`, `OPTIMIZE_OPT` handler):
```c
if (clp->negated || (clp->have_val && clp->val.i < 0))
  o = 0;
else
  o = (clp->have_val ? clp->val.i : 1);
…
def_output_data.optimizing = (def_output_data.optimizing & ~GT_OPT_MASK) | o;
```
`-O0` is accepted without error and means **"no optimization"** (clears the level mask) — a legitimately useful setting ("optimize = 0 → turn optimization off"). The man page lists three positive levels but the parser does not reject 0. No clamp needed; if anything, label 0 as "Off" in the GUI.

### ❌ N11 — "MainWindow.h missing from CMake sources → linker 'undefined reference to vtable'"
**Refuted for the current build files.** `qt_standard_project_setup()` (present in `CMakeLists.txt`, Qt ≥ 6.3) sets `CMAKE_AUTOMOC ON`, and AUTOMOC scans headers included by listed sources — `MainWindow.h` is `#include`d by `MainWindow.cpp`, so `moc_MainWindow.cpp` is generated and linked. The qmake path handles moc automatically too. Adding the header to the target sources is a harmless belt-and-braces measure, but there is no link failure to fix. (Caveat: `qt_standard_project_setup()` requires Qt ≥ 6.3; on older Qt you'd need manual `set(CMAKE_AUTOMOC ON)`.)

### ❌ N14 — "`gamma` defaults to 0.0 so `--gamma 0.0` is always emitted"
**Refuted.** `GifsicleSettings.h` already does what the finding recommends:
```cpp
double gamma = -1.0;   // --gamma; -1 = unchanged
…
if (s.gamma >= 0) { add(args_, "--gamma"); add(args_, f2s(s.gamma)); }
```
Sentinel is `-1.0`, gate is `>= 0`. No change needed.

### ⚠️ Downgraded (partially valid):
- **N6** ("build.sh runs g++ and cmake in the same directory, artifacts overwrite") — real but low-impact: the cmake fallback writes into the same `build/` and can overwrite `gifscythe-cli`; same program, messy not broken. Covered properly by U-B9/one-build-path.
- **N7** ("fragile `$2` argument passing") — `${2:-}` is already safe under `set -u`; advisory only.
- **N10** (`ls | wc -l` counting in `test_engine.sh`) — valid minor robustness nit (use `find … | wc -l` or a glob array).

### 5.1 Adjudication method & evidence provenance (transparency record)

This compilation **never refutes one agent with another agent's opinion**. Every refutation above is grounded in a primary source fetched or read during this compilation; agent claims are used only as *pointers to where to look*, never as proof. The exact evidence:

| Refuted claim | Primary evidence (quoted) | Source fetched/read by |
|---|---|---|
| N1 "`--loopcount=0` plays once" | Man page: *"Note that **--loopcount**=0 is equivalent to **--loopcount**=forever, not **--no-loopcount**."* · Committed engine source: `case 'l': … def_output_data.loopcount = (clp->have_val ? clp->val.i : 0);` | this compilation — lcdf.org man page; `reference_code/gifsicle/src/gifsicle.c` pulled from the repo via sparse checkout |
| N2 "`-O0` is invalid" | Committed engine source, `OPTIMIZE_OPT` handler: value 0 is accepted (`o = clp->val.i`) and clears the optimize mask — there is **no error path for 0** in the parser | this compilation — same source file |
| N14 "gamma defaults to 0.0, always emitted" | Project's own file `src/core/GifsicleSettings.h`: `double gamma = -1.0; // --gamma; -1 = unchanged` and `if (s.gamma >= 0) { … }` | this compilation — direct source read |
| N11 "missing MOC header → vtable link failure" | `CMakeLists.txt` calls `qt_standard_project_setup()`, which sets `CMAKE_AUTOMOC ON` (Qt ≥ 6.3); AUTOMOC mocs `MainWindow.h` because it is `#include`d by the listed source `MainWindow.cpp`; the qmake path mocs `Q_OBJECT` headers automatically | this compilation — Qt/CMake documented behavior. **Weakest grade: not execution-verified here (no compiler in sandbox).** Adding the header to sources remains a harmless precaution |

**Evidence grades used throughout this document:**

| Grade | Meaning | Examples |
|---|---|---|
| **[V1] Primary-verified by this compilation** | I read the actual file / fetched the official doc myself | All refutations above; the workflow file contents; the Chocolatey package-name check; every code-level root cause (C1 `system()` return, C4 `readAllStandardError`-only, C5 uninitialized `long v`, M2 STATIC-with-headers, M3 `inputs_ = files`, M4 missing `textChanged`, drag-drop absent, `save_settings` absent) |
| **[V2] Attributed reproduction from the forensic audits (S3–S5)** | Live-run transcripts I could **not** re-run (no compiler/Qt in this sandbox — the same constraint SESSION_HANDOFF records). Quoted verbatim, attributed, never altered | 35/35 unit assertions; 5/5 engine tests; 32512→exit-0 transcripts; 13-frame merge; 821-byte stdout trace; 9,458-byte output GIF; CMake 3.30.2 error log; zero-warning compile |
| **[V3] Inferred from documented toolchain behavior** | Correct per docs but not executed here | N11 refutation; CMake "no link language for header-only STATIC" (this one is additionally backed by the audit's [V2] CMake 3.30.2 reproduction, making it effectively confirmed) |

**Audit-vs-audit description differences (kept, not smoothed over):**
- **U-B3 input composition:** the attached audit (S2) describes the 13-frame result as "original + exploded frames"; the forensic audits (S3/S5) describe it as `logo.gif` (12 images) + `logo1.gif` (1 image). Both describe the same command (`gifsicle -m logo.gif logo1.gif -o out.gif`) and the same mechanism; the difference is descriptive only. This compilation quotes the forensic transcript.
- **U-B2 framing:** Site 1 frames it as "1 of 3 CWDs works"; Site 3's second pass frames it as "relative path fails, absolute path works." Same root cause (string-sniffed repo root); both framings preserved in the finding.
- The site counters ("23/20/21 findings filed") differ between the three renders while the body contains the same 18 finding cards — reported as observed, not reconciled silently.

**Nothing was dropped:** all 7 confirmed findings (C1–C5, G3, M2), all 15 N-findings, and all 18 forensic finding cards appear in this document — disputed ones retained *with their original claim text, severity and recommended fix* in §5, followed by the counter-evidence. One consequence noted for completeness: the attached audit's P1 recommendation *"Fix Argument Logic (N1, N2, N14)"* is deliberately **not** carried into the plan in §9, because all three of its premises are refuted above with [V1] evidence — implementing it would have broken correct code.

**Self-correction record:**
1. An earlier revision of this compilation stated "4 of 7 doc claims hold"; the forensic table says 3 hold / 3 partial / 1 fail. Corrected in §1 and §7.
2. An earlier revision of U-B8 claimed build.sh's `qmake6`/`qmake` name-lookup was the reason the Windows GUI never builds. Re-checking the source showed the stronger, verifiable reason: the Windows job invokes bare `./build.sh`, which *by design* never builds the GUI (U-B9.3), independent of qmake naming. Sharpened as above.

**Source-verification pass (performed 2026-09-06 against a fresh clone of `main`):** every `file:line` reference in this document was re-checked with `grep -n`/`sed` against the actual repo files. Results: all forensic-audit line citations matched exactly (`cli/main.cpp:54–56, 70, 90–113, 142–144`; `MainWindow.cpp:24, 94–101, 108–113, 122–138`; `SettingsIO.h:29–34`; `CMakeLists.txt:8–12`; version table — all 7 locations verified at `VERSION.md:4`, `CMakeLists.txt:2`, `gifscythe.pro:16`, `qtui/main.cpp:9`, `MainWindow.cpp:24`, `cli/main.cpp:70`, `test_engine.sh:13`). Quoted code fragments (`inputs_ = files; inputList_->clear();`, `--loopcount=0` emission, `if (s.gamma >= 0)`, the `case 'l':` and `OPTIMIZE_OPT` handlers, the CMake STATIC block, the `build.sh >/dev/null 2>&1` lines, the config.h dead branch, `package_portable.sh`'s `build/gui/` lookup) were verified verbatim. Additional confirmations: no `save_settings` anywhere (grep); no LICENSE/COPYING file in the repo (find); `test_engine.sh` referenced by neither `build.sh` nor the workflow (grep); unit suite is 35 CHECK assertions (+1 macro definition line); `test_gifsicle_command` contains no test for exit codes, CWD dependence, empty output, or malformed configs. The only adjustments made from this pass are the two corrections above — no finding changed verdict.

---

## 6. Master findings list — MISSING logic (promised by docs, never written)

### U-MISS-1 🟠 `SettingsIO` can't save — "load/save" is load-only
**Found by:** S3 (G1) · S1 (B1)
The header describes itself as the "load/save" serialization the GUI "will also use to persist per-file settings (and to round-trip state)". There is **no writer**. No presets, no per-file persistence, no CLI↔GUI round-trip.
**Fix:** `save_settings()` as the exact inverse of `load_settings()` (same keys, enums→strings) + round-trip unit test `load(save(s)) == s`.

### U-MISS-2 🟠 No validation layer — bad values vanish silently
**Found by:** S3 (G2) · S1 (C9)
Out-of-range settings are dropped without a trace: `colors = 999` emits no `-k`; `disposal = 9` emits nothing; `info=true` + `Mode::Batch` will be rejected by gifsicle itself but nothing pre-checks it. In a GUI, a control that "does nothing" is indistinguishable from a bug.
**Fix:** `gs::validate(const Settings&) → std::vector<Warning>{field, value, reason}`; CLI prints them, GUI shows inline hints; also makes U-B10's garbage values detectable.

### U-MISS-3 🟠 No safe-execution layer / no shell quoting
**Found by:** S3 (G3) · S1 (B5, B6) — see U-B6 for the broken side. Execution must use argv (`posix_spawn`/`execvp`/CreateProcess); display must `shell_quote()`. Also `~` paths are accepted by `resolve_path()` but never expanded — the engine receives a literal `~` and fails; expand `$HOME` or reject with a clear error.

### U-MISS-4 🟠 GUI: no async execution, no progress, no cancel
**Found by:** S3 (G4) · S2 (N15) — see U-B5 for the broken side. WORKLIST task 4 (progress, cancel) has no foundation until QProcess moves to signals (`started`/`readyReadStandardError`/`finished`), with a busy state disabling Run and Cancel→`kill()`.

### U-MISS-5 🟠 No engine discovery / startup health check
**Found by:** S3 (G5) · S1 (B4) — see U-B2. Both front-ends invented their own hardcoded locator. Put `gs::EngineLocator` in `src/core` (exe-dir → `GS_ENGINE` env → PATH → `release/<GS_VERSION>`), call it at startup, and show engine path+version in the GUI status bar (which also *proves* the "retained terminal control" story).

### U-MISS-6 🟠 Tests never exercise the flows that actually fail
**Found by:** S3 (G6) · S1 (B7)
35 assertions, all in-process argv assembly on happy paths. Nothing covers: `--run` exit codes (U-B1), CWD sensitivity (U-B2), merge-vs-batch semantics (U-B3), empty-output runs (U-B4), malformed conf (U-B10), paths with spaces (U-B6), SettingsIO round-trip (U-MISS-1). IMPROVEMENT_LOG's claim "Unit + integration tests pass" has **no integration test** — the only end-to-end run is manual. Additionally, `scripts/test_engine.sh` (the one real end-to-end suite) is **not called by build.sh or CI**, and CI uploads no artifacts.
**Fix:** smoke suite in CI: `--run` from ≥2 CWDs · malformed-conf · merge-vs-batch · round-trip · portable-folder file-existence checks · `test_engine.sh` in both jobs · artifact upload.

### U-MISS-7 🟡 Drag-and-drop: enabled but not implemented
**Found by:** S2 (N4) · S1 (A5)
`inputList_->setAcceptDrops(true)` with no `dragEnterEvent`/`dropEvent` override → dropping files does nothing. WORKLIST task 4 promises it.
**Fix:** subclass `QListWidget` (or event filter), accept `Qt::CopyAction`, append `mimeData()->urls()` in `dropEvent` (and dedupe).

### U-MISS-8 🟡 No queue management controls
**Found by:** S2 (N5)
No Remove-Selected, no Clear-All — users must restart to drop one file. Already on WORKLIST task 4.

### U-MISS-9 🟡 No mode selector — Batch/Explode/Auto unreachable in the GUI
**Found by:** S2 (N3)
`Mode::Merge` is hardcoded; `Mode::Batch`, `Mode::Explode`, `Mode::Auto` exist in the core layer and settings format but no widget exposes them.
**Fix:** `QComboBox` (Auto/Merge/Batch/Explode) — required anyway once U-B3 is fixed, so the user can choose "optimize each" vs. "merge into one".

### U-MISS-10 🟡 Dither is bool-only; docs promise method selection
**Found by:** S2 (N9)
FEASIBILITY_REVIEW's mapping table says `--dither=floyd-steinberg|ro64|none`. Core models `bool dither` and emits bare `-f` (default Floyd–Steinberg). Correct as far as it goes, but the promised richness is missing.
**Fix:** `std::string dither_method` ("" = off) emitting `-f` / `--dither=<method>`.

### U-MISS-11 🟡 No LICENSE / GPL compliance artifacts
**Found by:** S1 only
FEASIBILITY_REVIEW §5 is explicit: distributing gifsicle (GPL **v2-only**) alongside a GPLv3 UI requires shipping gifsicle's license text + source offer (the subprocess architecture was chosen precisely to make this clean). There is no `LICENSE`/`COPYING` in the repo, and `package_portable.sh` ships no license files.
**Fix:** add `COPYING` (GPLv2 text for the engine) + attribution; copy them into the portable folder; keep gifsicle a separate process forever unless Eddie Kohler grants a v3 exception.

### U-MISS-12 🟡 Windows packaging path incomplete
**Found by:** S3 (M7) · S2 (implicit) — WORKLIST tasks 1–2
No `windeployqt` automation, no Windows GUI build in CI, no artifact, no smoke test of the portable folder on a clean machine. This is the whole remaining path to 1.0.0.

### U-MISS-13 🟡 Remaining GifsicleSettings controls unexposed in GUI
**Found by:** WORKLIST task 5 (gap noted by all reviews)
The GUI exposes 2 of ~30 modeled controls. FEASIBILITY_REVIEW §3(2) already contains the complete slider↔flag mapping table — the retrofit is mechanical widgets↔settings work on top of the existing core. (Planned, not a bug — listed for completeness.)

---

## 7. Claim vs. code — every doc claim re-verified

From the forensic audit's §03, with this compilation's annotations:

| Claim | Source doc | Verdict | Note |
|---|---|---|---|
| P1 engine control layer complete | WORKLIST | ✅ HOLDS | Core compiles clean; tests pass; flag mapping faithful (independently re-verified vs. man page + engine source) |
| Qt6 GUI MVP scaffold + first retrofit | WORKLIST | ⚠️ PARTIAL | Window drives the core layer — but run flow is broken (U-B3, U-B4, U-B5) and blocks the UI (U-MISS-4) |
| Linux CI passes with Qt6 GUI | IMPROVEMENT_LOG / HANDOFF | ⚠️ PARTIAL | Job is green, but GUI build attempts are silenced/failure-tolerant (U-B7, U-M5) and **no artifact proves a GUI was built**. Green ≠ GUI built |
| CLI driver + integration tests | IMPROVEMENT_LOG | ❌ FAILS | CLI exists, but `--run` works from one CWD only and exits 0 on failure (U-B1, U-B2); **no automated integration test exists** (U-MISS-6) |
| Packaging scripts done | WORKLIST | ⚠️ PARTIAL | Scripts exist, pass `bash -n` — but expect the GUI where cmake never puts it; a "GUI release" can ship GUI-less (U-M6) |
| Don't bump to 1.0.0 before UI/UX retrofit | VERSION / HANDOFF / WORKLIST | ✅ HOLDS | Version discipline intact; the criticals above are exactly why the rule must keep holding |
| WebP/APNG deferred until GIF UI stable | VISION / bucket list | ✅ HOLDS | Scope clean — no premature codec code |

> **Pattern diagnosis (forensic audit, endorsed):** the project's failure mode isn't recklessness — it's **green-checkmark inflation**: claims marked done without machine-checked proof. Fix the proof (CI artifacts, exit codes, assertions), and the checkmarks become trustworthy again.

---

## 8. The 4 context documents, read as a system

| Doc | Role | What the audit found |
|---|---|---|
| `PROJECT_VISION.md` | what & why | Casual users, XnConvert-feel over gifsicle, GIF/APNG/WebP only, portable, terminal control retained. **U-B3/U-B4 violate "normal person" trust directly** — the vision is sound; the code must serve it, not the reverse |
| `WORKLIST.md` | task board | P0–P1 checked; Windows CI + GUI verification + UX retrofit remain before 1.0.0. Two checked boxes (GUI MVP, packaging) only partially hold |
| `SESSION_HANDOFF.md` | next-session state | "Linux CI passes; Windows CI fails, log unretrievable; no Qt in sandbox." The Windows failure **is** diagnosable from the workflow file itself (U-B8): wrong package + double engine build + no GUI step |
| `IMPROVEMENT_LOG.md` | decision log | Compile claims verify. The "integration tests pass" / "GUI built via Linux CI" claims do not survive contact with the code (§7) |
| `FEASIBILITY_REVIEW.md` | architecture context | Subprocess wrapper = right MVP; live raw-CLI pane = core UX promise; license math favors the wrapper. Current architecture already matches the review — **the gaps are executional (U-B6 unsafe exec, U-M3 unsynced pane), not architectural** |

---

## 9. THE PLAN — what to do, in order

Sequenced to respect the project's own rules: **version stays 0.x, APNG/WebP stays in the bucket, and every phase ends with CI evidence rather than a checked box.** Items cite the finding IDs they close.

### 🚨 P0 — Stop the silent failures (before ANY feature work)
Every critical shares one property: *the tool says success while doing wrong or nothing*. For a casual-user product that's the worst failure class.

| # | Action | Closes | Est. |
|---|---|---|---|
| P0-1 | CLI: replace `system()` with argv exec (`fork/execvp`; CreateProcess on Windows); decode exit with `WEXITSTATUS`; pre-flight engine existence check | U-B1, U-B6 | half day |
| P0-2 | CMake: `gifscythe_core` → `INTERFACE`; stop swallowing cmake output in build.sh; fail hard when GUI requested but unbuilt | U-B7, U-B9 | 1 h |
| P0-3 | Windows CI: aqtinstall (or `qt6-base-dev`), PATH to Qt bin, build engine ONCE, build GUI via CMake, `windeployqt`, upload artifact, run `test_engine.sh` | U-B8 | half day |
| P0-4 | GUI run flow: require/default an output path; verify output file exists+non-empty before claiming success; handle `waitForFinished` timeout (kill + honest error) | U-B4, U-B5 | half day |
| P0-5 | GUI queue semantics: `Mode::Batch` default (per-file), add mode combo; Merge only as explicit action | U-B3, U-MISS-9 | half day |
| P0-6 | SettingsIO: zero-init + extraction checks + load warnings; fix bool/rotation inconsistencies | U-B10, U-M8 | 2 h |

### ⚠️ P1 — Make the code honest (one truth per concept)

| # | Action | Closes | Est. |
|---|---|---|---|
| P1-1 | Single version source: generate `version.h` from `VERSION.md`; delete the 6 duplicate strings; `test_engine.sh` greps VERSION.md | U-M1 | half day |
| P1-2 | Engine locator in core (exe-dir → env → PATH → release/<ver>), used by CLI **and** GUI; startup health check; show engine path+version in status bar | U-B2, U-MISS-5 | half day |
| P1-3 | Queue UX: append+dedupe, Remove-Selected/Clear-All, working drag-and-drop subclass | U-M2, U-MISS-7, U-MISS-8 | half day |
| P1-4 | Live pane honesty: connect `outputEdit_::textChanged` + list-model changes (or one `settingsChanged()` signal); add `shell_quote()` for display | U-M3, U-MISS-3 | 2 h |
| P1-5 | Build hygiene: align build.sh header↔behavior; delete dead config.h branch; log Qt build output to file, tail on failure; one build path (CMake), one output dir; `~` expansion in resolve_path | U-M4, U-M5, U-B9, U-MISS-3 | half day |
| P1-6 | `save_settings()` + round-trip test; `gs::validate()` warnings surfaced in CLI/GUI | U-MISS-1, U-MISS-2 | half day |
| P1-7 | Packaging: packager follows the single CMake output dir; `windeployqt` inside `package_portable.sh` on Windows; CI asserts portable-folder contents | U-M6, U-MISS-12 | half day |
| P1-8 | Compliance: add `COPYING` (GPLv2) + attribution; ship in portable folder | U-MISS-11 | 1 h |

### 🛠 P2 — Prove it all (test & CI hardening)

| # | Action | Closes | Est. |
|---|---|---|---|
| P2-1 | Smoke suite in CI: `--run` from ≥2 CWDs; malformed-conf; merge-vs-batch scenario; paths-with-spaces; SettingsIO round-trip | U-MISS-6 | 1 day |
| P2-2 | Both jobs run `test_engine.sh`; both jobs upload artifacts; cache apt/choco/pip | U-MISS-6 | 2 h |
| P2-3 | Async QProcess (signals) + progress bar + cancel + busy-state; per-file progress in batch mode | U-MISS-4 | 1 day |
| P2-4 | Hygiene: root README folder name, .gitignore for packaged folders, replace deprecated `toStdVector`, retire `gifscythe.pro` or mark it secondary | U-M9 | 2 h |

### 🎨 P3 — The UI/UX retrofit → 1.0.0 (WORKLIST tasks 3–7)

| # | Action | Closes |
|---|---|---|
| P3-1 | Input / Actions / Output tab flow (XNConvert feel) on top of the now-honest core | WORKLIST 3 |
| P3-2 | Expose remaining `GifsicleSettings` controls using the FEASIBILITY_REVIEW §3(2) mapping table; keep the live raw-CLI pane; add dither-method selector | U-MISS-10, U-MISS-13 |
| P3-3 | Before/after preview, file size/count display, output-folder actions | WORKLIST 4 |
| P3-4 | Clean-machine Windows smoke test of the portable folder; document release procedure | WORKLIST 6 |
| P3-5 | **Only then:** bump `1.0.0`. WebP/APNG stays in the bucket until the GIF UI is stable | WORKLIST 7 |

---

## 10. Files-to-modify matrix

| File | Key actions | Finding IDs |
|---|---|---|
| `src/cli/main.cpp` | argv exec + WEXITSTATUS; engine existence pre-flight; realpath settings; exe-anchored engine locator; `~` expansion | U-B1, U-B2, U-B6, U-MISS-3, U-MISS-5 |
| `src/qtui/MainWindow.cpp` (+ `.h`) | batch-default + mode combo; output validation + post-run file check; async QProcess + progress/cancel; append/dedupe/remove/clear queue; drag-drop subclass; `textChanged` wiring; engine locator + status bar | U-B3, U-B4, U-B5, U-M2, U-M3, U-MISS-4, U-MISS-7, U-MISS-8, U-MISS-9 |
| `src/core/GifsicleCommand.h` | `shell_quote()` for `toString()` display only (exec uses argv) | U-B6, U-MISS-3 |
| `src/core/GifsicleSettings.h` | (after decision) dither_method string; keep loopcount/gamma/optimize exactly as-is (N1/N2/N14 refuted) | U-MISS-10 |
| `src/core/SettingsIO.h` | safe `to_long/to_double`; load warnings; `save_settings()`; parse_bool/`none` consistency | U-B10, U-M8, U-MISS-1 |
| `src/core/` (new) | `EngineLocator`, `validate()`, `version.h.in` | U-B2, U-MISS-2, U-MISS-5, U-M1 |
| `CMakeLists.txt` | INTERFACE core; version.h generation; keep `qt_standard_project_setup` (AUTOMOC is fine) | U-B7, U-M1 |
| `build.sh` | honest GUI dispatch (qmake6→qmake→cmake), no silencing, header=behavior, single output dir | U-B9, U-M4, U-M5 |
| `scripts/build_gifsicle.sh` | delete dead config.h branch; engine `-DVERSION=\"1.96\"`; guard `$2` docs | U-M5, U-M7 |
| `scripts/test_engine.sh` | grep version from VERSION.md; `find`-based counting | U-M1, N10-note |
| `scripts/package_*.sh` | follow single GUI output dir; windeployqt on Windows; ship COPYING | U-M6, U-MISS-11, U-MISS-12 |
| `.github/workflows/build.yml` | full Windows rewrite (§3 U-B8 YAML); test_engine.sh both jobs; artifacts; cache | U-B8, U-MISS-6 |
| `tests/` | round-trip, malformed-conf, spaces-in-paths, batch-vs-merge scenario, CLI smoke from 2 CWDs | U-MISS-6 |
| repo root | `COPYING`; README folder-name fix; `.gitignore` packaged folders | U-MISS-11, U-M9 |

---

## 11. Definition-of-done per phase (evidence, not checkmarks)

- **P0 done when:** `gifscythe-cli` returns non-zero for a missing engine (verified from 2 CWDs); GUI refuses empty output and verifies the file; queue of 2 GIFs produces 2 optimized files (verified with `gifsicle --info`); CMake configures cleanly; Windows CI job goes green **with an artifact**.
- **P1 done when:** version bump is a 1-file edit and all binaries/labels agree; drag-drop + remove/clear work; command pane matches actual execution byte-for-byte for paths with spaces; `save(load(s)) == s`; portable folder contains GUI + engine + licenses.
- **P2 done when:** the smoke suite in §P2-1 is green on Linux **and** Windows CI, with artifacts downloadable.
- **P3 done when:** clean-Windows-machine smoke test passes → then, and only then, `VERSION.md → 1.0.0`.

---

## 12. Source index

- Repository: https://github.com/freeforall1932-design/gifscythe (main)
- Audit site 1 (forensic, 23-counter render): https://01a075af-697d-7fc2-b5bd-182fd9399acf.arena.site/
- Audit site 2 (forensic, 20-counter render): https://01a075d0-709d-71ef-a314-c88c2a306508.arena.site/
- Audit site 3 (forensic, 21-counter render, incl. second verification pass): https://01a075d1-5370-7220-95cf-005482bf34e3.arena.site/
- Audit site 4 (22-finding report w/ N1–N15): https://01a07621-1747-7465-a5c9-cc183226787c.arena.site/
- Attached agent report: `Qwen_markdown_20260906_as7ahubyq.md` (identical content to site 4)
- Original review (predecessor of this compilation): `gifscythe-review.md`
- Verification references: gifsicle man page (https://www.lcdf.org/gifsicle/man.html); committed engine sources `reference_code/gifsicle/src/gifsicle.c`, `clp.c`; Chocolatey package `qt6-base-dev` (https://push.chocolatey.org/packages/qt6-base-dev)

*Compiled 2026-09-06. Every finding is traceable to a file and line; every refutation is traceable to committed source or the official man page.*
