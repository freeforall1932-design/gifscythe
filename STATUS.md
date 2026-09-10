# STATUS - the single status register

**One row per tracked item. Every item in exactly one of four states.**
This file is the roll-up; the detail stays where it already is.

| This file | The detail |
|---|---|
| **`STATUS.md`** (here) | the roll-up: one row per item, four states, machine-parseable. Answers *"how much is done?"* in one line. |
| `COMPILED_AUDIT.md` | the per-finding evidence for every `U-nn` row: source audit, verification mark, file:line, before/after. **Not** summarised away - the `U-nn` rows below are *generated from it*. |
| `WORKLIST.md` | the human task board (what to pick up next, in order). |
| `SESSION_HANDOFF.md` | context for the next session. |
| `IMPROVEMENT_LOG.md` | what each session did, newest first. |
| `PROJECT_VISION.md` | mission + hard constraints. |

**Do not hand-edit the generated block.** Run
`working_code/gifscythe/scripts/check_docs.sh --emit`. Plain
`check_docs.sh` regenerates to a temp file and diffs it against this file, so a
hand-fudged roll-up fails the gate.

## Vocabulary - four states, no synonyms

| State | Means | Rule |
|---|---|---|
| **DONE** | closed, with executed proof named in the row | Proof column names a command, a test id or a diff. |
| **PARTIAL** | partly closed | **The row must name what is still missing.** A bare `PARTIAL` is a gate failure. |
| **OPEN** | known, scoped, not started | Proof column carries the blocker; Next action carries the single next step. |
| **UNTRIAGED** | discovered, not yet scoped | The state that did not exist before. A new finding is recorded **in the session it is found**, here and as a pending line in `WORKLIST.md`. It may not sit unscoped past the next log entry (gate failure). |

**Session** = last session that touched the item, or `-` if untouched.
**Proof / Blocker** is never blank. **Next action** is `-` only for DONE.

**Counts (generated - do not edit by hand):** 61 DONE · 4 PARTIAL · 31 OPEN · 1 UNTRIAGED · 97 total
**Last regenerated:** S9 · 2026-09-10 · by scripts/check_docs.sh --emit

## Register, part 1 - derived from `COMPILED_AUDIT.md` §5

<!-- BEGIN GENERATED from COMPILED_AUDIT.md - do not edit; run check_docs.sh --emit -->
| ID | Item | State | Session | Proof / Blocker | Next action |
|----|-------|-------|---------|-----------------|-------------|
| U-01 | Batch auto-naming overwrites other outputs *and* the source GIF. | DONE | S8 | `src/core/OutputPlan.h` + CLI/GUI planning | - |
| U-02 | Portable packager reports success for an incomplete release. | DONE | S8 | packager fails closed + `scripts/test_package.sh` | - |
| U-03 | Threads "Auto" runs single-threaded. | DONE | S8 | bare `-j` for Auto (C++ + JS parity) | - |
| U-04 | CLI `--run` without `output` corrupts its own stdout. | DONE | S8 | `--run` commentary moved to stderr | - |
| U-05 | Documented PATH engine fallback is dead code. | DONE | S8 | real `find_on_path()`; `""` when not found | - |
| U-06 | Web demo binds `0.0.0.0` with no auth, no concurrency cap, 64 MB bodies, 120 s engine runs. | DONE | S8 | binds 127.0.0.1; `GS_WEB_HOST` to opt in | - |
| U-07 | Windows CLI execution is ANSI-only. | OPEN | - | not started; scoped as P1-4 in COMPILED_AUDIT.md §6 | P1-4: Windows Unicode process APIs. |
| U-08 | License set can ship incomplete, silently. | DONE | S8 | license set asserted, negative-tested | - |
| U-09 | Banked Windows snapshot is 5 commits behind the SHA its own notes claim. | OPEN | - | not started; scoped as P0-4 in COMPILED_AUDIT.md §6 | P0-4: Re-cut release evidence. |
| U-10 | The "read-only, identical-to-upstream" vendored engine is neither. | PARTIAL | S8 | manifest wording corrected; provenance still needs a clone | P2-3: Immutable + correctly-labelled upstream tree. |
| U-11 | Malformed booleans degrade silently. | DONE | S8 | `parse_bool_strict` warns, leaves field unchanged | - |
| U-12 | "Fully async" GUI still blocks the UI thread in 5 places — up to 5 s per run start. | OPEN | - | not started; NOT scoped in COMPILED_AUDIT.md §6 | scope it in COMPILED_AUDIT.md §6, then work it |
| U-13 | Drag-and-drop accepts any existing file. | DONE | S8 | drop filter `&&`; empty comments skipped (C++ + JS) | - |
| U-14 | Green CI does not enforce the claims used as release gates. | PARTIAL | S8 | negative packaging tests + manifest assertion in CI | P2-1: Run `verify_audit.sh` in CI. |
| U-15 | CMake writes into the source tree. | OPEN | - | not started; scoped as P2-2 in COMPILED_AUDIT.md §6 | P2-2: Stop CMake writing into `src/`. |
| U-16 | Settings persistence is non-atomic (Truncate + write). | OPEN | - | not started; scoped as P1-18 in COMPILED_AUDIT.md §6 | P1-18: Atomic settings persistence. |
| U-17 | Explode mode never verifies any frame was written. | OPEN | - | not started; scoped as P1-19 in COMPILED_AUDIT.md §6 | P1-19: Explode frame verification. |
| U-18 | The regression suite does not cover any of the failure classes above. | PARTIAL | S8 | planning/threads/validate/bool/comment unit tests, T17, package suite | P2-4: Regression suite expansion. |
| U-19 | `readFrom()` is not the "exact inverse" of `writeInto()`. | DONE | S8 | not reproducible with toggles on; wording fixed, pinned by unit test 26 | - |
| U-20 | "The CLI reads GUI-saved files without warnings" is false. | DONE | S8 | doc claim reworded; the `input` warning is expected | - |
| U-21 | Name-template sanitisation is POSIX-only — no Windows invalid chars, no trailing dot/space trim, no reserved-name guard. | DONE | S8 | new `src/core/OutputName.h` (`NameRules` parameterised, so the Windows rule set is unit-tested on Linux); test 29, 30 assertions | - |
| U-22 | `Validate.h` skips resize geometry, so a conf can reach the engine with `--resize-fit 0x0`. | DONE | S8 | resize/scale geometry validated (rules probed off the engine) | - |
| U-23 | Unknown CLI arguments are silently ignored — stronger than A's wording: nothing is printed at all, rc=0. | DONE | S8 | strict arg parser, rc=2 | - |
| U-24 | Web server doesn't verify `out.gif` exists/non-empty after rc=0, unlike the desktop. | DONE | S8 | 422 when rc=0 produced no output | - |
| U-25 | Web "Scale %" defaults to 50%, desktop to 100%. | DONE | S8 | web Scale default 100 | - |
| U-26 | Web engine discovery sorts versions lexicographically (`0.9.0` > `0.10.0`). | DONE | S8 | numeric version compare | - |
| U-27 | Current-state docs still listed a completed CI action as pending. | DONE | S7 | S7 doc re-sync (§6 P3-2); the completed CI action is no longer listed as pending | - |
| U-28 | `runCommand()` non-batch path has no `return` after the "could not start engine" dialog (batch path does return). | DONE | S8 | `return` added (CI-compiled) | - |
| U-29 | Web resize dropdown omits Touch, though `command.mjs:90-91` implements it and the desktop has 6 kinds. | DONE | S8 | Touch option added | - |
| U-30 | Web server has no `validate()` equivalent; out-of-range values go straight to the engine. | DONE | S8 | new `web/validate.mjs` mirrors `core/Validate.h`; 422 + issues; `web/test/validate.test.mjs` 19/19 parity vs the real CLI | - |
| U-31 | `build.sh` never links `-lstdc++fs`; `EngineLocator.h` uses `std::filesystem`, so g++ 7/8 hosts fail at link. | DONE | S8 | `build.sh` link-probes `-lstdc++fs` (writes `build/.fs_probe.cpp`, tries with/without, cleans up) | - |
| U-32 | POSIX `run_argv` returns 1 on a signalled child instead of the `128+signum` convention. | DONE | S8 | `run_argv` returns `128+WTERMSIG`; test 28: SIGTERM→143, SIGKILL→137, `exit 3`→3, missing binary→127 | - |
| U-33 | Setting only `position_x` or `position_y` in a conf sets `has_position = true`, yielding a half-specified `-p X,0`. | DONE | S8 | `-p` needs both halves | - |
| U-34 | Preview temp files leak within a session: cleanup removes only `preview_{seq-1}` and only on the non-stale path, so superseded... | OPEN | - | not started; scoped as P1-10 in COMPILED_AUDIT.md §6 | P1-10: Preview invalidation + cleanup. |
| U-35 | `setBusy(false)` re-enables Run without re-checking the engine, unlike `appendInputs`. | OPEN | - | not started; scoped as P1-22 in COMPILED_AUDIT.md §6 | P1-22: `setBusy(false)` engine re-check. |
| U-36 | A third parser for the settings format (`guiStateKey`) re-opens and re-parses the file twice per load. | OPEN | - | not started; scoped as P3-7 in COMPILED_AUDIT.md §6 | P3-7: Third-parser consolidation. |
| U-37 | "Persistence unavailable" is silent — no dialog, no status note. | OPEN | - | not started; scoped as P3-8 in COMPILED_AUDIT.md §6 | P3-8: "Persistence unavailable" status note. |
| U-38 | `verify_audit.sh` FAILs instead of SKIPping C6 when `cmake` is absent (`[B]` guards properly 13 lines later). | DONE | S8 | C6 SKIPs without cmake | - |
| U-39 | `docs/ci/build.yml.proposed` is a hand-maintained byte copy of the live workflow (already drifted once). | DONE | S8 | `verify_audit.sh` E9 drift guard | - |
| U-40 | CLI prints `validate()` warnings and runs anyway; the GUI refuses. | OPEN | - | not started; scoped as P3-5 in COMPILED_AUDIT.md §6 | P3-5: Document CLI warning policy. |
| U-41 | Web POC is single-file Auto mode only — no batch/merge/explode. | OPEN | - | not started; NOT scoped in COMPILED_AUDIT.md §6 | scope it in COMPILED_AUDIT.md §6, then work it |
| U-42 | Web has one `scalePct` for both axes; desktop has independent X/Y. | OPEN | - | not started; NOT scoped in COMPILED_AUDIT.md §6 | scope it in COMPILED_AUDIT.md §6, then work it |
| U-43 | Summary reads `Batch (1 files) → X … X` (plural + duplicated path) for one input with no Save-as. | DONE | S8 | "Batch (1 file)" (CI-compiled) | - |
| U-44 | The two dated review snapshots sit at repo root while newer material lives in `docs/`. | DONE | S8 | `git mv` to `docs/archive/`; the 3 prose references updated; README layout lists it | - |
| U-45 | Batch output destination can change during a run — `setBusy()` only disables `batchDirEdit_` text field, not the Browse button; the picker can still call `setText()` on the disabled field. | OPEN | - | not started; scoped as P0-1 in COMPILED_AUDIT.md §6 | P0-1: Plan all batch outputs before the first process starts. |
| U-46 | A previous web request can replace the current result — `setFile()` resets preview + re-enables Run without cancelling the pending fetch. | DONE | S8 | `requestGen` counter in `app.js`, checked after fetch, after blob read, in catch and finally | - |
| U-47 | Desktop preview invalidation happens too late — `previewSeq_` only advances when a *new eligible preview starts*, not on selection/settings change. | OPEN | - | not started; scoped as P1-10 in COMPILED_AUDIT.md §6 | P1-10: Preview invalidation + cleanup. |
| U-48 | Empty comments remove a required argv operand — `--comment` is emitted with no following argument when `s.comments` contains an empty string (from `comment = ` in a conf). | DONE | S8 | empty comments skipped in C++ + JS, parity fixture added | - |
| U-49 | Settings query values are decoded twice — `searchParams.get()` already decodes, but `decodeURIComponent(raw)` decodes again. | DONE | S8 | double `decodeURIComponent` removed; live-reproduced `{"comments":["100%"]}` → HTTP 400, now HTTP 200 / 8679 B + `transport.test.mjs` regression net | - |
| U-50 | Valid command text can break HTTP response headers — the full command (with CJK comments, newlines, or Unicode engine path) is inserted into `X-Gifscythe-Command` header without header-safe encoding. | DONE | S8 | header percent-encoded + decoded in `app.js`; `{"comments":["作品"]}` went HTTP 500 → HTTP 200 / 8681 B + `transport.test.mjs` regression net | - |
| U-51 | Unescaped settings values can become additional keys — the line-based serializer writes string values verbatim (including newlines), while the loader splits on newlines and treats each line as a new key. | DONE | S8 | `encode_line_value()` at 9 write sites (+ JS mirror); reproduced a comment hijacking `mode`, test 30 guards it | - |
| U-52 | Before-image object URLs are never released — `app.js` creates object URLs for Before preview but only revokes After URLs. | DONE | S8 | `beforeUrl` tracked and revoked on replacement in `app.js` | - |
<!-- END GENERATED -->

## Register, part 2 - hand-maintained (W worklist · D deferred · R risk · new findings)

These rows have no other machine-readable home, so they are written by hand and
preserved verbatim by `--emit`. Same schema, same vocabulary, same rules.

<!-- BEGIN HAND-MAINTAINED - sessions edit this block; --emit preserves it -->
| ID | Item | State | Session | Proof / Blocker | Next action |
|----|-------|-------|---------|-----------------|-------------|
| W-01 | Project setup: `reference_code/` vs `working_code/` separation | DONE | S1 | split in place; README "What is reference vs. working" states the rule | - |
| W-02 | GIF engine build + upstream identity verification | DONE | S8 | `verify_audit.sh` A10: `release/0.1.0/gifsicle --version` prints `LCDF Gifsicle 1.96` | - |
| W-03 | Qt-independent command/settings control layer | DONE | S4 | `src/core/*.h` compile with plain g++ (no Qt); unit suite 211 checks | - |
| W-04 | CLI driver + unit tests + integration smoke | DONE | S8 | `./build.sh` 211 checks, 0 failures; `scripts/smoke_cli.sh` 7/7 | - |
| W-05 | Qt6 GUI MVP (Batch default, mode combo, async run, queue, DnD) | DONE | S8 | CI run `34471563229` green on linux + windows; NOT compiled in the S8/S9 sandboxes (no Qt6) | - |
| W-06 | Portable + system-dependent packaging scripts | DONE | S8 | `scripts/test_package.sh` 9/9 negative cases; `verify_audit.sh` D1/D2/D5 | - |
| W-07 | Linux GitHub Actions path with Qt6 + artifacts | DONE | S8 | `.github/workflows/build.yml`; main runs #23/#24 green both jobs | - |
| W-08 | Root LICENSE / COPYING.gifsicle / .gitignore / .gitattributes | DONE | S8 | CI "Assert package manifest" step requires LICENSE + COPYING.gifsicle in the package | - |
| W-09 | P0 silent-failure fixes (2026-09-07) | DONE | S4 | `COMPILED_AUDIT.md` §6.A re-run green | - |
| W-10 | P1 honesty work (2026-09-07) | DONE | S4 | `COMPILED_AUDIT.md` §6.E 8/8 | - |
| W-11 | Smoke suite + engine test scripts | DONE | S8 | `scripts/test_engine.sh` 5/5; `scripts/smoke_cli.sh` 7/7 | - |
| W-12 | Apply `docs/ci/build.yml.proposed` to the live workflow | DONE | S4 | maintainer `821a310` + S4 hardening; copies kept byte-identical (gate E9) | - |
| W-13 | Merge the compiled audit into one register | DONE | S3 | `COMPILED_AUDIT.md` §5 holds all 52 findings | - |
| W-14 | One-command verification of `COMPILED_AUDIT.md` §6 | DONE | S9 | `scripts/verify_audit.sh` 25 PASS / 0 FAIL / 5 SKIP, exit 0 | - |
| W-15 | Windows engine + CLI proven under Wine | DONE | S4 | `gifsicle.exe` reports `1.96 (Windows)`; CLI E2E with `C:\` paths, spaces, honest exit 1 | - |
| W-16 | Valid push token | DONE | S4 | PR #5 merged into `main` as `0ad1ff5` | - |
| W-17 | Windows CI job green with downloadable artifact | DONE | S8 | PR #11 run `34471563229`: windows pass 2m56s, linux pass 1m14s | - |
| W-18 | Clean-machine portable smoke (gates C4/D3/D4) | OPEN | - | needs a clean Windows VM with no Qt/MinGW/dev tools; blocked by U-09 because the banked artifact predates S7 | re-cut artifacts (U-09), then run `docs/ci/CLEAN_WINDOWS_SMOKE.md` against the published zip |
| W-19 | One-time real-desktop GUI probes B5/B6/B14 | OPEN | - | needs a physical desktop: the offscreen harness cannot kill the engine mid-run, physically drop a file, or show the engine-missing dialog | run the three probes on a real desktop and record the evidence |
| W-20 | Input / Actions / Output tab flow (XNConvert feel) | DONE | S4b | harness T1; CI green on both jobs | - |
| W-21 | Debounced async before/after preview | DONE | S7 | harness T8/T14 (T8 rewritten in S8 — CI-compiled only, no Qt6 here) | - |
| W-22 | File size/count display + output-folder actions | DONE | S4b | harness T11 | - |
| W-23 | Free-form `{name}` naming templates | DONE | S7 | harness T16; `src/core/OutputName.h` unit tests 29/30 (Windows rule set tested on Linux) | - |
| W-24 | Expose the remaining `GifsicleSettings` controls | DONE | S4b | harness T11 asserts each control maps to its exact engine flag | - |
| W-25 | Queue reorder (Move Up / Move Down) | DONE | S7 | harness T15, incl. a merge E2E after reordering | - |
| W-26 | Two-way CLI pane, or keep one-way forever | OPEN | - | owner decision pending; the UI label already states one-way explicitly | get the owner decision and record it in this row |
| W-27 | Persist GUI settings between sessions | DONE | S7 | harness T14; run-scoped `GS_SETTINGS_PATH`; unit test 20 pins unknown-key tolerance | - |
| W-28 | Document the release procedure | DONE | S7 | `docs/release/RELEASE_PROCEDURE.md`; S9 added the doc gate to its pre-flight and evidence steps | - |
| W-29 | Bump `VERSION.md` to 1.0.0 | OPEN | - | gated on the UI/UX gates + an owner decision; 0.2.0 is allowed first per the minor-bump rule | decide 0.2.0 vs 1.0.0, then follow `docs/release/RELEASE_PROCEDURE.md` §2 |
| W-30 | Add the documentation status gate to the CI linux job | OPEN | S9 | blocked: the CI token has no `workflows` scope — push rejected with "refusing to allow a GitHub App to create or update workflow". The step lives in `docs/ci/build.yml.proposed` | a maintainer applies `docs/ci/PENDING_WORKFLOW_CHANGE.md` and deletes that marker in the same commit |
| D-01 | Common RGBA animation frame model (timing, disposal, blend, alpha, canvas, loop) | OPEN | - | deferred by policy until GIF 1.0.0 ships (`WORKLIST.md` deferred bucket) | pick up after 1.0.0 |
| D-02 | Animated WebP (libwebp AnimDecoder/AnimEncoder) | OPEN | - | deferred; hard constraint "no WebP/APNG before the GIF UI is stable" | pick up after 1.0.0 |
| D-03 | APNG (libpng/zlib with APNG support) | OPEN | - | deferred; same hard constraint as D-02 | pick up after 1.0.0 |
| D-04 | GIF/APNG/WebP convert, explode, merge, reorder, loop controls | OPEN | - | depends on D-01/D-02/D-03 landing first | pick up after the frame model exists |
| D-05 | Frame editor, text/watermark overlays, presets, richer previews | OPEN | - | deferred until GIF 1.0.0 ships | pick up after 1.0.0 |
| D-06 | Optional: logging framework, i18n, dark mode, system tray | OPEN | - | optional polish with no owner commitment yet | propose to the owner after 1.0.0 |
| D-07 | Web client-side `gifsicle.wasm` + web UI (Option 4) | OPEN | - | not the product path under the offline-only decision (S6); see `docs/web/WEB_FEASIBILITY.md` | only on an explicit owner decision |
| D-08 | Language migration spike: Rust + Tauri | OPEN | - | trigger-based only; comparison matrix in `docs/planning/OFFLINE_BUILD_REVIEW.md` §4 | watch for a documented trigger, then spike |
| R-01 | No cmake and no Qt6 in this sandbox, so `src/qtui/` and the GUI harness are CI-compiled only | OPEN | S9 | `command -v cmake` and `command -v qmake6` are both empty; `verify_audit.sh` gates C6 and B SKIP for that reason | run the harness in CI, or install `qt6-base-dev` + cmake and re-measure the runtime count |
| R-02 | The clone is shallow (one commit), so history-based checks have almost nothing to read | OPEN | S9 | `git log --oneline \| wc -l` returns 1; gates G10/G11 can only see `190d030` | `git fetch --unshallow` where history matters, and re-run `check_docs.sh` |
| R-03 | The CI token has no `workflows` scope, so `.github/workflows/` cannot be pushed | OPEN | S9 | push rejected: "refusing to allow a GitHub App to create or update workflow ... without `workflows` permission"; drift is tolerated only via `docs/ci/PENDING_WORKFLOW_CHANGE.md` | a maintainer applies the pending change with a `workflows`-scoped token |
| R-04 | Git does not copy `.githooks/` on clone, so the pre-push doc gate is inert in a fresh clone | PARTIAL | S9 | bootstrap is wired — `build.sh` calls `scripts/bootstrap_hooks.sh` and `git config core.hooksPath` is `.githooks` here. **Still missing:** a fresh clone is unprotected until it runs `build.sh` once, and nothing forces that | the one manual command is documented in `SESSION_HANDOFF.md` + `WORKLIST.md`; gate G15 fails a clone that never bootstrapped |
| N-01 | The pending-workflow marker outlived the change it described, so every doc quoted a gate count that was already wrong | DONE | S9 | marker still present while the two workflow copies were byte-identical, so E9 PASSED and the real run measured 24 passed / 0 failed / 4 skipped against 23/0/5 quoted in 8 doc locations; marker rewritten, numbers corrected, new gate G6 measures the real count | - |
| N-02 | `web/README.md` documented the pre-U-06 bind address | DONE | S9 | README said "binds 0.0.0.0" while `web/server.mjs` defaults to `127.0.0.1`; corrected, plus a `GS_WEB_HOST` note | - |
| N-03 | `docs/screenshots/*.png` may no longer match the UI, are linked from nowhere, and cannot be regenerated here | UNTRIAGED | S9 | 3 PNGs + a README claiming an S7 refresh; S8 changed `src/qtui/` afterwards; no Qt6 in this sandbox to re-shoot them | scope it: re-shoot on a Qt machine and link from `README.md`, or label them historical |
<!-- END HAND-MAINTAINED -->

## How to add a row

* **New audit finding** - add it to `COMPILED_AUDIT.md` §5 (that is the detail),
  then `check_docs.sh --emit` picks it up here automatically. IDs are
  discovered, never hardcoded.
* **New worklist / deferred / risk item** - add a row to the hand-maintained
  block above with the next free ID in its namespace, then run `--emit` to
  refresh the header counts.
* **Anything you just discovered** - `UNTRIAGED`, this session, today, plus a
  pending line in `WORKLIST.md`. Scope it or close it before the next session.
