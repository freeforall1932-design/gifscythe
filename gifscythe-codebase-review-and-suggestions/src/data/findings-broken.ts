import type { FindingSeed } from "./types";

export const brokenFindings: FindingSeed[] = [
  {
    id: "U-B1",
    code: "U-B1",
    title: "CLI --run exits 0 on total failure (false success)",
    category: "broken",
    severity: "critical",
    origin: "prior-audit",
    verification: "reproduced",
    evidenceGrade: "V1",
    files: "src/cli/main.cpp:54–56, 142–144",
    foundBy: "S3/S4/S5 C1 · S2 C1 · S1 A6",
    symptom:
      "When the engine binary cannot be found, the shell still runs the command, fails with 127, and the CLI prints `# -> exit code 32512` while returning 0 to the OS. CI and automation see a green run.",
    rootCause:
      "run_with_system() returns the raw system() wait status unmodified. Only the low 8 bits survive C process-exit truncation: 32512 = 127<<8 → 0. There is also no existence check on the engine before running.",
    evidence:
      "Reproduced this pass (g++ 12.2). Relative settings path: `sh: …/examples/working_code/gifscythe/release/0.1.0/gifsicle: not found` then `# -> exit code 32512` then `echo $?` → 0. Same false-zero from /tmp with an absolute settings file that still string-sniffs a nested engine path.",
    fix: "Pre-flight fs exists() on the engine and return 1. Decode wait status with WIFEXITED/WEXITSTATUS (or stop using system() entirely — see U-B6). Never return the raw wait status from main().",
    phase: "P0",
    sortOrder: 10,
  },
  {
    id: "U-B2",
    code: "U-B2",
    title: "Engine path resolution works only by accident (CWD-dependent)",
    category: "broken",
    severity: "critical",
    origin: "prior-audit",
    verification: "reproduced",
    evidenceGrade: "V1",
    files: "src/cli/main.cpp:70, 90–113",
    foundBy: "S3 C2 · S5 second pass · S2 C2 · S1 B4",
    symptom:
      "Default engine path is working_code/gifscythe/release/0.1.0/gifsicle resolved against a 'repo root' discovered only when the settings path is absolute AND contains /working_code/gifscythe, or when CWD is the repo root. The product README's own usage from working_code/gifscythe fails.",
    rootCause:
      "repo_root is derived by string-sniffing find(\"/working_code/gifscythe\") + three-levels-of-parent fallback + CWD-relative probing. Three anchors that coincide for exactly one working directory. base_dir is only absolutized when it equals \".\".",
    evidence:
      "This pass: from product dir, `gifscythe-cli examples/animation.conf --run` looks for `examples/working_code/gifscythe/release/0.1.0/gifsicle` (not found, exit 0). From repo root with $PWD/…/animation.conf: engine found, writes /tmp/gifscythe_demo.gif 9458 bytes, exit 0.",
    fix: "realpath the settings path at startup. Anchor engine search on the executable location (exe_dir/gifsicle, exe_dir/../release/<ver>/gifsicle), then GS_ENGINE, then PATH. Stop string-sniffing folder names.",
    phase: "P0",
    sortOrder: 20,
  },
  {
    id: "U-B3",
    code: "U-B3",
    title: "GUI welds the queue into one GIF (Merge vs batch intent)",
    category: "broken",
    severity: "critical",
    origin: "prior-audit",
    verification: "reproduced",
    evidenceGrade: "V1",
    files: "src/qtui/MainWindow.cpp:108–113",
    foundBy: "S3 C3 · S2 C3 · S1 B3",
    symptom:
      "The UI presents an 'Animation queue' with 'Add GIF files…' — batch semantics — but every run hardcodes s.mode = gs::Mode::Merge. Queue two GIFs to optimize and the tool concatenates them into one animation.",
    rootCause:
      "Merge was the CLI demo scenario (examples/animation.conf uses mode = merge). currentSettings() never reads a mode widget because none exists. gifsicle -m concatenates frame-by-frame.",
    evidence:
      "This pass against the built engine: logo.gif = 12 images, logo1.gif = 1 image. `gifsicle -m -O3 logo.gif logo1.gif -o /tmp/merged.gif` → 13 images, logical screen 60x132. Silent data mangling of user intent.",
    fix: "Default the queue to Mode::Batch (per-file, in-place or <name>_opt.gif). Keep Merge behind an explicit 'Merge into one animation' action. Add a mode combo (U-MISS-9).",
    phase: "P0",
    sortOrder: 30,
  },
  {
    id: "U-B4",
    code: "U-B4",
    title: "Empty output path vaporizes the result, then reports success",
    category: "broken",
    severity: "critical",
    origin: "prior-audit",
    verification: "reproduced",
    evidenceGrade: "V1",
    files: "src/qtui/MainWindow.cpp:122–138",
    foundBy: "S3 C4 · S2 C4 · S1 B2",
    symptom:
      "The output field is optional (placeholder: 'Output file (optional)'). Left empty, gifsicle writes the optimized GIF to stdout. MainWindow never reads stdout. Status bar still says 'Optimization complete.'",
    rootCause:
      "GifsicleCommand omits -o when output is empty (correct for the engine). runCommand() only calls readAllStandardError(). exitCode()==0 takes the success branch. The most common first run (add a GIF, click Optimize, skip output) is guaranteed data loss.",
    evidence:
      "This pass: `gifsicle -O3 logo1.gif | wc -c` → 821. MainWindow.cpp:131 is the only read. No readAllStandardOutput(), no QFileInfo existence check after the run.",
    fix: "Refuse empty output, or auto-derive <input>_opt.gif in batch mode. After a normal exit, verify the output file exists and size>0 before claiming success.",
    phase: "P0",
    sortOrder: 40,
  },
  {
    id: "U-B5",
    code: "U-B5",
    title: "waitForFinished() timeout ignored → zombie process + wrong status",
    category: "broken",
    severity: "critical",
    origin: "prior-audit",
    verification: "confirmed",
    evidenceGrade: "V1",
    files: "src/qtui/MainWindow.cpp:125–138",
    foundBy: "S3 G4 · S2 N15 · S1 A3",
    symptom:
      "proc.waitForFinished(60000) blocks the UI thread up to 60s; its bool return is ignored. On timeout the process is still running, but exitStatus()/exitCode() on a live QProcess default to NormalExit/0, so the UI can report success for a process that has not finished. The QProcess destructor then kills it.",
    rootCause:
      "Synchronous QProcess on the GUI thread with discarded timeout. Large animations — the app's entire reason to exist — exceed 60s routinely.",
    evidence:
      "Source: line 130 calls waitForFinished(60000) with no if. Lines 132–137 branch only on exitStatus/exitCode. QProcess docs: those getters are 0/NormalExit while Running.",
    fix: "Check the bool; on timeout proc.kill() and report failure. Real fix is async member QProcess with finished/readyReadStandardError, progress, and cancel (U-MISS-4).",
    phase: "P0",
    sortOrder: 50,
  },
  {
    id: "U-B6",
    code: "U-B6",
    title: "Shell execution of a concatenated command string (spaces + injection)",
    category: "broken",
    severity: "critical",
    origin: "prior-audit",
    verification: "confirmed",
    evidenceGrade: "V1",
    files: "src/cli/main.cpp:54–56, 139–144 · src/core/GifsicleCommand.h toString()",
    foundBy: "S3 G3 · S2 G3 · S1 A6",
    symptom:
      "toString() joins argv with bare spaces (no quoting). The CLI executes that string through std::system(). Any path with a space breaks. Any settings value with shell metacharacters is interpreted by /bin/sh. Power users copy the unquoted line from the live pane into a terminal and it fails the same way.",
    rootCause:
      "Display string and execution string are the same object. argv already exists on GifsicleCommand but --run never uses args(); it concatenates engine_path + ' ' + cli and hands it to the shell.",
    evidence:
      "toString() loop is `if (i) o << ' '; o << args_[i];` — zero quoting helpers in the file (confirmed grep). CLI: `std::string full = engine_path + \" \" + cli; run_with_system(full);`. GUI is safer here: QProcess setArguments uses argv, so this critical is CLI+display.",
    fix: "Execute via posix_spawn/execvp (CreateProcess on Windows) using args(). Add shell_quote() used only by toString() for the live pane.",
    phase: "P0",
    sortOrder: 60,
  },
  {
    id: "U-B7",
    code: "U-B7",
    title: "CMake cannot configure: header-only lib declared STATIC",
    category: "broken",
    severity: "major",
    origin: "prior-audit",
    verification: "confirmed",
    evidenceGrade: "V1",
    files: "CMakeLists.txt:8–12",
    foundBy: "S3 M2 · S2 M2 · S1 A2",
    symptom:
      "cmake generation fails: Cannot determine link language for target gifscythe_core. Linux CI still looks green because build.sh prefers qmake and swallows cmake stderr.",
    rootCause:
      "A STATIC library needs ≥1 compiled translation unit. target_sources(... PRIVATE src/core/SettingsIO.h) contributes only a header. cmake is not installed in this sandbox so configure was not re-run here; the CMakeLists block is verbatim.",
    evidence:
      "CMakeLists.txt lines 8–12: add_library(gifscythe_core STATIC) + SettingsIO.h only. build.sh:62–63 runs cmake … >/dev/null 2>&1. This sandbox: cmake: command not found, so V1 for the source defect, V2 for the historical CMake 3.30.2 transcript.",
    fix: "add_library(gifscythe_core INTERFACE) with INTERFACE includes. Stop silencing cmake. When --all is requested and the GUI cannot build, exit non-zero.",
    phase: "P0",
    sortOrder: 70,
  },
  {
    id: "U-B8",
    code: "U-B8",
    title: "Windows CI: wrong package name + missing GUI/deploy steps",
    category: "broken",
    severity: "major",
    origin: "prior-audit",
    verification: "confirmed",
    evidenceGrade: "V1",
    files: ".github/workflows/build.yml",
    foundBy: "S1 A1 · S2 N13 · SESSION_HANDOFF",
    symptom:
      "Windows job dies at Qt install. Even a correct install would not build the GUI: the job runs bare ./build.sh, which forces want_gui=0. No windeployqt, no artifact, engine built twice.",
    rootCause:
      "Three stacked defects: (1) choco install qt6-base — Linux job correctly uses apt qt6-base-dev; Windows uses a name that does not exist on Chocolatey. (2) bare ./build.sh never builds GUI (U-B9.3). (3) no PATH to Qt, no cmake GUI step, no windeployqt, no upload-artifact. Plus NEW-1: even a green install still cannot compile the engine with the Linux config.h.",
    evidence:
      "build.yml:28 `choco install qt6-base mingw`. build.yml:38 `./build.sh`. build.sh:34–36 want_gui=auto forced to 0. No windeployqt anywhere in the repo (grep).",
    fix: "aqtinstall or jurplel/install-qt-action; build engine once with a Windows config; build CLI+tests; cmake GUI + windeployqt; upload artifact; run test_engine.sh. Do not use choco qt6-base.",
    phase: "P0",
    sortOrder: 80,
  },
  {
    id: "U-B9",
    code: "U-B9",
    title: "build.sh GUI dispatch: three logic errors",
    category: "broken",
    severity: "major",
    origin: "prior-audit",
    verification: "confirmed",
    evidenceGrade: "V1",
    files: "build.sh",
    foundBy: "S1 A8 · S3 M5/M6",
    symptom:
      "Header comment promises GUI 'only where Qt6 is installed'. Default ./build.sh never builds it. --gui still builds engine+tests. cmake-success can still print 'GUI build failed' because the next block insists on qmake6.",
    rootCause:
      "(1) cmake path sets qt_found=1 then the qmake6 block runs anyway. (2) --gui does not skip steps 1–3. (3) want_gui=auto is immediately forced to 0. Also: cmake detection actually builds as a side effect of 'checking' for cmake, and $root is computed and never used.",
    evidence:
      "build.sh:6 header vs :34–36 force-0. :7 '--gui skip engine/tests' vs :38–51 always run. :60–73 cmake then qmake. :26 root= unused.",
    fix: "One honest GUI branch: qmake6 → qmake → cmake, report truthfully, exit non-zero if GUI was requested but unbuilt. Align the header with the code.",
    phase: "P0",
    sortOrder: 90,
  },
  {
    id: "U-B10",
    code: "U-B10",
    title: "Uninitialized reads on malformed settings (undefined behavior)",
    category: "broken",
    severity: "critical",
    origin: "prior-audit",
    verification: "reproduced",
    evidenceGrade: "V1",
    files: "src/core/SettingsIO.h:29–34",
    foundBy: "S3 C5 · S2 C5 · S1 A7",
    symptom:
      "lossy = abc (and optimize = xyz) emit --lossy=0 -O0 with no warning. On other compilers the indeterminate long can be any value; an in-range garbage color count would pass GifsicleCommand range checks silently.",
    rootCause:
      "`long v; i >> v; return v;` — if extraction fails, v is indeterminate. Same for to_double. No load-warning channel.",
    evidence:
      "This pass: a conf with lossy=abc optimize=xyz colors=999 produced `…gifsicle -m --lossy=0 -O0 /tmp/nope.gif -o /tmp/out.gif` (colors dropped: U-MISS-2). A probe program printed to_long(\"abc\")=0, to_double(\"abc\")=0 on gcc 12.2 — the 'lucky zero' the forensic audit observed.",
    fix: "Initialize v=0, return bool from to_long/to_double, keep the default and surface a load warning. Clamp parsed values to legal ranges (U-MISS-2).",
    phase: "P0",
    sortOrder: 100,
  },
];
