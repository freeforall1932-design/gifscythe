# Post-merge audit — PR #7 (S7) at `8190c08`

> **Status: superseded in part.** This is source C of three independent reviews. Read
> **`docs/audit/CONSOLIDATED_AUDIT_2026-09-10.md`** first — it merges this report with
> the GPT 5.6 sol xhigh and Seed 2.1 Pro Preview audits into one verified register
> (44 findings) and records two places where **this report was wrong**: the `threads`
> "benign" claim (withdrawn — it is bug **U-03**) and the incomplete statement of the
> overwrite finding (**U-01**, which also covers output-equals-input). Findings F-04,
> F-07, F-08, F-10, F-11, F-13 and the executed-verification tables below remain
> current and are cross-referenced there.

**Date:** 2026-09-10 · **Auditor:** Arena.ai agent session ·
**Commit audited:** `8190c0854f0b5427705e8afe47d43cd664b84179` (*Merge pull request #7
from freeforall1932-design/arena/s7-settings-persistence*, merged 2026-09-10T01:33:49Z)
**Diff reviewed:** `c5efe07..8190c08` — 23 files, **+1264 / −209**
**Product version:** 0.1.0 · **Engine:** gifsicle 1.96

This is an independent post-merge review, not a restatement of the PR description.
Every claim below is either (a) the output of a command run in this session, or
(b) a file:line citation. Items that could not be checked in this environment are
listed as such in §5 rather than assumed.

---

## 1. Verdict

The merge is **sound and shippable as 0.1.0**. The three features it adds (settings
persistence, queue reorder, `{name}` templates) are implemented in the right place,
with the project's existing honesty conventions largely intact, and the harness
additions (T14–T16) are genuinely thorough.

Three things need attention before 1.0.0:

1. **One real data-loss path survives the new collision guard** (F-01, High).
2. **Two documented claims do not hold as written** (F-02, F-03, Medium) — both are
   cheap to fix in code or in the docs, but they are currently stated as verified fact
   in `SESSION_HANDOFF.md` and `IMPROVEMENT_LOG.md`.
3. **`scripts/verify_audit.sh` reports FAIL instead of SKIP when `cmake` is absent**
   (F-09), so its headline number is not reproducible off the author's machine.

No security regressions were found. Execution stays argv-only in all three layers
(desktop `ProcessRunner`, CLI, web server); I could not make the web endpoint write
outside its scratch directory or honour a client-supplied `output`/`inputs`.

| Severity | Count | IDs |
|---|---|---|
| High | 1 | F-01 |
| Medium | 4 | F-02, F-03, F-04, F-05 |
| Low | 5 | F-06, F-07, F-08, F-09, F-10 |
| Nit | 3 | F-11, F-12, F-13 |

---

## 2. What was actually executed

Run in this sandbox on 2026-09-10 (Debian 12, gcc 12.2.0, node v22.22.3).
`apt` is blocked here, so **no Qt6, no cmake, no mingw** — see §5.

| Check | Command | Result |
|---|---|---|
| Engine build | `./build.sh` step 1 | `LCDF Gifsicle 1.96` |
| CLI build | `./build.sh` step 2 | ok |
| Unit suite (20 tests) | `./build.sh` step 3 | `ALL TESTS PASSED` |
| Engine pipeline | `./scripts/test_engine.sh` | **5 passed, 0 failed** |
| CLI smoke | `./scripts/smoke_cli.sh` | **7 passed, 0 failed** |
| Audit verifier | `./scripts/verify_audit.sh` | **19 passed, 1 FAILED (C6), 3 skipped** — see F-09 |
| Web command mirror | `node web/test/command.test.mjs` | 13 PASS, `ALL WEB COMMAND TESTS PASSED` |
| Workflow drift | `diff .github/workflows/build.yml docs/ci/build.yml.proposed` | **identical** (PR fix confirmed) |
| Dirty-tree check | `git status --short` after `./build.sh` | **clean** (PR fix confirmed: `version.h` generators now byte-identical) |
| Web E2E optimize | `POST /optimize` with `logo.gif` | `200 image/gif`, **8703 → 2246 bytes**, `60x132 → 18x40`, `global color table [64]` |
| Web argv override | `settings={"output":"/tmp/pwn.gif","inputs":["/etc/passwd"]}` | `200`, `/tmp/pwn.gif` **not created**; command used `/tmp/gsweb-*/in.gif` |
| Static traversal | 5 probes (`/../`, `%2e%2e/`, `..%2f`, …) | no escape; served `web/README.md` or `404` |
| GUI-saved conf → CLI | real serializer output → `gifscythe-cli` | exit **0**, **1 warning** (F-03) |
| GUI-saved conf → reload | real `save_settings` → real `load_settings` | 0 load warnings, **crop/position/scale values lost** (F-02) |
| Engine resize probe | `gifsicle --resize-fit 0x0` | **rc=1** `one of W and H must be positive` (F-05) |

---

## 3. Findings

### F-01 (High) — Batch auto-naming can still silently overwrite outputs

`runCommand()` refuses a batch run only when the template contains **no `{name}`**
(`src/qtui/MainWindow.cpp:730`, helper at `:521`). Any template *with* `{name}` is
assumed safe — but `{name}` is the input's **base name**, which is not unique across
the queue.

Reachable today, with the shipped default template:

```
queue:  /projects/a/hero.gif
        /projects/b/hero.gif      (different files, same base name)
batch dir: /out
→ both render /out/hero_opt.gif
```

`appendInputs()` de-dupes identical paths (`MainWindow.cpp:375`), so the *same* file
cannot be queued twice — but two different files with the same stem can. The batch
loop computes each output independently (`:756` for the first file, `:872` for every
subsequent one) and **never compares them**, so file 2 overwrites file 1 with no
warning, and the run still reports `Optimization complete — 2 file(s).` (`:891`).

This is precisely the failure class the project forbids elsewhere (audit B12 / E1
"no silent overwrite") and that the PR's own refusal dialog was added to prevent.
Harness T16 exercises the collision path with `a.gif` + `b.gif`
(`tests/test_gui_offscreen.cpp:1163-1164`) — distinct stems — so the gap is untested.

**Fix:** compute the whole output list before starting the run and refuse (or
auto-suffix `_2`, `_3`, …) on any duplicate. The check belongs next to the existing
refusal at `:730` so the live summary at `:568` can warn with the same wording:

> **Correction (2026-09-10).** This finding was **incomplete**: it only covered two
> inputs colliding with each other. The GPT 5.6 audit (GS-001) additionally notes that
> a template such as `{name}.gif` with an empty batch folder makes
> `defaultOutputFor()` (`:527-531`, which falls back to `fi.absolutePath()`) return
> **the input file itself** — so the run silently destroys the user's source. Verified
> by execution: `gifsicle self.gif -o self.gif` → **rc=0**, 8703 → 2246 bytes, md5
> changed, and the GUI would report "Optimization complete". The guard must therefore
> reject *target == source* as well as duplicate targets. See
> `CONSOLIDATED_AUDIT_2026-09-10.md` **U-01**.

```cpp
QStringList outs;
for (const QString& in : inputs_) outs << defaultOutputFor(in);
QString dup;
for (int i = 0; i < outs.size() && dup.isEmpty(); ++i)
  for (int j = i + 1; j < outs.size(); ++j)
    if (outs.at(i) == outs.at(j)) { dup = outs.at(i); break; }
if (!dup.isEmpty()) { /* warn + return, exactly like the {name} refusal */ }
```

Add a T16 case: two same-named files in different folders → summary warns, engine
never starts.

---

### F-02 (Medium) — `readFrom()` is *not* the exact inverse of `writeInto()`

`SESSION_HANDOFF.md:40` and `MainWindow.h`'s new header block describe
`SettingsPanel::readFrom()` as the "exact inverse of `writeInto()`". It restores every
control `writeInto()` reads, but the **serializer** drops fields whose parent toggle is
off, so those values do not survive a restart:

| Field group | Written only when | Citation |
|---|---|---|
| `crop_x/y/w/h`, `crop_transparency` | `s.crop` is true | `src/core/SettingsIO.h:249` |
| `position_x/y` | `s.has_position` | `SettingsIO.h:245` |
| `scale_x/y` | `resize_kind == Scale` | `SettingsIO.h:282` |
| `threads` | `s.threads > 0` | `SettingsIO.h:262` |

Reproduced with the project's own code (`gs::save_settings` → `gs::load_settings`),
starting from `crop=false, crop_w=200, crop_h=150, has_position=false, position=12x7`:

```
reloaded: crop=0 crop_w=0 crop_h=0 crop_transparency=0
reloaded: has_position=0 position=0x0
reloaded: resize_kind=1 resize=320x0 scale=1x1
reloaded: threads=-1 (GUI spin was 0 = Auto)
load warnings: 0
```

Impact is small but user-visible: set crop dimensions with **Crop** unticked, close,
relaunch → the spinners are back at 0. No warning is emitted in any of these cases,
which is what makes it worth fixing.

> **Correction (2026-09-10, superseded by `CONSOLIDATED_AUDIT_2026-09-10.md` §2).**
> This paragraph originally went on to call the `threads` case "benign (0 and −1 both
> mean *Auto*)". That was **wrong** and is withdrawn: neither value emits a flag, and
> the engine's own default is `thread_count = 0`
> (`reference_code/gifsicle/src/gifsicle.c:39`), so "Auto" actually means
> **single-threaded** — real auto is bare `-j`, which sets
> `GIFSICLE_DEFAULT_THREAD_COUNT = 8` (`:38`, `:1887-1893`).
> `GifsicleCommand.h:210` emits `-jN` only when `threads > 0`. This is a genuine bug,
> raised independently by the Seed audit as BUG-01 and recorded as **U-03**.

**Fix (pick one):** always serialize the dependent fields (they are inert when the
toggle is off), or drop the "exact inverse" wording in `SESSION_HANDOFF.md` /
`MainWindow.h` and state the carve-out explicitly the way the `optimize = -1` and
`disposal 4..7` carve-outs are already stated.

---

### F-03 (Medium) — "the CLI reads GUI-saved files without warnings" is false for a real GUI-saved file

`SESSION_HANDOFF.md:52` and the PR description both assert exit 0 **and zero
warnings**. Unit test 20 (`tests/test_gifsicle_command.cpp:312-329`) does pass, but its
fixture contains `input = a.gif`. A file actually written by `saveSessionState()` never
has one — `MainWindow.cpp:1073-1074` clears `inputs` and `output` on purpose — and
`gs::validate()` warns on an empty input list (`src/core/Validate.h:47`).

Executed: a conf produced by the real `gs::save_settings` + the two GUI keys, fed to
the real CLI:

```
$ build/gifscythe-cli /tmp/audit/gui_saved.conf
# Gifscythe 0.1.0 command (live CLI pane)
…/gifsicle -b -k 128 --dither=floyd-steinberg --lossy=40 --gamma=srgb …
CLI_EXIT=0
WARNING: input=: at least one input file is required     ← stderr, 1 line
```

The important half of the claim **is** verified: the GUI keys `batch_dir` /
`name_template` are ignored silently (0 *load* warnings) — that is what test 20 pins
and it holds. The overstatement is the "no warnings at all" phrasing for a whole
GUI-saved file.

**Fix:** reword the two doc claims to "GUI state keys are tolerated silently; the
`input` warning is expected because the queue is not persisted", or have test 20 assert
the real shape (no `input` key → exactly one `input` warning, zero *load* warnings).

---

### F-04 (Medium) — Name-template sanitisation is POSIX-only, on a product that ships Windows

`renderedOutputName()` strips only `/` and `\` (`src/qtui/MainWindow.cpp:508-510`).
Windows additionally rejects `< > : " | ? *`, trailing dots and spaces, and the
reserved device names `CON PRN AUX NUL COM1…9 LPT1…9`. With the Windows build as a
first-class target (CI windows job, `windeployqt` smoke C4/D3/D4):

* template `out:v1` → `out:v1.gif` → an NTFS **alternate data stream** is created
  instead of a file, or the write fails;
* template `con` → `con.gif` → writes to the console device, not the output folder;
* template `final.` → `final..gif` → invalid on Windows.

None of these is reachable on Linux, so CI's linux job will stay green and the failure
will only appear on a real Windows machine.

**Fix:** in the same guard block, strip `<>:"/\|?*`, trim trailing dots/spaces, and
prefix an underscore when the stem matches a reserved name (case-insensitive,
extension ignored). Add a harness case alongside the existing `../../evil` check at
`tests/test_gui_offscreen.cpp:1186`.

---

### F-05 (Medium) — `Validate.h` does not check resize geometry; the engine then fails at run time

`gs::validate()` checks colors, disposal, optimize, lossy, delay, info-mode conflict,
crop 0×0 and empty inputs — but **not** `resize_w`/`resize_h` against `resize_kind`.
Both default to 0 (`src/core/GifsicleSettings.h:90-91`), so a conf with a resize kind
and no dimensions produces a command the engine rejects:

```
$ gifsicle --resize-fit 0x0 -o t.gif logo.gif
gifsicle: one of W and H must be positive in '--resize-fit WxH'    (rc=1)
```

(verified; `40x0` is fine and yields `40x88`, so only the all-zero case breaks.)

Not reachable from the GUI — both spinners have `setRange(1, 65535)`
(`src/qtui/SettingsPanel.cpp:164,168`) — but the `.conf` workflow is a documented
first-class path (`gifscythe-cli settings.conf`), and the whole point of `Validate.h`
is "surface out-of-range / conflicting settings **before** they vanish". Today the
user finds out only from the engine's stderr after the run starts.

**Fix:** add to `Validate.h`:

```cpp
if (s.resize_kind != ResizeKind::None && s.resize_kind != ResizeKind::Scale &&
    s.resize_w == 0 && s.resize_h == 0)
  add("resize", "0x0", "resize_kind set but width and height are both 0");
```

plus a unit test beside the existing malformed-parse cases.

---

### F-06 (Low) — Settings file is not written atomically

`saveSessionState()` opens the destination with `QIODevice::WriteOnly |
QIODevice::Truncate` and then writes (`src/qtui/MainWindow.cpp:1086-1096`). A crash,
power loss or full disk between truncate and write leaves a truncated
`gifscythe.conf`. The next launch then applies a partial state and shows a warning —
honest, but avoidable: write `gifscythe.conf.tmp` and `QFile::rename()` over the
original. Cheap, and it matches the "never lose the user's work" posture of the rest
of the merge.

### F-07 (Low) — A third parser for the same file format

`guiStateKey()` (`src/qtui/MainWindow.cpp:68-84`) re-implements the `key = value` /
`#`-comment rules, re-opens the file, and is called twice per load
(`:1044`, `:1046`) — so a settings file is now parsed three times by two
implementations. It is correct today (last-occurrence-wins matches `SettingsIO`), but
it will silently diverge the moment the format grows (quoted values, escapes,
sections).

**Fix:** have `SettingsIO::load_settings` optionally collect unrecognised keys into a
`std::map<std::string,std::string>*` and let the GUI read `batch_dir` /
`name_template` from that map. One parser, one set of rules.

### F-08 (Low) — "Persistence unavailable" is silent

`saveSessionState()` returns `true` when `sessionFilePath()` is empty
(`src/qtui/MainWindow.cpp:1060`), so `closeEvent` shows no dialog, and
`loadSessionState()` returns without a word (`:1028`). On a platform with no writable
`AppConfigLocation` the user's settings are simply never saved, forever, with no
indication. The documented rule is "failed save → warning dialog on close, never
silent"; "unavailable" currently gets a pass. Consider a one-time status-bar note.

### F-09 (Low) — `verify_audit.sh` FAILs instead of SKIPping without `cmake`

```
  FAIL [C6] cmake configure/build
  SKIP [B] Qt6 not installed — run on a Qt machine or CI
==> Done: 19 passed, 1 failed, 3 skipped.      (exit 1)
```

`C6` (`scripts/verify_audit.sh:91-94`) shells out to `cmake` with no
`command -v` guard, while the `[B]` block four lines below (`:104`) guards properly and
skips. On any machine without cmake — including a fresh container — the verifier's
headline number changes and its exit code goes red for a reason that is not a product
defect. `IMPROVEMENT_LOG.md:100` records **21 PASS / 0 FAIL / 2 SKIP**, which is the
correct number *on a machine with cmake + Qt*; both numbers are true, only one is
portable.

**Fix:** wrap C6 in `if command -v cmake …; else skip "C6" "cmake not installed"; fi`,
mirroring `[B]`.

### F-10 (Low) — `docs/ci/build.yml.proposed` is a permanent drift magnet

PR #7 had to re-sync it after it drifted from the live workflow. `diff` is clean now
(verified this session), but nothing *keeps* it clean: it is a hand-maintained byte copy
of `.github/workflows/build.yml`, and the "byte-identical" constraint is enforced only
by human memory. Either delete it (the live workflow is the source of truth and is
already in the repo), or add a CI step that fails when the two differ.

### F-11 (Nit) — Summary label for a single-file batch

With mode Batch, one input and an empty Save-as, `refreshOutputSummary()` falls through
to the else branch at `src/qtui/MainWindow.cpp:575` and renders
`Batch (1 files) → /out/a_opt.gif … /out/a_opt.gif` — plural "1 files" and the same
path twice. Add a `inputs_.size() == 1` case.

### F-12 (Nit) — Web POC hardening (`web/server.mjs`)

Documented as a proof of concept, and nothing here was exploitable in testing, but:

* `serveStatic` guards with `file.startsWith(ROOT)` (`:105`) — no separator, so a
  sibling directory named `web*` would pass the prefix test. `new URL()` collapses `..`
  before this runs, and five probes found no escape, but the idiomatic check is
  `file === ROOT || file.startsWith(ROOT + sep)`.
* Body over `MAX_BODY` rejects into the generic handler → **500**, not **413**
  (`:90-91` reject → `:180` catch).
* The 500 handler echoes error text to the client (`:180-182` returns `err.message`; `:201-203` returns `String(err)`) — internal paths in error
  text.
* `findEngine()` sorts release dirs lexicographically (`:52`), so `0.9.0` beats
  `0.10.0` once a 0.10 exists. `build.sh` reads `VERSION.md`; this should too.
* No auth and no concurrency cap while binding `0.0.0.0` — fine for a local demo,
  worth one line in `web/README.md` if it is ever exposed.

Verified working: the JS builder's argv matches the C++ builder's ordering exactly
(`-k 64 --dither=floyd-steinberg --lossy=40 --resize-fit 40x40`), and client-supplied
`inputs`/`output` cannot be honoured because the server overwrites them after the
spread (`:156`).

### F-13 (Nit) — Placement of the two dated review snapshots

`gifscythe-comprehensive-review.md` and `gifscythe-final-code-review.md` sit at the repo
root while newer material lives under `docs/`. `SESSION_HANDOFF.md:34` already
carries a policy note explaining why they keep stale references, so this is cosmetic —
but a `docs/archive/` move would stop new readers mistaking them for current state.

---

## 4. What checked out clean

Verified rather than assumed:

* **No shell anywhere.** `ProcessRunner::run_argv` is `fork`/`execvp` (POSIX) and
  `CreateProcessA` with hand-built MSVCRT quoting (Windows); the web server uses
  `spawn` with an argv array; `verify_audit.sh` E3 still passes. Unit test 19 pins the
  quoting rules that fixed the 2026-09-07 Wine bug.
* **PR #7's hygiene fixes are real.** `diff` against the live workflow is clean; the
  `build_gifsicle.sh` shim is gone; `./build.sh` leaves the tree **clean**
  (`version.h` now byte-identical between the shell and CMake generators).
* **Queue reorder is correct.** `moveCurrent()` (`MainWindow.cpp:533-547`) keeps
  `inputs_` index-aligned with the list rows in both directions, bounds-checks both
  ends, and `setBusy()` really does disable both buttons (`:667`, `:673`) — the PR's
  "disabled while busy" claim holds. Merge order follows `inputs_`, which
  `currentSettings()` walks in order.
* **Template escaping works on POSIX.** Separators are stripped, `.gif` is appended,
  empty renders fall back, and T16 asserts `../../evil` → `evil.gif`.
* **T14 is a strong test**: it asserts file *contents* (not just widget state),
  dependent enabled-state restoration, the deliberately-not-persisted queue/Save-as,
  and corrupt-file honesty — including that a rejected key leaves the default and a
  valid key in the same file still applies.
* **Round-trip of everything the GUI can actually produce** is lossless: mode,
  optimize, lossy, colors, dither method, gamma (named and custom), resize kind+W,
  delay, loop, comments, background, careful, flips, batch dir, name template —
  all confirmed present in the generated file and correctly re-read.
* **`dither = none` is not a round-trip bug.** `SettingsIO` maps it to
  `dither=false, method=""` and `GifsicleCommand.h:108-111` emits nothing for
  `method == "none"` — behaviourally identical. `kDitherMethods`
  (`SettingsPanel.cpp:24-26`) has no `"none"` entry, so the GUI cannot produce it.
* **Duplicate queue entries are impossible** (`appendInputs` skips paths already in
  `inputs_`), so the only collision path is F-01.
* **No `TODO`/`FIXME`/`HACK` markers** anywhere in `src/` or `web/`.

---

## 5. Could not be verified in this environment

Stated plainly rather than glossed:

* **The Qt GUI was not compiled or run.** This sandbox has no Qt6, no `cmake` and no
  `qmake`, and `apt` cannot reach `deb.debian.org` (verified: HTTP 000 / connection
  failed; GitHub is reachable, so it is an allowlist, not a total block). Therefore:
  * the **243-check offscreen harness was not executed here**. `grep -c` counts **247
    `CHECK`/`CHECK_MSG` call sites**, which is consistent with 243 runtime checks, but
    that is arithmetic, not a test run. Every finding in `MainWindow.cpp` /
    `SettingsPanel.cpp` above is **static analysis** with file:line citations.
  * `verify_audit.sh` `[B]` and `C6` are skip/fail here for that reason (F-09).
* **Nothing was verified on Windows.** F-04 is derived from the Windows filename rules
  and the fact that `win32cfg.h` sets `PATHNAME_SEPARATOR='\\'`; it needs a real
  Windows run to confirm.
* The engine, CLI, unit suite, engine pipeline, CLI smoke, web tests and the live web
  server **were** all run — see §2.

**Partially closed after push.** This branch's CI run
[34427315414](https://github.com/freeforall1932-design/gifscythe/actions/runs/34427315414)
passed **both** jobs (`linux` 1m9s, `windows` 2m48s). The linux job runs
`./build.sh --all` and then `QT_QPA_PLATFORM=offscreen ./build-cmake/test_gui_offscreen`
under `set -euo pipefail` (`.github/workflows/build.yml:28-35`), and the harness
returns 1 on any failure (`tests/test_gui_offscreen.cpp:1220-1224`) — so a green job
means the harness **ran and exited 0** on ubuntu-latest, and the Qt GUI **compiles on
Windows** (Qt 6.7.3 / MinGW). That corroborates the "243 checks, 0 failures" claim
without me having run it. The raw log text could not be retrieved from this sandbox
(`gh run view --log` → `results-receiver.actions.githubusercontent.com … EOF`), so the
printed count itself remains unconfirmed by me.

**Still open:** F-01/F-02/F-04 fixes each need a new harness case, and F-04 needs a real
Windows desktop run (`windeployqt` smoke C4/D3/D4) — CI compiles the GUI but does not
exercise the filesystem rules F-04 depends on.

---

## 6. Suggested order of work

1. **F-01** duplicate-output guard + T16 case (High; data loss).
2. **F-09** one-line `verify_audit.sh` guard (restores a portable headline number).
3. **F-05** resize validation + unit test (small, closes a real engine-error path).
4. **F-04** Windows template sanitisation (bundle with the C4/D3/D4 Windows smoke).
5. **F-02 / F-03** decide: fix the behaviour, or fix the two sentences in
   `SESSION_HANDOFF.md` / `IMPROVEMENT_LOG.md` that overstate it.
6. **F-06 – F-08** opportunistic.
7. Owner decisions already parked in `SESSION_HANDOFF.md` (two-way CLI pane, version
   0.2.0 vs 1.0.0) are unchanged by this audit.
