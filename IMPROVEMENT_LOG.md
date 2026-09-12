# Improvement Log

Chronological log of decisions and changes. **Newest at the top.**

---

## S11 — Four findings closed with executed proof, Wine included; upstream provenance recorded  (2026-09-12)

Same branch family (`main` → PR), same version (0.1.0). The S11 sandbox had
working apt (uid 0): g++ 12.2, cmake 3.25.1, Qt 6.4.2, ninja, node v20 — the
S10-equivalent stack — **plus mingw-w64 12-win32 and Wine 8.0**, which no
sandbox had since S4. That flipped **U-07** ("leave OPEN unless mingw AND
wine") from blocked to executable here, and gawk was installed alongside mawk
so every awk change was verified under BOTH (the PR #13 CI scar). Clone was
full-depth; origin/main tip confirmed `2176573` (the PR #13 merge) before
starting; the two stale `414f5fc` mentions (G10) were re-synced first.

**Changed:**

* **U-15 (P2-2).** CMake no longer writes into `src/`: the second
  `configure_file` is gone, the template moved to
  `build_support/version.h.in`, and every include path now lists the generated
  dir FIRST (`core/version.h` includes converted from relative to path form in
  `cli/main.cpp`, `EngineLocator.h`, the unit tests). `build.sh` remains the
  only writer of the committed `src/core/version.h` fallback (A5 stays green).
  Executed before/after repro: with `src/` read-only under uid 65534 the OLD
  CMakeLists dies `Could not open file for write in copy operation
  .../src/core/version.h.tmp`; the NEW one configures AND builds. Deleting the
  committed fallback outright, the new tree still configures+builds+passes its
  unit tests from the generated header alone; the old one silently recreates
  the deleted file. New regression gate **C9** in `verify_audit.sh` (throwaway
  copy, fallback deleted, `BUILD_GUI=OFF`) and new §7 lines A16/C9. The G8
  `build_support` allowance in `check_docs.sh` was deleted as its own comment
  instructed — the directory now exists for real.
* **U-17 (P1-19).** Explode runs verify their frames. New
  `src/core/ExplodeVerify.h` (Qt-free, shared): snapshot `<prefix>.*`
  candidates BEFORE the run; after exit 0 require ≥1 NEW-or-CHANGED file with
  GIF87a/GIF89a magic; failures name the exact prefix + directory and list
  suspicious (empty/non-GIF) newcomers; stale leftovers can never fake a
  success. Prefix rule covers `-e` (`.NNN`), `-E` (by name) and the no-`-o`
  fallback (input basename in the CWD) — read off upstream `gifsicle.c:778` /
  `support.c:explode_filename`, never editing `reference_code/`. Wired into
  the CLI (`rc=1` + stderr, success prints the verified count) and the GUI
  (`No frames produced…` status + dialog; success reports `Explode complete —
  N frame(s)`). Tests: unit block 33, smoke cases 9–11 (real frames counted;
  lying `#!/bin/sh exit 0` engine refused; empty-output CWD-prefix rule),
  harness T7 extended with the `fake_engine_exit0` CMake fixture (cross-platform
  lying engine; refuses the false success, dialog names the prefix), and the
  whole matrix re-run under Wine (real engine: `explode wrote 12 frame(s)`;
  lying engine: rc=1).
* **U-07 (P1-4).** Windows execution is no longer ANSI-only:
  `ProcessRunner.h` builds the quoted UTF-8 line and hands `CreateProcessW` a
  strict (`MB_ERR_INVALID_CHARS`) UTF-16 conversion — invalid UTF-8 is refused
  with an honest error, never mangled. New `src/core/WinUnicode.h`: the MSVCRT
  command-line splitter (pure logic — unit-tested on Linux, block 34, exact
  inverse of `win_quote_arg` incl. the MSDN backslash/quote vectors),
  `GetCommandLineW` argv re-fetch (the MinGW CRT's argv is ACP-encoded),
  `GetEnvironmentVariableW` reads, and `u8path_compat`/`path_u8string`.
  **N-04 (new finding, closed in-session):** the Wine probe exposed that this
  MinGW libstdc++'s narrow `fs::path` conversions decode BYTEWISE but encode
  UTF-8 — an asymmetric mangling of every non-ASCII path at every core
  boundary (found via a `path("ré…").string()` round-trip probe). Every
  string↔path boundary in `EngineLocator.h`, `SettingsIO.h` (incl. a `_wopen`
  fsync path), `OutputPlan.h`, `ExplodeVerify.h`, `cli/main.cpp` and
  `MainWindow.cpp` now routes through the helpers. Executed proof (mingw
  cross-build, zero warnings, `-static`; Wine 8): the PRE-fix binary built
  from HEAD fails an `é`-path conf rc=1 with `rÃ©sumÃ©…: No such file or
  directory`; the fixed binary runs the same conf **rc=0** and writes
  `animé_opt.gif`; a non-ASCII conf PATH via argv works (re-fetch); a
  non-ASCII `GS_ENGINE` in a `résumé/` dir works (wide env); the CJK conf
  reaches the child's UTF-16 command line **byte-exact** (probe:
  `...\résumé\動画.gif -o ...\動画_opt.gif`); unit exe green under Wine
  (**289 checks** — the 4 POSIX-signal checks compile out).
* **U-41 (P2-11, scope line added FIRST).** The web demo grew all four
  desktop modes: `POST /run` (JSON `{settings, files[]}`) on the server with
  desktop semantics — batch = a per-file Auto run each with planned
  `<stem>_opt.gif` targets and collision/target-equals-source REFUSAL before
  any run (U-01 parity), merge = one `-m` run over the queue, explode =
  `-e`/`-E` against `<stem>_frame` with the P1-19 frame verification (rc=0 +
  zero new GIF frames → 422 naming the prefix), every output existence+size
  checked (U-24 parity), the U-30 validation layer shared. UI: mode selector,
  by-name explode option, multi-file queue with per-file removal and dedupe,
  results list with per-output downloads, U-46 generation counter and U-52
  URL-revoke discipline kept. Suites extended: command **15→17** (the `-b` and
  numeric `-e` builder shapes), validate **19→21** (info+explode, explode
  resize geometry), transport **18→30** (12 live `/run` cases against the real
  engine). `web/README.md` rewritten for both endpoints.
* **U-10 (P2-3, provenance half).** Fresh FULL clone of `kohler/gifsicle`;
  `diff -rq` against both vendored trees; per-file sha256 + tree list-digests
  recorded in `reference_code/REFERENCE_MANIFEST.md` with the reproduce
  recipe. Verdict: `gifsicle/` is byte-identical to upstream master
  `07f5c4c3` in every shared file — the "functional patch"
  (`FRAME_SELECTION_MODE_MASK 0x1F`) and the "extra test"
  (`012-framechange.testie`) are upstream commits `9efcc14`/`ed5b018` (5
  commits past the `v1.96` tag `a08e0f66`); `gifsicle-nested-1.96/` is
  pristine `v1.96`; the ONLY local addition is the handwritten `config.h`
  (digest pinned). The manifest's open question is answered and its identity
  claims are now evidence-backed. `reference_code/` trees untouched (git
  status proves it); the clone lives in the gitignored `gifsicle-upstream/`
  the manifest already described.
* **U-12 scoped, deliberately NOT implemented (P1-24).** All five waits named
  with re-measured line numbers (842/904 `waitForStarted(5000)`, 919
  `waitForFinished(3000)`, 171 `(2000)`, 1086 `(1000)`), the signal/timer fix
  sketched, and the testability analysis recorded: a slow process start — the
  actual harm — cannot be produced offscreen (`waitForStarted` returns when
  the OS exec succeeds; a sleeping fake engine proves nothing), and rewiring
  cancel semantics that T2/T9/T10 pin, with no executable way to show the
  freeze is gone, is exactly the risky refactor the repo rules say to avoid.
  An honest scoped OPEN beats a green-but-unproven rewrite. A-12's stale §3
  line numbers refreshed in the same pass.
* **Tests/fixtures added:** unit blocks 33 (ExplodeVerify) + 34 (splitter) →
  runtime **293 checks** (source occurrences: unit 258, harness 245 `CHECK(`
  sites); `tests/fake_engine_exit0.cpp` + CMake target (static under MINGW);
  harness T7 extension → **317 runtime checks**; smoke **9→12**; verify_audit
  C9 gate + gate B now builds the fixture target.
* **Docs/status:** §5 rows for U-07/U-10/U-12/U-15/U-17/U-41 (+Verif marks
  SRC→EXEC), §3/§4 detail statuses for A-06/A-09/A-12/A-15/A-17/B-08/B-09,
  §6 rows P1-24 + P2-11, §7 A12 12/12 + A16/C9, §8 risk rows (P1-24 pointer;
  engine-argv-ACP residual), STATUS hand rows + N-04, WORKLIST S11 section +
  next actions (6 OPEN findings → 2), README/RELEASE_PROCEDURE/PROJECT_VISION/
  docs/ci/docs/web count resync, `web/README.md` API rewrite.

**Partial:**

* **U-10** — provenance half closed with executed proof; the row stays
  PARTIAL naming what is missing: CI hash-pinning (proposal-only, needs the
  `workflows` scope) and moving the product-owned `config.h` out of the
  read-only tree into `build_support/` (A-09's other half).

**Left:**

* **U-09** (release re-cut — needs tag/release infra), **U-12** (scoped
  P1-24, OPEN), **U-14**/**U-18** PARTIAL (CI enforcement / suite expansion),
  W-18/W-19 (physical machines), W-26/W-29/W-30 (owner decisions /
  `workflows` scope), D-01…D-08 (post-1.0.0 policy) — all untouched by rule.
* The Windows CI job is the first NATIVE compilation of the S11 Windows code
  (CreateProcessW/_wopen/splitter under a real ACP); everything Windows here
  ran under Wine 8 with ACP 1252. Re-check the PR CI, don't trust this row.
* CJK **through the engine** still needs a UTF-8 ACP system: Wine 8 ignores
  the registry/locale ACP overrides (probed: `iDefaultANSICP`, `Nls\CodePage`,
  `LANG=ja_JP.UTF-8` all leave GetACP()=1252), and upstream gifsicle has no
  `wmain` (reference_code read-only). Gifscythe's own chain is lossless
  (proven byte-exact to the child's UTF-16 line); the residual is documented
  in `WinUnicode.h`, §8 and the U-07 row.
* PR open, not merged — merge is the owner's call (rule 3 re-applies).

**Verified (run in this sandbox):**

* `./build.sh` → **293 checks, 0 failures**; `test_engine.sh` 5/5;
  `smoke_cli.sh` **12/12**; `test_package.sh` 9/9.
* GUI offscreen harness (compiled AND run locally, Qt 6.4.2): **317 checks,
  0 failures** (T1–T20, T7 extended) — the new last-measured figure (306 @ S10).
* Web: `command.test.mjs` **17**, `validate.test.mjs` **21**,
  `transport.test.mjs` **30** (live server + real engine).
* `verify_audit.sh` → **28 PASS / 0 FAIL / 3 SKIP, exit 0** (skips: E9
  declared-pending workflow, CI-gated, clean-Windows); `check_docs.sh` →
  **21 passed, 0 failed, 1 skipped** (the skip is G7, same declared drift).
* Wine matrix (all executed): old CLI rc=1 + mojibake repro; new CLI é-conf
  rc=0 + output written; é-conf-path argv rc=0; é-`GS_ENGINE` rc=0; CJK child
  cmdline byte-exact (probe); unit exe green under Wine at **289 runtime checks (0 failures)** — the 4 POSIX-signal checks compile out on Windows; explode
  real-engine `12 frame(s)` rc=0; lying engine rc=1 naming the prefix. Engine
  exe rebuilt via `build_engine.sh --windows`: `LCDF Gifsicle 1.96 (Windows)`.
* U-15 before/after repro executed both halves (read-only `src/` uid 65534;
  deleted-fallback copy build). U-10: clone + `diff -rq` + digests executed;
  `git status` proves the vendored trees were never modified.
* awk parity: `check_docs.sh`/`verify_audit.sh` behavior verified under mawk
  AND gawk (gawk apt-installed this session — the PR #13 CI-awk scar).

**Not verifiable here:**

* **Native Windows execution.** Every Windows proof above ran under Wine 8
  (ACP 1252), not on Windows; CI's windows job is the first native compile +
  run of the S11 code. Wine ≠ Windows: the `Z:` drive mapping could not even
  see `"$ARENA_WORKSPACE"` in this sandbox (staged under `/tmp` instead), and Wine's
  ACP is immovable (registry + locale probed), so the UTF-8-ACP scenario
  (Windows 10 1903+ option) that would carry CJK **into upstream gifsicle's**
  file APIs stays review-verified only.
* **The U-41 browser UI itself** (index.html/app.js DOM behavior) has no
  browser or jsdom here: the server endpoint, builder and validation parity
  are executed-proof via the three node suites, but the UI wiring
  (queue rendering, object-URL lifecycle, mode switching) is review-verified
  and syntax-checked only.
* **U-12's benefit** is untestable offscreen by construction — that analysis
  is the reason it stays a scoped OPEN (P1-24).
* Clean-Windows smoke (C4/D3/D4), desktop probes B5/B6/B14, and the U-09
  release re-cut still need machines/credentials this sandbox does not have.
* CI hash-pinning for `reference_code/` (U-10's other half) cannot be pushed
  without a `workflows`-scoped token — proposal text only.

**Docs touched:**

* `COMPILED_AUDIT.md` (header base commit + S11 verification session, §3 A-06/
  A-09/A-12/A-15/A-17, §4 B-08/B-09, §5 six rows, §6 P1-24/P2-11, §7 A12/A16/
  C9, §8 two risk rows), `STATUS.md` (rows via --emit + hand rows W-03/W-04/
  W-11/W-14/R-01/R-02/N-04), `WORKLIST.md` (S11 section, ticks, gates count,
  next actions, web suite counts), `SESSION_HANDOFF.md` (rewritten for S12),
  `README.md` (S11 paragraph + version-sync wording),
  `working_code/gifscythe/README.md` (counts, layout, harness + fixture
  notes), `docs/release/RELEASE_PROCEDURE.md` (counts + version wording),
  `PROJECT_VISION.md` (S11 snapshot + gate count), `docs/ci/README.md`
  (harness figure), `docs/web/WEB_FEASIBILITY.md` (suite counts),
  `web/README.md` (both endpoints, modes, counts),
  `reference_code/REFERENCE_MANIFEST.md` (provenance verdict + digests — the
  only file touched under `reference_code/`, by explicit instruction).

---

## S10 — Nine findings + N-03 closed with executed proof; the harness ran locally again  (2026-09-11)

Same branch family (`main`), same version (0.1.0). The S10 sandbox turned out to
have a working apt: g++ 12.2, then cmake 3.25.1 + Qt 6.4.2 + ninja, plus node
and DejaVu fonts. For the first time since S7 every layer — core, CLI, GUI,
harness, web — was compiled AND run in one sandbox.

**Changed:**

* **U-45 (the last P0-1 gap).** `browseOutputButton`/`browseBatchDirButton` are
  now members locked by `setBusy()`, and `chooseOutput()`/`chooseBatchDir()`
  guard `busy_`. A picker can call `setText()` on a DISABLED QLineEdit, which is
  exactly how the batch destination used to move mid-run; T18 clicks the locked
  button and force-writes the field mid-run and proves the outputs still land
  where the plan said. P0-1 (plan + refuse + lock) is now fully closed.
* **U-35.** `setBusy(false)` re-enables Run only through `ensureEngine()` (the
  `&&` short-circuits, so entering busy never re-probes or stomps the status).
* **U-16.** `save_settings_file` is now tmp + fsync + rename (fail-closed, stray
  cleaned); the GUI save is `QSaveFile`. Unit test 32 (success/overwrite/
  directory-target/empty-path) + T19 (exactly one file left, no strays).
* **U-36.** `guiStateKey` — the third parser for the conf format — deleted.
  `set_field` reports recognised keys; `load_settings` collects the rest into a
  map the GUI reads `batch_dir`/`name_template` from. Unit test 31 + T14/T19
  round-trips.
* **U-37.** One-time status-bar note when `sessionFilePath()` is empty.
* **U-34/U-47 (P1-10).** `invalidatePreview()` (seq++) now runs on every
  schedule/clear/cancel/busy-entry, so stale completions die at the guard
  instead of repainting the panes; stale and failed runs delete their own file
  and every success sweeps `preview_*.gif` except the displayed one. T20 drives
  a 360-frame preview in flight through clear-queue and Explode-switch and
  asserts zero leaked files and no ghost After image.
* **U-40.** `--strict`: any parse or validation warning refuses with exit 3
  before anything prints or runs; `--help` documents the policy and the exit
  code table. Smoke suite 7 → 9 cases.
* **U-42.** Web demo: one shared "Scale %" input replaced by Scale X % / Scale
  Y % feeding `scale_x`/`scale_y` like the desktop; asymmetric parity fixture
  (`0.5x2`) + a live transport case (1×1 GIF → 1×2, `--scale 0.5x2` on the wire).
* **N-03.** Screenshots re-shot offscreen (Qt 6.4.2) from the CURRENT MainWindow
  with the documented scenario (logo+logo1 queued, lossy=30, colors=128, preview
  finished) and linked from the root README; `docs/screenshots/README.md`
  rewritten. The capture driver is a throwaway dev tool kept outside the repo
  (`~/devtools/capture` in this sandbox) on purpose.
* **Docs/status:** register rows for the nine findings + N-03 + R-01/R-02;
  WORKLIST S10 section; gate numbers re-synced to this sandbox's measurements
  (G6/G9/G10/G11 were red on a fresh `main` clone before this session: docs
  quoted the S9 sandbox's 25/0/5 and base `190d030`, while origin/main is
  `414f5fc`).

**Partial:**

* Nothing new. U-10/U-14/U-18 stay PARTIAL from S8, untouched this session.

**Left:**

* **Not pushed** is no longer true: branch `arena/s10-gifscythe` pushed and
  **PR #13** opened against `main` after the push-time gate run; CI on the PR
  is the first compilation of the S10 code (incl. the Windows `_commit` half
  of the atomic save). Merge remains the owner's call (rule 3 re-applies).
* **PR #13's first CI runs FAILED, and both failures were real findings about
  sandbox asymmetry, fixed in the follow-up commit:**
  - windows harness T20: on Windows a live `QMovie` delete-locks its GIF file,
    and `PreviewPanel::stopMovie` used `deleteLater()`, so the same-tick sweep
    (and the teardown sweep, movie still alive) could not remove preview
    files. `stopMovie` now deletes synchronously and `releaseMovies()` drops
    both handles before the destructor's temp-dir sweep.
  - linux docs gate: CI checks out with fetch-depth 1, so the clone has no
    `origin/main`/`main` ref and gate **G10** failed every base-commit claim as
    "unknown to this clone". G10 now SKIPs when the clone has no main ref at
    all (full clones and the pre-push hook still enforce it). Verified with a
    depth-1 clone sim: 19 passed, 0 failed, 4 skipped with `--no-gate-run`.
  - linux docs gate, second layer: `fix_order_map`'s id-split class
    `[^U-0-9]` reads `U-0` as a range — gawk (the CI awk) rejects it with
    "Invalid range end" while mawk (every sandbox so far) accepts it, so the
    FIXMAP died only in CI and the emitter regenerated every scoped row as
    "NOT scoped" (G0 drift). Class fixed to `[^0-9U-]` (dash last); the whole
    gate + verify_audit verified identical under mawk AND gawk (21/0/1 and
    20/0/2 with --no-gate-run).
  - Also learned: the Actions **job logs ARE reachable** from this sandbox via
    the jobs API redirect (S9's unreachable-host note applied to a different
    endpoint) — that is how all three failures were diagnosed.
  - **Verdict:** run `34571933676` on `9435d71` — **linux success + windows
    success**. PR #13 fully green at that commit; re-check the tip before
    merging (rule 3 re-applies: check_docs green before merge).
* U-07/U-09/U-12/U-15/U-17/U-41 remain OPEN (Windows-only, release infra, or
  unscoped); W-18/W-19/W-26/W-29/W-30 and D-01…D-08 unchanged.
* The U-37 empty-path branch and the Windows `_commit` half of the atomic save
  are review-verified only (see below).

**Verified (run in this sandbox):**

* `./build.sh` → **238 checks, 0 failures**; `test_engine.sh` 5/5;
  `smoke_cli.sh` **9/9**; `test_package.sh` 9/9.
* GUI offscreen harness (compiled locally, first time since S7): **306 checks,
  0 failures** (T1–T20) — the new last-measured figure (was 243 @ S7).
* Web: `command.test.mjs` **15/15**, `validate.test.mjs` 19/19,
  `transport.test.mjs` **18/18** (live server + real engine).
* `verify_audit.sh` → **27 PASS / 0 FAIL / 3 SKIP, exit 0** (skips: E9
  declared-pending workflow, CI-gated, clean-Windows); `check_docs.sh` →
  **21 passed, 0 failed, 1 skipped** (the skip is G7, same declared drift).
* Screenshots viewed pixel-by-pixel after capture (queue rows, reorder
  buttons, savings readout, template row, per-file command pane all present).

**Not verifiable here:**

* The U-37 empty-`sessionFilePath()` branch cannot be forced on Linux: Qt falls
  back to `getpwuid()` for the home dir, so a config location always resolves
  (T19 pins that reality instead of pretending). The status note is
  review-verified for platforms where the path really is empty.
* The `_WIN32` half of the atomic save (`_commit`/`_fileno`) never executed here
  (no Windows, no mingw installed this session); CI's windows job is its first
  compilation.
* Clean-Windows smoke (C4/D3/D4), desktop probes B5/B6/B14, and the U-09
  release re-cut still need machines/credentials this sandbox does not have.
* GitHub Actions for the S10 changes — read-only token, no push possible.

**Docs touched:**

* `COMPILED_AUDIT.md` (§5 rows for the nine findings, §6 P2-5, §7 A12 count,
  header base commit), `STATUS.md` (rows + regenerated), `WORKLIST.md` (S10
  section, ticks, next actions), `SESSION_HANDOFF.md` (S10 rewrite),
  `README.md` (S10 paragraph + screenshots section), `PROJECT_VISION.md`,
  `docs/release/RELEASE_PROCEDURE.md`, `docs/ci/README.md`,
  `working_code/gifscythe/README.md`, `docs/screenshots/README.md` + the three
  PNGs, `web/README.md` untouched (its settings list was already accurate).

---

## S9 — Status-tracking system: one register, one vocabulary, one gate  (2026-09-10)

Same branch family, same version (0.1.0). No product code under `src/` changed
this session; everything here is process, gates and docs.

**Changed:**

* **`STATUS.md` (new, repo root)** — the single status register. One row per
  tracked item in exactly one of four states: **DONE / PARTIAL / OPEN /
  UNTRIAGED**. Schema `| ID | Item | State | Session | Proof / Blocker | Next
  action |`, IDs namespaced (`U-` audit, `W-` worklist, `D-` deferred, `R-`
  risk, `N-` new finding). The `U-nn` half is **generated from
  `COMPILED_AUDIT.md` §5**; the `W/D/R/N` half is a marked hand-maintained block
  that `--emit` preserves verbatim. `COMPILED_AUDIT.md` keeps the per-finding
  evidence and now says so in both directions.
* **`working_code/gifscythe/scripts/check_docs.sh` (new)** — emits the register
  and enforces it. `--emit` regenerates; plain mode regenerates to a temp file
  and **diffs** against the committed one, so a hand-fudged roll-up cannot pass.
  16 checks: vocabulary, no cross-doc contradictions, PARTIAL-must-explain,
  header-matches-table, register↔audit agreement (both directions), real gate
  numbers, workflow copies, referenced files exist, `CHECK(` count kinds,
  branch/SHA freshness, log currency, nothing-sits-untriaged, rules-written-down,
  hook exists, hook is live. Every expected value is derived from the repo.
* **`verify_audit.sh` gates F1/F2 (new)** — the doc gate is inside the
  one-command suite, so it cannot be skipped by forgetting a second script. F2
  is deliberately independent of `check_docs.sh` so a broken checker cannot hide
  a broken register.
* **`.githooks/pre-push` (new)** — blocks a push with a red doc gate.
  **`scripts/bootstrap_hooks.sh` (new)**, called from `build.sh`, because git
  does not copy `.githooks/` on clone. Gate **G15** fails a clone that never
  bootstrapped, so "the hook exists" is never mistaken for "the hook is live".
* **`docs/ci/build.yml.proposed`** — a `Documentation status gate` step in the
  linux job (`check_docs.sh --no-gate-run`).
* **Docs:** `SESSION_HANDOFF.md` rewritten as the S9 handoff with the status
  rules under "Important constraints"; `WORKLIST.md` restructured (status rules,
  a "Found this session" pending list, contradictory ticked boxes fixed);
  `docs/release/RELEASE_PROCEDURE.md` pre-flight + §7 now carry the doc gate;
  `README.md`, `working_code/gifscythe/README.md`, `PROJECT_VISION.md`,
  `docs/ci/README.md`, `web/README.md`, `COMPILED_AUDIT.md` header, and a
  dated-snapshot banner on `docs/audit/REMEDIATION_2026-09-10.md`.

**Partial:**

* **W-30 — the CI step is written but not live.** `.github/workflows/build.yml`
  could not be pushed: the token has no `workflows` scope (real rejection
  quoted below). The step exists in `docs/ci/build.yml.proposed` only, so the two
  copies differ and **E9/G7 report SKIP** under
  `docs/ci/PENDING_WORKFLOW_CHANGE.md`. **Missing:** a maintainer applying it and
  deleting that marker, then re-running both gates so the doc numbers refresh.
* **R-04 — the pre-push hook is live *here*, not everywhere.** `build.sh`
  bootstraps `core.hooksPath`, and it is set in this clone. **Missing:** a fresh
  clone is unprotected until it runs `build.sh` once, and nothing forces that.
* **U-10 / U-14 / U-18 remain PARTIAL** from S8, unchanged this session.

**Left:**

1. Scope **N-03** (the `docs/screenshots/` question) — the only `UNTRIAGED` row.
2. Apply **W-30** with a `workflows`-scoped token; then re-run `verify_audit.sh`
   and `check_docs.sh` (removing the marker turns E9 back into a PASS and
   changes the totals, and **G6 fails until every doc quotes the new number**).
3. **U-09** release re-cut → **W-18** clean-Windows smoke → **W-19** desktop
   probes → the owner decisions (**W-26** two-way CLI, **W-29** version).
4. **No PR was opened.** The branch is pushed; opening and merging it is the
   owner's call. Whoever does it must run `check_docs.sh` again first (rule 3 —
   before `gh pr create` *and* before `gh pr merge`), and must apply
   `docs/ci/PENDING_WORKFLOW_CHANGE.md` with a `workflows`-scoped token.

**A push interruption, recorded because the recovery matters.** The GitHub token
went bad part-way through the session (`gh api user` → `Bad credentials`,
`git push` → `Invalid username or token`), so the S9 commits sat locally for a
while. It recovered later in the same session and the branch was pushed as
`089f76b`; the remote ref matches local `HEAD`.

The push itself is the end-to-end proof of rule 4: **`.githooks/pre-push` ran
the whole documentation gate as part of `git push`** and only then let it
through —

```
==> Done. 21 passed, 0 failed, 1 skipped.
==> pre-push: documentation gate green.
 * [new branch]      HEAD -> arena/01a08bb3-gifscythe
```

**Honest limitation this exposed:** this very paragraph's predecessor said "this
branch was not pushed" and stayed true in the file for one commit after that
stopped being the case. `check_docs.sh` cannot catch that — **G6** validates
gate-number triples, **G8** validates paths, **G10** validates SHAs, but no gate
can tell whether a sentence of prose still describes reality. The register can
be made unfudgeable; prose cannot. Which is the argument for keeping state in
`STATUS.md` rows rather than in paragraphs.

**Mutation-tested, and it mattered.** The gate was mutation-tested in five
directions (flip a row's state; same ID DONE in one doc and OPEN in another;
PARTIAL with no explanation; a wrong gate count in `README.md`; a doc pointing
at a file that does not exist). All five now fail with a named gate. **Three of
them passed at first**, which is the whole argument for mutation-testing a
checker:

* **G8 never checked anything.** `awk '{print $2" "$1}'` emitted
  `path source`, while the reader destructured `source path` — so the existence
  test ran against each *document's own filename*, which always exists. G8 had
  been passing unconditionally.
* **G10 failed on every commit.** It compared the documented base commit against
  `HEAD`, and a session commits on top of `main`. It now compares against
  `origin/main`.
* **G6 missed wrapped claims.** `23 PASS / 0 FAIL /` + `5 SKIP` across a line
  break never matched a line-based regex, so the stalest number in `README.md`
  was invisible. It now flattens the doc and matches with a context window.

**Verified:** (every line below was actually run in this sandbox)

| Command | Result |
|---|---|
| `./build.sh` | `==> 211 checks, 0 failures` / `ALL TESTS PASSED`, exit 0 |
| `./scripts/test_engine.sh` | `5 passed, 0 failed` |
| `./scripts/smoke_cli.sh` | `7 passed, 0 failed` |
| `./scripts/test_package.sh` | `9 passed, 0 failed` |
| `node web/test/command.test.mjs` | `ALL WEB COMMAND TESTS PASSED` (14) |
| `node web/test/validate.test.mjs` | `ALL WEB VALIDATION TESTS PASSED` (19) |
| `node web/test/transport.test.mjs` | `ALL WEB TRANSPORT TESTS PASSED` (17) |
| `./scripts/check_docs.sh --emit` then `./scripts/check_docs.sh` | `==> Done. 21 passed, 0 failed, 1 skipped.`, exit 0 (the skip is G7, the declared-pending workflow change) |
| `./scripts/verify_audit.sh` | `==> Done. 25 passed, 0 failed, 5 skipped.`, exit 0 |
| baseline `verify_audit.sh` **before** any change | `==> Done. 24 passed, 0 failed, 4 skipped.` — this is the number that proved N-01 |
| `grep -o 'CHECK(' tests/test_gui_offscreen.cpp \| wc -l` | **226 occurrences** (also 226 lines) |
| `grep -o 'CHECK(' tests/test_gifsicle_command.cpp \| wc -l` | **196 occurrences** across **193 lines** — three lines hold two |
| `./build/test_gifsicle_command` | `211 checks, 0 failures` (runtime counter) |
| `git push` probe touching `.github/workflows/build.yml` | `! [remote rejected] ... refusing to allow a GitHub App to create or update workflow` |

Counts are labelled by kind on purpose: **211** and **243** are *runtime*
counter values; **196** and **226** are *source `CHECK(` occurrences*; **193**
is *lines*. The last session conflated these and had to correct itself.

**Not verifiable here:**

* **No cmake, no Qt6** (`command -v cmake` and `command -v qmake6` are both
  empty; `/usr/lib/x86_64-linux-gnu/cmake/Qt6` does not exist). So
  `tests/test_gui_offscreen.cpp` was **not built or run** — `verify_audit.sh`
  gates **C6** and **B** SKIP for exactly this. Anything touching `src/qtui/` or
  the harness is **CI-COMPILED ONLY** and is labelled that way everywhere it is
  claimed. The harness runtime count **243** is the **S7 sandbox measurement**
  and has not been re-measured; it would need `qt6-base-dev` + `cmake`.
* **No CI run** was triggered this session (nothing under `src/` changed, and the
  one workflow edit could not be pushed). So the new `Documentation status gate`
  step is **unexercised in CI**; it was run locally instead.
* **Clean-Windows smoke (C4/D3/D4)** and the **desktop probes B5/B6/B14** still
  need a real clean Windows VM and a physical desktop.
* **`docs/screenshots/*.png`** could not be regenerated or compared (no Qt6) —
  which is precisely why N-03 is `UNTRIAGED` rather than guessed at.

**Docs touched:** `STATUS.md` (new) · `SESSION_HANDOFF.md` (rewritten for S9;
status rules added to "Important constraints") · `WORKLIST.md` (rules, pending
list, contradictory ticked boxes fixed, gate numbers) · `IMPROVEMENT_LOG.md`
(this entry) · `COMPILED_AUDIT.md` (roll-up/detail relationship, real branch and
base SHA, U-27 proof note) · `README.md` + `working_code/gifscythe/README.md`
(STATUS.md pointers, gate numbers, repo layout) · `PROJECT_VISION.md` (status
register pointer, gate numbers, harness caveat inline) ·
`docs/release/RELEASE_PROCEDURE.md` (doc gate in pre-flight and §7) ·
`docs/ci/README.md` + `docs/ci/PENDING_WORKFLOW_CHANGE.md` (rewritten: the old
change is applied, this one is pending) · `web/README.md` (N-02 bind address) ·
`docs/audit/REMEDIATION_2026-09-10.md` (dated-snapshot banner).

**Why this session existed.** Status lived in five places and they drifted. A
hand-run check found `COMPILED_AUDIT.md`'s header claiming "21 findings closed"
while its own register held 31, and naming a branch that was no longer checked
out. This session found the same class of error still live: eight doc locations
quoted **23/0/5** when the gate actually measured **24/0/4**, because a pending-
change marker had outlived the change. Gate **G6** now measures the number
instead of trusting any document.

---

## 2026-09-10 (S8, batch 2) — ten more findings closed with executed proof

Same branch, same version. Evidence per finding is in
**`docs/audit/REMEDIATION_2026-09-10.md` §2b**.

### New files

* **`src/core/OutputName.h`** — `sanitize_output_name()`,
  `is_windows_reserved_device_name()`, and a `NameRules` enum
  (`Host`/`Windows`/`Posix`). `MainWindow::renderedOutputName()` now delegates
  here. Parameterising the rule set is what made **U-21** verifiable on Linux:
  the *Windows* rules are unit-tested here (30 assertions) instead of being an
  untested claim. Separators are stripped in both flavours on every platform,
  which preserves the GUI's existing POSIX behaviour.
* **`web/validate.mjs`** + **`web/test/validate.test.mjs`** — **U-30**. The web
  demo had no validation layer, so identical settings produced a clear message
  on desktop and a raw engine error in the browser. The new test runs the *real*
  `gifscythe-cli` and requires the `(field, value, reason)` triples to match the
  JS mirror exactly: 19/19.

### Fixes

| ID | What changed | Proof |
|---|---|---|
| **U-32** | `run_argv` returns `128+WTERMSIG` instead of `1` for a signalled child | test 28: SIGTERM→143, SIGKILL→137, `exit 3`→3, missing binary→127 |
| **U-51** | `encode_line_value()` at all 9 string write sites + JS mirror | reproduced a comment hijacking `mode`; test 30 guards the round-trip |
| **U-49** | dropped the double `decodeURIComponent` in `server.mjs` | `{"comments":["100%"]}`: HTTP 400 → **200 / 8679 B** |
| **U-50** | `X-Gifscythe-Command` percent-encoded, decoded in `app.js` | `{"comments":["作品"]}`: HTTP 500 → **200 / 8681 B** |
| **U-46** | `requestGen` counter in `app.js`, checked after fetch, after blob read, in `catch` and `finally` | a stale response can no longer populate a newer preview |
| **U-52** | `beforeUrl` tracked and revoked on replacement | `node --check` + review (browser-only) |
| **U-31** | `build.sh` link-probes `-lstdc++fs` and cleans up | probe ran, flag correctly empty on g++ 12, no artefact left |
| **U-44** | dated review snapshots `git mv`'d to `docs/archive/` | 3 prose references updated; no path links existed |

The strongest before/after is **U-30** on `--scale 0x1`: the engine exits **0**
and the output is **byte-identical to `--scale 1x1`** (`cmp` clean) — the user
asked for a resize and got nothing, with no message. The server used to answer
HTTP 200; it now answers 422 naming the field.

### Two things this batch broke and then fixed

1. **My own comment tripped `verify_audit.sh` E3.** The line "(no /bin/sh, no
   cmd.exe)." in `ProcessRunner.h` matched the no-shell-execution grep, and the
   exemption words were on the *previous* line. A green run became
   **22 passed, 1 failed**. Fixed by putting the exemption on the same line and
   by exempting `//` / `*` comment lines in E3 — then mutation-tested in four
   directions so the guard still catches a real `system()`, a real `/bin/sh`
   literal, and a trailing `// spawns sh -c`.
2. **`web/test/validate.test.mjs` initially reported the C++ side as silent.**
   `execFileSync` returns only **stdout**, so on a rc=0 run the piped stderr was
   thrown away. Switched to `spawnSync`. Writing the parity test also surfaced
   that `delay < 0` and `lossy < 0` never survive a conf round-trip — *both*
   writers skip them because `-1` means "unset" — while the C++ *reader* does
   accept them. Those two fixtures now feed the C++ side raw conf text, and the
   property is documented rather than hidden.

### Tests

| Check | Result |
|---|---|
| `./build.sh` | **211 checks, 0 failures** (tests 28–30 added) |
| `scripts/verify_audit.sh` | **23 passed, 0 failed, 5 skipped, exit 0** (new gates W1/W2/W3; E9 SKIPs — see below) |
| `node web/test/command.test.mjs` | **14/14 PASS** |
| `node web/test/validate.test.mjs` (new) | **19/19 PASS** |
| `node web/test/transport.test.mjs` (new) | **17/17 PASS** against a live server |
| Transport mutation testing | double decode → 4 FAIL · raw header → 6 FAIL · no `validate()` → 3 FAIL · restored → 0 |
| `scripts/test_package.sh` / `test_engine.sh` / `smoke_cli.sh` | 9/9 · 5/5 · 7/7 |
| E3 mutation testing | 4 cases, all correct |

### CI

The linux job now runs all three web suites. It previously ran **none** of them, so
the JS copies of the command builder and the validation rules had no automated
guard at all. `.github/workflows/build.yml` and `docs/ci/build.yml.proposed`
were edited together. **They are NOT identical on this branch, on purpose:**
the push was rejected with *"refusing to allow a GitHub App to create or update
workflow `.github/workflows/build.yml` without `workflows` permission"*, so the
live workflow was reverted to base and the change lives in
`docs/ci/build.yml.proposed` + **`docs/ci/PENDING_WORKFLOW_CHANGE.md`** —
exactly the situation `docs/ci/README.md` says that copy exists for. E9 was
extended with one tolerated state: declared drift (that marker file present) is
a SKIP, undeclared drift still FAILs. Mutation-tested in all four states.

### Documentation consistency sweep

Current-state docs had drifted during S8, so each was re-read against the
register instead of being left to disagree with it — root `README.md`,
`SESSION_HANDOFF.md` (a full **"What S8 did"** section added, and its
*"all run locally, linux + Qt 6.4"* header corrected, since this sandbox has
neither), `WORKLIST.md` (new S8 board), `PROJECT_VISION.md`,
`docs/release/RELEASE_PROCEDURE.md`, `docs/ci/README.md`, both product READMEs,
`web/README.md` and `docs/web/WEB_FEASIBILITY.md`. The full table is
`docs/audit/REMEDIATION_2026-09-10.md` §3b.

**A confusion this closes:** the GUI harness holds **226 `CHECK(` sites in
source** but its last **runtime** count was **243**. Not a conflict — `CHECK`
increments at runtime and several sites sit in loops. The unit suite is the
same shape: 196 source sites, **211** runtime checks. Quote the runtime number
and say where it was measured. Dated audit snapshots under `docs/archive/` and
the per-session entries below keep their historical numbers by policy.

### CI result — the Qt-only caveat is now closed

This sandbox has no cmake and no Qt6, so every `src/qtui/` edit and
`tests/test_gui_offscreen.cpp` T8/T17 were written blind and pushed for CI to
compile. **PR #11, run `34471563229`: linux pass (1m14s), windows pass
(2m56s).** The linux step runs the offscreen harness under `set -euo pipefail`
and the harness `return 1`s on any failure, so a green job is a green harness —
T8 and T17 had never been compiled before this run. Windows passing also covers
the `NameRules::Host` compile path and the `CreateProcessA` process layer.

Two honest gaps remain. The three web suites were **not** in this CI run — the
workflow change adding them is blocked on a `workflows`-scoped token — so they
are locally verified only. And the harness's exact check count was not
retrievable (the Actions log host is unreachable from here), so **243 stays the
last measured figure** and must not be quoted as current.

Still genuinely unproven anywhere: the g++ ≤ 8 branch of the `-lstdc++fs`
probe, and the runtime behaviour of the Windows name rules on a real Windows
build (only the rule set is unit-tested, on Linux).

---

## 2026-09-10 (S8, batch 1) — Audit remediation: 21 findings closed with executed proof

Working on branch `arena/01a08a10-gifscythe` (off `main` @ `a55a68d`).
Version stays **0.1.0**. Full per-finding evidence, including the before/after
command output and the mutation-test record, is in
**`docs/audit/REMEDIATION_2026-09-10.md`**; the pick rationale that preceded it is
`docs/audit/FIX_PICK_2026-09-10.md`.

### The headline fix: batch output planning (U-01)

New Qt-independent **`src/core/OutputPlan.h`**. `gs::plan_outputs(inputs, outputs)`
refuses a target that is any queued input, two inputs mapping to one target, or an
empty target, and *reports* (without refusing) targets already on disk. Paths are
compared through `weakly_canonical`, so `./a.gif`, `a.gif` and an absolute path to
the same file cannot slip past. Both drivers now plan **before the first process
starts**: the CLI refuses with rc=2, and the GUI plans the whole queue in
`runCommand()` and states the verdict in the summary label before Run is clicked.

Verified against the real engine: `input = solo.gif` / `output = solo.gif` used to
replace the source in place with rc=0; it now exits 2 and the file's md5 is
unchanged.

**Deliberately not done:** temp-sibling + rename. It would only protect a previous
output from a crashed engine, and it would put a staging path into the live command
pane, breaking the contract harness T1/T16 assert.

### Everything else closed

`-j` for threads "Auto" (U-03, mirrored in `web/command.mjs` — the parity harness
failed 4 fixtures the instant the C++ side changed) · CLI stdout purity (U-04) ·
real PATH engine search (U-05) · web binds loopback (U-06) · packager fails closed
+ license set asserted (U-02/U-08) · `scripts/test_package.sh` negative suite +
CI manifest assertion (U-14) · `parse_bool` warns (U-11) · drop filter `&&` (U-13) ·
resize/scale geometry validated, rules probed off the engine (U-22) · strict CLI
arg parser (U-23) · web 422 for "rc=0 but no output" (U-24) · web Scale default 100
and Touch option (U-25/U-29) · numeric version sort (U-26) · missing `return` (U-28) ·
`-p` needs both halves (U-33) · `verify_audit.sh` C6 SKIPs without cmake (U-38) ·
workflow-drift guard E9 (U-39) · "Batch (1 file)" (U-43) · empty comments skipped
in C++ **and** JS (U-48).

### Three register rows corrected

* **U-19 was not a data bug.** A round trip with the parent toggles *on* returns
  `crop_w 200 → 200`, `position 12,7 → 12,7`, `scale 0.5 → 0.5`, 0 load warnings.
  The original repro started from `crop=false`, where dropping the children is
  correct. Downgraded to a wording nit; unit test 26 pins the real behaviour.
* **U-20** — a GUI-saved conf does produce one *validate* warning (no `input`
  key); the "no warnings at all" phrasing is removed from the handoff and this log.
* **U-10** — `REFERENCE_MANIFEST.md` no longer calls `gifsicle/` "identical to
  upstream master"; it lists the observed deltas and marks provenance open.

### Tests

| Check | Result |
|---|---|
| `./build.sh` | **166 checks, 0 failures** (tests 21–27 added) |
| `scripts/test_package.sh` (new) | **9 passed, 0 failed** |
| `scripts/verify_audit.sh` | **21 passed, 0 failed, 4 skipped** (was 19/1/3, exit 1) |
| `scripts/test_engine.sh` / `smoke_cli.sh` | 5/5 · 7/7 |
| `node web/test/command.test.mjs` | all PASS (14 fixtures) |
| Mutation testing | 7 core guards each broken on purpose → 3–9 FAIL each; restored → 0 |
| GUI harness **T17** + T8 rewrite | written, **CI-compiled only** — this sandbox has no cmake/Qt6 |

### Constraint kept

`.github/workflows/build.yml` and `docs/ci/build.yml.proposed` were byte-identical
at this point, and the new `verify_audit.sh` **E9** check fails if they drift.
*(Superseded later in this session: the batch-2 push was rejected for lacking the
`workflows` scope, so the live workflow was reverted and the change moved to
`docs/ci/PENDING_WORKFLOW_CHANGE.md`. E9 now SKIPs for that declared state and
still FAILs on undeclared drift — see the batch-2 entry.)*

---

## 2026-09-10 (S7) — Settings persistence, queue reorder, naming templates, release doc

Working on branch `arena/s7-settings-persistence` (off `main` @ `c5efe07`).
Version stays **0.1.0** — the minor-bump/1.0.0 decision is the owner's.

### Implemented (the sandbox-codeable Phase-1 remainder)

1. **GUI settings persistence** — closes the S6 gap
   (`docs/planning/OFFLINE_BUILD_REVIEW.md` §6), audit §2.3 row 15 ("Config
   persistence", OPEN since S3), and the WORKLIST item. Design decisions:
   - **Serializer: core `SettingsIO`, not `QSettings`.** One format for CLI
     conf files and GUI sessions means the two can read each other's files;
     the flat `key = value` text stays diffable and Qt-independent.
   - **Location:** `QStandardPaths::AppConfigLocation` + `/gifscythe.conf`
     (`%APPDATA%\Gifscythe\gifscythe.conf` on Windows); **`GS_SETTINGS_PATH`**
     env override mirrors the established `GS_ENGINE` pattern (portable use,
     test isolation).
   - **`SettingsPanel::readFrom()`** restores every control `writeInto()` reads
     (not a byte-exact inverse of the serializer — crop geometry, the position
     pair and the scale factors are only written while their parent toggle is
     on; see the U-19 correction in
     `docs/audit/REMEDIATION_2026-09-10.md` §4):
     every control restored signal-blocked (no `changed()` storm → no
     preview/pane churn), dependent enabled states synced explicitly, values
     the GUI cannot represent (`optimize = -1`, disposal 4..7, unknown
     methods) leave the control at its default rather than forcing a wrong
     "off".
   - **GUI-only keys** `batch_dir` + `name_template` are appended after the
     core dump in a commented "GUI state" section. `SettingsIO` ignores
     unknown keys on load, so the CLI consumes GUI-saved files silently —
     pinned by new **unit test 20** and a live CLI E2E run.
   - **Not persisted (deliberate):** the queue (files move between sessions;
     stale rows teach users to distrust the queue) and the Save-as field (a
     per-run choice — silently restoring it could overwrite a stale path).
   - **Honesty:** first launch (no file) → defaults, no error, nothing
     written until first close; existing-but-unreadable file → status says
     so; parse warnings → surfaced in the status bar (S5 rule: never
     discard); failed save on close → warning dialog (never silent).
   - Saved in `closeEvent` (standard desktop behavior; crash-loss window
     accepted — settings are preferences, not work product).
2. **Queue reorder (S3-9 remainder)** — `Move Up` / `Move Down` on the Input
   tab. List rows and `inputs_` stay index-aligned (take/insert mirrored on
   both), selection follows the item, bounds are honest no-ops, buttons
   disabled while busy. Merge consumes the queue in order, and the live pane
   + merge E2E prove it (harness T15).
3. **Free-form naming templates (S3-25)** — Output-tab `Name template`
   field, default `{name}_opt.gif`. The default renders **exactly** the
   historical auto-name, so audit E4 ("auto `<name>_opt.gif` next to each
   input") is byte-identical for untouched installs; harness T1/T2/T4/T13
   keep pinning it. Safety/honesty rules: `{name}` = input base name
   (case-insensitive replace); **path separators stripped** (a template
   cannot escape the chosen output folder — `../../evil` → `evil.gif`);
   `.gif` appended when missing (the engine only writes GIF); empty render
   falls back to the default; **constant template (no `{name}`) + >1 queued
   files = collision → summary warns and the run is REFUSED** with an
   explanatory dialog instead of silently overwriting N-1 results.
   The template is persisted (`name_template` GUI key) and restored.
4. **Release procedure documented** — `docs/release/RELEASE_PROCEDURE.md`
   (snapshot vs version rules, pre-flight suite + expected counts, version
   bump mechanics, CI evidence, packaging contents incl. licenses, GitHub
   Release/banking conventions with sha256, post-publish clean-Windows +
   persistence spot checks, rollback = append-only).

### Hygiene fixes found while reviewing

- **`docs/ci/build.yml.proposed` had drifted** from the live workflow (line
  98 still said `build_gifsicle.sh` after the maintainer updated
  `.github/workflows/build.yml` in `c5efe07`) — the "byte-identical"
  constraint was violated by the copy, not the original. Synced; `diff` is
  now clean.
- **`scripts/build_gifsicle.sh` shim removed.** Its only purpose was the
  GitHub App's missing `workflows` permission; the maintainer's `c5efe07`
  removed that reason (S5 log: "remove it once that happens"). No live
  reference remains (the two dated review snapshots keep historical refs by
  policy).
- **Dead code:** the unused bool-string lambda in `SettingsIO::save_settings`
  (silenced with `(void)b`) is gone.
- **Stale user-facing strings:** two "<name>_opt.gif" texts now describe the
  template default (dialog + comment) — behavior unchanged.
- **Stale doc counts:** root/product READMEs said "143 checks" (S4b) while
  the harness reported 150 since S5; both now state the S7 truth (243) with
  history. The product README's S4-era status block (incl. the long-resolved
  "PAT is invalid" note) is rewritten.

### Tests

- Harness `test_gui_offscreen.cpp`: **150 → 243 checks**. New **T14**
  (persistence round-trip incl. file-content assertions, dependent-state
  restore, not-persisted assertions, corrupt-file honesty), **T15** (reorder
  sync/selection/bounds/merge-order + E2E), **T16** (template default =
  E4 name, custom template pane/summary/E2E, separator escape, collision
  refusal dialog + engine-never-started). `main()` now points
  `GS_SETTINGS_PATH` at a run-scoped scratch file (no test touches the real
  user config dir; run-order deterministic). T1 gained existence/default
  checks for the three new widgets.
- Unit suite: test 20 (unknown-key tolerance, no warnings).
- Full local rerun (linux, Qt 6.4, offscreen): build.sh green · engine 5/5 ·
  smoke 7/7 · **verify_audit 21 PASS / 0 FAIL / 2 SKIP** · harness 243/0 ·
  web parity all passed · CLI-on-GUI-conf exit 0.
- Windows side must be confirmed by CI on this branch (only Windows Qt
  verification available).

---

## 2026-09-09 (S5) — GUI honesty fixes, naming alignment, web build review + POC

Working on branch `arena/01a086c5-gifscythe`. Version stays **0.1.0**.

### GUI review fixes (3 bugs found in `src/qtui/MainWindow.*`)

1. **Cancel popped a spurious error dialog.** `cancelRun()` → `kill()` +
   `waitForFinished()` made gifsicle exit non-zero/crash, which synchronously
   fired `onProcessFinished` → the failure branch → a modal "optimization
   failed" dialog *before* the status flipped to "Cancelled.". The old B2
   harness check only asserted the final status (its DialogKiller silently
   closed the dialog), so it passed. Added a `cancelling_` flag so
   `onProcessFinished` skips the alarm path during a cancel; harness T9 now
   asserts **no dialog** appears in the cancel window.
2. **Batch live pane showed a command the app never runs.** The pane printed a
   single `gifsicle -b <all inputs> -o <first>_opt.gif` line while `runCommand()`
   actually launches **N separate Auto-mode invocations** (one per file, never
   `-b`). `refreshCommand()` now renders the real per-file commands (E1
   explicit Save-as honored for a single file; capped at 20 lines for huge
   queues). Harness T2/T13 now assert the pane contains the derived
   `<name>_opt.gif` names and no ` -b `.
3. **`gs::validate()` results were thrown away in the GUI.** `runCommand()`
   erased only the `input` warning and ignored the rest (comment claimed it
   was about batch output — no such warning exists). The one GUI-triggerable
   warning (crop 0×0) was silently dropped. It now surfaces remaining warnings
   in a dialog before running.

Regression checks added to `tests/test_gui_offscreen.cpp` (T2, T9, T13).
Core/CLI unaffected.

### Naming alignment — product is Gifscythe; engine stays gifsicle

Policy: **Gifscythe** = the product; **gifsicle** = the upstream engine only
(bundled `gifsicle[.exe]` binary, `reference_code/gifsicle/`, engine version
1.96, its flags/options — these MUST keep the name; audit A10 and the
"engine identity 1.96" rule depend on it).

Applied:
- Renamed `scripts/build_gifsicle.sh` → **`scripts/build_engine.sh`** (git mv)
  and updated every live reference (build.sh, CMakeLists.txt, verify_audit.sh,
  test_engine.sh, READMEs, WORKLIST, SESSION_HANDOFF, COMPILED_AUDIT,
  IMPROVEMENT_LOG, gifscythe.pro). A thin `scripts/build_gifsicle.sh`
  **compatibility shim** remains because the GitHub App cannot edit
  `.github/workflows/build.yml` (no `workflows` permission); it forwards to
  `build_engine.sh` and should be removed once a maintainer updates the
  workflow. The two dated review snapshots (now under `docs/archive/`) intentionally keep
  their historical line
  refs.
- User-facing strings now say the brand or "the GIF engine": CLI help, GUI
  bottom-bar label, mode combo ("Auto (engine decides)"), tooltips, error
  statuses/dialogs ("Could not start the GIF engine", "The GIF engine
  returned an error (exit N)", "Failed to start the GIF engine."), the batch
  pane annotation. Engine/source attributions (e.g. "gifsicle 1.96 source",
  "engine (gifsicle)") were kept where they name the engine precisely.

### Web build — reviewed + working POC (not built/reviewed before)

`FEASIBILITY_REVIEW.md` only mentioned web-tech UI as an alternative; nothing
was built. Delivered:
- **`docs/web/WEB_FEASIBILITY.md`** — full review of 4 options
  (Qt-for-WASM ❌ QProcess/subprocess can't exist in wasm; Tauri ⚠️ still a
  desktop app; server-side engine ✅ works today; **client-side
  `gifsicle.wasm` ✅ best end-state** for portable offline web).
- **`web/`** — zero-dependency Node server (`server.mjs`, spawn argv-array,
  never a shell) + browser UI (upload → optimize → before/after + download)
  + **`command.mjs`**, a line-by-line JS mirror of `GifsicleCommand.h` that is
  the *single* builder for the browser live-pane and the server.
- **Parity proof:** `web/test/command.test.mjs` serializes 12 settings
  fixtures to confs, runs the real C++ `gifscythe-cli`, and asserts the JS
  `toString()` is byte-identical — **13/13 PASS**. Server verified E2E: valid
  12-frame GIF out (8703→6856 B with `--lossy=40 --resize-fit 100x100 -O3`),
  honest 422 + stderr for non-GIF input.

### Verification this session

- `./build.sh`, unit suite, `test_engine.sh` 5/5, `smoke_cli.sh` 7/7, audit
  static probes (E3/E4/E8/A5): **all green** after the edits.
- **Qt GUI could NOT be compiled here:** sandbox network allows only
  pypi/npm/github; apt, Qt CDN and emscripten hosts are unreachable, and
  PySide6 wheels ship no C++ Qt headers/`moc`. GUI fixes are therefore
  logic-reviewed + harness-checked by inspection only; the offscreen harness
  must run on CI (it already runs on both OSes in `build.yml`).

### Still open (unchanged)

C4/D3/D4 clean-Windows smoke, desktop probes B5/B6/B14, optional polish
(naming templates, queue reorder), 0.2.0-vs-1.0.0 owner decision. WebP/APNG
stay blocked. Option-4 wasm build is a later release, not 1.0.0 scope.

---

## 2026-09-07 (S4c-close) — recovery pushed, PR #5 merged, CI green on main

- Token #3 (fresh fine-grained PAT) worked where tokens #1–#2 were rejected
  (expired/revoked at source). The 10 S4/S4b commits pushed to
  `verify/windows-ci-fixes` **SHA-identical** to the local originals
  (independently audited post-push: zero mangling, tree byte-identical to
  PR merge result).
- Run #20's windows hang (offscreen step, zero output ~28 min) diagnosed via
  post-cancel log pull; `160fea3` added the 8-min watchdog + stage markers,
  `timeout-minutes: 12` on both GUI steps, `qoffscreen.dll` staging beside
  the harness exe (presumed root cause), `isValidColorName` deprecation fix.
  Runs #21/#22 green; hang not recurred.
- **PR #5 merged** into `main` as true merge `0ad1ff5` (parents `821a310` +
  `160fea3`). Main green on both jobs twice: run #23 (`0ad1ff5`) and run #24
  (`d3544b1`, id 34092786153), incl. both GUI offscreen steps.
- Artifacts `gifscythe-windows` (~27.8 MB) + `gifscythe-linux` (~0.7 MB) up;
  default expiry 2026-12-06 → retention capped at 14 days in workflow and
  binaries **banked on GitHub Release `snapshot-2026-09-07`** (prerelease;
  nothing binary enters the git tree — caesium-bin precedent 95d62eb).
- Workspace archive banked: `gifscythe-workspace-2026-09-07.zip` (36.0 MB,
  sha256 `67e55363f79ddd2dc8236a5bed07d2032dec5fd360b4d2fb1eef5669a1d87b2a`),
  also attached to the Release.
- Merged/stale remote branches deleted afterwards (`verify/windows-ci-fixes`,
  `arena/01a07959-gifscythe`, `arena/01a0746d-gifscythe`,
  `arena/01a07410-gifsicle-1-96`); PR refs preserve history.
- **Still open (unchanged):** C4/D3/D4 clean-Windows smoke
  (`docs/ci/CLEAN_WINDOWS_SMOKE.md`), desktop probes B5/B6/B14, optional
  polish (naming templates, queue reorder, release-procedure doc), owner's
  0.2.0-vs-1.0.0 decision. Version stays 0.1.0.

---

## 2026-09-07 (S4b) — XNConvert-style UI retrofit: tabs, full controls, async preview

Owner approved starting the P1 GUI retrofit while the push token is dead.
Implemented + verified the same day; version stays **0.1.0** (bump is an
owner decision after Windows CI green).

**New structure** (`src/qtui/`):
- `SettingsPanel.{h,cpp}` — the Actions tab: ~30 controls covering the whole
  `GifsicleSettings` surface (mode, optimize, lossy, colors, dither,
  color-method, careful, resize kind/W×H/scale %/method, rotate, flips,
  interlace, position, crop + crop-transparency, delay, loop, disposal,
  unoptimize, threads, gamma, background/transparent with color pickers,
  metadata removals, comments, explode-by-name). Value lists are
  **engine-truth**, extracted from gifsicle 1.96 source: dither names from
  `set_dither_type()` (floyd-steinberg/atkinson/o3x3…ro64/diag45/halftone/
  sqhalftone), resize methods from `RESIZE_METHOD_TYPE` (point/mix/box/
  catrom/lanczos2/lanczos3/mitchell), disposal from `DISPOSAL_TYPE`
  (none/asis/background/previous), color methods from `COLORMAP_ALG_TYPE`
  (diversity/blend-diversity/median-cut), gamma from `GAMMA_OPT`
  (srgb/oklab/numeric). Every widget has a stable objectName.
- `PreviewPanel.{h,cpp}` — Before (QMovie of selected original) / After
  (QMovie of preview output) + size-savings readout + honest captions.
- `MainWindow.{h,cpp}` — rewritten layout: Input/Actions/Output tabs +
  preview in a splitter; bottom bar unchanged (live one-way pane, progress,
  run/cancel, status). Output tab: Save-as, batch output folder (mkpath'd
  before running), Open-folder via QDesktopServices, per-mode summary.
  Queue rows show per-file size; footer shows count+total. **Run semantics
  deliberately unchanged** (batch N→N `_opt.gif`, E1 explicit save-as,
  merge refuses empty output, explode auto-prefix, honest failures).
- Preview pipeline: 1200 ms debounce; **fresh QProcess per run + captured
  sequence number** so killed/stale runs can never be misread; killed on
  main-run start, cancel, close, and destructor; temp dir per PID, cleaned
  on destruction; previous preview file deleted after each success.

**Bug found & fixed during harness bring-up:** `previewProcess_` member
dangled after the completion lambda's `deleteLater()` → segfault in
`~MainWindow`/`killPreview` (gdb backtrace). Member is now nulled in both
completion paths.

**Harness grew 81 → 143 checks** (T1–T13): tabs exist and are named; all
control→flag mappings incl. regression guards for VP-1 (`--loopcount=0`),
VP-2 (`-O0`), VP-3 (no `--gamma` unless chosen), VP-5 (crop `1,2+30x40`
plus-form), E7 (delay label says **1/100 s**, never ms); preview pipeline
(savings appear, regenerates on change, honest Explode refusal); batch
output folder honored end-to-end; all original B-series semantics re-proven
against the rewritten MainWindow.

**verify_audit.sh** stays green: 21 PASS / 0 FAIL / 2 SKIP (E4 probe moved
to SettingsPanel.cpp). qmake **and** cmake build paths both compile the new
files (`gifscythe.pro` updated). Engine 5/5, smoke 7/7, unit ALL PASSED.

**Second PAT also rejected** (format-valid, 93 chars — GitHub says Bad
credentials/Invalid token). Push still blocked; everything accumulates on
local branch `verify/windows-ci-fixes`.

---

## 2026-09-07 (S4) — §6 verification executed + Windows CI root-caused & fixed

**Environment upgrade:** apt reachable again → gcc 12.2, cmake 3.25,
**Qt 6.4.2**, **mingw-w64 12**, **Wine 8** installed in sandbox. Everything
the S3 session marked "needs a Qt machine / real Windows" became testable.

**Push blocker:** the provided fine-grained PAT is invalid (API 401 "Bad
credentials"; push "Invalid username or token"; public-repo reads work
anonymously, which masks the failure). All S4 work is committed on local
branch `verify/windows-ci-fixes`; a new token (Contents+Workflows+PR write)
is required to push.

**Verification (COMPILED_AUDIT §6, evidence-tagged):**
- §6.A 12/12 + new A13 (Windows unit exe under Wine). ASan+UBSan clean
  (unit suite + CLI, incl. honest exit 1 on missing engine).
- §6.B: new offscreen harness `tests/test_gui_offscreen.cpp` — 81 checks,
  0 failures (batch N→N + frame counts, merge 13=12+1, explode `.NNN`,
  merge-empty-output refusal, honest failure, cancel mid-run on a
  4800-frame GIF, close-while-running kill, dedupe, multi-remove, live pane
  sync/quoting, Batch default, explicit-output E1). B5/B6-plumbing/B14-variant
  remain one-time desktop probes. Qt 6.4 note: synthetic QDropEvents are
  ignored without an active platform drag session → harness emits
  `filesDropped` (the signal the drop handler emits) instead.
- §6.C: C1 confirmed green (Actions linux, run #18). C7 simulation exposed a
  **new pit**: stale GUI binaries let `build.sh --all` claim "GUI built" with
  Qt hidden → build.sh now deletes stale GUI outputs before probing.
  C6 configure ±Qt, C8 honest default: green.
- §6.D: portable package now includes the GUI (D1/D2); **D5 fixed** —
  `reference_code/caesium-bin` untracked (62 files / 74 MB, `git rm
  --cached`), manifest documents re-fetch. History purge still optional.
- §6.E 8/8 green.
- One-command suite: `scripts/verify_audit.sh` → 21 PASS / 0 FAIL / 2 SKIP.

**Windows CI failure (run #18 step 4) root-caused & fixed:**
- gifsicle 1.96 sources do *unconditional* `#include <config.h>`; the
  `--windows` compile line lacked `-I.` → "config.h: No such file". Fixed
  per upstream `src/Makefile.mingw`: `-include src/win32cfg.h` first (owns
  `GIFSICLE_CONFIG_H` guard; root config.h resolves via `-I.` but is
  guard-neutralized), `-DHAVE_CONFIG_H=1 -DHAVE_UINTPTR_T -DHAVE_INTTYPES_H`,
  no `-DVERSION` (win32cfg.h defines `1.96 (Windows)`; the old flag only
  warned and never took effect).
- Reproduced the exact CI failure locally with mingw-w64, then verified the
  fix: valid PE x86-64, runs under Wine, optimizes GIFs, `--version` →
  `LCDF Gifsicle 1.96 (Windows)`.
- Wine CLI E2E (first ever): confs with `C:\gs\...` paths → exit 0 + valid
  outputs (12 frames preserved); spaces in input+output → exit 0 **after**
  fixing a second Windows-only bug: MinGW `_spawnvp` does **not quote**
  argv → space paths split. `ProcessRunner.h` now uses **CreateProcessA**
  with MSVCRT-rule quoting (`win_quote_arg`); unit test 19 guards the edge
  cases (empty arg, tabs, embedded quotes, trailing backslashes).
- Missing engine under Wine → exit 1 + honest stderr (A2 parity).
- **Static linking** for Windows CLI/tests (`-static`; KERNEL32+msvcrt only):
  dynamic exes die silently without MinGW runtime DLLs — unacceptable for
  portable artifacts. CMake `if(MINGW)` applies the same (GUI:
  `-static-libgcc -static-libstdc++`, Qt stays dynamic via windeployqt).

**Workflow hardening** (`build.yml` + `build.yml.proposed`, byte-identical):
`-static` flags; **Ninja generator** for the GUI step (windows-latest cmake
defaults to Visual Studio, which cannot consume aqt's MinGW Qt — latent
landmine); native **Windows engine+CLI E2E smoke step**; GUI offscreen
harness steps on linux + windows.

**Decisions:** keep `docs/ci/build.yml.proposed` synced with the live
workflow until retired; harness ships as a CMake target + ctest
(`QT_QPA_PLATFORM=offscreen`, TIMEOUT 600) so CI runs it on both OSes;
version stays 0.1.0; WebP/APNG stay blocked.

---

## 2026-09-07 — Docs sync + PR for P0/P1 remediation + compiled audit

- Updated `SESSION_HANDOFF.md`, `WORKLIST.md`, root/`working_code` READMEs,
  `PROJECT_VISION.md` status line, and `VERSION.md` “where version lives” to
  match implemented code and `COMPILED_AUDIT.md`.
- PR from `arena/01a07959-gifscythe` → `main` (version stays **0.1.0**).
- **CI note:** GitHub App lacks `workflows` permission, so the rewritten
  workflow ships as `docs/ci/build.yml.proposed` instead of replacing
  `.github/workflows/build.yml` in this PR. Next session (or a human with
  workflow rights) should copy it into place and confirm Windows CI green.

---

## 2026-09-07 — Merged compiled audit (S1+S2+S3)

- Fetched branch audit
  `codebase-review-and-optimization-3bbfe` / `WORKLIST_CODE_REVIEW.md` (30 items).
- Adjudicated each item against the forensic compilation:
  real duplicate, weaker restatement, false positive, or net-new backlog.
- S3 is strong as a **UX backlog** but **missed** critical silent-failure bugs
  already in S1/S2 (exit-0, Merge weld, empty-output loss, shell injection,
  CMake INTERFACE, SettingsIO UB, Win config.h static_assert, dangling Settings&).
- S3 false premises called out: SettingsIO is not JSON; CLI has no
  `--settings`/`settings.json`; `ci.yml` vs `build.yml`; `GIFSYCYTHE` typo in
  proposed version fix; installers vs portable vision.
- Wrote **`COMPILED_AUDIT.md`**: master checklist with fix status, §6
  next-session verification (including “new pit” probes), remaining work order.
- Prior long reviews remain on disk for history; day-to-day checklist is
  `COMPILED_AUDIT.md`.

---

## 2026-09-07 — P0/P1 remediation (silent failures + honesty)

Implemented the consolidated code-review plan without bumping past 0.1.0 and
without touching WebP/APNG.

**P0 — stop silent failures**
- CLI: replaced `system()` with argv `fork/execvp` (CreateProcess/`_spawnvp` on
  Windows); honest `WEXITSTATUS`; engine pre-flight; `std::filesystem` paths;
  `~` expansion; dropped `unistd.h`.
- CMake: `gifscythe_core` is `INTERFACE`; `version.h` generated from `VERSION.md`.
- Windows engine build uses `-include src/win32cfg.h` (no Linux `config.h`);
  engine `-DVERSION=1.96` (product version only names the release dir).
- GUI: default mode **Batch** (per-file `_opt.gif`); mode combo; require/auto
  output; verify output file exists before success; async `QProcess` + cancel +
  progress; timeout/zombie path removed.
- SettingsIO: safe `to_long`/`to_double`, unified `parse_bool`, load warnings,
  `save_settings` + round-trip, `load_settings_file` → `optional`.

**P1 — one truth per concept**
- `EngineLocator` shared by CLI and GUI; status bar shows engine path.
- Queue append+dedupe, Remove/Clear, working drag-and-drop (`DropListWidget`).
- Live command pane wired to all controls; `shell_quote` for display only.
- `GifsicleCommand` stores `Settings` by value (no dangling ref).
- Packaging probes `gifsicle`/`gifsicle.exe` and all GUI output dirs; ships
  `COPYING.gifsicle` + `LICENSE`; optional `windeployqt`.
- `build.sh` honest GUI branch (fails non-zero when `--all` and Qt missing).
- FEASIBILITY mapping table corrected (delay 1/100 s, crop `+` form); live pane
  doc descoped to honest **one-way** sync (two-way = optional later).
- Root `.gitignore`, `.gitattributes` (`*.sh eol=lf`), include-guard rename
  `GIFSYCYTHE_*` → `GIFSCYTHE_*`.
- Expanded unit tests + `scripts/smoke_cli.sh`; CI runs engine + smoke tests and
  uploads artifacts; Windows job uses aqtinstall + win32 config.

**Local proof (sandbox):** unit ALL PASSED (74 CHECKs); engine 5/5; smoke 7/7;
missing engine exits 1; engine version string 1.96; demo GIF written end-to-end.

**Intentionally not changed (verified correct):** `--loopcount=0` = forever,
`-O0` = off, gamma sentinel `-1`, crop plus-form emitter.

---

## 2026-09-06 — GUI MVP and packaging follow-up

- Improved the Qt GUI with an animation queue, optimization and lossy controls,
  output selection, generated command preview, status messages, and process
  error handling.
- Added portable and system-dependent packaging scripts.
- Fixed clean engine builds by creating the versioned release directory.
- Added SettingsIO parser coverage to the Qt-independent tests.
- Added GitHub Actions CI. Linux passes with Qt6; Windows currently fails and is
  the next investigation target.
- Confirmed WebP/APNG remain deferred until the GIF UI/UX retrofit is complete.

---

## 2026-09-06

- **Created** `FEASIBILITY_REVIEW.md` — feasibility verdict for the 4 product
  asks (Caesium GUI base, XNConvert-like ease-of-use + terminal control,
  portable click-and-run, exclusive GIF/APNG/WebP like eZgif).
- **Created** the project doc set for the next session: `PROJECT_VISION.md`,
  `SESSION_HANDOFF.md`, `IMPROVEMENT_LOG.md`, `WORKLIST.md`.
- **Resolved Blocker 1 (UI base):** use Caesium's UI/UX files as the base (not a
  full copy), modeled on XNConvert's "feels & vibe". Icons/styles are
  open-source/public with no exclusive trademark (logo excluded).
- **Deferred Blocker 2 (WebP/APNG):** moved to the bucket list, to be done only
  after the pending UI/UX retrofit task.
- **Separated reference from working code:** created `reference_code/` (read-only
  source material) and `working_code/` (the product). Moved gifsicle trees +
  Caesium bundle into `reference_code/`. Auto-fetched `gifsicle-upstream`
  (kohler/gifsicle master) and `caesium-source` (Lymphatus/caesium-image-compressor
  UI) from GitHub.
- **Named the product `gifscythe`** and set the version scheme: v0.1.0 →
  1.0.0–1.9.9 (finished GIF product) → 2.0.0–3.0.0 (WebP + APNG). Created
  `working_code/gifscythe/` skeleton + `VERSION.md`.
- **Built the gifsicle engine natively** (P0): hand-wrote `config.h` in
  `reference_code/gifsicle/` (no autotools in sandbox); created
  `scripts/build_engine.sh`; engine lives in `release/0.1.0/gifsicle`.
  Verified via `scripts/test_engine.sh` (info/optimize/lossy/resize/explode).
- **Built the engine control layer** (P1): `src/core/` with `GifsicleSettings`,
  `GifsicleCommand` (argv + live CLI string), `SettingsIO`; `src/cli/main.cpp`
  → `gifscythe-cli` (prints or runs the command). Unit + integration tests pass.
  Fixed optional-value gifsicle options to attached form (`--lossy=N`, `-O3`,
  `-j4`, `--loopcount=0`).
- **Constraint discovered:** cannot compile the Qt6 GUI or Windows `gifsicle.exe`
  in this sandbox — Qt not installed, apt offline, Qt mirrors fail SSL. Only
  GitHub HTTPS is reliable. These builds must happen on a machine/CI with the
  toolchains.
- **Reviewed the authored diff** (engine control layer, CLI driver, Qt GUI
  scaffold, build scripts): C++ compiles clean with `-Wall -Wextra -pedantic`;
  shell scripts pass `bash -n`. External clones (`caesium-source/`,
  `gifsicle-upstream/`) are treated as reference only and gitignored.
- **Created + merged PR** for the P0/P1 work (engine + control layer + Qt GUI
  scaffold + one-command build). Committed the authored work and repo
  reorganization to `main`.
