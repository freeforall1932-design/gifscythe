import type { FindingSeed } from "./types";

export const refutedFindings: FindingSeed[] = [
  {
    id: "N1",
    code: "N1",
    title: "REFUTED — loopcount=0 is not 'play once'",
    category: "refuted",
    severity: "advisory",
    origin: "prior-audit",
    verification: "refuted",
    evidenceGrade: "V1",
    files: "src/core/GifsicleCommand.h:160–165 · reference_code/gifsicle/src/gifsicle.c:1853–1858",
    foundBy: "S2 N1 (false positive)",
    symptom:
      "Secondary audit claimed --loopcount=0 emits the wrong flag and should be changed to --loopcount=forever. Implementing that 'fix' is unnecessary; changing the semantics would be the bug.",
    rootCause:
      "Misread of the man page. --loopcount=0 is equivalent to --loopcount=forever, not --no-loopcount.",
    evidence:
      "This pass, committed source gifsicle.c case 'l': if negated, loopcount=-1 (--no-loopcount); else loopcount = (clp->have_val ? clp->val.i : 0). Official man page: '--loopcount=0 is equivalent to --loopcount=forever, not --no-loopcount.' Current code emitting --loopcount=0 for 'loop forever' is exactly right.",
    fix: "Do not change this. Both forms work; =0 is canonical. Keep the unit test that asserts --loopcount=0.",
    phase: "none",
    sortOrder: 400,
  },
  {
    id: "N2",
    code: "N2",
    title: "REFUTED — -O0 is valid ('optimization off')",
    category: "refuted",
    severity: "advisory",
    origin: "prior-audit",
    verification: "refuted",
    evidenceGrade: "V1",
    files: "src/core/GifsicleCommand.h:166–168 · gifsicle.c OPTIMIZE_OPT:1866–1878",
    foundBy: "S2 N2 (false positive)",
    symptom:
      "Secondary audit claimed -O0 is invalid and the spinbox should clamp to 1. The GUI already exposes 0–3. Clamping to 1 would remove a legitimate 'off' setting.",
    rootCause: "Man page lists three positive levels; the parser still accepts 0 and uses it to clear the optimize mask.",
    evidence:
      "This pass, OPTIMIZE_OPT handler: if negated or value<0 then o=0; else o = have_val ? val.i : 1; then `optimizing = (optimizing & ~GT_OPT_MASK) | o`. No error path for 0. GUI spinbox range 0–3 is correct; label 0 as 'Off' if anything.",
    fix: "Do not clamp. Optionally setSpecialValueText('Off') on optimizeSpin_ like lossy already does.",
    phase: "none",
    sortOrder: 410,
  },
  {
    id: "N11",
    code: "N11",
    title: "REFUTED — MainWindow.h missing from CMake does not break moc",
    category: "refuted",
    severity: "advisory",
    origin: "prior-audit",
    verification: "refuted",
    evidenceGrade: "V3",
    files: "CMakeLists.txt:32–33",
    foundBy: "S2 N11 (false positive)",
    symptom:
      "Secondary audit claimed omitting MainWindow.h from target sources causes a linker 'undefined reference to vtable'.",
    rootCause:
      "qt_standard_project_setup() (Qt ≥ 6.3) sets CMAKE_AUTOMOC ON. AUTOMOC scans headers included by listed sources — MainWindow.h is #included by MainWindow.cpp. qmake path mocs Q_OBJECT headers automatically too.",
    evidence:
      "CMakeLists.txt calls qt_standard_project_setup() then qt_add_executable(gifscythe src/qtui/main.cpp src/qtui/MainWindow.cpp). Weakest grade: not execution-verified here (no Qt in this sandbox). Adding the header to sources remains a harmless belt-and-braces measure.",
    fix: "No link failure to fix. Optional: list MainWindow.h in target sources for older Qt.",
    phase: "none",
    sortOrder: 420,
  },
  {
    id: "N14",
    code: "N14",
    title: "REFUTED — gamma does not default to 0.0",
    category: "refuted",
    severity: "advisory",
    origin: "prior-audit",
    verification: "refuted",
    evidenceGrade: "V1",
    files: "src/core/GifsicleSettings.h:80 · GifsicleCommand.h:87",
    foundBy: "S2 N14 (false positive)",
    symptom:
      "Secondary audit claimed gamma defaults to 0.0 so --gamma 0.0 is always emitted. That would have been a real bug. It is not the code.",
    rootCause: "Misread of the default. Sentinel is -1.0; gate is >= 0.",
    evidence:
      "This pass, GifsicleSettings.h: `double gamma = -1.0; // --gamma; -1 = unchanged`. GifsicleCommand.h: `if (s.gamma >= 0) { add --gamma; add f2s(s.gamma); }`. --gamma is Clp_ValString (required value), so the two-arg form is also correct.",
    fix: "No change. Do not implement the attached audit's P1 'Fix Argument Logic (N1, N2, N14)' — all three premises are wrong.",
    phase: "none",
    sortOrder: 430,
  },
];
