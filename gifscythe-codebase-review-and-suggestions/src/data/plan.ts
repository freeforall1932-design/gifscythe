import type { ClaimSeed, PlanSeed } from "./types";

export const planItems: PlanSeed[] = [
  {
    id: "P0-1",
    phase: "P0",
    title: "Stop lying about process exit",
    detail:
      "Replace system() with argv exec (fork/execvp; CreateProcess on Windows). Decode WEXITSTATUS. Pre-flight engine existence. Use std::filesystem so Windows drive letters work. This is also the quoting/injection fix.",
    closes: "U-B1, U-B6, U-MISS-3, NEW-9, NEW-12",
    estimate: "half day",
    definitionOfDone:
      "gifscythe-cli returns non-zero for a missing engine, verified from at least two CWDs (repo root and working_code/gifscythe). A path with a space round-trips. A settings value with ';' is not interpreted by a shell.",
    sortOrder: 1,
  },
  {
    id: "P0-2",
    phase: "P0",
    title: "Make CMake and build.sh honest",
    detail:
      "gifscythe_core → INTERFACE. Stop swallowing cmake/qmake output. Fail hard when GUI was requested but unbuilt. One GUI branch: qmake6 → qmake → cmake. Align the header comment with behavior.",
    closes: "U-B7, U-B9, U-M4, U-M5",
    estimate: "1–2 h",
    definitionOfDone:
      "cmake -S . configures cleanly without Qt. ./build.sh --all exits non-zero if the GUI did not actually appear in the output dir.",
    sortOrder: 2,
  },
  {
    id: "P0-3",
    phase: "P0",
    title: "Windows engine + CI that can actually compile",
    detail:
      "Do not share Linux config.h. For --windows, use win32cfg.h values (SIZEOF_UNSIGNED_LONG 4, PATHNAME_SEPARATOR '\\\\', RANDOM rand). Install Qt via aqtinstall / jurplel/install-qt-action, not choco qt6-base. Build engine once, CLI+tests, CMake GUI, windeployqt, upload artifact, run test_engine.sh.",
    closes: "U-B8, NEW-1, NEW-16, U-MISS-12",
    estimate: "half day",
    definitionOfDone:
      "Windows job green WITH a downloadable artifact that contains gifsicle.exe + gifscythe.exe + Qt runtime. gifsicle --version still says 1.96 (U-M7), not 0.1.0.",
    sortOrder: 3,
  },
  {
    id: "P0-4",
    phase: "P0",
    title: "GUI run flow: require output, verify file, handle timeout",
    detail:
      "Refuse empty output or auto-derive <name>_opt.gif. After exit 0, check exists+size>0. Honor waitForFinished's bool; on timeout kill and report failure. Disable Run while busy.",
    closes: "U-B4, U-B5",
    estimate: "half day",
    definitionOfDone:
      "Empty output never prints 'Optimization complete.' A missing engine is a dialog, not a green status. A 60s timeout is a visible failure.",
    sortOrder: 4,
  },
  {
    id: "P0-5",
    phase: "P0",
    title: "Queue means batch; Merge is an explicit action",
    detail:
      "Default Mode::Batch. Add a mode combo (Auto/Merge/Batch/Explode). Loop one run per input in batch, writing <name>_opt.gif (or in-place behind a checkbox).",
    closes: "U-B3, U-MISS-9",
    estimate: "half day",
    definitionOfDone:
      "A queue of logo.gif + logo1.gif produces two optimized files. gifsicle --info on each is 12 and 1, never 13. Merge still available as an explicit action.",
    sortOrder: 5,
  },
  {
    id: "P0-6",
    phase: "P0",
    title: "Safe settings parse + engine locator used by both fronts",
    detail:
      "Zero-init to_long/to_double with extraction checks and load warnings. parse_bool() consistency. EngineLocator in core: exe-dir (with .exe) → GS_ENGINE → PATH → release/<ver>. GUI status bar shows path+version or 'engine missing'.",
    closes: "U-B10, U-M8, U-MISS-5, NEW-2",
    estimate: "half day",
    definitionOfDone:
      "lossy=abc does not emit --lossy=0; it warns and keeps the default. GUI launched from build/gui/ finds release/0.1.0/gifsicle without copying.",
    sortOrder: 6,
  },
  {
    id: "P1-1",
    phase: "P1",
    title: "One version, one identity, one output dir",
    detail:
      "Generate version.h from VERSION.md. Engine -DVERSION=1.96. test_engine.sh greps VERSION.md. Packager follows the single CMake output dir and copies gifsicle.exe when present. windeployqt inside package_portable.sh. Ship COPYING.",
    closes: "U-M1, U-M6, U-M7, NEW-5, U-MISS-11",
    estimate: "half day",
    definitionOfDone:
      "A version bump is a 1-file edit. gifsicle --version is 1.96. Portable folder on Windows contains GUI + engine + licenses + Qt runtime.",
    sortOrder: 7,
  },
  {
    id: "P1-2",
    phase: "P1",
    title: "Queue UX, live pane honesty, save/validate",
    detail:
      "Append+dedupe, Remove/Clear, working drag-and-drop. Connect every control to the command pane. shell_quote for display. Decide two-way vs read-only and document it. save_settings() + round-trip test. gs::validate() warnings. Store Settings by value in GifsicleCommand. Include what you use.",
    closes: "U-M2, U-M3, U-MISS-1, U-MISS-2, U-MISS-7, U-MISS-8, NEW-3, NEW-6, NEW-7",
    estimate: "1 day",
    definitionOfDone:
      "Second 'Add' keeps the first file. Command pane matches actual argv for paths with spaces. load(save(s))==s. GifsicleCommand(Settings{}) as a prvalue does not dangle.",
    sortOrder: 8,
  },
  {
    id: "P1-3",
    phase: "P1",
    title: "Correct the mapping table before anyone implements it",
    detail:
      "Delay is centiseconds, not ms. Crop is X,Y+WxH, not X,Y,WxH. Do not touch loopcount=0, -O0, or gamma=-1 sentinels. Drop or gitignore caesium-bin. Add root .gitignore / .gitattributes.",
    closes: "NEW-4, NEW-13, N1, N2, N14, NEW-8",
    estimate: "2 h",
    definitionOfDone:
      "FEASIBILITY_REVIEW mapping table matches the man page and the committed engine. caesium-bin is not required to clone the product.",
    sortOrder: 9,
  },
  {
    id: "P2-1",
    phase: "P2",
    title: "Prove it: smoke suite in CI, both OSes, artifacts",
    detail:
      "--run from ≥2 CWDs, malformed-conf, merge-vs-batch, paths-with-spaces, SettingsIO round-trip, test_engine.sh, portable-folder file-existence checks. Fix ((PASS++)). Replace tautological test 4. Cache toolchains.",
    closes: "U-MISS-6, NEW-11, NEW-15",
    estimate: "1 day",
    definitionOfDone:
      "The smoke suite is green on Linux and Windows CI with downloadable artifacts. A missing engine fails the job.",
    sortOrder: 10,
  },
  {
    id: "P2-2",
    phase: "P2",
    title: "Async QProcess + progress + cancel",
    detail:
      "Member QProcess, signals, progress bar, cancel, busy-state, per-file progress in batch. This is the substrate WORKLIST task 4 stands on.",
    closes: "U-MISS-4, U-B5",
    estimate: "1 day",
    definitionOfDone:
      "A long run leaves the window interactive. Cancel kills the engine. Batch of N files reports k/N.",
    sortOrder: 11,
  },
  {
    id: "P3-1",
    phase: "P3",
    title: "UI/UX retrofit → 1.0.0 (only after P0–P2 evidence)",
    detail:
      "Input/Actions/Output tabs (XnConvert feel). Remaining GifsicleSettings widgets from the now-corrected mapping table. Dither method selector. Before/after preview, size/count, output-folder actions. Clean-Windows-machine smoke of the portable folder. Then — and only then — VERSION.md → 1.0.0. WebP/APNG stay in the bucket.",
    closes: "U-MISS-10, U-MISS-13, WORKLIST 3–7, NEW-14",
    estimate: "the rest of 0.x",
    definitionOfDone:
      "A person who has never heard of gifsicle can optimize two GIFs without surprises. Power users see a command that actually runs. Clean Windows machine, double-click, it works. Then bump 1.0.0.",
    sortOrder: 12,
  },
];

export const claims: ClaimSeed[] = [
  {
    id: "P1-engine",
    claim: "P1 engine control layer complete",
    source: "WORKLIST",
    verdict: "holds",
    note: "Core compiles clean with -Wall -Wextra -pedantic. 35/35 unit assertions pass. Flag mapping is faithful vs man page + committed gifsicle.c (loopcount=0, -O0, gamma sentinel all correct).",
    sortOrder: 1,
  },
  {
    id: "gui-mvp",
    claim: "Qt6 GUI MVP scaffold + first retrofit",
    source: "WORKLIST",
    verdict: "partial",
    note: "Window drives the core layer, but run flow is broken (U-B3 merge-weld, U-B4 stdout loss, U-B5 timeout lie) and the engine locator cannot find the default build output (NEW-2).",
    sortOrder: 2,
  },
  {
    id: "linux-ci",
    claim: "Linux CI passes with Qt6 GUI",
    source: "IMPROVEMENT_LOG / HANDOFF",
    verdict: "partial",
    note: "Job is green, but GUI build attempts are silenced/failure-tolerant (U-B7, U-M5) and no artifact proves a GUI was built. Green ≠ GUI built.",
    sortOrder: 3,
  },
  {
    id: "cli-tests",
    claim: "CLI driver + integration tests",
    source: "IMPROVEMENT_LOG",
    verdict: "fails",
    note: "CLI exists, but --run works from one CWD only and exits 0 on failure (U-B1, U-B2). No automated integration test exists (U-MISS-6). Reproduced this pass.",
    sortOrder: 4,
  },
  {
    id: "packaging",
    claim: "Packaging scripts done",
    source: "WORKLIST",
    verdict: "partial",
    note: "Scripts exist and pass bash -n, but expect the GUI where cmake never puts it and never copy gifsicle.exe (U-M6, NEW-5). A 'GUI release' can ship GUI-less and engine-less on Windows.",
    sortOrder: 5,
  },
  {
    id: "no-1.0",
    claim: "Don't bump to 1.0.0 before UI/UX retrofit",
    source: "VERSION / HANDOFF / WORKLIST",
    verdict: "holds",
    note: "Still 0.1.0. The criticals above are exactly why the rule must keep holding.",
    sortOrder: 6,
  },
  {
    id: "webp-deferred",
    claim: "WebP/APNG deferred until GIF UI stable",
    source: "VISION / bucket list",
    verdict: "holds",
    note: "Scope clean — no premature codec code in working_code/gifscythe/src.",
    sortOrder: 7,
  },
];
