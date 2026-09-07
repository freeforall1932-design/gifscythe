# Gifscythe — Compiled Audit (Master)

**Compiled:** 2026-09-07 · **Verification session S4:** 2026-09-07 (Qt 6.4.2 + mingw-w64 + Wine sandbox)  
**Branch (fixes):** `arena/01a07959-gifscythe` → merged to `main`; S4 work on `verify/windows-ci-fixes`  
**Product version:** 0.1.0 (do **not** bump to 1.0.0 yet)  
**Companion docs:** `SESSION_HANDOFF.md` · `WORKLIST.md` · `IMPROVEMENT_LOG.md`

This document **merges and adjudicates**:

| ID | Source | What it is |
|----|--------|------------|
| **S1** | `gifscythe-final-code-review.md` | Original full code review + remediation plan (BR/AL/MS/VP IDs) |
| **S2** | `gifscythe-comprehensive-review.md` | Consolidated forensic compilation (U-B / U-M / U-MISS / NEW IDs) |
| **S3** | Branch `codebase-review-and-optimization-3bbfe` → [`WORKLIST_CODE_REVIEW.md`](https://github.com/freeforall1932-design/gifscythe/blob/codebase-review-and-optimization-3bbfe/WORKLIST_CODE_REVIEW.md) | Separate 30-item worklist audit |

Every S3 item is graded: **REAL / PARTIAL / FALSE POSITIVE / DUPLICATE / DEFERRED**, mapped to S1/S2 IDs when applicable, and marked against the **2026-09-07 fix session**.

---

## 0. How to use this file (next session)

1. Do **not** re-open items marked ✅ FIXED without first running the **regression checklist** in §6.
2. Prefer fixing ⬜ OPEN items in the order of §5 (P0 leftovers → P1 product → deferred).
3. After any fix, run §6 and tick the boxes with evidence (command + exit code / screenshot).
4. Watch for **new pits** (§7): closing one hole by opening another is a failed fix.
5. WebP / APNG / frame editor stay **blocked** until GIF UI is stable (PROJECT_VISION).

---

## 1. Executive cross-audit summary

| Metric | Count |
|--------|------:|
| S3 items that are **real** and already in S1/S2 | 18 |
| S3 items that are **real but weaker / incomplete** vs S1/S2 | 5 |
| S3 items that are **false positive or factually wrong** | 3 |
| S3 items that are **net-new** (not in S1/S2, still valid product work) | 4 |
| Critical silent-failure class from S1/S2 **missing entirely from S3** | **9** |
| Items **fixed in code this session** (local tests green) | see §3 |
| Items still **open** before 1.0.0 | see §5 |

### Critical gaps: what S3 missed (already in S1/S2)

S3 is useful as a **product/UX backlog**, but it is **not** a substitute for the forensic compilation. These silent-failure / correctness bugs were **already flagged by S1/S2** and were **absent or underspecified** in S3:

| ID | Problem | Why it matters |
|----|---------|----------------|
| U-B1 / BR-1 | CLI `--run` exits **0** when engine missing (`system()` wait-status truncation) | CI false green |
| U-B3 / BR-3 | GUI hardcodes **Merge** → queue of N GIFs becomes **one** welded animation | silent data mangling |
| U-B4 / BR-4 | Empty output → gifsicle writes stdout, GUI discards bytes, still says success | silent data loss |
| U-B6 / BR-6 | `system()` + unquoted `toString()` → spaces break; shell metachar injection | security + correctness |
| U-B7 / BR-7 | CMake `STATIC` header-only lib → configure fails | build broken |
| U-B10 / BR-10 | `to_long` uninitialized on parse fail → UB | garbage options |
| U-B11 / BR-11 / NEW-1 | Windows engine built against **Linux** `config.h` → `static_assert` | Windows never compiles even after CI package fix |
| U-B13 / BR-13 / NEW-6 | `GifsicleCommand` holds `const Settings&` → dangling temporary | latent UB |
| U-MISS-1 | SettingsIO “load/save” had **no save** | docs lied |

**Rule for next session:** if S3 and S1/S2 disagree on severity, **trust S1/S2 + primary source** (gifsicle man page / committed engine source).

---

## 2. S3 item-by-item adjudication

Legend for **Status (code @ 2026-09-07)**:
- ✅ **FIXED** — implemented this session; needs §6 verification on real machines
- 🟡 **PARTIAL** — foundation present; product polish or full scope still open
- ⬜ **OPEN** — not done (or only documented)
- ❌ **FP** — false positive / do not implement as stated
- ⏸️ **DEFERRED** — correctly postponed (post-1.0 or after GIF UI)

### 2.1 S3 P0 — Critical blockers

#### S3-1 · CLI engine path resolution broken
| | |
|--|--|
| **Verdict** | **REAL** — already **U-B2 / BR-2 / NEW-9 / U-MISS-5** |
| **Fresh?** | No — prior audits had it; S3 restates a weaker version |
| **S3 accuracy** | Partial. Hardcoded `working_code/.../0.1.0/gifsicle` was real. Proposed `resolve_engine_path()` from settings path alone is **weaker** than the real fix (exe-anchored locator + `GS_ENGINE` + `std::filesystem`). Verification steps cite `settings.json` and flags the CLI **does not have** (`--settings`, `--input`) — format is `key=value` conf. |
| **Status** | ✅ **FIXED** — `EngineLocator.h` + CLI uses `locate_engine(argv[0])`; smoke tests from `/tmp` and product dir |
| **Verify next** | §6.A items A1–A4 |

#### S3-2 · GUI process blocks UI thread
| | |
|--|--|
| **Verdict** | **REAL** — already **U-B5 / BR-5 / U-MISS-4 / N15** |
| **Fresh?** | No |
| **S3 accuracy** | Correct problem. Also needed: ignore-`waitForFinished` bool → false success (S1/S2); empty-output path (U-B4) is adjacent. |
| **Status** | ✅ **FIXED** (async member `QProcess`, `finished`/`errorOccurred`, cancel, busy state, indeterminate progress). Not full per-byte gifsicle progress parse (gifsicle has little structured progress). |
| **Verify next** | §6.B B1–B5 · watch **new pit**: cancel mid-batch must not leave `busy_` stuck or partial outputs claimed complete |

#### S3-3 · Windows cross-compilation fails
| | |
|--|--|
| **Verdict** | **REAL but incomplete** — already **U-B8 / BR-8 + U-B11 / BR-11** |
| **Fresh?** | No |
| **S3 accuracy** | **Misses the deeper bug**: even with mingw present, Linux `config.h` (`SIZEOF_UNSIGNED_LONG 8`) fails Win64 `static_assert`. Also names `ci.yml` — real file is `.github/workflows/build.yml`. Wrong choco package `qt6-base` was the CI install killer. |
| **Status** | 🟡 **NEAR-GREEN** — S4 found the real step-4 killer: 1.96 sources do *unconditional* `#include <config.h>` and the Windows line lacked `-I.` (guard-neutralized root config.h). Recipe fixed (upstream `Makefile.mingw` flags), local cross-build green, **exe verified running under Wine** (`1.96 (Windows)`, valid GIF output). Workflow applied to main (821a310); remaining: push fix → confirm Actions windows green. |
| **Verify next** | §6.C C2 (CI rerun) — C3/C5 already evidenced under Wine |

---

### 2.2 S3 P1 — High priority (before 1.0.0)

#### S3-4 · GUI missing most GifsicleSettings controls
| | |
|--|--|
| **Verdict** | **REAL** — **U-MISS-13 / WORKLIST 5** |
| **Fresh?** | No — known gap; S3 list is useful checklist |
| **S3 accuracy** | Mostly good. Mode selection was also a **correctness** bug (Merge hardcoded = U-B3), not only “missing control”. Crop form in S3 text `X,Y+W,H` is informal; emitter must stay **`X,Y+WxH`** (VP-5). |
| **Status** | 🟡 **PARTIAL** — mode combo + optimize + lossy + output present; ~25 other fields still core-only |
| **Verify next** | When adding widgets: one control → one flag unit test; do **not** “align” crop/delay/loopcount to wrong docs |

#### S3-5 · No Input/Actions/Output tab flow
| | |
|--|--|
| **Verdict** | **REAL** — WORKLIST task 3 / P3-1 |
| **Fresh?** | No (planned UX) |
| **Status** | ⬜ **OPEN** — still single-panel MVP |
| **Verify next** | After tabs: Batch default preserved; live pane still syncs; no Merge-by-default regression |

#### S3-6 · Drag-and-drop not implemented
| | |
|--|--|
| **Verdict** | **REAL** — **U-MISS-7 / N4** |
| **Fresh?** | No |
| **S3 accuracy** | Correct (`setAcceptDrops` without handlers). Accepting png/webp/apng in GIF-only phase would **violate scope** — GIF-only until 1.0.0. |
| **Status** | ✅ **FIXED** — `DropListWidget` with `filesDropped` → append+dedupe |
| **Verify next** | §6.B B6–B7 · non-GIF drops should not crash; scope = GIF for now |

#### S3-7 · No before/after preview
| | |
|--|--|
| **Verdict** | **REAL** — WORKLIST 4 / P3-3 |
| **Fresh?** | No (planned) |
| **Status** | ⬜ **OPEN** |
| **New-pit risk** | Live re-encode on every slider move can freeze UI again — must debounce + async (do not reintroduce S3-2) |

#### S3-8 · No progress bar or cancel
| | |
|--|--|
| **Verdict** | **REAL** — **U-MISS-4** (overlaps S3-2) |
| **Fresh?** | No |
| **Status** | 🟡 **PARTIAL** — cancel + indeterminate `QProgressBar` + per-file batch status text. No ETA / parsed gifsicle % (engine doesn’t expose clean %). |
| **Verify next** | §6.B B3–B5 |

#### S3-9 · Queue management missing
| | |
|--|--|
| **Verdict** | **REAL** — **U-M2 / U-MISS-8** |
| **Fresh?** | No |
| **Status** | 🟡 **PARTIAL** — append+dedupe, Remove, Clear done. No move up/down, no total byte size display yet. |
| **Verify next** | §6.B B8–B10 |

#### S3-10 · Output folder actions missing
| | |
|--|--|
| **Verdict** | **REAL** — WORKLIST 4 |
| **Fresh?** | Product gap (lightly covered as packaging/UX in S1) |
| **Status** | ⬜ **OPEN** — Browse save-as exists; no “Open folder”, no `{name}_opt` template UI (batch auto `_opt.gif` exists in code) |
| **Verify next** | When adding Open Folder, use `QDesktopServices`; don’t shell-out |

#### S3-11 · Version string hardcoded in GUI
| | |
|--|--|
| **Verdict** | **REAL** — **U-M1 / AL-1** (7 places, not just GUI title) |
| **Fresh?** | No |
| **S3 accuracy** | Right problem; proposed guard name `GIFSYCYTHE_*` **repeats the typo** S1/S2 told us to fix (U-M11). Real fix: `VERSION.md` → `version.h` / `GS_VERSION`. |
| **Status** | ✅ **FIXED** — `version.h` + `version.h.in`; build.sh/CMake sync from VERSION.md; GUI/CLI use `GS_VERSION` |
| **Verify next** | §6.A A5 |

#### S3-12 · Packaging scripts untested
| | |
|--|--|
| **Verdict** | **REAL** — **U-M6 / U-MISS-12 / NEW-5** |
| **Fresh?** | No — S1/S2: wrong GUI path, no `.exe` probe, no licenses |
| **Status** | 🟡 **PARTIAL** — scripts fixed (multi-path probe, `.exe`, COPYING, optional windeployqt); portable folder built in sandbox **without GUI**. Clean-machine / Windows package still unproven. |
| **Verify next** | §6.D D1–D4 |

#### S3-13 · No input validation in SettingsIO
| | |
|--|--|
| **Verdict** | **REAL intent, WRONG details** — **U-B10 / U-MISS-2 / U-MISS-16** |
| **Fresh?** | No |
| **S3 accuracy** | **False premise:** SettingsIO is **not JSON** — it is flat `key = value` text. There is no `SettingsIO.cpp`. Validation belongs in parse + `gs::validate()`. |
| **Status** | ✅ **FIXED** (for actual format) — safe `to_long`/`to_double`, load warnings, `Validate.h`, save/load round-trip tests, missing file → `nullopt` |
| **Verify next** | §6.A A6–A8 · do **not** rewrite as JSON unless product decision changes |

---

### 2.3 S3 P2 — Medium (post 1.0.0 / later)

| S3 | Topic | Verdict | Fresh? | Status |
|----|-------|---------|--------|--------|
| **14** | Logging framework | Real enhancement, not a blocker | **Yes vs S1/S2** (net-new backlog) | ⏸️ optional post-1.0 |
| **15** | Config persistence (QSettings) | Real; S1 had save_settings for conf, not app prefs | Partial overlap U-MISS-1 | ⬜ OPEN (app prefs) · conf save ✅ |
| **16** | i18n | Real enhancement | **Net-new** | ⏸️ post-1.0 |
| **17** | RGBA frame model | Real — WORKLIST deferred bucket | No | ⏸️ **blocked** until GIF 1.0.0 |

---

### 2.4 S3 P3 — Low / enhancements

| S3 | Topic | Verdict | Notes | Status |
|----|-------|---------|-------|--------|
| **18** | Magic number timeouts | Soft / partially obsolete | Async path removed finish-timeout false success; start wait still 5000 | 🟡 low |
| **19** | Unit tests for SettingsIO | Real — was U-MISS-6/16 | Round-trip + malformed now in unit + smoke | ✅ largely FIXED |
| **20** | Benchmark suite | Enhancement | Net-new | ⏸️ |
| **21** | Native installers | Conflicts with vision “no installer / portable” | **Mostly FP vs PROJECT_VISION** — portable is the goal; .deb/.msi optional later | ❌ as requirement · ⏸️ optional |
| **22** | Animated WebP | Deferred bucket | **Blocked** | ⏸️ |
| **23** | APNG | Deferred bucket | **Blocked** | ⏸️ |
| **24** | Frame editor | Deferred bucket | **Blocked** | ⏸️ |
| **25** | Batch rename templates | Real UX | Overlaps S3-10 | ⬜ OPEN |
| **26** | Preset system | Real UX | Uses save_settings foundation | ⬜ OPEN |
| **27** | System tray | Enhancement | Net-new | ⏸️ |
| **28** | Keyboard shortcuts | Partial — Open/Quit exist | Expand later | 🟡 |
| **29** | Dark mode | Enhancement | Net-new | ⏸️ |
| **30** | Crash reporting | Optional | Net-new | ⏸️ |

---

## 3. Full S1/S2 master list — fix checklist (this session)

Use this as the **authoritative** broken/misaligned/missing register. S3 IDs cross-linked where relevant.

### 3.1 BROKEN (must not regress)

| ID | Summary | S3 | Code status | Next-session verify |
|----|---------|----|-------------|---------------------|
| U-B1 / BR-1 | CLI exit 0 on failure | (missed) | ✅ `run_argv` + preflight | A2: missing engine → rc≠0 |
| U-B2 / BR-2 | CWD-only engine path | S3-1 | ✅ `EngineLocator` | A1, A3 |
| U-B3 / BR-3 | GUI Merge welds queue | S3-4 partial | ✅ Batch default + mode combo | B11: 2 GIFs → 2 outputs |
| U-B4 / BR-4 | Empty output data loss | (missed) | ✅ require/auto out + exists check | B12 |
| U-B5 / BR-5 | waitForFinished ignored | S3-2 | ✅ async finished handler | B1–B5 |
| U-B6 / BR-6 | shell `system()` / injection | (missed) | ✅ argv exec + `shell_quote` display | A4 spaces path |
| U-B7 / BR-7 | CMake STATIC header-only | (missed) | ✅ INTERFACE | C6 cmake configure |
| U-B8 / BR-8 | Windows CI package/steps | S3-3 | 🟡 engine recipe fixed + Wine-proven; workflow hardened (static link, Ninja, E2E smoke); **push blocked (dead token)** | C2 CI rerun after push |
| U-B9 / BR-9 | build.sh GUI dispatch lies | (missed) | ✅ honest `--all` / fail | C7 |
| U-B10 / BR-10 | SettingsIO UB parse | S3-13 | ✅ safe parse | A6 |
| U-B11 / BR-11 | Win Linux config.h | S3-3 miss | ✅ win32cfg.h include | C2 engine builds |
| U-B12 / BR-12 | CLI POSIX-only paths | (missed) | ✅ `std::filesystem` | C3 CLI on Windows |
| U-B13 / BR-13 | Settings& dangling | (missed) | ✅ store by value | A9 prvalue test |
| U-B14 / BR-14 | CMake bash engine target | (missed) | ✅ find bash or skip | C6 |

### 3.2 MISALIGNED

| ID | Summary | S3 | Status | Verify |
|----|---------|----|--------|--------|
| U-M1 / AL-1 | Version in 7 places | S3-11 | ✅ VERSION.md → version.h | A5 |
| U-M2 / AL-2 | Add replaces queue | S3-9 | ✅ append+dedupe | B8 |
| U-M3 / AL-3 | Live pane not live | (weak) | ✅ all controls wired | B13 |
| U-M4 / AL-4 | build.sh docs vs behavior | (missed) | ✅ | C7 |
| U-M5 / AL-5 | Dead config.h writer / silenced Qt | (missed) | ✅ | C7 |
| U-M6 / AL-6 | Packager paths / no .exe | S3-12 | 🟡 scripts fixed | D1–D4 |
| U-M7 / AL-7 | Engine version = product version | (missed) | ✅ engine 1.96 | A10 |
| U-M8 / AL-8 | Bool/rotation parse inconsistent | S3-13 | ✅ | A7 |
| U-M9 / AL-9 | Hygiene / gitignore / includes | (missed) | ✅ S4: caesium-bin **untracked** (`git rm --cached`, 62 files/74 MB); manifest documents re-fetch; history purge remains optional | D5 ✅ |
| U-M10 / AL-10 | Delay “ms” in FEASIBILITY | (missed) | ✅ table fixed | docs only |
| U-M11 / AL-11 | GIFSYCYTHE guard typo | S3-11 bad fix | ✅ GIFSCYTHE_* | grep |

### 3.3 MISSING

| ID | Summary | S3 | Status | Verify |
|----|---------|----|--------|--------|
| U-MISS-1 | save_settings | S3-15 partial | ✅ | A8 round-trip |
| U-MISS-2 | validate() | S3-13 | ✅ | A6 |
| U-MISS-3 | safe exec / quote / ~ | (missed) | ✅ | A4 |
| U-MISS-4 | async/progress/cancel | S3-2,8 | 🟡 | B1–B5 |
| U-MISS-5 | EngineLocator shared | S3-1 | ✅ | A1, B14 |
| U-MISS-6 | Real integration tests | S3-19 | ✅ smoke_cli + engine in CI recipe | A1–A4, CI |
| U-MISS-7 | Drag-drop | S3-6 | ✅ | B6 |
| U-MISS-8 | Queue remove/clear | S3-9 | 🟡 | B9 |
| U-MISS-9 | Mode selector | S3-4 | ✅ | B11 |
| U-MISS-10 | Dither method | S3-4 | 🟡 core string; no GUI widget | later |
| U-MISS-11 | LICENSE/COPYING | (missed) | ✅ | D2 |
| U-MISS-12 | Windows package path | S3-12 | 🟡 | D3–D4 |
| U-MISS-13 | Remaining GUI controls | S3-4 | ⬜ | before 1.0 |
| U-MISS-14 | Two-way CLI pane | (S1) | 🟡 **descoped**: honest one-way + FEASIBILITY wording updated | don’t claim two-way in 1.0 without `parse_args` |
| U-MISS-15 | gamma srgb\|oklab | (missed) | 🟡 core `gamma_str`; no GUI | later |
| U-MISS-16 | Tautological test / load swallow | S3-19 | ✅ | unit suite |

### 3.4 Verified-correct — do **NOT** “fix”

| ID | Behavior | Why |
|----|----------|-----|
| VP-1 / N1 refuted | `--loopcount=0` = forever | man page + gifsicle.c |
| VP-2 / N2 refuted | `-O0` = optimization off | gifsicle.c OPTIMIZE_OPT |
| VP-3 / N14 refuted | gamma sentinel `-1` / `gamma_str` | no always-emit |
| VP-4 / N11 refuted | AUTOMOC via qt_standard_project_setup | no vtable crisis |
| VP-5 / NEW-13 | crop `X,Y+WxH` emitter correct | man page; FEASIBILITY table was wrong |

---

## 4. What this session changed (implementation map)

| Area | Files |
|------|-------|
| Core | `GifsicleSettings.h`, `GifsicleCommand.h`, `SettingsIO.h`, `Validate.h`, `EngineLocator.h`, `ProcessRunner.h`, `version.h`, `version.h.in` |
| CLI | `src/cli/main.cpp` |
| GUI | `MainWindow.cpp/.h`, `DropListWidget.h`, `main.cpp` |
| Build | `CMakeLists.txt`, `build.sh`, `scripts/build_gifsicle.sh`, `package_*.sh`, `test_engine.sh`, **new** `smoke_cli.sh` |
| CI | Proposed rewrite in `docs/ci/build.yml.proposed` (apply over `.github/workflows/build.yml` with `workflows` permission; push of workflow files blocked for this App) |
| Tests | `tests/test_gifsicle_command.cpp` (expanded) |
| Docs/license | `LICENSE`, `COPYING.gifsicle`, root `.gitignore`, `.gitattributes`, WORKLIST/HANDOFF/IMPROVEMENT_LOG, FEASIBILITY table fixes |
| **This file** | `COMPILED_AUDIT.md` (merge of S1+S2+S3) |

### S4 verification-session changes (2026-09-07, later)

| Area | Files | Why |
|------|-------|-----|
| Windows engine recipe | `scripts/build_gifsicle.sh` | `-I.` + upstream mingw flags; drop `-DVERSION` (win32cfg.h owns it) — fixes CI run #18 step 4 |
| Windows argv exec | `src/core/ProcessRunner.h` | `_spawnvp` does NOT quote args → space paths split (found via Wine E2E). Now `CreateProcessA` + MSVCRT-rule `win_quote_arg` |
| Quoting regression test | `tests/test_gifsicle_command.cpp` | test 19 covers win_quote_arg edge cases (empty, tabs, embedded quotes, trailing backslashes) |
| GUI harness | `tests/test_gui_offscreen.cpp` (new) | 81 automated checks for §6.B under `QT_QPA_PLATFORM=offscreen`; runs in CI on both OSes |
| CMake | `CMakeLists.txt` | harness target + ctest registration; MinGW static-link options (CLI/tests fully static; GUI static-libgcc/libstdc++) |
| build.sh honesty | `build.sh` | delete stale GUI binaries before probing (stale `build/gui/gifscythe` faked "GUI built" with Qt removed) |
| CI workflow | `.github/workflows/build.yml` (+ proposed copy) | `-static` CLI/tests, native Windows engine+CLI E2E smoke step, **Ninja generator** (VS default can't consume MinGW Qt), GUI offscreen steps |
| One-command §6 | `scripts/verify_audit.sh` (new) | 21 PASS / 0 FAIL / 2 SKIP locally |
| Hygiene | `reference_code/caesium-bin` untracked; `REFERENCE_MANIFEST.md` note | D5 |

### Local evidence already collected (sandbox)

```text
Unit tests:          ALL TESTS PASSED  (74 CHECK assertions)
Engine pipeline:     5/5 passed
CLI smoke:           7/7 passed
  - missing engine → exit 1
  - run from /tmp with absolute settings → OK
  - paths with spaces → OK
  - malformed conf warnings → OK
  - shell_quote on display → OK
Engine --version:    LCDF Gifsicle 1.96
E2E demo GIF:        /tmp/gifscythe_demo.gif 9458 bytes
Portable package:    release/0.1.0/Gifscythe/ (engine+CLI+licenses; no GUI in sandbox)
```

**Not verified here:** Qt6 GUI runtime, Windows CI job, windeployqt folder on clean Windows.

---

## 5. Remaining work ordered (next sessions)

### Gate 0 — PUSH BLOCKED (do first)
0. 🔑 **The fine-grained PAT provided 2026-09-07 is INVALID** ("Bad
   credentials" on API, "Invalid username or token" on push; reads only work
   because the repo is public). All S4 fixes are **committed locally on
   `verify/windows-ci-fixes`** but cannot be pushed. Need a new token with
   **Contents: R/W + Workflows: R/W + Pull requests: R/W** (Metadata: R).

### P0-ish until proven on CI (fixes staged locally)
1. ⬜ Push → GitHub Actions **windows** green + downloadable artifact (C2)
   — engine recipe fix + static linking + Ninja + smoke steps all staged
2. ⬜ Clean-Windows windeployqt smoke from the CI artifact (C4/D3/D4)

### P1 product (before claiming 1.0.0)
3. ⬜ Input / Actions / Output tabs (S3-5)
4. ⬜ Expose remaining settings controls (S3-4 / U-MISS-13) — keep VP-1..5 sacred
5. ⬜ Before/after preview with debounce + async (S3-7) — harness now guards
   the no-UI-block invariant (T9 ticks) so regressions get caught
6. ⬜ Output folder actions + naming templates (S3-10 / S3-25)
7. ⬜ Queue size/count display; optional reorder (S3-9 remainder)
8. ⬜ One-time real-desktop GUI probes: B5, B6 physical drop, B14 engine-missing
9. ⬜ Decide two-way CLI: implement `parse_args` **or** keep one-way forever
   (U-MISS-14) — docs already one-way honest

### Explicitly blocked until GIF 1.0.0
- ⏸️ S3-17,22,23,24 (frame model, WebP, APNG, frame editor)
- ⏸️ Do not bump VERSION to 1.0.0 as a placeholder

### Optional post-1.0
- S3-14 logging, 15 app QSettings, 16 i18n, 20 bench, 26 presets, 27–30 UX chrome
- S3-21 installers only if vision changes (portable is default)
- caesium-bin git-history purge (optional; untracked since S4)
- Windows live-pane display: `shell_quote` is POSIX-style; a cmd.exe-style
  quoter for Windows users is a nice-to-have (display only — exec is argv)

---

## 6. Verification checklist — RUN 2026-09-07 (session S4, Linux sandbox + Wine)

> **Status:** Executed in full on Linux (Debian 12 sandbox, gcc 12.2, cmake 3.25,
> **Qt 6.4.2**, mingw-w64 12, **Wine 8**). One-command rerun:
> `working_code/gifscythe/scripts/verify_audit.sh` (21 PASS / 0 FAIL / 2 SKIP —
> skips are the CI-gated and clean-Windows-desktop items).
> Evidence tags: **[L]** Linux sandbox · **[W]** Windows PE binary under Wine ·
> **[H]** offscreen GUI harness (`test_gui_offscreen`, 81 checks) ·
> **[CI]** GitHub Actions.

### 6.A CLI / core (no Qt required) — ALL GREEN [L]

- [x] **A1** `./build.sh && ./build/gifscythe-cli examples/animation.conf --run`
      → exit 0, `/tmp/gifscythe_demo.gif` 9458 bytes **[L]**
- [x] **A2** `--engine /nope` → exit **1**, stderr `ERROR: engine not found` **[L]**
      (also under Wine: Windows CLI exe exits 1 on `C:\nope\gifsicle.exe` **[W]**)
- [x] **A3** run from `/tmp` with absolute settings+engine → exit 0 **[L]**
- [x] **A4** conf with spaces in input/output → exit 0, files written; print
      mode shows `'…my vacation…'` quoted **[L]** — and under Wine with
      `C:\gs\in\my vacation\b.gif` → exit 0 after ProcessRunner fix **[W]**
- [x] **A5** no hardcoded `0.1.x` in `src/cli`/`src/qtui`; bump test done:
      VERSION.md → 0.1.1 → build.sh + CMake regenerated `GS_VERSION "0.1.1"`,
      CLI banner + GUI title (harness T1) followed; reverted to 0.1.0 **[L][H]**
- [x] **A6** malformed conf (`lossy=abc`, `colors=999`) → warnings, no crash;
      defaults kept (unit tests 8/18 + smoke 5–6) **[L]**
- [x] **A7** `info=yes`, `careful=on`, `rotation=none` parse (unit test 15) **[L]**
- [x] **A8** save/load round-trip (unit test 9) — ALL TESTS PASSED **[L]**
- [x] **A9** prvalue `GifsicleCommand(Settings{...})` (unit test 11) + full unit
      suite and CLI under **ASan+UBSan**: clean, honest exits, no leaks **[L]**
- [x] **A10** `release/0.1.0/gifsicle --version` → `LCDF Gifsicle 1.96` **[L]**;
      Windows exe → `LCDF Gifsicle 1.96 (Windows)` (upstream win32cfg identity) **[W]**
- [x] **A11** `./scripts/test_engine.sh` → 5/5 **[L]**
- [x] **A12** `./scripts/smoke_cli.sh` → 7/7 **[L]**
- [x] **A13 (new)** Windows unit-test exe under Wine → ALL TESTS PASSED
      (core layer logic identical on Windows; static-linked, no MinGW DLLs) **[W]**

### 6.B GUI — GREEN via offscreen harness [H] + desktop notes

New `tests/test_gui_offscreen.cpp` (CMake target `test_gui_offscreen`, runs in
CI on both OSes) drives the real MainWindow with `QT_QPA_PLATFORM=offscreen`:
**81 checks, 0 failures**.

- [x] **B1** Run-click returns in <3 s while a 3.6 s engine run continues
      async; event loop ticks ≥20×/600 ms during the run (UI thread alive) **[H]**
      (visual smoothness still worth one desktop glance)
- [x] **B2** Cancel mid-run (4800-frame GIF, t≈0.6 s) → status `Cancelled.`,
      process NotRunning, controls re-enabled, progress hidden **[H]**
- [x] **B3** Indeterminate progress bar visible + Run disabled + Cancel enabled
      while running **[H]**
- [x] **B4** Missing input (`ghost.gif`) → status `Optimization failed (exit 1)`
      + error dialog; never "complete" **[H]**
- [~] **B5** kill-engine-binary-mid-run race not simulated; cancel/close kill
      paths covered instead (B2/B15) — keep as manual desktop probe
- [x] **B6** drop signal → append (queue 0→2). Note: Qt only dispatches
      QDropEvents during a real platform drag session, so the harness emits
      `DropListWidget::filesDropped` (the exact signal the drop handler
      emits); the 15-line event overrides stay a one-time desktop check **[H]**
- [x] **B7** second drop appends, duplicate ignored (2+2→3) **[H]**
- [x] **B8** multi-select Remove (rows 0+2 of 3) → 1 left, correct survivor,
      no index corruption **[H]**
- [x] **B9** Clear → queue empty + Run disabled **[H]**
- [x] **B10** Batch default: 2 inputs → 2 `*_opt.gif`; frame counts 12 and 1
      match sources (`gifsicle --info`) **[H]**
- [x] **B11** Merge: 2 inputs → 1 output, 13 frames = 12+1 **[H]**
- [x] **B12** Merge + empty output → refuses with dialog, process never
      starts, no files written (no silent stdout loss) **[H]**
- [x] **B13** optimize/lossy/mode/output/queue changes update the pane
      immediately; space paths shown shell-quoted **[H]**
- [x] **B14** status shows real engine path; Run disabled on empty queue **[H]**
      (engine-missing variant covered by CLI A2; GUI variant = desktop probe)
- [x] **B15** close window mid-run → engine process killed, NotRunning, no crash **[H]**

### 6.C Build / CI

- [x] **C1** GitHub Actions **linux** GREEN (run #18, sha 821a310): build+GUI,
      engine 5/5, smoke 7/7, package, artifact uploaded **[CI]**
- [ ] **C2** Actions **windows** job: FAILED at run #18 step 4 (engine build,
      missing `-I.` → `config.h: No such file`). **Root cause fixed** in
      `build_gifsicle.sh` (recipe now mirrors upstream `Makefile.mingw`);
      local cross-compile green. **Needs push to re-run CI** (token dead —
      see SESSION_HANDOFF) **[L][W]**
- [x] **C3** Windows CLI runs confs with `C:\`-style paths (Wine E2E:
      `C:\gs\conf\one.conf` → exit 0, `C:\gs\out\a_opt.gif` 8627 bytes,
      12 frames; spaces path → exit 0) **[W]**
- [ ] **C4** `windeployqt` folder on a machine without Qt — needs CI artifact
      or desktop (harness/CLI statically linked; GUI relies on windeployqt)
- [x] **C5** Windows engine built with win32cfg semantics: LLP64
      static_asserts pass at compile time; exe runs and reports
      `1.96 (Windows)`; produced valid GIFs under Wine **[W]**
- [x] **C6** `cmake -S . -B build && cmake --build build` configures+builds
      with Qt (GUI+harness+CLI+tests) and without Qt (skips GUI honestly) **[L]**
- [x] **C7** `./build.sh --all` with Qt hidden (cmake configs + qmake6 moved
      out) → exit 1 + honest error. **New pit found & fixed:** stale GUI
      binaries used to pass the `-x` probe and fake "GUI built" — build.sh now
      deletes stale GUI outputs before probing **[L]**
- [x] **C8** default `./build.sh` prints "GUI not requested", claims nothing **[L]**

### 6.D Packaging / license / hygiene

- [x] **D1** `package_portable.sh` after full build → engine + CLI + **GUI**
      + VERSION/README/README.txt in `release/0.1.0/Gifscythe/` **[L]**
- [x] **D2** package contains `LICENSE` + `COPYING.gifsicle` **[L]**
- [ ] **D3/D4** Windows portable double-click + clean-VM DLL smoke — needs CI
      artifact / real Windows (CLI+engine now static/self-contained **[W]**,
      GUI needs windeployqt folder test)
- [x] **D5** `reference_code/caesium-bin` **untracked** (`git rm --cached`,
      62 files, 74 MB) — gitignore now truthful; product builds without it;
      REFERENCE_MANIFEST.md documents re-fetch. History purge still optional **[L]**

### 6.E "Did we dig a new pit?" probes — ALL GREEN

- [x] **E1** Batch of 1 + explicit Save-as path → that exact path used
      (`my explicit result.gif` written; no `a_opt.gif`) **[H]**
- [x] **E2** Explode + empty output → auto `<stem>_frame` prefix; frames
      `.000`–`.011` written; no merge-style output requirement **[H]**
- [x] **E3** no `system(` / `sh -c` / `cmd.exe` / `/bin/sh` in `src/`
      (grep clean; Windows exec = CreateProcessA, POSIX = fork/execvp) **[L]**
- [x] **E4** Batch stays default (combo index 0 + harness T1 + probe) **[H][L]**
- [x] **E5** Windows engine line: `-include src/win32cfg.h` first, `-I.`
      guard-neutralized config.h, no `-DVERSION` override **[L]** (proof: **[W]**)
- [x] **E6** unit test 4 still `has(args,"a.gif")`, not tautological **[L]**
- [x] **E7** FEASIBILITY delay = 1/100 s; GUI has no delay widget yet, so no
      ms mislabel exists (keep this true when adding one) **[L]**
- [x] **E8** guards all `GIFSCYTHE_*`; zero `GIFSYCYTHE` hits **[L]**

### Remaining unchecked (gated on push / real Windows)

- [ ] **C2** Windows CI green (fix staged; **blocked on a valid push token**)
- [ ] **C4/D3/D4** clean-Windows windeployqt smoke (after C2 artifact exists)
- [ ] **B5/B6-plumbing/B14-engine-missing** one-time real-desktop GUI probes

---

## 7. Known residual risks / possible new pits after fixes

| Risk | Why | Mitigation |
|------|-----|------------|
| Batch auto-output overwrites existing `*_opt.gif` | No prompt yet | Add overwrite confirm before 1.0 |
| Indeterminate progress only | gifsicle lacks rich progress | Acceptable; don’t block UI “parsing” fake % |
| `waitForStarted(5000)` still sync on start | Short block only | OK; full async start optional |
| GUI untested in this sandbox | ~~No Qt6 here~~ RESOLVED S4: Qt 6.4.2 installed; 81-check offscreen harness green | Keep harness in CI; one desktop pass for B5/B6-plumbing |
| Windows CI complexity (aqt + mingw shim) | Step-4 root cause FIXED + Wine-proven; generator/static-link landmines defused in workflow | Push → treat C2 as gate |
| One-way CLI pane vs old “two-way” marketing | Doc updated; labels must stay honest | U-MISS-14 |
| Drop accepts any existing path | Non-GIF could be queued | Filter `*.gif` harder in drop handler if needed |
| `caesium-bin` may still exist in git history | gitignore stops new adds | Optional history purge later (not required for build) |

---

## 8. False positives & bad prescriptions (do not implement blindly)

| Claim | Source | Why rejected |
|-------|--------|--------------|
| SettingsIO is JSON / SettingsIO.cpp | S3-13 | Format is `key=value` headers only |
| CLI flags `--settings` / `--input` / `settings.json` | S3-1 verify steps | Real CLI: `gifscythe-cli <file.conf> [--run] [--engine]` |
| Fix version with `GIFSYCYTHE_VERSION` guard | S3-11 | Propagates include-guard typo |
| Workflow file `ci.yml` | S3-3 | Actual: `build.yml` |
| Installers required for 1.0 | S3-21 | Vision = portable click-and-run |
| Change `--loopcount=0` / ban `-O0` / “fix” crop commas | older N-series | Refuted by engine source (VP-1,2,5) |
| Accept APNG/WebP in drop before 1.0 | S3-6 expected | Scope violation |

---

## 9. Definition of done (evidence, not vibes)

| Milestone | Done when |
|-----------|-----------|
| **P0 honesty** | §6.A all green on Linux; A2 never exits 0 on missing engine |
| **Windows path** | §6.C C2–C5 green with artifacts |
| **GUI MVP trustworthy** | §6.B B10–B14 green (batch vs merge, no silent loss) |
| **1.0.0** | Tabs + major controls + preview + clean Windows portable (§6.D) + no open U-B* | then bump VERSION.md |
| **2.x** | Only after 1.0.0: frame model → WebP/APNG |

---

## 10. Source index

- S3 branch file: https://github.com/freeforall1932-design/gifscythe/blob/codebase-review-and-optimization-3bbfe/WORKLIST_CODE_REVIEW.md  
- S1: `gifscythe-final-code-review.md`  
- S2: `gifscythe-comprehensive-review.md`  
- Product docs: `PROJECT_VISION.md`, `WORKLIST.md`, `SESSION_HANDOFF.md`, `IMPROVEMENT_LOG.md`, `FEASIBILITY_REVIEW.md`  
- Engine truth: `reference_code/gifsicle/` + https://www.lcdf.org/gifsicle/man.html  

---

## 11. Handoff one-liner for next session

> §6 was **executed with evidence on 2026-09-07 (S4)** — rerun
> `working_code/gifscythe/scripts/verify_audit.sh` (expect 21 PASS / 2 SKIP)
> plus `test_gui_offscreen` before trusting anything new. The ONE blocker is
> the **dead push token** (§5 Gate 0): push `verify/windows-ci-fixes`, confirm
> Actions windows green (C2), then start the P1 GUI retrofit (tabs → controls →
> preview). Never “fix” §3.4 verified-correct behaviors; never start WebP/APNG
> before GIF 1.0.0. If a change breaks A2/B10/B12/E3–E5 or any harness test,
> it is a **new pit** — revert and redo.

*End of compiled audit.*
