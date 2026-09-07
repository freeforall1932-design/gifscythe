# Gifscythe — Code Review & Remediation Plan

**Repository:** `freeforall1932-design/gifscythe` (branch `main`) · **Version:** 0.1.0 · **Date:** 2026-09-06
**Scope:** complete read of `working_code/gifscythe/` (core headers, CLI, Qt GUI, tests, build scripts, CMake/qmake, CI workflow), audited against the project's own documents (`PROJECT_VISION.md`, `WORKLIST.md`, `SESSION_HANDOFF.md`, `IMPROVEMENT_LOG.md`, `FEASIBILITY_REVIEW.md`, `VERSION.md`) and against the gifsicle 1.96 man page and the gifsicle 1.96 source committed under `reference_code/`.

---

## 1. Executive summary

**Verdict:** The architecture is sound and the foundation genuinely works — but the product currently has a failure class worse than crashes: **silent false success**. In five separate places the tool reports success while doing nothing, doing the wrong thing, or destroying data. For a product whose mission is *"a normal person edits without surprises"* this is the worst possible failure class.

**Finding counts (all verified against source, latest pass 2026-09-06):**

| Category | Count |
|---|---|
| BROKEN — wrong results or silent failure at runtime | **14** |
| MISALIGNED — code contradicts its own docs/labels | **11** |
| MISSING — behavior the docs promise that doesn't exist | **16** |
| Verified-correct behaviors that *look* suspicious (do **not** "fix") | **5** (§6) |

### The five headline facts (each reproduced against the built engine)

1. **`gifscythe-cli --run` exits 0 when the engine doesn't exist.** `system()` returns the wait status 32512 (= 127«8); only the low 8 bits survive C's exit truncation → 0. CI and automation see a green run while nothing happened.
2. **The engine is only found from exactly one working directory** (repo root, absolute settings path). The README's own documented usage fails.
3. **The GUI welds the whole queue into one GIF.** `Mode::Merge` is hardcoded; `gifsicle -m -O3 logo.gif logo1.gif -o out.gif` produces **13 images** (12+1 concatenated) where the user expected two optimized files.
4. **An empty output field vaporizes the result.** gifsicle writes the optimized GIF to stdout; the GUI never reads stdout; the status bar still says *"Optimization complete."* Measured: 821 bytes discarded for a 1-frame GIF.
5. **The CMake build cannot configure at all.** `add_library(gifscythe_core STATIC)` with only a header source fails generation — reproduced with CMake 3.30.2: `Cannot determine link language for target "gifscythe_core"`. `build.sh` hides this with `>/dev/null 2>&1`.

Plus the **Windows CI failure**, diagnosable from the workflow file itself: `choco install qt6-base` references a package that does not exist (the real package is `qt6-base-dev`), the job never adds Qt to PATH, it runs bare `./build.sh` which by design never builds the GUI, and there is no `windeployqt` step. **And even after fixing all of that, the Windows engine still cannot compile** (BR-11): the build feeds the Linux `config.h` to the Win64 compiler, which trips gifsicle's own `static_assert`.

---

## 2. What actually works — verified, not assumed

| Verified working | Evidence |
|---|---|
| Core compiles clean: `g++ -std=c++17 -Wall -Wextra -pedantic`, zero warnings across `GifsicleSettings.h` / `GifsicleCommand.h` / `SettingsIO.h` (g++ 12.2) | build run |
| **35/35 unit assertions pass** in `tests/test_gifsicle_command.cpp` (merge, resize-fit, explode, crop, parser case-insensitivity) — 35 `CHECK()` calls + 1 macro definition | test run |
| Engine builds from the reference source via the direct-gcc path → `release/0.1.0/gifsicle` | build run |
| **5/5 engine pipeline tests pass**: `--info` (12 frames), `-O3` optimize, `--lossy=80`, `--resize-fit 30x66` (logical screen verified), explode → 12 frames | `scripts/test_engine.sh` live run |
| CLI produces a valid optimized GIF end-to-end (9,458-byte merged+optimized output) — **but only from repo-root CWD** (see BR-2) | live run |
| Architecture matches `FEASIBILITY_REVIEW.md`: Qt-independent header-only core shared by CLI+GUI; engine as subprocess (keeps GPL v2-only / GPLv3 separation clean); WebP/APNG correctly deferred | doc-vs-code check |
| Version discipline (still 0.1.0) and scope discipline (no premature APNG/WebP code) both hold perfectly | doc-vs-code check |

```text
$ ./scripts/test_engine.sh
==> Testing engine pipeline (release/0.1.0/gifsicle)
  PASS: info reads multi-frame gif
  PASS: optimize -O3 writes output
  PASS: lossy writes output
  PASS: resize-fit -> logical screen 30x66
  PASS: explode extracts 12 frames
==> Done. 5 passed, 0 failed.
# note: hardcoded path on line 13 — after the first version
# bump this tests the PREVIOUS engine (see AL-1).
```

### Option-mapping correctness (verified against the gifsicle 1.96 man page + committed engine source)

These generated flags were checked one-by-one and are **correct — no action needed**:

- `--crop X,Y+WxH` — the `+` form takes width/height per the man page (`--crop 0,0+30x60` valid; `2,2+-2x-2` shaves 2px borders).
- `--loopcount=0` = loop forever (see §6).
- `-O3`, `-j4`, `--lossy=N` attached forms — correct (optional-value options need attached form in CLP).
- Mode flags (`-m`/`-b`/`-e`) placed before filenames — required by gifsicle, done correctly.
- `-o` placed after inputs — allowed (general option).
- `-k N` (colors 2–256), `-f` (dither), `-d` (delay in 1/100 s), `-D 0..7` (disposal; man page allows 0–7 even though only 0–3 are meaningful), `-p X,Y`, `-E` (explode-by-name), `--flip-*`, `--rotate-*`, `-i`, `--careful`, `--no-comments/-names/-extensions` — all correct mappings.
- `scripts/build_gifsicle.sh` OBJS list matches upstream `gifsicle_SOURCES` exactly (clp, fmalloc, giffunc, gifread, gifsicle, gifunopt, gifwrite, kcolor, merge, optimize, quantize, support, xform).

---

## 3. BROKEN code — 14 findings

Severity: 🔴 CRITICAL (data loss / false success / security) · 🟠 MAJOR.

---

### BR-1 🔴 CLI `--run` exits 0 on total failure (false success)
**File:** `src/cli/main.cpp:54–56, 142–144`

**Symptom.** When the engine binary can't be found, the shell still runs the command, fails with 127, and the CLI prints `# -> exit code 32512` while returning **0** to the OS. Automation/CI sees a green run.

**Root cause.** `run_with_system()` returns the raw `system()` wait status unmodified; only the low 8 bits survive. There is also no existence check on the engine before running.

**Reproduction:**
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
Better still, stop using `system()` entirely (see BR-6).

---

### BR-2 🔴 Engine path resolution works only by accident (CWD-dependent)
**File:** `src/cli/main.cpp:70, 90–113`

**Symptom.** The default engine path is `working_code/gifscythe/release/0.1.0/gifsicle` resolved against a "repo root" that is only discovered when (a) the settings path is absolute **and** contains the literal string `/working_code/gifscythe`, or (b) the CWD *is* the repo root. Run from `examples/`, or from `working_code/gifscythe` as the README documents, and the engine "isn't there" — silently (BR-1 masks the error).

**Root cause.** `repo_root` derived by string-sniffing (`find("/working_code/gifscythe")`) + three-levels-of-parent fallback + CWD-relative probing: three anchor points that coincide for exactly one working directory. Sharpening detail (lines 72–81): the comment says *"Make base_dir absolute so it works regardless of CWD"*, but `base_dir` is only absolutized when it equals `"."` — i.e., only when the settings path contains **no slash**. For the documented `examples/animation.conf`, `base_dir` stays the relative string `examples`, `repo_root` becomes `examples`, and the engine probe becomes `examples/working_code/gifscythe/release/0.1.0/gifsicle`.

**Reproduction:**
```text
# relative settings path → bogus nested engine path → fail, exit 0(!)
$ gifscythe-cli examples/animation.conf --run
sh: 1: examples/working_code/gifscythe/release/0.1.0/gifsicle: not found
# -> exit code 32512

# absolute settings path → works
$ gifscythe-cli $PWD/examples/animation.conf --run
# -> exit code 0   →  /tmp/gifscythe_demo.gif 9458 bytes ✓
```

**Fix.** Resolve the settings path to absolute (`realpath` / `std::filesystem::absolute`) at startup — this deletes most of the repo_root sniffing; anchor engine search on the **executable location**, not CWD:
```cpp
const char* candidates[] = {
  exe_dir + "/gifsicle",                              // packaged layout
  exe_dir + "/../release/" + GS_VERSION + "/gifsicle" // dev layout
};
for (auto* c : candidates) if (fs_probe::exists(c)) { engine_path = c; break; }
// then: GS_ENGINE env var override, then PATH
```
(See AL-1 for the single-version-source that makes `GS_VERSION` possible.)

---

### BR-3 🔴 GUI welds the queue into one GIF (Merge semantics vs. batch intent)
**File:** `src/qtui/MainWindow.cpp:108–113` (`currentSettings()`)

**Symptom.** The UI presents an *"Animation queue"* with *"Add GIF files…"* — batch semantics — but every run hardcodes `s.mode = gs::Mode::Merge`. gifsicle `-m` concatenates all inputs frame-by-frame. Queue two GIFs to "optimize" and the tool fuses them into one animation. Silent data mangling of the user's intent.

**Reproduction:**
```text
$ gifsicle --info logo.gif      → 12 images
$ gifsicle --info logo1.gif     → 1 image
# exactly what runCommand() builds for the 2-file queue:
$ gifsicle -m -O3 logo.gif logo1.gif -o out.gif
$ gifsicle --info out.gif
* out.gif 13 images     ← 12 + 1, concatenated; logical screen 60x132
```

**Root cause.** Merge was the CLI demo scenario; the queue concept needs batch semantics. Merging is a deliberate *Convert-branch* action (per `FEASIBILITY_REVIEW.md`), not a default.

**Fix.** Default the queue to `Mode::Batch` (per-file processing, in-place or `<name>_opt.gif` outputs), or loop one run per input. Keep Merge only behind an explicit "Merge into one animation" action. Add a mode selector either way (see MS-9).

---

### BR-4 🔴 Empty output path vaporizes the result, then reports success
**File:** `src/qtui/MainWindow.cpp:122–138` (`runCommand()`)

**Symptom.** The output field is optional. Left empty, gifsicle writes the optimized GIF to **stdout** — which QProcess never reads or redirects. The bytes evaporate and the status bar announces *"Optimization complete."* Guaranteed silent data loss for the most common first run (user adds a GIF, clicks Optimize, skips the output field).

**Evidence:**
```text
$ gifsicle -O3 logo1.gif | wc -c
821                          # engine provably writes the whole file to stdout

# MainWindow reads ONLY stderr (MainWindow.cpp:131); readAllStandardOutput()
# is never called. → 821 bytes discarded, success branch taken because
# exitCode() == 0.
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

### BR-5 🔴 GUI run: `waitForFinished()` timeout ignored → zombie process + wrong status
**File:** `src/qtui/MainWindow.cpp:125–138`

**Symptom.** `proc.waitForFinished(60000)` (line 130) blocks the UI thread up to 60 s; its **bool return is ignored**. On timeout the process is still running, but `exitStatus()`/`exitCode()` on a live QProcess default to `NormalExit`/`0`, so the UI can report **success for a process that hasn't finished**; the `QProcess` destructor then kills it (`QProcess: Destroyed while process is still running`). Large animations — the app's entire reason to exist — exceed 60 s routinely.

**Fix.** Check the bool; on timeout `proc.kill(); proc.waitForFinished(1000);` and report failure. Proper fix is async (see MS-4): member `QProcess`, `finished`/`readyReadStandardError` signals, progress + cancel. This is the substrate WORKLIST task 4 stands on.

---

### BR-6 🔴 Shell execution of a concatenated command string (spaces + injection)
**Files:** `src/cli/main.cpp:54–56, ~140–144` · `src/core/GifsicleCommand.h` (`toString()`, ~184–188)

**Symptom.** `toString()` joins argv with bare spaces (no quoting), and the CLI executes exactly that string through `std::system()`. Any path with a space breaks; any settings value with shell metacharacters is *interpreted by the shell*. Power users are also invited to copy the unquoted line from the "show me the command" pane into a terminal, where it fails the same way.

**Reproduction:**
```text
# input = /tmp/my vacation/in.gif    output = /tmp/my vacation/out.gif
$ gifscythe-cli space.conf --run
…gifsicle -O3 /tmp/my vacation/in.gif -o /tmp/my vacation/out.gif
sh: 1: vacation/in.gif: not found
# -> exit code 32512   →  $? == 0   (see BR-1)

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

### BR-7 🔴 CMake cannot configure: header-only lib declared STATIC
**File:** `CMakeLists.txt:8–12`

**Reproduction (CMake 3.30.2):**
```text
-- Qt6 NOT found -> GUI skipped.
-- Configuring done (0.3s)
CMake Error: Cannot determine link language for target "gifscythe_core".
CMake Error: CMake can not determine linker language for target: gifscythe_core
-- Generating done (0.0s)
CMake Generate step failed. Build files cannot be regenerated correctly.
```

**Root cause.** A `STATIC` library needs ≥1 compiled translation unit. `target_sources(gifscythe_core PRIVATE src/core/SettingsIO.h)` contributes only a header → no link language. Linux CI passes only because `build.sh` prefers the qmake path; the cmake fallback swallows this error with `>/dev/null 2>&1` (build.sh:62–63) and prints nothing.

**Fix.**
```cmake
add_library(gifscythe_core INTERFACE)
target_include_directories(gifscythe_core INTERFACE src)
# remove the set_target_properties(... PUBLIC_HEADER ...) block
```
And stop silencing: when `--all` is requested and the GUI can't build, exit non-zero so CI fails honestly.

---

### BR-8 🟠 Windows CI: wrong package name + missing GUI/deploy steps — *the* CI failure
**File:** `.github/workflows/build.yml`

**Root causes (three stacked), each verified line-by-line:**
1. `choco install qt6-base` (build.yml:28) — **no such package exists**; the community package is **`qt6-base-dev`** (Qt 6 SDK, MinGW flavor). `choco install` exits non-zero → the job dies at install. (The asymmetry is visible in the file itself: the Linux job installs the correctly-named `qt6-base-dev` apt package at build.yml:14.)
2. Even with Qt installed, the GUI would still never build: the Windows job runs **bare `./build.sh`** (build.yml:38), and bare `build.sh` never builds the GUI by design (AL-4: `want_gui=auto` → forced to 0, build.sh:34–36). Qt's bin dir is also never added to PATH, and there is no CMake GUI step (broken anyway — BR-7). The job installs Qt for nothing.
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
(Alternative for the Qt step: `jurplel/install-qt-action@v3`.)

---

### BR-9 🟠 `build.sh` GUI dispatch: three logic errors
**File:** `build.sh`

1. **cmake→qmake misreport:** if `qmake*` is absent, the script builds via cmake and sets `qt_found=1` (line 64); the *next* block (lines 67–73) then tries `qmake6` anyway, fails, and prints **"GUI build failed (see Qt errors above)"** — even though cmake just succeeded, and no errors are visible because output went to `/dev/null`.
2. **`--gui` doesn't match its own header** ("build only the Qt6 GUI, skip engine/tests") — steps 1–3 always run.
3. **Plain `./build.sh` never builds the GUI** even where Qt6 exists (`""` → `want_gui=auto` → forced to `0`, lines 34–36), contradicting its header comment "+ GUI (only where Qt6 is installed)".

**Fix.** One honest GUI branch: try `qmake6` → `qmake` → cmake in order, report truthfully, and when a GUI was requested but can't build, **exit non-zero**.

---

### BR-10 🟠 Uninitialized reads on malformed settings (undefined behavior)
**File:** `src/core/SettingsIO.h:29–34` (`to_long`/`to_double`)

**Symptom.** `long v; i >> v; return v;` — if extraction fails (`lossy = abc`), `v` is indeterminate → UB; the garbage flows into the option pipeline. Observed live with gcc 12.2: a `bad.conf` of garbage values produced `--lossy=0 -O0` — the stack happened to read as 0. That is **not guaranteed**: other builds/compilers can yield any value, and an in-range garbage color count would pass `GifsicleCommand`'s range checks silently.

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

### BR-11 🔴 Windows engine is compiled against the Linux config.h — static_assert will fire
**Files:** `reference_code/gifsicle/config.h` · `reference_code/gifsicle/src/win32cfg.h` · `scripts/build_gifsicle.sh:28–34, 55–76` · `reference_code/gifsicle/src/gifsicle.c` (main, ~1462–1468)

**Symptom.** Even after BR-8 (the Chocolatey package name) is fixed, `./scripts/build_gifsicle.sh --windows` still feeds the handwritten **Linux** `config.h` to a Win64 compiler. `gifsicle.c`'s `main()` begins with:
```c
static_assert(sizeof(unsigned long) == SIZEOF_UNSIGNED_LONG, "unsigned long has the wrong size.");
```
On Win64 `sizeof(unsigned long)` is **4**, but `config.h:56` says `#define SIZEOF_UNSIGNED_LONG 8` → the compile fails. `config.h:61` also sets `PATHNAME_SEPARATOR '/'` and `config.h:39` `#define RANDOM random` — the Windows variants are `'\\'` and `rand`, exactly as the dedicated `src/win32cfg.h` provides (`SIZEOF_UNSIGNED_LONG 4`, `RANDOM rand`, `PATHNAME_SEPARATOR '\\'`). The engine source even says so in the comment above the asserts: *"If these assertions fail, you've used the wrong Makefile. You should've used Makefile.w32 for 32-bit Windows and Makefile.w64 for 64-bit Windows."*

**Root cause.** `build_gifsicle.sh` uses the identical `COMPILE_ARGS=(-O2 -DHAVE_CONFIG_H -I. …)` for `TARGET=windows` and native (lines 71/74), so `-I.` always picks up the Linux `config.h`. The "if no config.h, writing a generated one" branch is dead (AL-5) and would still write the Linux file. `win32cfg.h` is never included.

**Fix.** For `--windows`, use a Windows config: `-include src/win32cfg.h` (and do **not** include the Linux `config.h`), or copy/adapt `win32cfg.h` as the config: `SIZEOF_UNSIGNED_LONG 4`, `SIZEOF_VOID_P 8`, `PATHNAME_SEPARATOR '\\'`, `RANDOM rand`, and drop `HAVE_UNISTD_H`/`HAVE_MKSTEMP`/`HAVE_SYS_SELECT_H` as needed. Never share the Linux `config.h` across targets.

---

### BR-12 🟠 CLI is POSIX-only on a Windows-first product (`unistd.h`, `/` splits, `p[0]=='/'`)
**File:** `src/cli/main.cpp:21, 44, 74, 93, 109`

**Symptom.** `PROJECT_VISION.md` says **Windows-first**, but the CLI includes `<unistd.h>` (line 21), splits paths on `'/'` only (lines 74, 93, 100–102), and treats `p[0] == '/'` as the only absolute-path test (lines 44, 109). A settings path `C:\…\animation.conf` is classified "relative"; a drive-letter engine path never matches `engine_path[0] != '/'`. MinGW may compile it; MSVC will not. Combined with BR-11/AL-6 the Windows CLI story has no working path today.

**Fix.** Use `std::filesystem::path` everywhere (C++17 is already the project standard): `exists()`, `absolute()`, `parent_path()`. Drop `unistd.h` (also see BR-1's execvp → CreateProcess). This also fixes BR-2's base_dir problem.

---

### BR-13 🟠 `GifsicleCommand` stores a `const Settings&` — temporaries dangle (latent UB)
**File:** `src/core/GifsicleCommand.h:20, 31`

**Symptom.** `explicit GifsicleCommand(const Settings& s) : settings_(s) {}` with member `const Settings& settings_;`. The pattern `GifsicleCommand cmd(currentSettings()); cmd.toString();` is undefined behavior: `currentSettings()` returns by value, the temporary binds to the ctor, then is destroyed, leaving `settings_` dangling. Today's call sites happen to use named locals (`const auto settings = currentSettings(); GifsicleCommand cmd(settings);`) — safe **by accident**. During the GUI retrofit every new helper will be tempted by the temporary form.

**Fix.** Store `Settings` **by value** (it's small), or take the ctor argument by value. Add a unit test that constructs from a prvalue and reads `args()`.

---

### BR-14 🟡 CMake `build_engine` target invokes a bash script — broken on Windows generators
**File:** `CMakeLists.txt:42–48`

**Symptom.** `add_custom_target(build_engine COMMAND ${CMAKE_SOURCE_DIR}/scripts/build_gifsicle.sh …)` with no interpreter. On a Windows CMake build (the exact path BR-8's fix introduces) this target cannot run unless the generator happens to provide bash; `USES_TERMINAL` does not supply one.

**Fix.** Find bash and prefix it, or skip the target on Windows and document that engine builds go through the script. Do not pretend CMake builds the engine on Windows.

---

## 4. MISALIGNED code — 11 findings (code contradicts its own docs/labels)

### AL-1 🟠 Version "single source of truth" hardcoded in 7 places
`0.1.0` appears in: `VERSION.md:4` (the declared truth), `CMakeLists.txt:2`, `gifscythe.pro:16`, `src/qtui/main.cpp:9` (`setApplicationVersion`), `src/qtui/MainWindow.cpp:24` (window title), `src/cli/main.cpp:70` (default engine path), `scripts/test_engine.sh:13`. A version bump means editing 7 files; `test_engine.sh` would silently test the **previous** engine after a bump.
**Fix:** generate `version.h` from `VERSION.md` at configure time (`configure_file`), use `GS_VERSION` everywhere; scripts grep `VERSION.md` like the packaging scripts already do.

### AL-2 🟠 "Add GIF files…" replaces the queue instead of appending
`chooseInputs()` (MainWindow.cpp:94–101) does `inputs_ = files; inputList_->clear();` (line 97) — the second selection throws away the first. Opposite of every batch queue (XnConvert, Caesium) and of the button's own label.
**Fix:** append + dedupe; add Remove-Selected and Clear-All (already on WORKLIST task 4).
```cpp
for (const auto& f : files) {
  if (inputs_.contains(f)) continue;
  inputs_.append(f);
  inputList_->addItem(QFileInfo(f).fileName());
}
```

### AL-3 🟠 The "live" command pane isn't live
The hint label promises *"The command preview is always kept in sync"*, but only the two spinboxes are connected to `refreshCommand()`. `outputEdit_` has no `textChanged` connection (only the Browse handler refreshes), and queue changes don't refresh either. The pane is the product's flagship power-user feature (`FEASIBILITY_REVIEW.md` core UX promise).
**Fix:** connect `outputEdit_::textChanged` and the list model's row changes — or better, one `settingsChanged()` signal owned by MainWindow that every control emits.

### AL-4 🟠 build.sh docs promise a GUI the default path never builds
Header comment (lines 4–6) says `./build.sh` builds "+ GUI (only where Qt6 is installed)"; the code (lines 34–36) never does on a bare invocation. The build script is this repo's contract with its next session — pick one story (see BR-9).

### AL-5 🟠 Dead config.h writer + swallowed Qt-build errors
`scripts/build_gifsicle.sh:57–60` contains an `if [[ ! -f config.h ]]` block that announces "writing a generated one" and **writes nothing** — misdirection about where the hand-written `reference_code/gifsicle/config.h` comes from. Meanwhile `build.sh` sends the entire Qt build to `/dev/null` (lines 62–63, 70–71), so the most fragile step leaves zero diagnostics, and its error message says "see Qt errors above" when no errors are visible.
**Fix:** delete the dead branch (state the real config.h source in one comment); log Qt build output to a file and `tail` it on failure.

### AL-6 🟠 Packagers look for the GUI where cmake doesn't put it — and never copy `gifsicle.exe`
The cmake path emits the GUI into `build/`, the qmake path into `build/gui/` — but both packaging scripts only copy `build/gui/gifscythe` (`package_portable.sh:12`, `package_system.sh:8`). Depending on which builder ran, **the "portable GUI release" silently ships without the GUI**. `windeployqt` (what makes Windows packages actually portable) appears nowhere in any script.

Additionally, the Windows engine can never be packaged: `build_gifsicle.sh --windows` writes `release/<ver>/gifsicle.exe` (line 31), but `package_portable.sh:7` requires `-x release/<ver>/gifsicle` (no `.exe`) and exits 1 when it's absent (line 9), and `package_system.sh:8` loops the same extensionless name. A product that is Windows-first has engine-less Windows packaging.

**Fix:** one build path (cmake), one output location; probe `gifsicle` **then** `gifsicle.exe` and copy whichever exists; `package_portable.sh` runs `windeployqt` automatically on Windows; CI asserts the portable folder contains the engine **and** the GUI.

### AL-7 🟡 Engine identity vs. product identity conflated
`build_gifsicle.sh:71/74` compiles with `-DVERSION="<product version>"`, so `gifsicle --version` reports *LCDF Gifsicle 0.1.0* instead of 1.96. Engine version ≠ product version; keeping 1.96 in the engine output preserves upstream traceability.
**Fix:** keep `-DVERSION=\"1.96\"` for the engine; use the product version only for the release directory name. *(Related: the product version is also baked into the engine's release path and into six other places — see AL-1.)*

### AL-8 🟡 Settings parser inconsistencies
- Bool parsing is inconsistent: `info` accepts `1/true/yes`, every other bool only `1/true` (SettingsIO.h `set_field`).
- `rotation` accepts `90/180/270` but no `none`.
- `(unsigned)to_long(v)` wraps negative inputs silently.
**Fix:** one `parse_bool()` helper; accept `none` for rotation; reject/clamp negatives.

### AL-9 🟡 Doc & repo hygiene drift
- Root `README.md:10` layout block still says `gifsicle-1.96/` (stale folder name).
- `.gitignore` (working_code) ignores `release/*/gifsicle` but **not** the packaged `release/<v>/Gifscythe*/` folders the package scripts create.
- **No root `.gitignore` at all** (verified on `main`) — and `reference_code/` currently commits the **74 MB Caesium 2.8.5 Windows binary bundle** (`caesium-bin/`, 62 files: `Caesium Image Compressor.exe` + full Qt6 DLLs) plus a duplicate full gifsicle tree (`gifsicle-nested-1.96/`). `FEASIBILITY_REVIEW.md` Caveat A says adapt Caesium **from source, not binaries**, and the GPL v3-vs-v2 math makes redistributing someone else's packaged app exactly the wrong move. Repo weight: ~106 MB for a 0.1.0 skeleton. Fix: gitignore/remove `caesium-bin` (replace with a README pointer), drop or document `gifsicle-nested-1.96`, add a root `.gitignore` for reference clones, build trees, and portable folders.
- `scripts/test_engine.sh:28` uses `((PASS++))` — when PASS is 0 the arithmetic command exits 1. It survives today only because the script omits `-e`; the moment someone "fixes" the script by adding `set -e`, the first passing test aborts the suite. Use `PASS=$((PASS+1))` (or `((++PASS))`), and add a root `.gitattributes` with `*.sh text eol=lf` (none exists — CRLF checkouts on Windows break shebangs).
- Qt6 deprecations in use: `QStringList::toStdVector()` / `QStringList::fromStdVector()` (deprecated since Qt 6.0, used in MainWindow.cpp) — use range constructors.
- Include-what-you-use gaps: `SettingsIO.h` calls `std::tolower` without including `<cctype>` (lines 11–15 include only fstream/sstream/string/vector), and `src/cli/main.cpp` uses `std::istringstream` (line ~120) without including `<sstream>`. Both compile today only via transitive includes — a different standard library or a header cleanup breaks them. Add the missing headers.
- Two parallel build systems (`gifscythe.pro` + CMake) with drifting truth — standardize on CMake (required for Windows CI anyway).

### AL-10 🟠 FEASIBILITY mapping table says delay is in **ms** — gifsicle `-d` is 1/100 s
`FEASIBILITY_REVIEW.md:109` maps *"Per-frame delay (ms)"* → `-d`. gifsicle's `-d` takes **hundredths of a second** (man page; `GifsicleSettings.h:69` correctly names the field `delay_cs // -d, in 1/100 sec`). A future GUI slider labelled "milliseconds" that writes `delay_cs` makes **every animation 10× too slow**. The code is right; the doc the next session is told to implement is wrong.
**Fix:** correct the table now; the GUI label must say "Delay (1/100 s)" or convert ms÷10 visibly; add a unit test that `delay = 5` emits `-d 5` (not 50).

### AL-11 🟡 Include-guard typo `GIFSYCYTHE` (missing C) in every header
Guards are `GIFSYCYTHE_CORE_*` / `GIFSYCYTHE_MAINWINDOW_H` in all four headers (e.g. `GifsicleSettings.h:12`). Harmless today; a later "fix the typo in one header" creates a second include of the same file under a new guard — redefinition trouble during the retrofit.
**Fix:** rename to `GIFSCYTHE_*` in **one commit, all headers together**.

---

## 5. MISSING logic — 16 findings (promised by docs, never written)

### MS-1 🟠 `SettingsIO` can't save — "load/save" is load-only
The header (SettingsIO.h:1) describes itself as the "load/save" serialization the GUI "will also use to persist per-file settings (and to round-trip state)". There is **no writer** anywhere (verified by grep). No presets, no per-file persistence, no CLI↔GUI round-trip.
**Fix:** `save_settings()` as the exact inverse of `load_settings()` (same keys, enums→strings) + round-trip unit test `load(save(s)) == s`.

### MS-2 🟠 No validation layer — bad values vanish silently
Out-of-range settings are dropped without a trace: `colors = 999` emits no `-k`; `disposal = 9` emits nothing; `info=true` + `Mode::Batch` will be rejected by gifsicle itself (man page: `--info` "cannot be combined with mode options like --batch") but nothing pre-checks it. In a GUI, a control that "does nothing" is indistinguishable from a bug.
**Fix:** `gs::validate(const Settings&) → std::vector<Warning>{field, value, reason}`; CLI prints them, GUI shows inline hints; also makes BR-10's garbage values detectable.

### MS-3 🟠 No safe-execution layer / no shell quoting / no `~` expansion
See BR-6 for the broken side: execution must use argv (`posix_spawn`/`execvp`/CreateProcess); display must `shell_quote()`. Additionally, `resolve_path()` (cli/main.cpp:44) returns `~/...` paths **unchanged** — the engine receives a literal `~` and fails. Expand `$HOME` or reject with a clear error.

### MS-4 🟠 GUI: no async execution, no progress, no cancel
WORKLIST task 4 (progress, cancel) has no foundation until QProcess moves to signals (`started`/`readyReadStandardError`/`finished`), with a busy state disabling Run and Cancel→`kill()`. Currently `MainWindow.cpp` has no `QProgressBar`, no signal connections, and a blocking synchronous run (see BR-5).

### MS-5 🟠 No engine discovery / startup health check
Both front-ends invented their own hardcoded locator: GUI assumes `applicationDirPath()/gifsicle` (MainWindow.cpp:26 — the **only** assignment to `enginePath_`, never probed or verified at startup), CLI assumes a repo-layout path (cli/main.cpp:70). The default build layout defeats the GUI locator outright: `build.sh` puts the engine at `release/<ver>/gifsicle` and the GUI at `build/gui/gifscythe` (qmake) or `build/gifscythe` (cmake) — different directories. On Windows the binary is `gifsicle.exe` and the locator never appends `.exe`. Net effect: **first-run GUI users always hit "Could not start gifsicle"** unless someone manually copies the engine beside the GUI — which the packager also fails to do for `.exe` (AL-6).
**Fix:** put `gs::EngineLocator` in `src/core` (exe-dir → `GS_ENGINE` env → PATH → `release/<GS_VERSION>`, with `.exe` handling on Windows), call it at startup; if not found, disable Run and show the missing path in the status bar instead of "Ready". Show engine path+version in the GUI status bar (which also *proves* the "retained terminal control" story).

### MS-6 🟠 Tests never exercise the flows that actually fail
35 assertions, all in-process argv assembly on happy paths. Nothing covers: `--run` exit codes (BR-1), CWD sensitivity (BR-2), merge-vs-batch semantics (BR-3), empty-output runs (BR-4), malformed conf (BR-10), paths with spaces (BR-6), SettingsIO round-trip (MS-1). IMPROVEMENT_LOG's claim "Unit + integration tests pass" has **no integration test** — the only end-to-end run is manual. Additionally, `scripts/test_engine.sh` is **not called by build.sh or the workflow** (verified by grep), and CI uploads no artifacts.
**Fix:** smoke suite in CI: `--run` from ≥2 CWDs · malformed-conf · merge-vs-batch · round-trip · portable-folder file-existence checks · `test_engine.sh` in both jobs · artifact upload.

### MS-7 🟡 Drag-and-drop: enabled but not implemented
`inputList_->setAcceptDrops(true)` (MainWindow.cpp:44) with no `dragEnterEvent`/`dropEvent` override → dropping files does nothing. WORKLIST task 4 promises it.
**Fix:** subclass `QListWidget` (or event filter), accept `Qt::CopyAction`, append `mimeData()->urls()` in `dropEvent` (and dedupe).

### MS-8 🟡 No queue management controls
No Remove-Selected, no Clear-All (verified: no `removeItem`/`takeItem` in MainWindow) — users must restart to drop one file. Already on WORKLIST task 4.

### MS-9 🟡 No mode selector — Batch/Explode/Auto unreachable in the GUI
`Mode::Merge` is hardcoded (MainWindow.cpp:109); `Mode::Batch`, `Mode::Explode`, `Mode::Auto` exist in the core layer and settings format but no widget exposes them (no `QComboBox` anywhere).
**Fix:** `QComboBox` (Auto/Merge/Batch/Explode) — required anyway once BR-3 is fixed, so the user can choose "optimize each" vs. "merge into one".

### MS-10 🟡 Dither is bool-only; docs promise method selection
`FEASIBILITY_REVIEW.md`'s mapping table says `--dither=floyd-steinberg|ro64|none`. Core models `bool dither` (GifsicleSettings.h:78) and emits bare `-f` (default Floyd–Steinberg). Correct as far as it goes, but the promised richness is missing.
**Fix:** `std::string dither_method` ("" = off) emitting `-f` / `--dither=<method>`.

### MS-11 🟡 No LICENSE / GPL compliance artifacts
`FEASIBILITY_REVIEW.md` §5 is explicit: distributing gifsicle (GPL **v2-only**) alongside a GPLv3 UI requires shipping gifsicle's license text + source offer (the subprocess architecture was chosen precisely to make this clean). There is **no LICENSE/COPYING file anywhere in the repo** (verified by find), and `package_portable.sh` ships no license files.
**Fix:** add `COPYING` (GPLv2 text for the engine) + attribution; copy them into the portable folder; keep gifsicle a separate process forever unless the gifsicle author grants a v3 exception.

### MS-12 🟡 Windows packaging path incomplete
No `windeployqt` automation, no Windows GUI build in CI, no artifact, no smoke test of the portable folder on a clean machine. This is the whole remaining path to 1.0.0 (WORKLIST tasks 1–2).

### MS-13 🟡 Remaining GifsicleSettings controls unexposed in GUI
The GUI exposes 2 of ~30 modeled controls (optimize + lossy). `FEASIBILITY_REVIEW.md` §3(2) already contains the complete slider↔flag mapping table — the retrofit is mechanical widgets↔settings work on top of the existing core. (Planned — WORKLIST task 5 — listed for completeness.)

### MS-14 🟠 FEASIBILITY promised a **two-way** live CLI pane — the reverse parser doesn't exist
`FEASIBILITY_REVIEW.md` §3(2), which the project treats as law: *"exposing a **live raw-CLI pane** that two-way syncs with the widgets (change a slider → the CLI text updates; **edit the CLI text → the widgets update**). Power users get 100% of the tool."* The actual pane is read-only (`MainWindow.cpp:76 commandPane_->setReadOnly(true)`), one-way, and not even fully one-way (AL-3). There is no argv→Settings parser anywhere in `src/core` — the MVP implemented Settings→argv (`GifsicleCommand`) and stopped; the reverse direction **is** the "retain terminal control" feature.
**Fix:** after the P0 honesty work, either (a) keep the pane read-only and change the doc/label so it stops promising two-way, or (b) add `gs::parse_args()` and make the pane editable. Do **not** ship 1.0.0 claiming two-way if it's (a).

### MS-15 🟡 Named `--gamma` values (`srgb|oklab`) from the mapping table are unmodeled
`FEASIBILITY_REVIEW.md:116` maps *"Gamma / color math"* → `--gamma=srgb|oklab|NUM`. The engine accepts a string (`gifsicle.c:261 { "gamma", 0, GAMMA_OPT, Clp_ValString, Clp_Negate }`), but the core stores a `double gamma` and emits `--gamma <float>` as two argv tokens — `srgb`/`oklab` cannot be expressed. Also `f2s()` uses default ostream precision, so some values round-trip noisily.
**Fix:** store gamma as a string ("" = unchanged); emit `--gamma=srgb` / `--gamma=2.2`. Not P0.

### MS-16 🟡 Unit test 4 is tautological; `load_settings_file` swallows a missing file
`tests/test_gifsicle_command.cpp:87–94` (defaults test): `CHECK(args.empty() || has(args, "a.gif"));` — if the builder **dropped** the input, `args.empty()` is true and the test still passes. Separately, `SettingsIO.h:118 load_settings_file` returns `Settings{}` when the file can't be opened, with no error channel. Together they make "green tests" compatible with silent failure.
**Fix:** `CHECK(has(args, "a.gif")); CHECK(!args.empty());`; make `load_settings_file` return `std::optional<Settings>` (or set an error); add the missing-file case to the smoke suite.

---

## 6. Verified-correct behaviors — do NOT "fix" these

These four look suspicious on a quick read and have been flagged as bugs more than once, but each was checked against the **committed engine source** (`reference_code/gifsicle/src/gifsicle.c`) and/or the **official gifsicle man page**. Changing them would introduce real bugs.

### ✅ VP-1 — `--loopcount=0` is correct for "loop forever"
Man page: *"Note that **--loopcount**=0 is equivalent to **--loopcount**=forever, not **--no-loopcount**."* Committed engine source (`gifsicle.c:1853`):
```c
case 'l':
  MARK_CH(output, CH_LOOPCOUNT);
  if (clp->negated)
    def_output_data.loopcount = -1;                            // --no-loopcount
  else
    def_output_data.loopcount = (clp->have_val ? clp->val.i : 0); // bare -l → 0 = forever
```
`GifsicleCommand.h:160–164` emitting `--loopcount=0` for "forever" is exactly right. Do **not** switch to `--loopcount=forever` thinking `=0` means "play once".

### ✅ VP-2 — `-O0` is valid and means "no optimization"
Committed engine source (`gifsicle.c:1866`, `OPTIMIZE_OPT` handler):
```c
if (clp->negated || (clp->have_val && clp->val.i < 0))
  o = 0;
else
  o = (clp->have_val ? clp->val.i : 1);
…
def_output_data.optimizing = (def_output_data.optimizing & ~GT_OPT_MASK) | o;
```
`-O0` is accepted without error and clears the optimization mask — a legitimately useful setting ("turn optimization off"). The man page lists three positive levels, but the parser has no error path for 0. Do **not** clamp the GUI spinbox minimum to 1; if anything, label 0 as "Off".

### ✅ VP-3 — `gamma` already uses a safe sentinel
`GifsicleSettings.h:80` already reads:
```cpp
double gamma = -1.0;   // --gamma; -1 = unchanged
```
and `GifsicleCommand.h:87` gates emission with `if (s.gamma >= 0)`. The "always emits `--gamma 0.0`" concern doesn't match the actual source. No change needed.

### ✅ VP-4 — Q_OBJECT/MOC is handled by the current build files
`CMakeLists.txt:32` calls `qt_standard_project_setup()`, which sets `CMAKE_AUTOMOC ON` (Qt ≥ 6.3); AUTOMOC mocs `MainWindow.h` because it is `#include`d by the listed source `MainWindow.cpp`. The qmake path mocs `Q_OBJECT` headers automatically. Adding the header to the target sources is a harmless belt-and-braces measure, but there is no vtable/link failure to fix. (Caveat: `qt_standard_project_setup()` requires Qt ≥ 6.3; on older Qt set `CMAKE_AUTOMOC ON` manually.)

### ✅ VP-5 — Do not "fix" the crop syntax — the FEASIBILITY table is wrong, the code is right
`FEASIBILITY_REVIEW.md:107` writes the crop flag as `--crop X,Y,WxH` (comma before the size). gifsicle wants the **plus** form `--crop X,Y+WIDTHxHEIGHT` (verified against the man page and the built engine). The code already emits `0,0+30x60` (GifsicleCommand.h crop block) and the unit test asserts exactly that. A well-meaning retrofit that "aligns the code to the table" would generate a flag gifsicle rejects.
**Fix:** edit the table, not the emitter. Same discipline as VP-1–VP-4: do not change correct code to match a wrong document.

---

## 7. Doc claims vs. code — every status claim re-verified

| Claim | Source doc | Verdict | Note |
|---|---|---|---|
| P1 engine control layer complete | WORKLIST | ✅ HOLDS | Core compiles clean; tests pass; flag mapping faithful (re-verified vs. man page + engine source) |
| Qt6 GUI MVP scaffold + first retrofit | WORKLIST | ⚠️ PARTIAL | Window drives the core layer — but run flow is broken (BR-3, BR-4, BR-5) and blocks the UI (MS-4) |
| Linux CI passes with Qt6 GUI | IMPROVEMENT_LOG / HANDOFF | ⚠️ PARTIAL | Job is green, but GUI build attempts are silenced/failure-tolerant (BR-7, AL-5) and **no artifact proves a GUI was built**. Green ≠ GUI built |
| CLI driver + integration tests | IMPROVEMENT_LOG | ❌ FAILS | CLI exists, but `--run` works from one CWD only and exits 0 on failure (BR-1, BR-2); **no automated integration test exists** (MS-6) |
| Packaging scripts done | WORKLIST | ⚠️ PARTIAL | Scripts exist, pass `bash -n` — but expect the GUI where cmake never puts it; a "GUI release" can ship GUI-less (AL-6) |
| Don't bump to 1.0.0 before UI/UX retrofit | VERSION / HANDOFF / WORKLIST | ✅ HOLDS | Version discipline intact; the criticals above are exactly why the rule must keep holding |
| WebP/APNG deferred until GIF UI stable | VISION / bucket list | ✅ HOLDS | Scope clean — no premature codec code |

**Pattern diagnosis:** the project's failure mode isn't recklessness — it's **green-checkmark inflation**: claims marked done without machine-checked proof. Fix the proof (CI artifacts, exit codes, assertions), and the checkmarks become trustworthy again.

---

## 8. Project context (from the repo's own documents)

| Doc | Role | What a newcomer must know |
|---|---|---|
| `PROJECT_VISION.md` | what & why | Casual users, XnConvert-feel GUI over gifsicle, **animated images only** (GIF now; APNG/WebP later), portable click-and-run, full terminal control retained. BR-3/BR-4 violate "normal person" trust directly — the vision is sound; the code must serve it |
| `WORKLIST.md` | task board | P0–P1 checked; Windows CI + GUI verification + UX retrofit remain before 1.0.0. Two checked boxes (GUI MVP, packaging) only partially hold |
| `SESSION_HANDOFF.md` | next-session state | "Linux CI passes; Windows CI fails, log unretrievable; no Qt in sandbox." The Windows failure is diagnosable from the workflow file itself (BR-8) |
| `IMPROVEMENT_LOG.md` | decision log | Compile claims verify. The "integration tests pass" / "GUI built via Linux CI" claims do not survive contact with the code (§7) |
| `FEASIBILITY_REVIEW.md` | architecture context | Subprocess wrapper = right MVP; live raw-CLI pane = core UX promise; license math favors the wrapper. Current architecture already matches the review — the gaps are executional (BR-6 unsafe exec, AL-3 unsynced pane), not architectural |

**Hard rules carried by the docs (do not break while fixing):**
- `reference_code/` is read-only reference material; all product edits go in `working_code/gifscythe/`.
- Stay on 0.x until the UI/UX retrofit is complete — never bump 1.0.0 as a placeholder.
- No WebP/APNG work before the GIF UI is stable.
- Keep gifsicle a **separate subprocess** (GPL v2-only vs GPLv3 UI — see MS-11).

---

## 9. Remediation plan — ordered

Sequenced to respect the project's own rules: version stays 0.x, APNG/WebP stays in the bucket, and every phase ends with CI evidence rather than a checked box.

### 🚨 P0 — Stop the silent failures (before ANY feature work)
Every critical shares one property: *the tool says success while doing wrong or nothing*. For a casual-user product that's the worst failure class.

| # | Action | Closes | Est. |
|---|---|---|---|
| P0-1 | CLI: replace `system()` with argv exec (`fork/execvp`; CreateProcess on Windows); decode exit with `WEXITSTATUS`; pre-flight engine existence check; move all path handling to `std::filesystem` (drops `unistd.h`, fixes drive-letter paths) | BR-1, BR-6, BR-12 | half day |
| P0-2 | CMake: `gifscythe_core` → `INTERFACE`; stop swallowing cmake output in build.sh; fail hard when GUI requested but unbuilt | BR-7, BR-9 | 1 h |
| P0-3 | Windows CI + engine: aqtinstall (or `qt6-base-dev`), PATH to Qt bin, build engine ONCE **with a Windows config.h** (win32cfg.h values), build GUI via CMake, `windeployqt`, upload artifact, run `test_engine.sh` | BR-8, BR-11 | 1 day |
| P0-4 | GUI run flow: require/default an output path; verify output file exists+non-empty before claiming success; handle `waitForFinished` timeout (kill + honest error) | BR-4, BR-5 | half day |
| P0-5 | GUI queue semantics: `Mode::Batch` default (per-file), add mode combo; Merge only as explicit action | BR-3, MS-9 | half day |
| P0-6 | SettingsIO: zero-init + extraction checks + load warnings; fix bool/rotation inconsistencies | BR-10, AL-8 | 2 h |

### ⚠️ P1 — Make the code honest (one truth per concept)

| # | Action | Closes | Est. |
|---|---|---|---|
| P1-1 | Single version source: generate `version.h` from `VERSION.md`; delete the 6 duplicate strings; `test_engine.sh` greps VERSION.md | AL-1 | half day |
| P1-2 | Engine locator in core (exe-dir → env → PATH → release/<ver>), used by CLI **and** GUI; startup health check; show engine path+version in status bar | BR-2, MS-5 | half day |
| P1-3 | Queue UX: append+dedupe, Remove-Selected/Clear-All, working drag-and-drop subclass | AL-2, MS-7, MS-8 | half day |
| P1-4 | Live pane honesty: connect `outputEdit_::textChanged` + list-model changes (or one `settingsChanged()` signal); add `shell_quote()` for display | AL-3, MS-3 | 2 h |
| P1-5 | Build hygiene: align build.sh header↔behavior; delete dead config.h branch; log Qt build output to file, tail on failure; one build path (CMake), one output dir; `~` expansion in resolve_path; fix `build_engine` target for Windows | AL-4, AL-5, BR-9, MS-3, BR-14 | half day |
| P1-6 | `save_settings()` + round-trip test; `gs::validate()` warnings surfaced in CLI/GUI | MS-1, MS-2 | half day |
| P1-7 | Packaging: packager follows the single CMake output dir; probe `gifsicle` **and** `gifsicle.exe`; `windeployqt` inside `package_portable.sh` on Windows; CI asserts portable-folder contents | AL-6, MS-12 | half day |
| P1-8 | Compliance: add `COPYING` (GPLv2) + attribution; ship in portable folder | MS-11 | 1 h |
| P1-9 | Correct the FEASIBILITY mapping table **before anyone implements it**: delay units (ms → 1/100 s), crop syntax (`+` form), gamma named values — the code is right, the table is wrong (VP-5, AL-10, MS-15) | AL-10, VP-5 | 1 h |
| P1-10 | Core hardening for the retrofit: store `Settings` by value in `GifsicleCommand` (dangling-reference landmine); decide two-way pane (implement `gs::parse_args()` or de-scope the doc); add missing `<cctype>`/`<sstream>` includes | BR-13, MS-14, AL-9 | half day |

### 🛠 P2 — Prove it all (test & CI hardening)

| # | Action | Closes | Est. |
|---|---|---|---|
| P2-1 | Smoke suite in CI: `--run` from ≥2 CWDs; malformed-conf; merge-vs-batch scenario; paths-with-spaces; SettingsIO round-trip | MS-6 | 1 day |
| P2-2 | Both jobs run `test_engine.sh`; both jobs upload artifacts; cache apt/choco/pip | MS-6 | 2 h |
| P2-3 | Async QProcess (signals) + progress bar + cancel + busy-state; per-file progress in batch mode | MS-4 | 1 day |
| P2-4 | Hygiene: root README folder name, .gitignore for packaged folders + **root** `.gitignore` for reference clones, remove/gitignore the 74 MB `caesium-bin` bundle and the duplicate gifsicle tree, replace deprecated `toStdVector`, retire `gifscythe.pro` or mark it secondary, rename `GIFSYCYTHE_*` guards in one commit, fix `((PASS++))` + add `.gitattributes`, fix tautological test 4 + `load_settings_file` error channel | AL-9, AL-11, MS-16 | half day |

### 🎨 P3 — The UI/UX retrofit → 1.0.0 (WORKLIST tasks 3–7)

| # | Action | Closes |
|---|---|---|
| P3-1 | Input / Actions / Output tab flow (XNConvert feel) on top of the now-honest core | WORKLIST 3 |
| P3-2 | Expose remaining `GifsicleSettings` controls using the `FEASIBILITY_REVIEW.md` §3(2) mapping table; keep the live raw-CLI pane; add dither-method selector | MS-10, MS-13 |
| P3-3 | Before/after preview, file size/count display, output-folder actions | WORKLIST 4 |
| P3-4 | Clean-machine Windows smoke test of the portable folder; document release procedure | WORKLIST 6 |
| P3-5 | **Only then:** bump `1.0.0`. WebP/APNG stays in the bucket until the GIF UI is stable | WORKLIST 7 |

---

## 10. Files-to-modify matrix

| File | Key actions | Finding IDs |
|---|---|---|
| `src/cli/main.cpp` | argv exec + WEXITSTATUS; engine existence pre-flight; realpath settings; exe-anchored engine locator; `~` expansion; `std::filesystem` everywhere (drop `unistd.h`); add `<sstream>` | BR-1, BR-2, BR-6, BR-12, MS-3, MS-5 |
| `src/qtui/MainWindow.cpp` (+ `.h`) | batch-default + mode combo; output validation + post-run file check; async QProcess + progress/cancel; append/dedupe/remove/clear queue; drag-drop subclass; `textChanged` wiring; engine locator + status bar | BR-3, BR-4, BR-5, AL-2, AL-3, MS-4, MS-7, MS-8, MS-9 |
| `src/core/GifsicleCommand.h` | `shell_quote()` for `toString()` display only (exec uses argv); store `Settings` by value; guard rename | BR-6, BR-13, AL-11, MS-3 |
| `src/core/GifsicleSettings.h` | (after decision) dither_method string; gamma as string (srgb/oklab support); keep loopcount/optimize/crop exactly as-is (see §6) | MS-10, MS-15, AL-11 |
| `src/core/SettingsIO.h` | safe `to_long/to_double`; load warnings; `save_settings()`; parse_bool/`none` consistency; `load_settings_file` error channel; add `<cctype>` | BR-10, AL-8, AL-9, MS-1, MS-16 |
| `src/core/` (new) | `EngineLocator` (with `.exe` handling), `validate()`, `parse_args()` (two-way pane, if chosen), `version.h.in` | BR-2, MS-2, MS-5, MS-14, AL-1 |
| `CMakeLists.txt` | INTERFACE core; version.h generation; keep `qt_standard_project_setup` (AUTOMOC is fine); fix/skip `build_engine` target on Windows | BR-7, BR-14, AL-1 |
| `build.sh` | honest GUI dispatch (qmake6→qmake→cmake), no silencing, header=behavior, single output dir | BR-9, AL-4, AL-5 |
| `scripts/build_gifsicle.sh` | delete dead config.h branch; **Windows target uses win32cfg.h values, not the Linux config.h**; engine `-DVERSION=\"1.96\"` | AL-5, AL-7, BR-11 |
| `scripts/test_engine.sh` | grep version from VERSION.md; `find`-based counting; `PASS=$((PASS+1))` | AL-1, AL-9 |
| `scripts/package_*.sh` | follow single GUI output dir; probe `gifsicle`/`gifsicle.exe`; windeployqt on Windows; ship COPYING | AL-6, MS-11, MS-12 |
| `.github/workflows/build.yml` | full Windows rewrite (§BR-8 YAML + BR-11 config fix); test_engine.sh both jobs; artifacts; cache | BR-8, BR-11, MS-6 |
| `tests/` | round-trip, malformed-conf, spaces-in-paths, batch-vs-merge scenario, CLI smoke from 2 CWDs, delay-unit test (`delay=5` → `-d 5`), prvalue-ctor test, fix tautological test 4 | MS-6, AL-10, BR-13, MS-16 |
| `FEASIBILITY_REVIEW.md` | fix the mapping table: delay units (1/100 s), crop `+` form, gamma named values — before the retrofit implements it | AL-10, VP-5, MS-15 |
| `reference_code/` | gitignore/remove `caesium-bin` (74 MB binary bundle), drop or document `gifsicle-nested-1.96`; root `.gitignore` + `.gitattributes` | AL-9, AL-11 |
| repo root | `COPYING`; README folder-name fix; `.gitignore` | MS-11, AL-9 |

---

## 11. Definition of done per phase (evidence, not checkmarks)

- **P0 done when:** `gifscythe-cli` returns non-zero for a missing engine (verified from 2 CWDs); GUI refuses empty output and verifies the file; queue of 2 GIFs produces 2 optimized files (verified with `gifsicle --info`); CMake configures cleanly; Windows CI job goes green **with an artifact**.
- **P1 done when:** version bump is a 1-file edit and all binaries/labels agree; drag-drop + remove/clear work; command pane matches actual execution byte-for-byte for paths with spaces; `save(load(s)) == s`; portable folder contains GUI + engine + licenses.
- **P2 done when:** the smoke suite in P2-1 is green on Linux **and** Windows CI, with artifacts downloadable.
- **P3 done when:** clean-Windows-machine smoke test passes → then, and only then, `VERSION.md → 1.0.0`.

---

## 12. References & verification note

- Gifsicle 1.96 man page: https://www.lcdf.org/gifsicle/man.html (crop syntax, `--loopcount=0`=forever, disposal ranges, `--info` vs batch exclusivity, `-d` in 1/100 s)
- Chocolatey Qt6 package (correct name): https://push.chocolatey.org/packages/qt6-base-dev
- Engine semantics verified against the committed source `reference_code/gifsicle/src/gifsicle.c` (loop handler at line 1853, `OPTIMIZE_OPT` handler at line 1866, `static_assert` block in `main()` at ~1462, gamma option `Clp_ValString` at line 261) and `src/win32cfg.h` vs `config.h` for the Windows build finding (BR-11).
- All `file:line` references were verified against `main` (state of 2026-09-06, including the review-dashboard commit) by direct source inspection; reproduction transcripts were captured from live runs of the built engine and tests.
