# Improvement Log

Chronological log of decisions and changes. **Newest at the top.**

---

## S30 — U-59/P0-7 GUI partial-output guard + offscreen regression coverage (2026-09-26)

**Changed:** On PR #5's active branch, ordinary GUI Batch/Merge/Auto runs now
write to the shared helper's same-directory `<target>.gs-partial` path. They
remove stale partials, snapshot and verify the isolated output, then promote it
onto the user's target only on verified success. Non-zero exit, start failure,
verification/promotion failure, and cancellation discard the partial; Explode
remains prefix-based and unchanged. Every subsequent batch item gets the same
guard. Added `fake_engine_partial_failure` (writes corrupt output then exits 7)
and offscreen coverage for preserving existing output on failure and cancel,
successful promotion to the existing explicit output, and sidecar cleanup.
Re-inspected G16 per the session-start rule: actual `<date>`, `<owner>`, and
P2 slot placeholders remain in §1–§10, so the plan correctly stays SKELETON;
clarified two S14 snapshot references that had described this historical plan
state as current. While running preflight, G16 was nondeterministic: its
`grep -q` could close the AWK pipeline early under `pipefail`. Changed the
matcher to consume its full input. Mutation probes: skeleton+slots passes;
filled content with SKELETON fails; WORKING PLAN with remaining slots fails.

**Partial:** U-59 remains PARTIAL until fresh Qt-enabled Linux and Windows PR
CI compiles and executes the GUI harness. The existing green PR run predates
these changes.

**Left:** An independent review and rerun of `test_gui_offscreen` is recorded
as a separate next-agent task in WORKLIST.md, conditional on that agent having
CMake + Qt6. No merge is authorized.

**Verified:** `git diff --check` passed. The local environment has g++ but no
CMake or Qt6, so GUI compilation and offscreen execution could not be run here.

**Not verifiable here:** Qt6/CMake compilation and `test_gui_offscreen`; Windows
runtime behavior. Fresh PR CI is required.

**Docs touched:** `COMPILED_AUDIT.md`, `STATUS.md` (generated), `WORKLIST.md`,
`SESSION_HANDOFF.md`, `web/WEB_PLAN_TEMPLATE.md` (clarified historical S14
state references; current SKELETON token retained), `working_code/gifscythe/README.md` (fixture and test count),
`working_code/gifscythe/scripts/check_docs.sh` (G16 pipeline reliability), and
this entry.

## S29 — PR #4 post-merge sync + U-94/P2-18 seeded engine-oracle gate (2026-09-26)

**Changed:**

- With explicit owner approval, merged PR #4 as true merge commit `7c035fd`.
  It is the post-PR-3 documentation sync: PR #3's ledger SHA and merged batch
  totals, P6 moved through PR #3, and both G10 base lines re-anchored. Main's
  repair run `36225207865` succeeded; the prior main run `36218999564` on
  `42306bb` failed exactly at G10.
- **U-94 / P2-18 — seeded real-engine oracle.** Added
  `working_code/gifscythe/scripts/oracle_fuzz.mjs`, a zero-dependency offline
  harness with 24 curated boundary cases plus 40 deterministic combinations
  from xorshift seed `0x47534631`. Every sample compares JS argv with the real
  C++ CLI's parsed-conf command, checks JS validation against `--strict`, probes
  the bundled gifsicle engine directly, and verifies successful outputs are
  non-empty GIF87a/GIF89a files. Product-accepted settings must succeed against
  the engine; an engine refusal must be warned/refused by the product. Cases
  where gifsicle returns success but the product correctly warns (including
  scale `0x1` no-op and invalid gamma) are covered too. `--quick` checks the 24
  curated prefix; `--full` checks all 64. The committed outcomes/settings are
  `tests/oracle_fuzz_matrix.json` (about 32 KB), so any drift requires an
  intentional reviewed matrix update via `--write`.
- Wired `--quick` into `.githooks/pre-push`; added verify_audit **W7** and a
  Linux CI full run. The workflow copy `docs/ci/build.yml.proposed` was
  re-synchronized byte-for-byte. Opened PR #5 from the fixed session branch
  after P1/P2/P3/P3b/P4/P6 preflight; initial push run 36225748067 passed on
  linux, windows, and csharp-spike.

**Partial:** none for U-94; its acceptance criteria are covered by both modes,
its committed matrix, and the CI/verify-audit hooks.

**Left:** the separate open rows are unchanged. In particular U-59 remains
PARTIAL because the Qt GUI path is not covered in this sandbox; no claim is
made about the offscreen GUI or Windows-only execution.

**Verified:** `./build.sh` → **372 checks, 0 failures**; `test_engine.sh` 5/5;
`smoke_cli.sh` 61/61; all nine web suites pass; oracle `--quick` 24/24 and
`--full` 64/64, including JS/C++ argv parity, product/engine acceptance
invariants, output verification, and committed-matrix comparison. Mutation probes
confirmed a changed matrix is rejected, a removed colors bound is caught against
strict C++ CLI behavior, pre-push blocks a forced quick-oracle failure, and W7
fails against a changed matrix. PR #4's post-merge main run `36225207865` is
SUCCESS. Final `check_docs.sh` and
`sweep_stale.sh` measurements are recorded in the session handoff.

**Not verifiable here:** Qt6/CMake and the offscreen GUI harness; Windows
MinGW/Wine behavior. The oracle's engine measurements are for the Linux-native
bundled gifsicle 1.96 build only.

**Docs touched:** `COMPILED_AUDIT.md` (U-94/P2-18 evidence and S29/G10 headers),
`STATUS.md` (re-emitted after U-94 closure), `WORKLIST.md`,
`SESSION_HANDOFF.md` (PR #4 ledger/P6 sync and S29 proof), and this log. CI
copies were synced in `.github/workflows/build.yml` and
`docs/ci/build.yml.proposed`.

---

## S28 — U-59 / P0-7 (the last data-loss row) fixed on the CLI/core half, red main diagnosed as G11, capability triage of the whole OPEN board (2026-09-26)

**Changed:**

- **U-59 / P0-7 — the engine no longer writes straight onto the user's file
  (CLI/core half, test-first).** `src/core/OutputVerify.h` gains the tmp+rename
  guard: `partial_output_path()` (target + `.gs-partial`), `redirect_output_operand()`
  (rewrites the SINGLE `-o <target>` operand of a full argv vector, refusing
  anything ambiguous), `promote_partial()` (one `std::filesystem::rename`, which
  has replace semantics on POSIX and Windows) and `discard_partial()`.
  `src/cli/main.cpp` now, for every run that writes a real file (not `-o -`, not
  explode): drops any partial a previous crash left, redirects `-o` to the
  partial, and — only after the run returns 0 and, where the mode verifies at
  all, `verify_output()` passes on the PARTIAL — renames it onto the target.
  Every other outcome (non-zero rc, signal, refusal) discards the partial.
  Failure messages keep naming the real target: the partial is an
  implementation detail of the guard, and renaming it in the message would have
  made smoke's existing `grep -F "$WORK/verify.gif"` assertion pass vacuously
  against the partial's path (review rule R2).
- **Failing test FIRST (the §17.1 order, and the R2 proof that the cases are not
  vacuous).** Three U-59 cases were added to `scripts/smoke_cli.sh` and run
  against the UNFIXED build: all three reported `cmp=DIFFERS` — the
  pre-existing output really was destroyed — for (a) an engine that writes
  garbage to `-o` and exits 0, (b) an engine killed by SIGTERM mid-write
  (rc=143, the U-32 convention) and (c) the audit's literal scenario, a SIGTERM
  cancel of the CLI while the engine is still writing. After the fix all three
  are green with the target byte-identical (`cmp -s` against a saved copy).
  A fourth case pins the self-heal contract: a partial that a hard kill could
  not clean up is swept by the next guarded run, which still produces the real
  target. Smoke went 54 → **58/58**.
- **`tests/test_output_verify.cpp` 12 → 25 assertions** pinning the helpers:
  the partial's name, a successful redirect that leaves the rest of argv alone,
  refusal of a non-matching `-o`, of a DOUBLED `-o` and of a command line with
  no `-o` at all, promote really replacing the old bytes (read back, not just
  `exists`), promote of a missing partial returning an error, and `discard`
  being idempotent.
- **Harness trap found and fixed while writing the tests:** `smoke_cli.sh` runs
  with `set -e` ACTIVE (case 1 turns it on and never turns it off), so a `wait`
  on a SIGTERM'd job returned 143 and silently aborted the whole suite — the
  three new cases reported and then the script died before `==> Done.` with exit
  143 and no trap fired. The cancel case now runs inside an explicit
  `set +e` / `set -e` window like every other negative case in the file.

**Changed (second batch — the whole high-confidence lane, same session):**

Seven more rows closed, each with executed proof. The web lane needed no compiler
and the CLI lane had one, so every one of these was measurable here.

- **U-92 / P2-19 — upload admission (GS-205's web twin).** Both endpoints now
  require **strict base64** (shape + canonical round trip; `Buffer.from(s,"base64")`
  alone silently decodes truncated and foreign payloads) and a **GIF87a/89a
  signature on the decoded buffer**, before engine discovery, slot and temp tree.
  A non-GIF upload is a named 400 that names the file instead of relaying the
  engine's opinion of somebody else's bytes. The decoded buffers are kept, so
  nothing decodes twice. `/optimize` gets the same signature check on its body.
- **U-84 / P3-14 — one body-cap contract.** `handleOptimize` reads and admits the
  body BEFORE `findEngine()`, like `handleRun`. An oversized upload to an
  engine-less server is now 413 on both endpoints instead of 503 on one, and a
  refused upload never takes an engine slot. `body-limit.test.mjs`'s own header
  comment described the old ordering as current fact and explained its
  `GS_ENGINE=node` workaround; corrected, and a new case proves the 413 with
  `GS_ENGINE` pointing at a path that does not exist.
- **U-85 / P3-13 — PORT.** Validated once at startup: decimal-integer shape +
  0..65535, a named `ERROR:` with a usage line on stderr, **exit 2**. No more raw
  `ERR_SOCKET_BAD_PORT` RangeError at module top level.
- **U-93 / P2-20 — three transport bounds.** Engine stderr capture capped
  (`GS_MAX_STDERR`, 16 KiB default) with the cap DISCLOSED in the message; the
  fixture had to write to **stderr**, not stdout, because the server discards
  stdout — measured, not assumed. `/favicon.ico` answers 204 and `index.html`
  carries an empty data-URI icon, so a page load no longer logs a 404. The `/run`
  base64 output envelope is documented in `web/README.md` as a deliberate shape.
- **U-86 / P3-16 — three comment/message truths.** The `/run` collision message
  claimed it protects "the uploaded file of the same name" (uploads are renamed
  `inN.gif`); it now says a planned output collides with an upload name, and the
  **two transport assertions that pinned the old wording were re-pinned, not
  deleted** — a message-truth fix must move its pin, not lose it. `expand_home`'s
  comment now says what the code does (bare `~` expands to `$HOME`; only `~user`
  is left alone). `run()`'s double-resolve (timeout then close, first wins by
  accident) is now an explicit `settleOnce` guard.
- **U-81 / P1-46 — MEASURED FIRST, and the intake's repro was wrong on both
  halves.** `gifsicle -e -o - in.gif` writes ZERO bytes to stdout and drops
  `in.gif.000..011` into the CWD (rc=0): it is N-05's scatter class, not the
  "honest stdout run" the row described. And `--info` + explode is refused by the
  ENGINE itself (`'--info' suppresses normal output`, rc=1), so no false frame
  failure existed there either. Fix: `verify_explode` carries the same
  `!stream_output && !info` exemptions as `verify_file` (the verifier can no
  longer add its own verdict to an exempt run), and explode + `output = -` gets
  N-05's treatment — a named refusal, rc=2, before any process starts, with print
  mode still printing. This is the §19 discipline paying for itself: both halves
  of a "verified" intake row were re-probed and both were wrong.
  **The scatter is not theoretical — this session produced it.** The first,
  pre-fix run of the new explode case wrote `-.000` … `-.011` (12 frames, 533–1001 B)
  straight into `working_code/gifscythe/`, and `git add -A` picked them up; they
  were caught in the commit's own diffstat and removed before the push. That is
  the defect happening to the person testing it, which is why the fixed case runs
  in its own temp CWD and asserts the directory is still empty.
- **U-83 / P3-15 — pinned, not refused**, because the measured behaviour is
  sound: batch + one input + an `output` key writes the `-o` target and leaves the
  source byte-identical. The case asserts exactly that, so a future engine that
  changes it fails the suite instead of drifting.

**Partial:**

- **U-59 is PARTIAL, not DONE — the Qt half is still exposed.** `runCommand()`
  in `src/qtui/MainWindow.cpp` still passes the real target to the engine, so
  the GUI's Cancel keeps the old behaviour. Deliberately NOT edited this
  session: no Qt6/cmake here, so the edit could not be compiled, and an
  unverified Qt change is a red CI build the next session inherits. The
  register row names exactly what remains (wire the same guard + a harness
  cancel-with-preexisting case).

**Left:**

- Every other OPEN row. A capability triage of the whole board against this
  sandbox's measured toolchain was written up for the owner in the session
  response rather than guessed at: provable here (node/CLI/core lane) vs
  CI-provable (Qt/Windows) vs impossible anywhere in a sandbox (W-18
  clean-Windows machine, W-19 physical desktop, the OD-* owner decisions).
- The Qt half of U-59 (above); the signal-handler cleanup that would remove the
  partial at the moment of a hard kill instead of on the next run.

**Verified:**

- `./build.sh` → engine `LCDF Gifsicle 1.96` + CLI + **372 checks, 0 failures**.
- `scripts/smoke_cli.sh` → **61 passed, 0 failed** (was 54: +4 U-59 cases, all RED
  before that fix, +2 U-81 and +1 U-83).
- `scripts/test_output_verify.sh` → **25 assertions, 0 failures** (was 12).
- `scripts/test_engine.sh` 5/5 · `scripts/test_package.sh` **36/36** ·
  `scripts/verify_audit.sh` **30 PASS / 1 FAIL / 5 SKIP** where the 1 FAIL is
  F1, its own re-report of the doc gate below.
- All nine web suites green, including the engine-backed ones: `transport`
  79 → **86 cases** (+7 U-92), `body-limit` 8 → **13** (+2 U-92, +2 U-84, +1
  re-pin), `server-bounds` 5 → **10 groups** (+1 U-93, +4 U-85), `command`/
  `validate` parity against the real CLI unchanged, `static-hygiene` still green
  after the favicon route.
- **The red main diagnosed, not assumed:** `gh run view 35904935321` → linux
  job `failure` at the step **Documentation status gate**, which runs
  `./scripts/check_docs.sh --no-gate-run` (`build.yml:35-37`); windows
  **all 11 steps green** (including Package portable + manifest assert, which is
  the platform proof U-97 asked for) and csharp-spike green. Reproduced locally
  on the same tree: gate **G11** — newest `IMPROVEMENT_LOG.md` entry 2026-09-23
  vs the merge commit's own author date 2026-09-24 +0700 — which THIS entry
  clears. `check_docs.sh` here: 23 passed / 1 failed (G11 only) / 1 skipped
  after `build.sh` bootstrapped the hooks (G15 was the second failure before
  that, the known fresh-clone R-04 state).
- Also measured this session: the new remote has **zero releases and zero
  tags** (`gh release list` and `git ls-remote --tags origin` both empty), so
  U-95's "mark the published Release superseded" has no artifact left to act
  on, and the only downloadable build is the CI artifact `gifscythe-windows`
  (52,955,462 B, expires 2026-10-07) off run 35904935321.

**Not verifiable here:**

- The Qt half of U-59 (no cmake/Qt6 — measured: both absent), so the GUI Cancel
  path is unchanged and unproven either way.
- Every Windows-only row (no mingw-w64, no wine) and the wasm rows (no emcc).
- **The CI log TEXT for the red step** — `gh run view --log-failed` fails at
  `results-receiver.actions.githubusercontent.com` (EOF), as in S26/S27. "The
  red step is G11" is therefore an exact local reproduction of that step's own
  command, not a log read.

**Docs touched:** `COMPILED_AUDIT.md` (§5: U-59 ⬜ OPEN → ◐ PARTIAL with the
executed proof and the named remainder, and U-81/U-83/U-84/U-85/U-86/U-92/U-93
⬜ OPEN → ✅ FIXED with their proofs; §6 P0-7 annotated), `STATUS.md` (re-emitted
twice — 123/8/38/0 → 123/9/37/0 → **130/9/30/0**), `web/README.md` (the
`GS_MAX_STDERR` row, the two admission rules, the favicon contract and the
documented `/run` output envelope), this file,
`SESSION_HANDOFF.md` (header, the P6 sync to PR #2 + its merge sha, the S28
section, the verification table, the toolchain section), `WORKLIST.md` (road to
1.0.0 step 1 marked as the CLI half landed / GUI half open).

---

## S27 — the repo was re-created from a zip: root license set restored (U-97), doc gate re-synced (G10/G11), double-red main diagnosed (2026-09-23)

**Changed:**

- **The GitHub repo was re-created from a zip on 2026-09-22 — measured, not
  assumed.** The remote's `created_at` is 2026-09-22T03:27:39Z and its whole
  history is four commits: `04a1cd4` (initial) → `60d3df4` (zip upload) →
  `ce5fd51` (unpack to root) → PR #1 merge `824bf20`. The old remote's history
  (S1–S26, its PRs #1–#33, the old base `5c93680`) is gone from GitHub, so the
  docs' enforced base lines named shas this clone cannot resolve — gate G10's
  exact complaint, and the linux CI failure ("Documentation status gate") in all
  three runs of the new main. The unpacked tree IS the S26 state, verified by
  markers rather than trust: `numOrNull()` in `web/command.mjs` + `web/app.js`,
  `g6_missing_tools()` in `check_docs.sh`, the 27-row device table (device-names
  suite green), transport's 79-case write-up in the handoff. The unpack merge
  itself changed nothing: `git diff ce5fd51..824bf20` is empty.
- **The re-creation lost the four root license files — new finding U-97, found
  AND fixed in-session (rule 2's strong form).** `COPYING.ms-pl`,
  `COPYING.lgplv3`, `COPYING.gplv3`, `COPYING.gifsicle` were absent while
  `package_common.sh` hard-requires the first three (`copy_required`, fail-closed
  since U-02/U-08; the fourth has an in-tree fallback). That is where CI windows
  "Package portable (Windows)" died (run 35684055250; every build/test step
  before it green, manifest assert + artifact upload skipped after). **Executed
  local repro** — the licence check is platform-independent, so no Windows
  runner was needed: real `package_portable.sh --engine-cli-only`, fixture
  binaries on gitignored paths, REAL repo root → `ERROR: COPYING.ms-pl missing
  or empty (no usable same-target candidate)`, exit 1. The linux packaging step
  would have failed identically once the doc gate passed.
- **Restored from canonical sources, provenance recorded here per the legal
  README's rule (6):**
  - `COPYING.gplv3` — GNU GPLv3 text, 35147 B, sha256
    `8ceb4b9ee5adedde47b31e975c1d90c73ad27b6b165a1dcd80c7c545eb65b903`, fetched
    from `raw.githubusercontent.com/gcc-mirror/gcc/master/COPYING3` (gnu.org is
    unreachable from this sandbox; the gcc mirror carries the GNU text verbatim —
    size and head/tail match gnu.org's `gpl-3.0.txt`).
  - `COPYING.lgplv3` — standalone GNU LGPLv3, 7639 B, sha256
    `a853c2ffec17057872340eee242ae4d96cbf2b520ae27d903e1b2fef1a5f9d1c`, same
    mirror, `COPYING3.LIB`. The SPDX `LGPL-3.0-only.txt` was fetched first and
    **rejected**: 42098 B because it concatenates the GPLv3 text at offset 7428 —
    the canonical `COPYING.lgplv3` is the standalone text (the GPLv3 companion
    ships separately as `COPYING.gplv3`, which is the repo's documented layout).
  - `COPYING.ms-pl` — canonical Ms-PL, 2663 B, sha256
    `7a162b1da10f1c22db4c68f07bec4a8355259f8d4aab5b00a2b2bbd423d833dd`, from
    `raw.githubusercontent.com/spdx/license-list-data/main/text/MS-PL.txt`;
    structure verified (sections 1 Definitions / 2 Grant of Rights / 3
    Conditions and Limitations, ending with the as-is clause (E)).
  - `COPYING.gifsicle` — byte-copy of `reference_code/gifsicle/COPYING` (GPL v2,
    18092 B), sha256 `8177f97513213526df2cf6184d8ff986c675afb514d4e68a404010521b880643`
    identical on both sides of the copy. Nothing was written INTO the read-only
    tree.
  - All four verified LF-only, non-empty, exact names. **Repro flipped:**
    `Package created`, exit 0, the packager's own check listing all 11 required
    files present and non-empty (fixture binaries; the real-binary proof is this
    PR's CI packaging steps).
- **Doc-gate re-sync (G10 + G11).** Both enforced base lines (the handoff's
  "Based on" header line and `COMPILED_AUDIT.md`'s Base line) now name
  `824bf20` — the new main tip, and after this PR merges also its merge first
  parent, so the lines stay legal across the merge (the S26 mechanic). This
  entry (dated 2026-09-23 — the session opened on the 22nd and crossed the
  sandbox's UTC midnight) clears G11: the newest log entry no longer trails the
  newest non-doc commit (the unpack of 2026-09-22 and this session's licence
  restore of 2026-09-23).
- **The re-creation is recorded where the doc machine reads it.** Handoff
  header: session/branch lines, a re-creation note explaining the old-repo shas,
  the **Docs synced through** line moved to the new remote's PR #1 (branch
  `arena/01a0c72c-gifscythe`, merged as `824bf20`). Ledger: a separator marking
  rows #1–#33 as the OLD repo's (kept as the written record) and a new-repo
  table starting with the unpack row. `COMPILED_AUDIT.md`: an S27 addendum
  banner, the verification-sessions line, §5 heading now "all 97 unique
  findings", U-97 row appended. Register re-emitted via `check_docs.sh --emit`:
  **122/8/38/0 = 168 → 123/8/38/0 = 169**; quoted tallies in the handoff
  (fast-handoff Register line + orientation item 0) moved in the same pass
  (sweep rule S2).
- **WORKLIST:** U-97 line added to "Found this session" (ticked — fixed the same
  session); the "96-row audit register" reference updated to 97.

**Partial:** none claimed. U-97's DONE rests on the executed repro flip + the
recorded digests; the platform-level confirmation is this PR's CI packaging
steps (windows stages the real `.exe`s; linux reaches its packaging step for the
first time since the re-creation once the doc gate passes).

**Left:** the windows packaging diagnosis remains an inference from an exact
local reproduction — the CI log text could not be read (blob storage answers
HTTP 401 from this sandbox, as in S26), so if that step still fails after this
repair, the next session must diagnose from the runner side. Every C++/Qt/
Windows/wasm row is untouched (no compiler here): **U-59/P0-7 remains the top
product row** and needs a toolchain sandbox. `PLANNING.md` §5's copy-paste
block was deliberately NOT refreshed (it claims to hold no state and was
already stale at S26 — refreshing it is P2-22/U-89 territory). No gate logic
was touched (no R1 edits). The old repo's published `snapshot-2026-09-07`
release and its U-95/P1-45 note did not survive into the new remote's releases
list (zero releases now) — whether to re-publish anything is the owner's P0-4
call, unchanged.

**Verified:** `scripts/check_docs.sh` — session start 21 passed / 3 failed
(G10 ×2 docs, G11, G15) / 2 skipped; after the edits + `--emit`: see the S27
handoff verification table for the final measurement · `scripts/sweep_stale.sh`
5/0/0 · `python3 tests/test_sweep_stale.py` 20/20 · packager repro before
(exit 1, `COPYING.ms-pl`) and after (exit 0, `Package created`) · engine-free
web suites green: numeric-honesty, request-guard, device-names (27 rows),
body-limit, static-hygiene; server-bounds 3/5 here (its 2 failures need a
discoverable engine — environmental, printed as `Engine [none]`) ·
`review_change.sh --commit` on both commits (flags recorded in the S27 handoff
table) · GitHub API via python urllib: token owner `freeforall1932-design`
(id 300004558 — the commit-identity rule), main's three runs all `failure`,
failing steps named from the job summaries, `git diff ce5fd51..824bf20` empty.

**Not verifiable here:** the CI packaging steps themselves (no mingw/wine/
Windows — the fixture repro proves the licence-staging logic, not the real
binaries); CI log text (401); the engine-backed suites (`command`/`validate`
parity, `transport`, smoke, unit, engine — no compiler in this sandbox); the
Qt/GUI harness (no cmake/Qt6); `gh` is absent, so preflight P4/P6 skip and the
PR was opened via the API with python urllib (the S24 pattern).

**Docs touched:** `COMPILED_AUDIT.md` (S27 banner, verification-sessions line,
Base line, §5 heading + U-97 row), `SESSION_HANDOFF.md` (header block, ledger
separator + new-repo rows, S27 section, toolchain bullet, verification table,
orientation counts), `WORKLIST.md` (U-97 line, 97-row), `STATUS.md`
(regenerated via `--emit` — never hand-edited), `IMPROVEMENT_LOG.md` (this
entry). Root license files restored: `COPYING.ms-pl`, `COPYING.lgplv3`,
`COPYING.gplv3`, `COPYING.gifsicle`.

---

## S26 — P1-44 proved and closed (U-78/U-87), two register drifts fixed, red `main` repaired (2026-09-17)

**Changed:**

- **P1-44 finished and closed: U-78 + U-87 → ✅ FIXED (S26), §6 row DONE.** S25
  implemented it (PR #32, `e885d58`) and logged it *Partial* because its sandbox
  had no compiler; S26's does, so the missing proof was **executed** instead of
  deferred: `./build.sh` green (gifsicle 1.96 + CLI + 372 unit checks / 0
  failures), `smoke_cli.sh` 54/54, `test_engine.sh` 5/5, and the engine-backed
  web suites — `command`/`validate` parity against the real CLI and `transport`
  against a live server.
- **The three fixture batches P1-44's own row asked for were missing; added.**
  `web/test/validate.test.mjs` (+7 keys), `web/test/command.test.mjs` (+2),
  `web/test/transport.test.mjs` (+7 cases, 72 → 79). Fixtures only — **no product
  behaviour was changed in S26.**
- **The wrong-type class needed a new fixture shape, for a measured reason.** A
  non-numeric conf value is caught by the C++ **parser**, not by `Validate.h`:
  `WARNING: settings key 'colors' value 'abc': not an integer`, counted in
  `WARNING-SUMMARY: parse=1`, refused by `--strict` with **rc=3** (measured for
  colors/optimize/lossy/delay/threads/loopcount/disposal). The web has no parse
  layer — JSON hands `validate()` the raw type — so its finite-number gate *is*
  the mirror. Wordings differ by design; the refusal must not, and that is what
  the new section pins on both surfaces in one case.
- **The empty-field state has no C++ counterpart, so it stays a JS contract.**
  Measured: an absent `resize_w` is re-defaulted to 0 by `SettingsIO` and the CLI
  prints `--resize-fit 0x200`, while `buildArgs` omits the flag for `null`. Faking
  that into a parity row would have asserted a falsehood, so the parity fixtures
  pin what *does* have a counterpart (an explicit 0 survives as `0x200` / `0x1` on
  both surfaces) and the empty-vs-zero contract stays in `numeric-honesty` +
  `transport`.
- **Register drift 1 — DS-09 said OPEN/S15 but was closed in S22.** §6 P1-31
  reads "DONE S22", §16 records the proof, `Validate.h:44` and `web/validate.mjs:47`
  carry the rule, and `threads = -7` is asserted at
  `tests/test_gifsicle_command.cpp:380,543` (passing in the 372). Only the
  hand-maintained `STATUS.md` row was never moved — four sessions of contradiction
  that **no gate could see**: G0 regenerates the §5-derived U-rows only, and
  G17/S5 compares narrative against the U-row register, so a hand-block row that
  disagrees with §6 is nobody's job. Row synced to DONE with re-measured proof.
- **Register drift 2 — the G10 base line, which is why `main` was red.** Linux run
  **35225055959** (the PR #32 merge) failed at *Documentation status gate* while
  windows and csharp-spike passed. Reproduced locally: G10 accepts main's tip or
  its merge first parent, so naming the PR #30 merge was legal until PR #31's
  merge landed, then stale — the U-82/H:F-06 failure mode recurring one merge
  later in the very line S24 rewrote to be G10-enforceable. Both enforced lines
  (`COMPILED_AUDIT.md` **Base:**, `SESSION_HANDOFF.md` "Based on `main` commit")
  now name `e885d58`.
- **A third doc-truth gap from PR #32: the suite inventories.** S25 added a ninth
  web suite (`numeric-honesty.test.mjs`) and wired it into both CI copies, but
  `web/README.md` still listed **three** suites with hand-typed counts frozen at
  S17 (17/23/63 while `transport` alone is now 79), and `web/README.md` +
  `WORKLIST.md` both still said "all eight suites". Fixed: the README lists all
  nine, grouped by what they need (built CLI / discoverable engine / engine-free),
  and **carries no PASS counts at all** — a doc-quoted count goes stale the first
  time a fixture is added, which is exactly U-89's "hand-typed counts" complaint
  and the reason WORKLIST already says to quote the runtime counter instead.
- **The doc machine was under-claiming the toolchain too (gate G6).** Its SKIP
  message named all five required tools unconditionally —
  `(gcc/g++/Node/CMake/Qt6 missing)` — so this sandbox, which HAS g++ 12.2 and
  node v22, was told it had none of them. That is the same mistake S25 made in
  prose, emitted by a gate. The five tests are now factored into
  `g6_missing_tools()` and the message names what is actually absent
  (`missing: cmake Qt6` here). **Message-only:** the helper is the old OR-chain
  verbatim, so the skip condition cannot have changed — verified in four states
  (empty PATH → all five named; only node absent → `node`; this sandbox →
  `cmake Qt6`; all present → empty string, i.e. the condition is false and G6
  runs its comparison exactly as before). `review_change.sh` flags this as **R1**
  (a check-logic file), which is why the four-state probe is recorded here.
- **Handoff sync (rule 1 / preflight P6):** PR #32 written up, **Docs synced
  through** moved to PR #32, the ledger's #31 cell filled in (`f1c5bc8` — merged,
  still marked **open**; rule 2 makes that the merger's edit and nobody did it),
  a #32 row appended, the verification table replaced with S26's measurements, and
  the toolchain section corrected: S26 has g++/node/gh, no cmake/Qt6/mingw/wine/
  emcc/dotnet, and Actions **log blobs are unreachable** (a red run can be
  identified but not read). `git fetch --unshallow` was run (risk R-02).

**Partial:** none claimed. Every row S26 moved is backed by a command that was
executed in this sandbox; the register is now **122 DONE · 8 PARTIAL · 38 OPEN ·
0 UNTRIAGED · 168 total** (was 119/8/41/0).

**Left:** every Qt/GUI row (U-12, U-58, U-59's harness half, U-70/U-72, GS-203's
GUI half, GS-205, DS-10 — no cmake/Qt6), every Windows-binary row (U-55, U-71,
GS-204's architecture checks, W-18 — no mingw/wine), `web/wasm/` (U-57 — no emcc,
plus OD-16), the release rows (U-09/U-95 — owner decision + Windows artifacts),
and the whole S24 web-intake remainder (U-81, U-83..U-86, U-88..U-94, U-96). None
of those moved, because S26 changed no product behaviour.

**Verified:** `./build.sh` (372 checks / 0 failures) · `scripts/test_engine.sh`
5/5 · `scripts/smoke_cli.sh` 54/54 · `scripts/verify_audit.sh` (30/1/5 at session
start, the 1 being F1←G10; re-run after the edits) · `node web/test/command.test.mjs`
· `node web/test/validate.test.mjs` · `node web/test/transport.test.mjs` (79 PASS)
· `node web/test/numeric-honesty.test.mjs` · **mutation test of every new
assertion (review rule R2)**: dropping `validate.mjs`'s finite gate → 7 validate +
3 transport FAIL; `numOrNull("")`→0 → numeric-honesty FAIL; server-side `""`→0 in
`buildArgs` → the 3 U-87 transport cases FAIL; treating an explicit 0 as unset →
the command-parity fixture FAIL; all four files restored byte-identical afterwards
(`git diff` clean) · `scripts/check_docs.sh` and `scripts/sweep_stale.sh` re-run to
green (24 passed / 0 failed / 1 skip) · `scripts/verify_audit.sh` **31 passed /
0 failed / 5 skipped** (was 30/1/5 — the 1 was F1←G10), W3 now reporting 79
cases · the G6 four-state toolchain probe above · `gh run view 35225055959` for
the failing step name.

**Not verifiable here:** the GUI offscreen harness, `windeployqt` packaging and the
clean-Windows smoke (no cmake/Qt6/mingw); **the log text of the red run** —
`results-receiver.actions.githubusercontent.com` and the blob host are unreachable
from this sandbox, so "the failing check is G10" is an inference from *which* step
failed plus an exact local reproduction of that single failure, not a read log
line. Pushing this branch is what confirms it. Nothing here proves the Windows or
C# jobs beyond their existing green runs.

**Docs touched:** `COMPILED_AUDIT.md` (base line, verification-sessions line, §5
U-78/U-87, §6 P1-44, §19.3 pairing note, §20.1 GN-02 + §20.3 U-78 dispositions),
`STATUS.md` (DS-09 hand row + `--emit` regeneration), `WORKLIST.md` (P1-44 line
ticked with its proof), `SESSION_HANDOFF.md` (header, base, ledger #31/#32, S26
write-up, verification table, toolchain reality, register tallies),
`IMPROVEMENT_LOG.md` (this entry), `web/README.md` (nine-suite inventory, stale
hand-typed counts removed), `WORKLIST.md` (build-commands block: ninth suite +
"all nine"), `working_code/gifscythe/scripts/check_docs.sh` (G6 skip message
names the tools actually missing). `WORKLIST.md` needed no further edit for the
G6 change: it documents the gate commands, not their skip wording.

---

## S25 — P1-44 web numeric honesty implementation (2026-09-17)

**Changed:** Implemented the high-confidence portion of P1-44: finite-number
validation in `web/validate.mjs`, empty-versus-zero handling through
`numOrNull()` in `web/command.mjs` and `web/app.js`, and omission of incomplete
resize/scale arguments. Added focused numeric-honesty Node coverage and wired
it into both byte-identical CI workflow copies.

**Partial:** U-78/U-87 remain OPEN in `STATUS.md` until the CI C++ parity and
engine-backed web suites provide the required full proof.

**Left:** The medium- and low-confidence audit items remain untouched.

**Verified:** `node web/test/numeric-honesty.test.mjs`, Node syntax checks for
`app.js`, `command.mjs`, and `validate.mjs`, `sweep_stale.sh`, and the change
review script all passed. Workflow copies remain byte-identical.

**Not verifiable here:** The compiler, C++ CLI build, and engine-dependent
parity/transport suites are unavailable in this sandbox.

**Docs touched:** `SESSION_HANDOFF.md`, `IMPROVEMENT_LOG.md`.

---

## S24 addendum — attribution repair of the S24 commits + post-#33 doc sync (2026-09-17)

**Changed:**

- **History repair (owner-directed):** the two S24 commits carried a fabricated
  numeric id inside the noreply author/committer email (48732176+...); GitHub
  maps users.noreply.github.com addresses by numeric id and 48732176 belongs to
  an unrelated account, so both commits were attributed to that account on the
  PR/commit views. `git filter-branch --env-filter` rewrote author+committer of
  exactly those commits to the owner's real identity
  (300004558+freeforall1932-design@users.noreply.github.com, id from GET /user);
  every other commit regenerated byte-identically, and all descendants (the
  #31/#33 merges, the S25/S26 commits) kept identical trees and messages with
  new shas. Proof: `git diff <old main tip> <new main tip>` empty for the pure
  rewrite; `git log --format='%ae %ce'` shows zero occurrences of the bad email
  afterwards. Sha map (old to new): 719e961 to 1175275, 31097ce to 10c25d2,
  #31 merge f1c5bc8 to e1d61fb, #32 sha e885d58 to 825ff2c, #33 merge 3167aa8 to
  5c93680. Main was force-pushed to the repaired lineage; the three merged head
  branches of the old lineage (docs/s24-consolidation-intake,
  arena/01a0af41-gifscythe, arena/01a0afc4-gifscythe) were deleted so the
  misattributed ancestry is unreachable from any branch. Residual, disclosed:
  the PR pages #31/#32/#33 keep their pre-repair head refs (GitHub retains
  them), so the old shas remain viewable there as the review-time record.
- **Doc sync caught up (rules 2/3, P6):** ledger #33 cell filled with the
  post-repair merge sha 5c93680; #31/#32 cells re-pointed to their post-repair
  shas with provenance notes; Docs-synced-through moved PR #32 to PR #33; the
  G10-enforced base lines (this file's sibling SESSION_HANDOFF.md and
  COMPILED_AUDIT.md) re-pointed to 5c93680; the standing commit-identity rule
  added to SESSION_HANDOFF's sandbox section.

**Partial:**

- Nothing partial; the repair is complete and content-preserving.

**Left:**

- Queenkomal remains visible only where GitHub immutably retains history: the
  three PR pages' pre-repair head refs and the old push/merge event entries.
  No branch, tag, or default-branch commit references the bad identity any
  more, so the contributors graph and main's history are clean.
- The next session re-confirms CI green on the repaired main tip (the
  force-push retriggered Actions; the annotation commit rides the same run
  set).

**Verified:**

- check_docs.sh green on the annotation tree before pushing (G10 accepts
  5c93680 as the tip's first parent); sweep green; tree equality old-vs-new
  main proven by empty diff; attribution verified via git log emails locally
  and via the commits API after push (author login = freeforall1932-design).

**Not verifiable here:**

- GitHub's contributors-graph cache refresh lag (server-side); Actions log
  blobs remain unreadable from sandboxes (S26 note).

**Docs touched:**

- SESSION_HANDOFF.md (ledger cells, Docs-synced, base line, identity rule),
  COMPILED_AUDIT.md (Base line), IMPROVEMENT_LOG.md (this addendum).

---

## S24 — external-review intake (v4, U-77..U-96) + the owner-ordered docs consolidation + stale sweep (2026-09-17)

**Changed:**

- **COMPILED_AUDIT.md → v4.** Incorporated the four 2026-09-16 external review
  files (uploaded at 4d49919, deleted in PR #30, owner-restored at a4ba82c) per
  the owner's instruction — "delete the already done, incorporate the four md
  files into the compiled audit, make sure the problem added is new and hasn't
  been worked on". Every one of their 54 findings was re-verified against main
  3c67e14 BEFORE disposition (they pinned 794a996; PR #29/#30 had already fixed
  or mooted several): **20 new §5 rows U-77..U-96** — U-77/U-79/U-80 recorded
  ✅ FIXED (PR #30, commit shas cited; the in-file "FIXED (S23)" markings
  superseded by stronger attribution), U-82 ✅ FIXED (S24), 16 ⬜ OPEN scoped as
  **P1-44..P1-46, P2-18..P2-22, P3-13..P3-19** in §6; already-fixed-at-intake
  (H:F-02/03/04 → S23's to_int_strict + loopcount domain + threads tri-state;
  H:F-05 + G:GN-01 → PR #30's U-79) and the 16 J re-frames got disposition rows
  in **§20** instead of duplicate register entries; refuted-with-evidence:
  G:GN-03 (probe executed: validate.mjs refuses resize/scale 0x0, accepts crop
  0x0 — pairing added to §19.3), G:GN-06 + J:F-17 (README already states the
  parked direction and the format deferral). G:GN-05's falsify-line fired: the
  CI doc gate IS live, so the stale rows closed instead (below). The four files
  were then deleted; §20.5 is the completeness checklist (54 findings →
  20 rows + 9 already-fixed/tracked + 16 re-frames + 3 refuted + 3
  adopted-as-edits + policy notes; none dropped). §1/§5-title/§10/§11/§12/§13
  counts and pointers updated (six audits → ten reviews, 76 → 96 findings).
- **U-82 fixed (F-06):** this audit's own header named base 2d51347 (six merges
  stale) in a "**Branch:** … at …" shape G10's trigger list never matched — a
  live R2-class vacuous gate. The line is now "**Base:** `main` at `3c67e14`",
  a shape G10 DOES enforce, so it cannot silently stale again.
- **Stale sweep (proof-backed deletions).** docs/ci/PENDING_WORKFLOW_CHANGE.md
  deleted in the same commit that re-synced docs/ci/build.yml.proposed to the
  live workflow line: the maintainer had fixed the live cygpath fallback in
  414f5fc (green on every Windows run since) and the doc copy simply lagged —
  the marker's premise ("intended change lives in proposed, needs workflows
  scope") was false, and no workflow push was needed. E9/G7/S1 enforce
  byte-equality again with no standing exception. **W-30, R-03, GS-208 → DONE**
  (the doc-gate CI step is live at build.yml:35-37; scope was granted S18 and
  exercised by PR #28); **U-14's** blocker text corrected (verify_audit stays
  out of CI by design, not by scope); **P2-7** annotated DONE; W-14's stale
  gate-triple replaced with a "quote the live run, not this row" note (latent
  G6 landmine). §19.4 corrected (U-56 closed S23; U-71 CI-testable per GN-14 —
  the windows job already runs the unit exe).
- **Consolidation, 49 → 25 md files** (owner order: merge every md file into
  its own newly merged file, brief and clear). New merged files:
  docs/archive/AUDIT_HISTORY.md (absorbs the seven dated snapshots — the two
  2026-09-06 archive reviews, POST_S7_AUDIT, CONSOLIDATED_AUDIT_2026-09-10,
  FIX_PICK, REMEDIATION_2026-09-10, EXTERNAL_REVIEW_INTAKE_2026-09-12 — as a
  condensed index + citation map; subsumption proof: findings live in
  COMPILED_AUDIT §2-§5/§13/§15-§18 per its own §18 checklist, the files carry
  their own supersession banners, and full texts stay in git history at
  3c67e14) and docs/planning/PLANNING.md (absorbs OFFLINE_BUILD_REVIEW,
  CSHARP_SHELL_PLAN, SKILLOPT_INTEGRATION_QUERY, SEQUENTIAL_WORK_HANDOFF,
  NEXT_SESSION_PROMPT — decision-relevant content kept, prose condensed, prompt
  regenerated for post-S24). Merged into existing homes: FEASIBILITY_REVIEW →
  PROJECT_VISION.md (architecture + the flag-map table with VP-5's crop-form
  correction applied; verify_audit.sh E7 re-pointed — the only gate-script edit
  this session); WEB_FEASIBILITY → web/README.md §History; WHY_MSPL +
  COPYING_RULES + WASM_LICENSE_QUESTION → docs/legal/README.md §1-§4;
  CLEAN_WINDOWS_SMOKE + DESKTOP_PROBES → docs/ci/README.md §2-§3 (its §1
  rewritten for the resolved drift); THIRD_PARTY_NOTICES → web/wasm/README.md;
  csharp/spike/README → csharp/README.md (spike exit-code collision with the
  CLI noted → U-91). Rewritten brief: root README.md (session narrative out —
  it duplicated this log; honesty summary + new layout in), PROJECT_VISION.md
  (U-90's amendment-proposal block included, clearly marked NOT approved),
  WORKLIST.md (per-session history sections folded out; rules + board + S24
  intake pending lines kept), SESSION_HANDOFF.md (condensed history, PR #30
  ledger row + write-up, Docs-synced-through moved to PR #30 per P6, new doc
  map). Every live reference repointed; deleted filenames are cited WITHOUT
  backticks in current-state docs (the S21 G8 trick). Gate-excluded scripts'
  dead exclusion entries (check_docs/sweep/review_change) left untouched on
  purpose: patterns matching nothing are inert, and test_sweep_stale.py pins
  the intake-path exclusion fixture.
- **STATUS.md:** hand-block truth corrections above + repointed paths;
  regenerated with check_docs.sh --emit → **119 DONE · 8 PARTIAL · 41 OPEN · 0
  UNTRIAGED · 168 total** (was 112/8/28/0 = 148).

**Partial:**

- The consolidation is docs-complete but the branch's compile/web-suite proof is
  CI's job (no toolchain here). verify_audit.sh got exactly one edited line
  (E7's grep path); the line was executed standalone to confirm the string
  "1/100 s" lives in PROJECT_VISION.md.

**Left:**

- The 16 new OPEN rows untouched (fixing them was explicitly out of scope —
  this session was intake + consolidation; P1-44 has an executed repro and is
  the natural first fix). U-95's release-notes edit is an owner action. The
  register-mechanics rows (U-88/U-89) deliberately NOT implemented mid-
  consolidation. OD backlog unchanged. IMPROVEMENT_LOG's older entries left
  verbatim (append-only history; compressing them would destroy provenance the
  gates and future sessions cite).

**Verified:**

- check_docs.sh: green baseline on untouched main (23/0/3), then re-run after
  every batch and at the end (final numbers in the S24 handoff table); G0 diff
  clean after --emit; G2/G5/G5b green with the 20 new rows (emitter parsed all
  96 U-rows, sessions and tiers extracted correctly).
- sweep_stale.sh green; python3 working_code/gifscythe/tests/test_sweep_stale.py
  14 tests pass (the script's EXCLUDED regex untouched, fixtures still valid).
- Node probes executed: U-78 repro (validate.mjs returns zero issues for
  color_count:'abc', lossy:'lots', threads:'many', delay_cs:'soon'); GN-03
  disposition (resize fit 0x0 → issue, scale 0x0 → issue, crop 0x0 → none);
  GN-18 (stemOf('.gif')='.gif', stemOf('a.')='a').
- Engine-free web suites executed: request-guard, device-names, body-limit and
  static-hygiene all green; server-bounds 3/5 — its two failures are
  engine-gated (rate-limit case gets 503 engine-not-found, release/current pin
  case reports "Engine [none]"; both need the repo-built binary this sandbox
  cannot compile), not regressions: no web code was touched this session.
- GitHub API: no open PRs at session start; main runs — 3c67e14 green
  (35112077599), ff35227 + 4d49919 red on the linux doc gate (the G10-lag +
  stale-tally story recorded in the PR #30 write-up); releases — only
  snapshot-2026-09-07, created 2026-09-07, body names no licence (U-95
  evidence).
- Source reads on main for every registered finding: main.cpp explode blocks
  (U-81), SettingsIO/Validate/GifsicleCommand (F-02/03/04 already-fixed
  confirmations), server.mjs ordering + PORT + admission + stderr (U-84/85/92/
  93), app.js numeric fields (U-87), Program.cs (U-91), glue_harness.mjs
  (U-80 fixed), transport.test.mjs + d7f8ef9 diff (U-77/U-79 fixed).

**Not verifiable here:**

- No gcc/g++/cmake/Qt6/mingw/wine/dotnet/emcc/gh in this sandbox (node v20 +
  python3 + git only): nothing C++ was compiled or executed, so U-81/U-83/U-86
  and the already-fixed C++ confirmations are source-read (✅ SRC) only; the
  engine-dependent web suites (command/validate/transport) and smoke/unit/
  harness were NOT run; verify_audit.sh was not run end-to-end (capability
  skips); the Qt side of U-96 needs a Qt machine; U-95's zip contents were not
  downloaded (API metadata only). CI on this branch is the compile+suite proof.

**Docs touched:**

- COMPILED_AUDIT.md (v4: header/base fix, §1, §5 +20 rows, §6 +15 ids +
  P2-7/P2-17 notes, §10-§13 pointers, §15 provenance, §19.3/§19.4, §11 item
  12, new §20, footer), STATUS.md (hand block + re-emit), SESSION_HANDOFF.md
  (rewritten; PR #30 row + write-up; synced-through moved), WORKLIST.md
  (rewritten), README.md (rewritten), PROJECT_VISION.md (rewritten; absorbed
  the feasibility review), docs/archive/AUDIT_HISTORY.md (new),
  docs/planning/PLANNING.md (new), docs/ci/README.md (merged+rewritten),
  docs/legal/README.md (merged), docs/release/RELEASE_PROCEDURE.md (blockers
  table + refs), docs/planning/OWNER_DECISIONS.md (OD-08 resolved + refs),
  web/README.md (§History), web/WEB_PLAN_TEMPLATE.md (3 refs),
  web/wasm/README.md (notices folded), csharp/README.md (spike folded),
  working_code/gifscythe/README.md (1 ref), reference_code/REFERENCE_MANIFEST.md
  (stale scope note), verify_audit.sh (E7 path), docs/ci/build.yml.proposed
  (cygpath line re-synced); deleted: the four root review files,
  FEASIBILITY_REVIEW.md, the five docs/audit snapshots, the two docs/archive
  reviews, docs/web/WEB_FEASIBILITY.md, docs/ci/{CLEAN_WINDOWS_SMOKE,
  DESKTOP_PROBES,PENDING_WORKFLOW_CHANGE}.md, docs/legal/{WHY_MSPL,
  COPYING_RULES,WASM_LICENSE_QUESTION}.md, docs/planning/{CSHARP_SHELL_PLAN,
  OFFLINE_BUILD_REVIEW,SKILLOPT_INTEGRATION_QUERY,NEXT_SESSION_PROMPT,
  SEQUENTIAL_WORK_HANDOFF}.md, web/wasm/THIRD_PARTY_NOTICES.md,
  csharp/spike/README.md (names listed without backticks per the G8 rule; full
  texts in git history at 3c67e14).

---

## S23 — the Tier-1 batch: settings model, CLI honesty, web bounds, request ownership (2026-09-16)

**Changed:**

- **P0-2 (DS-06)** — `threads` is a tri-state in `src/core/GifsicleCommand.h` and
  `web/command.mjs`: `<0` emits nothing (the engine's single-threaded default),
  `0` emits a bare `-j` (auto = `GIFSICLE_DEFAULT_THREAD_COUNT`), `>0` emits `-jN`.
  U-03's fix had merged the first two, so the documented "unset" sentinel silently
  meant 8 threads. Sentinels are named `GS_THREADS_UNSET`/`GS_THREADS_AUTO`.
- **P1-28 (GS-206)** — every integer control parses with `std::from_chars` into its
  own width (`to_int_strict`/`to_uint_strict` in `SettingsIO.h`), replacing `long` +
  `static_cast<int>`; `Validate.h` + `web/validate.mjs` gained the missing domains:
  `loopcount` 0..65535 (measured: `--loopcount=65536` → "loop forever", rc=0),
  `color_method` and `resize_method` against the engine's registered lists, `gamma`
  shape (finite number | srgb | oklab — no range, because the engine accepts 0 and
  20 and a range here would be invented policy), `threads < -1`. `dither_method` is
  deliberately not enum-checked — the engine grammar is parameterised (`o8`, `o,4`,
  `ro64x64`) — and unit 21c pins that as a non-rule so the mirror cannot grow one.
- **P1-40 (U-63, loop half)** — `loopcount = -2` → `--no-loopcount`, in C++, the JS
  mirror and the web Loop control ("Play once"); `save_settings` writes any value
  `!= unset` so the state survives a round trip (the old `>= 0` guard would have
  turned "play once" back into "unchanged").
- **P1-13 (DS-12)** — `encode_line_value` quotes a value only when it would
  otherwise be lossy (leading/trailing space-or-tab, or a leading quote), escaping
  `"`/`\`; `decode_line_value` runs in exactly one place (`set_field`, which every
  caller funnels through, plus the GUI's unknown-key path) so it cannot apply twice.
- **P1-43 (U-73, U-74, U-76)** — `resolve_path` keeps the CWD fallback but reports
  it per input and `--strict` refuses it (new `resolve_path_mode`); `--run` refuses
  Batch with >1 input and one output (measured: `gifsicle -b a.gif b.gif -o out.gif`
  exits 0 and `out.gif` is a byte copy of `b.gif`, `a.gif`'s result existing
  nowhere) while the legal single-input shape still runs; explode with no prefix
  writes `<stem>_frame` in the CWD under `--run` only, so print mode and the
  JS⇄C++ parity stay exact.
- **P1-41 (U-65, U-66)** — `exe_path_of` asks the OS (`/proc/self/exe`,
  `_NSGetExecutablePath`, `GetModuleFileNameW`) before falling back to argv0 → PATH
  → CWD; `GS_ENGINE_CURRENT` (`release/current`) is checked before `release/<GS_VERSION>/`
  in `EngineLocator.h` and before newest-numeric in `web/server.mjs`.
- **P1-5 (U-06)** — `web/server.mjs` gained one engine semaphore for both endpoints
  (`GS_MAX_CONCURRENT`, `GS_MAX_QUEUED`, 429 + explanation past the cap, acquired
  after validation and released in `finally`), a per-client POST window
  (`GS_RATE_LIMIT_PER_MIN`, static exempt) and `GS_ENGINE_TIMEOUT_MS`.
- **P1-36 (U-56)** — `is_windows_reserved_device_name()` folds the superscript
  aliases U+00B9/B2/B3 after COM/LPT (not after CON, not full-width digits —
  nothing was measured to support those); COM0/LPT0 stay refused for asymmetric
  cost. New `tests/windows_reserved_names.txt` (27 rows) is read by the C++ unit
  case AND `web/test/device-names.test.mjs`, so the two surfaces cannot re-diverge.
- **P3-5 (DS-08)** — the advisory contract is documented in `--help` and a
  continued warned run ends with one `WARNING-SUMMARY: parse=N validation=M
  mode=advisory outcome=print|run-continued` line, placed after the strict refusal.
- **P1-34 (U-54) + U-69** — request ownership moved out of `web/app.js` into a
  pure module `web/request-guard.mjs` (generation + active-run + AbortController +
  launch-time settings snapshot); every control change invalidates and clears, and
  the failure path clears the previous success instead of leaving it on screen.
  `web/server.mjs` routes the new module and `web/test/static-hygiene.test.mjs`
  now derives app.js's imports and asserts each is listed and served — the class of
  bug where a security allow-list silently breaks the shipped page.
- **P3-12 (U-75)** — product README gained "What the engine can do that this
  control layer does not model" (the `-E`/`--name` positional interaction from
  `gifsicle.c:785-804`, the unsigned `crop_w/h` that cannot express a negative
  extend-past-edge span, colormap/extension surgery); the web `-E` control says in
  place why it can be a no-op.
- `check_docs.sh` G8 now allows `release/current` (a pin that is supposed not to
  exist in the tree) next to the existing `release/[0-9]*` allowance.

**Reviewing my own merged tree before opening the PR turned up two real bugs, and they are recorded here
rather than quietly amended.** `SettingsPanel.cpp` could not represent either sentinel this batch introduced:
`loopcount = -2` displayed as "Keep original" and was written back as `-1` by an ordinary close, and a conf's
`threads = -1` fell through `if (s.threads >= 0)` to the spinner's 0. Before P0-2 each rewrote a value into
one that meant the same thing; after P0-2 the threads half silently *added a `-j`*. That is the silent-rewrite
class DS-06 was filed under, in code I had just written — which is what a review step is for. The fix follows
P1-30's own prescription (‑1 as the spinner minimum, agreeing with DS-06) and appends a 4th `Looping` item,
because the harness addresses items 1 and 2 by index; the web side needed no change, so the reviewer's "Loop
count UI maps it incorrectly" half was true of the Qt GUI only and could not be credited to #28. Both halves
And the review's own fix needed a CI iteration, which is worth recording because it is the
harness's rule, not mine: the first version persisted by `delete w`, which never reaches
`MainWindow::closeEvent` (persistence lives there), so the block compiled clean in CI, ran, and
failed step 9 while writing nothing. Step 4's success is still useful evidence — it means both Qt
files compile with the new `data() == 3` item and the ‑1 spinner end on Qt 6 on Linux and on
Windows. The corrected block closes the window the way T14 does, and the comment in it says why.
**Partial:** **U-76** — the prefix NAME is shared across surfaces, the DIRECTORY
is not (the scoped wording wrote 12 frames into `reference_code/`), so the choice
is filed as owner decision **OD-18**. **U-67** — the engine-gated
`transport.test.mjs` no-regression re-run is done (69 cases green on the merged tree)
and #28's `W4/W5` CI wiring has since landed, so the register carries U-67 as
**FIXED (S22)**: with both halves in one tree there is nothing left to mark partial. **P1-40** — crop half landed in PR #28, loop
half here; **P1-41** — U-64 landed in PR #28, U-65/U-66 here.

**Left:** every Qt-bound row (`U-58`, `U-59`/P0-7, `U-70`, `U-72`, GS-205,
DS-10, GS-203/204/210 remainders) — no Qt6 or cmake in this sandbox; **P1-35**
(U-55), unverifiable without a Windows host; `U-68`'s numeric cap (documented and
left as-is deliberately); CI/gate wiring (P2-7/GS-208, P2-1); `OD-03…OD-10`,
`OD-13…OD-16`, `OD-18`; the whole deferred bucket.

**Verified (re-measured after the PR #28 merge, see below):** `./build.sh` →
**372 checks, 0 failures** (296 on arrival, 308 in PR #28; +62 from this batch's
threads/loop/sentinel, round-trip, parsing-width and device-name-table blocks, +2
from extending PR #28's `threads` pin to the shipped tri-state).
`scripts/smoke_cli.sh` → **54 passed, 0 failed** (40 on arrival, 45 in PR #28;
12g threads, 12h
play-once end-to-end against the real engine, 12i oversized-int refusal, 12j CWD
input, 12k/12l Batch refuse + must-not-over-refuse, 12m summary presence/absence,
12n symlink install, 12o current pin; case 15 rewritten for the new explode
prefix). `node web/test/request-guard.test.mjs` 13 assertions,
`web/test/device-names.test.mjs` 27 rows, `web/test/server-bounds.test.mjs` 5
groups, `web/test/command.test.mjs`, `web/test/validate.test.mjs` (with the new
`expect:` mechanism so two empty lists can no longer pass as parity),
`web/test/transport.test.mjs` (69 PASS lines), `web/test/static-hygiene.test.mjs`
(48 — the same number `verify_audit.sh` **W5** reports), `web/test/body-limit.test.mjs`
all green. The three new suites are then wired into the linux CI job's web step
(and the `docs/ci/build.yml.proposed` copy, kept in step the way #28 left it) and
into `verify_audit.sh` as **W6** (18 PASS lines), so CI runs all eight — a web
regression no gate executes is not a regression test, which is the exact finding
#28 had just closed for its own two suites, and the reason this batch did not
leave its own three as repo-only files. `scripts/check_docs.sh` green;
`scripts/verify_audit.sh` **30 passed / 0 failed / 6 skipped** in this sandbox (the skips
are Qt/CMake, clean-Windows and the declared workflow item). Engine probes for every claim
quoted above were run on the bundled 1.96 build.

**Reconciled with PR #28 (merged into `main` as `794a996` while this batch was in
flight).** S23 was written against the pre-merge base `6cd7c7b`, so the whole batch
was replayed onto `main` and the overlaps resolved rather than stacked:

- **Adopted #28's names, not mine.** It had landed its own `is_special_input_token()`
  / `is_stream_output_token()` helpers for the same U-60/U-61 tokens S23's P1-43
  touched, plus a simpler `exe_path_of()` and the `INFO_UNSUPPORTED` 400 in
  `web/server.mjs`. The PR keeps **its** helper names (their callers are already
  merged) and its crop-`0x0` and info rules; only the `exe_path_of` body is replaced,
  because S23's U-65 version asks the OS (`/proc/self/exe`, `GetModuleFileNameW`)
  where #28's only tried argv[0] and the CWD.
- **Its `threads` pin was superseded, not duplicated.** #28's unit block 21b asserted
  `threads=-7` → bare `-j`, the pre-P0-2 builder behavior — the opposite of the
  shipped contract. The block keeps #28's warning checks and gains builder checks for
  all three states (`-7` says nothing, `-1` says nothing, `0` is the only `-j`). Same
  for the `threads < -1` message: #28 wrote it as documentation, P0-2 makes it true.
- **Its `U-67` finding text was restored to the narrowed S21 wording** (#28 had
  reverted it to the pre-measurement claim) while keeping #28's FIXED (S22) status
  cell, so the register credits the merge that actually closed it.
- **`WORKLIST.md` DS-08 and DS-09 are both closed** — DS-08 by S23's `WARNING-SUMMARY`
  line, DS-09 by #28's validation rule, with a note that the builder half of #28's pin
  was superseded. The two `smoke_cli.sh` case sets stay side by side (12b–12f and
  12g–12o) and every count above was re-measured on the merged tree, not added.

**Register after the merge: 110 DONE · 8 PARTIAL · 29 OPEN · 0 UNTRIAGED · 147
total** (S23 alone reached 104/9/34 on the pre-merge base; the five rows #28 closed
account for the rest).

**Not verifiable here:** the Qt6 GUI **could not be compiled or run by me at all**
(no cmake/Qt6), so `test_gui_offscreen`, the GUI build and both S23 Qt edits are attributed to CI
— the rule S8 used for its T8 rewrite — and DS-07 closes on that basis with no harness count
claimed. Windows/macOS behaviour (no
Wine/mingw: the `release/current` pin on a Windows path, `GetModuleFileNameW`,
the U-55 case fold, and whether the Win32 reserved-name check really folds ¹²³ —
the code follows the finding's claim and the shared table does not depend on it),
any `gh release`/workflow write, browser rendering of `app.js` (the *rule* is
tested in the module, the wiring is verified only by `node --check` and by
static-hygiene's routability assertions), and clean-machine packaging.
`verify_audit.sh` here reports its own toolchain SKIPs (C6/C7*/C9/B/Windows) —
its full-toolchain totals are quoted in `WORKLIST.md` from the last measured
checkpoint and were not re-measured, because this sandbox cannot measure them.

**Docs touched:** `COMPILED_AUDIT.md` (§5 status cells + the §2E/§2F narrative
`Status:` lines for U-06/54/56/63/65/66/69/73/74/75/76), `STATUS.md` (hand rows
DS-06/DS-08/DS-12/GS-206 → DONE, new N-08 row, W-03/W-04 counts, then
`--emit`), `WORKLIST.md` (five pending lines ticked with their proof, S23
section, build commands), `SESSION_HANDOFF.md` (S23 section, register tally,
gate baseline, the PR #28 reconciliation notes), `docs/planning/OWNER_DECISIONS.md`
(new **OD-18**), `README.md`, `web/README.md`, `working_code/gifscythe/README.md`,
`docs/release/RELEASE_PROCEDURE.md`.

## S22 continuation — closed U-53/U-60/U-61/U-62/U-64 and DS-09 on this branch (2026-09-16)

**Changed:**

- `src/core/SettingsIO.h` now treats frame position as an all-or-nothing pair even when one coordinate parses and the other does not: `set_field()` no longer flips `has_position` on a single successful half, and `load_settings()` only enables the pair when both keys were seen and both parsed. Otherwise it emits one pair-level warning and clears both coordinates.
- `src/cli/main.cpp` now preserves gifsicle special tokens instead of path-resolving them: input selectors like `#0` and stdin `-` stay literal, `output = -` stays stdout/streaming, stream outputs bypass file planning/verification, and the planner ignores non-path inputs.
- `src/core/Validate.h` and `web/validate.mjs` now accept crop width/height `0` (the engine's "extend to edge" syntax) and warn on `threads < -1` in lockstep.
- `web/server.mjs` now rejects `info:true` early with a clear HTTP 400 on both `/optimize` and `/run`, instead of falling through to GIF verification and misreporting a 422 with `exitCode: 0`.
- Regression coverage landed first in the native unit suite, `scripts/smoke_cli.sh`, `web/test/command.test.mjs`, `web/test/validate.test.mjs`, and `web/test/transport.test.mjs`.

**Partial / left:**

- `U-63` (`--no-loopcount` / play once) remains open, so fix-order row **P1-40** is now only **PARTIAL S22**.
- `U-65` (CLI symlink/PATH engine-beside-executable discovery) and `U-66` (desktop/web version-policy split) remain open, so **P1-41** is also **PARTIAL S22**.
- The other E/F intake rows (`U-54`..`U-59`, `U-63`, `U-65`, `U-66`, `U-69`..) are untouched here.

**Verified:**

- `working_code/gifscythe/./build.sh` → **308/308 PASS**
- `node web/test/command.test.mjs`
- `node web/test/validate.test.mjs`
- `node web/test/transport.test.mjs`
- `working_code/gifscythe/./scripts/smoke_cli.sh` → **45 passed / 0 failed**

**Docs touched:** `COMPILED_AUDIT.md`, `STATUS.md` (re-emitted after the row-state updates), `SESSION_HANDOFF.md`, `IMPROVEMENT_LOG.md`, `WORKLIST.md`, `web/README.md`, `working_code/gifscythe/README.md`, `docs/planning/NEXT_SESSION_PROMPT.md`.

---

## S22 — PR #26/#27 automation gap fixed; PR #27 web files ported to this branch (2026-09-16)

**Changed:**

- Ported the PR #27 web changes from `origin/main` onto this branch before fixing the review finding, because the checkout had stopped at the PR #26 merge `d1d7939`: `web/server.mjs` now has the static allow-list + explicit HEAD/static contract, `web/test/static-hygiene.test.mjs` is present, and `web/wasm/README.md` now states that `web/server.mjs` intentionally returns **404** for `wasm/`.
- Fixed the delivered review finding: `.github/workflows/build.yml` and `docs/ci/build.yml.proposed` now run **all five** Node web suites, adding `web/test/body-limit.test.mjs` and `web/test/static-hygiene.test.mjs` to the existing command/validate/transport block.
- `working_code/gifscythe/scripts/verify_audit.sh` gained **W4** (oversized-body 413 regression) and **W5** (static allow-list / HEAD contract regression), so the two new web regressions are covered by the local one-command suite as well as CI.
- Audit sync: `COMPILED_AUDIT.md` now records **U-67 FIXED (S22)** and **U-68 PARTIAL (S22)**, and the handoff header / PR ledger were advanced through merged **PR #27**. `STATUS.md` was re-emitted from the audit after the row-state change.

**Partial:**

- **U-68** stays PARTIAL. The 413 mapping is now re-proven and automated, but the limit value itself is still the existing **64 MB HTTP-envelope cap** (about **48 MB effective decoded GIF** for `/run`), documented in source rather than changed.

**Left:**

- **U-69** (stale After image / stale result UI) remains untouched.
- The workflow-copy marker in `docs/ci/PENDING_WORKFLOW_CHANGE.md` still exists for the older declared drift; this session only kept the two workflow copies aligned on the newly-added web suites.

**Verified:**

- `working_code/gifscythe/./build.sh`
- `node web/test/command.test.mjs`
- `node web/test/validate.test.mjs`
- `node web/test/transport.test.mjs` → **67/67 PASS**
- `node web/test/body-limit.test.mjs` → **8/8 PASS**
- `node web/test/static-hygiene.test.mjs` → **43/43 PASS**
- `working_code/gifscythe/./scripts/verify_audit.sh` — final rerun **29 passed / 0 failed / 6 skipped** in this sandbox; new **W4/W5** both PASS. The first run failed only because `check_docs.sh` correctly rejected the dirty tree during the in-progress edit set (G18), not because of the new web checks.

**Not verifiable here:**

- Full Qt/CMake GUI verification and Windows runtime behavior are still outside this sandbox's toolchain. `verify_audit.sh` still SKIPs the same CMake/Qt and clean-Windows items here.

**Docs touched:** `COMPILED_AUDIT.md`, `STATUS.md` (re-emitted), `SESSION_HANDOFF.md`, `IMPROVEMENT_LOG.md`, `.github/workflows/build.yml`, `docs/ci/build.yml.proposed`, `working_code/gifscythe/scripts/verify_audit.sh`, `web/server.mjs`, `web/test/static-hygiene.test.mjs`, `web/wasm/README.md`.

---
## S21 — COMPILED_AUDIT v3 consolidation + U-68/NF-11 oversized-body 413 (2026-09-15)

**Changed:**

- Docs — `COMPILED_AUDIT.md` → **v3**. The owner pointed at the closed branch
  `codebase-review-and-fix-implementation-b8d7e` (it still carries
  `AUDIT_A_extracted.md` / `AUDIT_B_extracted.md`) and asked whether the
  compilation was missing anything. Checked ID-by-ID and body-by-body: all 36
  A+B findings and all 24 E+F findings were already present and faithful, so the
  register stayed 76. What v2 had *dropped* was non-finding content, recovered
  here — **§15** the VP-1..VP-5 + false-positive guardrails (§12 cited
  "VP-1/2/3/5" but v2 never defined them), **§16** Audit A/B positives + method +
  fix-order rationale, **§17** intake E/F verdicts + delivery paths + the 19
  regression cases, **§18** the merge-completeness checklist, **§19** the
  next-session review ask. The two scattered root intake copies were folded in
  and removed (one was actively failing gate **G17/S2** on a stale tally); their
  text survives in git history at `c4f9e1c`.
- Code — one task, the highest-confidence one this sandbox can actually prove:
  **U-68 / NF-11**. An oversized HTTP body answered `400 "bad JSON request body"`
  on `/run` and `500` on `/optimize` instead of `413`. In `web/server.mjs`,
  `readBody` now rejects with a typed `BodyTooLargeError` (`statusCode 413`) and
  stops accumulating without destroying the socket; both handlers map that tag to
  a real `413` via `sendTooLarge` (destroy-after-flush), kept distinct from a
  `400` parse error; `GS_MAX_BODY` injects the limit for tests; the 64 MB
  envelope ≈ 48 MB effective decoded GIF is documented at the constant.

- Code — second task, same session: **U-67 / NF-10** (`serveStatic` hygiene),
  **measured before fixing**. Probing the live server with raw un-normalised HTTP
  (`http.request`, because `fetch()` normalises the URL before it goes on the
  wire) showed only **one of the finding's three sub-claims reproduced**:
  "every file under `web/` served" is TRUE (`/server.mjs` → 200 / 26 KB,
  `/test/transport.test.mjs` → 200 / 35 KB, plus `run-paths.mjs`, `validate.mjs`,
  `output-verify.mjs`, `README.md`, `WEB_PLAN_TEMPLATE.md`, `wasm/*`);
  "raw prefix containment" is OVERSTATED (`new URL()` collapses dot-segments
  first, so `/../STATUS.md`, `/../../etc/hostname` and
  `/../working_code/gifscythe/VERSION.md` already returned 404 — `/../server.mjs`
  returned 200 only because it normalises to `/server.mjs`, inside ROOT, and the
  intake text itself conceded "not exploitable today"); "HEAD returns a body" is
  **FALSE** (measured `bodyLen=0` — Node suppresses HEAD bodies; proven at socket
  level, server wrote 5000 bytes and the client received 0). Two confident-looking
  fixes for non-bugs avoided.
  Owner decisions taken first: allow-list is **UI-only** (this Node server is the
  supported shipped surface; `web/wasm/` is experimental and not shippable), and
  **U-67 is corrected to match the measurements** rather than preserving the
  inaccurate three-part wording — §2F keeps the intake text verbatim for
  attribution and gains a per-claim measurement table, §5 states only what is true.
  `web/server.mjs`: `STATIC_FILES` allow-list of the four files the UI actually
  loads (`/`→`index.html`, `/index.html`, `/style.css`, `/app.js`,
  `/command.mjs`) — a closed set verified from `index.html`'s only two asset
  references and `app.js`'s only import (`./command.mjs`, a leaf module);
  everything else under `web/` → 404; `assertContainedPath(ROOT, …)` replaces the
  raw `startsWith` prefix check as defence in depth (same resolved-path
  containment `run-paths.mjs` enforces on engine outputs, already unit-tested by
  `transport.test.mjs`); one `sendStatic()` path gives 200/403/404 a single
  explicit HEAD contract with `Content-Length`; `serveStatic()` gained a `req`
  parameter and `normalize` is no longer imported.

**Why these two tasks:** the sandbox has **no compiler** (node v20.20.2 / python3 / git
only — no gcc/g++/cmake/Qt6/mingw/wine/emcc), so every C++/CLI/Qt/Windows/wasm
finding is unprovable here and the existing web suites cannot run (command and
validate spawn the C++ CLI; transport needs a discoverable engine; glue needs
emcc). Both U-68 and U-67 are HTTP-transport/static-serving concerns — the
desktop has no HTTP server, so there is **no C++ parity mirror to diverge from**
(unlike a JS-only `validate.mjs` or `command.mjs` fix, which would silently break
parity fixtures this sandbox cannot run). And both are provable with no engine:
`/run` reads the body before `findEngine()`, and `serveStatic` never calls it.

**Partial:** U-68 and U-67 are both **PARTIAL**, not DONE. Each is
executed-proven for its own behaviour, but the full `web/test/transport.test.mjs`
no-regression re-run is engine-gated (no gifsicle buildable here) and neither new
test is wired into CI yet. For U-68 the numeric cap is also documented rather than
changed (the owner may want a deliberate value). For U-67 the transport suite's
only static dependency is the readiness `GET /`, which is preserved and
byte-exact verified. U-69 (stale After image) — the last third of fix-order row
**P2-16** — is untouched: it is browser-DOM behaviour this sandbox cannot
exercise.

**Left:** execute `COMPILED_AUDIT.md` §19 in a tooled session (re-prove every
fixed row, failing-test-first, the new-pit pairings, and measure each sub-claim
before fixing it); close U-69 (browser-DOM, not exercisable here); finish U-67
and U-68 to DONE via the engine-gated `transport.test.mjs` re-run; wire both
`body-limit.test.mjs` and `static-hygiene.test.mjs` into CI (`build.yml` needs
`workflows` scope + has a byte-identical twin copy; `verify_audit.sh` needs a
`DOC_GATE_CHECKS` bump for the new W-gates). The owner
approved opening the PR after this entry was first written, so it is now open
from branch `audit/compiled-v3-consolidation`. This session also completed the
**PR #25 doc sync** that `034ad65` had landed without (ledger row + the
`Docs synced through:` line moved to #25 + the S21 header), which is what
`pr_preflight.sh --online` step **P6** requires before merge. Branch
`audit/compiled-v3-consolidation`.

**Verified:**

- `web/test/body-limit.test.mjs` **8/8, red to green**: stashing the `server.mjs`
  fix reproduces `/run 400` and `/optimize 500` where 413 is expected, while the
  4 control cases still pass (so the test isolates the bug instead of passing
  vacuously); restoring the fix turns all 8 green. No real engine was used —
  `/run` is pre-discovery, and `/optimize` reaches `readBody` through an inert
  `GS_ENGINE` stub (`process.execPath`) that is never executed because the
  oversize rejection precedes `run()`.
- `web/test/static-hygiene.test.mjs` **43/43, red to green**: stashing the fix
  reproduces **18** failures (every over-exposed path, the `server.mjs`
  disclosure check, and the two traversals that normalise to a file inside ROOT)
  while the UI byte-exact cases, the HEAD-contract cases and the above-ROOT
  traversal cases already passed — so the test isolates the real defect rather
  than the two sub-claims that did not reproduce. Asserts the four allow-listed
  assets are served **byte-exact** against the on-disk files with correct MIME,
  that `index.html`'s `style.css`/`app.js` references and `app.js`'s
  `./command.mjs` import still resolve (so the allow-list cannot silently break
  the shipped page), 14 internal paths 404, six raw traversals never yield a
  body, HEAD is empty-bodied with headers intact, and `POST /run` / `PUT /` are
  unaffected. `body-limit.test.mjs` re-run afterwards: still 8/8.
- `web/wasm/README.md`'s "use any static server rooted at `web/`" advice
  re-verified by execution (`python3 -m http.server -d web`): `/wasm/index.html`
  200 and its `../style.css` / `../command.mjs` / `../validate.mjs` imports all
  resolve. That page needs `validate.mjs`, which the shipped UI does not — the
  concrete reason it must not be routed through the allow-listed server.
- `check_docs.sh` 23/0 after `--emit` regenerated `STATUS.md` (89 DONE · 8
  PARTIAL · 49 OPEN · 0 UNTRIAGED · 146 total); `sweep_stale.sh` green. The
  consolidation also cleared the G17/S2 failure that was live on `main`.

**Not verifiable here:** the full transport and parity suites, every
C++/Qt/Windows/wasm row, the `/optimize` 413 against a *real* engine, and whether
the cap value should change — no compiler, no engine, no Qt, no Windows, no emcc.

**Docs touched:** `COMPILED_AUDIT.md` (v3 — new §15–§19; §5 U-67 + U-68; §2F
F-10 + F-11; §6 P2-16; §10; §11 items 10–12; §12; §19 measure-before-fixing
mandate), `STATUS.md` (re-emitted), `SESSION_HANDOFF.md` (S21 section, tally,
PR #25 sync + ledger rows #25/#26), `docs/planning/NEXT_SESSION_PROMPT.md` (§19
ask), `web/wasm/README.md` (the server does not route `wasm/`, by design),
`web/server.mjs`, `web/test/body-limit.test.mjs` (new),
`web/test/static-hygiene.test.mjs` (new).

---

## S20 — Windows-only product, Linux demoted to test rig (2026-09-14)

**Changed:**

- Direction (owner, `OD-17 = a`): the shipped product is Windows-only
  (exe) + web app. Linux stays as the CI job and sandbox scripts — the
  automated test battery — and ships nothing. The Linux release zip and
  the `gifscythe-linux` CI upload are deleted; the top README no longer
  presents Wine emulation as Windows proof.
- CI (`build.yml` live + proposed, identical — the 1-line cygpath drift
  is untouched): the linux job keeps build/test/package/assert and drops
  its upload step; the windows job gains `package_portable.sh --windows`
  plus a manifest assert over the staged `.exe` set + licence texts
  (the shipped folder rides to `gifscythe-windows` inside `release/`).
  The negatives suite stays on Linux: its tools-dir uses symlinks, which
  stock Windows runners cannot create — porting it is follow-up work for
  a Windows-iterated session, not this one.
- Release: `RELEASE_PROCEDURE.md` releases only
  `gifscythe-<ver>-windows.zip`; CI §3 re-titled (Windows ships, Linux
  tests); the pending-marker's drift line number corrected 178→171 (the
  step removal shifted it; the drift itself is unchanged).

**Partial:** the new Windows packaging steps have never run — no Windows
exists in this loop, so the first green is pending CI observation after
the push (`GS-204` stays PARTIAL regardless; clean-machine proof and
architecture checks still need the equipped agent).

**Left:** observe the Windows CI green (or fix what it finds); port the
negatives suite to Windows runners (symlink-free tools dir); the
clean-Windows smoke + desktop probes; `OD-16`; blockers
`GS-204`/`GS-208`/`U-09`/`DS-06`.

**Verified:**

- `diff` of the two workflow copies shows only the cygpath line; the new
  steps' indentation shape matches the proven Linux assert block
  line-for-line (no YAML parser in this sandbox, so structural mirroring
  + `git diff` review instead).
- `test_package.sh` re-run **36 passed, 0 failed** (packager untouched,
  stager still green); `check_docs.sh` + `sweep_stale.sh` green;
  `review_change.sh` on the range clean. `git status` clean at commit.
- Post-push, same session: Windows CI run `34812043127` green (linux +
  windows + spike; new package/assert steps green on a real runner;
  artifacts confirm no Linux upload). `GS-204` proof updated, still
  PARTIAL — architecture checks and clean-machine proof remain.

**Not verifiable here:**

- The Windows CI verdict on this commit (new steps never executed on a
  real runner; this sandbox is Linux-only).
- Everything S19 already listed: clean-Windows smoke, desktop probes,
  real-Qt packaging proof, the `OD-16` answer.

**Docs touched:** `docs/planning/OWNER_DECISIONS.md` (OD-17),
`docs/release/RELEASE_PROCEDURE.md` (windows-only release),
`.github/workflows/build.yml` + `docs/ci/build.yml.proposed` (linux
upload dropped, Windows packaging gates),
`docs/ci/PENDING_WORKFLOW_CHANGE.md` (drift line number),
`docs/ci/README.md` (artifact list), `README.md` (Wine reframe),
`STATUS.md` (--emit stamp; W-06/W-07 notes), `WORKLIST.md` (W-07 box),
`SESSION_HANDOFF.md` (S20), `IMPROVEMENT_LOG.md`,
`docs/planning/NEXT_SESSION_PROMPT.md` (S20 refresh).

## S19 — exe stays C++/Qt6 (park C#, close U-08) + wasm MVP scaffold, unproven (2026-09-14)

**Changed:**

- Direction (owner): the exe stays C++17/Qt6, no rewrite. C# shell parked
  (`OD-C7 = park` in `docs/planning/CSHARP_SHELL_PLAN.md` §8): plan
  `PARKED`, `csharp/spike/` inert but still CI-run, offline-review §4
  reinstated. The S18 "Phase 2 GO" verdict is suspended, not deleted.
- `U-08` closed (was the last licence-set remainder): verbatim
  `COPYING.lgplv3` + `COPYING.gplv3` staged by both packagers
  (`package_common.sh`), generated `QT_NOTICE.txt` in GUI packages only
  (version via `qmake -query`, `unknown` fallback), packaging suite 36/36
  (4 new required-file negatives fire by name), CI manifest (live +
  proposed, 1-line cygpath drift preserved) and `verify_audit.sh` D1/D2
  assert the set. Release blockers five → four.
- Owner answers executed: `OD-11 = a` (stay 0.1.0 — `W-29` stays OPEN, no
  version file touched) and `OD-12 = a` (two-way CLI out — `W-26` DONE, no
  code change). `OD-16` added for the wasm in-process licence question
  (`docs/legal/WASM_LICENSE_QUESTION.md`, open, blocks shippable).
- Desktop evidence docs: `docs/ci/CLEAN_WINDOWS_SMOKE.md` re-pointed at a
  green CI artifact (run id + commit recorded; banked snapshot disqualified
  — it cannot validate the current tree); new `docs/ci/DESKTOP_PROBES.md`
  procedures the three W-19 probes (external engine kill, physical
  drag-drop, engine-missing GUI) against the real `MainWindow` branches.
- Wasm MVP scaffold (`web/wasm/`, additive — `web/command.mjs`,
  `web/validate.mjs`, server, page untouched): `build_wasm.sh` (emcc,
  single-threaded `config.wasm.h`, same source list as the native engine,
  MEMFS only, stages `COPYING.gifsicle`), one-screen `index.html` +
  `wasm.js` (main-thread sync `callMain`, no Worker; 6 setting groups;
  live pane and run refusal via the verbatim builders), `prove_wasm.mjs`
  byte-proof script, `THIRD_PARTY_NOTICES.md`. Register now 89/7/26/0
  over 122 rows.
- Sync: PR #22 (`f760ebe`, S17 work) + PR #23 (`8230247`, S18 work)
  reviewed from history after landing with no doc sync; ledger, `Docs
  synced through:`, and header base moved to #23.

**Partial:** the wasm track is scaffold, not proof — no `.wasm` binary is
built anywhere (see Not verifiable here), and `OD-16` is unanswered, so
the Node server stays the shipped web path. `GS-204` stays PARTIAL (real
Windows/Qt deployment + clean-machine proof still need the equipped
agent). `D-07` stays OPEN.

**Left:** `OD-16` answer (owner/counsel); the first emcc run of
`build_wasm.sh` + `prove_wasm.mjs` printing real proof bytes; the
clean-Windows smoke + desktop probes on real hardware; remaining release
blockers `GS-204`/`GS-208`/`U-09`/`DS-06`.

**Verified:**

- `./build.sh`: engine `LCDF Gifsicle 1.96` + CLI + unit suite
  **296 checks, 0 failures**; `test_engine.sh` 5/5; `smoke_cli.sh` 40/40;
  `test_package.sh` **36 passed, 0 failed** (new `COPYING.lgplv3` /
  `COPYING.gplv3` missing-cases fail by name, both kinds).
- Real packager: headless bundle carries both texts; hiding
  `COPYING.lgplv3` makes `package_system.sh` ERROR with no stale
  directory; GUI-scope run generates `QT_NOTICE.txt` in both branches
  (`unknown` without qmake, `6.4.2` with a stub qmake6); headless runs
  omit the notice (pinned by the suite).
- Web suites (untouched code, re-run): command ALL PASSED, validate ALL
  PASSED, transport ALL PASSED.
- Wasm glue (scratch stub-DOM harness, real engine behind the fake
  module): live pane `gifsicle -O3 -j logo.gif -o logo_opt.gif`,
  `callMain` argv exits 0, output 8703→8637 B with GIF magic, savings +
  download render, out-of-range lossy refused with the validator's
  wording. `node --check` on both scripts; `build_wasm.sh --help` rc=0,
  `--bogus` rc=2, no-emcc rc=1; `prove_wasm.mjs` without `dist/` fails
  rc=1 naming the build step; all 24 JS-referenced element ids exist in
  the page.
- Native oracle for the future proof: `logo.gif` 8703 B → 8637 B under
  `-O3` (GIF89a, 12 images, 60x132), → 4106 B under `-O3 --resize-fit
  30x66`.
- `check_docs.sh` + `sweep_stale.sh` green (S2 stale handoff tally
  corrected in-session; G10/G15 fixed: base re-synced to `8230247`,
  hooks bootstrapped). `git status` clean at commit.

**Not verifiable here:**

- CI's verdict on this commit (live workflow manifest change included —
  needs a `workflows`-scoped push; staged separately at push time).
- The Emscripten build itself: `./emsdk install latest` fails in this
  sandbox downloading from `storage.googleapis.com` (TLS EOF; host
  unreachable, `nodejs.org` likewise), so `build_wasm.sh` never ran
  green and no `.wasm` bytes exist yet.
- Real-Qt packaging (windeployqt runtime, `QT_NOTICE.txt` with a true Qt
  version), the clean-Windows smoke, and the three desktop probes — all
  need Windows/Qt hardware this sandbox lacks.
- The `OD-16` licence answer (owner/counsel), and any counsel review of
  the staged Qt notices.

**Docs touched:** `COMPILED_AUDIT.md` (U-08 narrative + §5 row to FIXED;
P1-26 + §13 GS-204 counts 32→36), `STATUS.md` (--emit: U-08 DONE;
hand block: W-08/W-26/W-29/D-07/GS-204), `WORKLIST.md` (S19 section,
GS-204 count, parked spike, OD answers, wasm note), `SESSION_HANDOFF.md`
(S19 header/sync/ledger/sections/tally/toolchain), `IMPROVEMENT_LOG.md`,
`docs/planning/OWNER_DECISIONS.md` (OD-11/OD-12 answers, OD-16),
`docs/planning/CSHARP_SHELL_PLAN.md` (PARKED + OD-C7),
`docs/planning/SEQUENTIAL_WORK_HANDOFF.md` (GS-204 count),
`docs/planning/NEXT_SESSION_PROMPT.md` (S19 refresh), `docs/legal/`
(WASM_LICENSE_QUESTION.md new; WHY_MSPL.md pointer; README.md state),
`docs/release/RELEASE_PROCEDURE.md` (blockers four, §4 file list),
`docs/ci/CLEAN_WINDOWS_SMOKE.md` (CI-artifact asset),
`docs/ci/DESKTOP_PROBES.md` (new), `README.md` (licence lines, parked
spike), `LICENSE` (Qt texts pointer), `web/wasm/` (new track).

---

## S18 — Phase 1 spike GREEN, Phase 2 GO (2026-09-14)

- Run `34804350470`: `csharp-spike` all green, zero skips — happy path,
  é+space, honest 0/2/3/4/5 incl. both lying-engine exit-5 cases,
  self-contained single-file publish runs on the stock runner.
- The spike's first red was decision-grade, not waste: the combined é+CJK
  step re-proved the engine-ACP residual (`src/core/WinUnicode.h`) from C#,
  correcting the plan's own acceptance criterion (CJK now pinned to fail
  honestly; the step flips red if the residual ever closes).
- Two CI-harness gotchas, both fixed without touching the C# source (first
  compile passed untouched): `command -v true` returns the shell builtin,
  so the lying-engine lookup must use `type -P`; and multi-path
  `upload-artifact` rooting must never be assumed — `find` the engine.
- First native-Windows é-path proof anywhere in the repo (U-07's é evidence
  is Wine-only). Verdict recorded in plan §4: **Phase 2 GO**.

## S18 — Phase 1 spike scaffolded (csharp/spike/ + CI job) (2026-09-14)

- `csharp/spike/`: net9.0 console, hardcoded optimize-3 settings → argv,
  real engine spawn via an argv array (never a shell), GIF87a/89a output
  verify, honest exit codes 0/2/3/4/5.
- `csharp-spike` CI job (live + proposed `build.yml`, drift still one line):
  happy path, Unicode + space paths, every failure code incl. lying-engine
  exit-5 cases, then a self-contained single-file publish that must be one
  `.exe` over 5 MB and must run.
- Outcome pending: first CI run decides Phase 2 go/no-go. A red run is a
  valid, decision-grade result, not a failure to hide.

## S18 follow-up — licence story consolidated into docs/legal/ (2026-09-14)

**Changed:**

- New `docs/legal/` folder (owner request): `README.md` (licence single source of truth + maintenance contract), `WHY_MSPL.md` (full OD-09 = b / OD-C6 rationale, rejected alternatives, accepted costs, one-way-door warning), `COPYING_RULES.md` (per-file fork-copying checklist). Live docs now point here instead of carrying their own rationale prose: vision UI-approach, plan §1.4, offline S18 note, OD-09 answer, `LICENSE` pointer line.
- Stale/confusing leftovers fixed: README auto-fetch line no longer lists the retired Caesium trees; nested `reference_code/.gitignore` drops `caesium-source/` (guard comment forbids re-add); manifest fetch note rewritten (retired absence is policy, not fetch-away). Dated history (old log entries, dated reviews, audit evidence, intake reports) deliberately left verbatim as evidence.

**Partial:** none — docs only.

**Left:** spike session (unchanged).

**Verified:**

- `check_docs.sh --emit` re-run (register unchanged), then `check_docs.sh --no-gate-run` green with zero failures; `sweep_stale.sh` clean. New files written gate-aware (existing repo paths only, no counts, no volatile phrasing).

**Not verifiable here:**

- CI's verdict on this commit (not yet pushed at write time).

**Docs touched:** `docs/legal/README.md`, `docs/legal/WHY_MSPL.md`, `docs/legal/COPYING_RULES.md`, `LICENSE`, `PROJECT_VISION.md`, `README.md`, `docs/planning/OFFLINE_BUILD_REVIEW.md`, `docs/planning/OWNER_DECISIONS.md`, `docs/planning/CSHARP_SHELL_PLAN.md`, `reference_code/REFERENCE_MANIFEST.md`, `reference_code/.gitignore`, `WORKLIST.md`, `SESSION_HANDOFF.md`, `IMPROVEMENT_LOG.md`.

---

## S18 follow-up — workflows scope verified, relicense CI success (2026-09-14)

**Changed:**

- Run `34801397493` (relicense commit `161e862`) concluded success on both jobs (2026-09-14): the manifest step asserts `COPYING.ms-pl` in the package, and the packaging-negative step passes with both new required-file cases (fixture-pure, run before any artifact-dependent case in each kind), confirming the 30 — 32 count by construction plus the executed portable delta. Pre-push hook green 23/0/3. The `build.yml` push itself is the workflows-scope proof, so the pending marker now tracks only the 1-line cygpath drift.

**Partial:** none — close-out entry.

**Left:** spike session (unblocked: workflow edits push cleanly).

**Verified:**

- `gh run view 34801397493` conclusion success (linux + windows); `gh run watch --exit-status` 0; `check_docs.sh --no-gate-run` green with zero failures; `sweep_stale.sh` clean.

**Not verifiable here:**

- Raw CI log text (the results-receiver host is unreachable from this sandbox); step-level conclusions stand as the proof, and both packaging steps are fail-closed (any failure reds the job).

**Docs touched:** `docs/ci/PENDING_WORKFLOW_CHANGE.md`, `SESSION_HANDOFF.md`, `WORKLIST.md`, `IMPROVEMENT_LOG.md`.

---

## S18 follow-up — UI relicensed to Ms-PL, Caesium dropped (2026-09-14)

**Changed:**

- **Relicense executed (`OD-09 = b`, `OD-C6`):** first-party code is now Ms-PL. `LICENSE` rewritten (with revision note), full text added as `COPYING.ms-pl`, both packagers hard-require it (`package_common.sh` + fixture/negative case in `test_package.sh`), CI manifest (live + proposed) asserts it. No third-party code was in the tree (verified: no foreign copyright headers, empty `assets/`/`resources/`), so the relicense needed no outside permission. Engine untouched (GPLv2 subprocess); Qt LGPL line kept.
- **Caesium dropped:** the base was never incorporated (zero files in the tree; the 74 MB binary bundle was removed in S7 and only a packaging pattern was ever taken from it). Vision UI-approach rewritten to open-parts rebuild (system fonts, MIT/Apache icon sets); `reference_code/REFERENCE_MANIFEST.md` rows retired (provenance kept, re-fetch forbidden); `.gitignore` lines + README layout lines removed; dated reviews annotated, not rewritten.
- **U-08 narrowed (stays PARTIAL):** "no full text / grant unstated" closed by the Ms-PL text + `LICENSE` grant; still missing: Qt LGPL notices. Packaging count 30 — 32 (one new required-file case per kind).
- **OD-C1 superseded:** fork is no longer reference-only — Ms-PL fork files may now be copied with notices retained (plan §1.4/§8 updated, OD-C6 recorded).

**Partial:** Qt LGPL notices still unstaged (U-08 remainder); full 32-green packaging run needs real artifacts (CI).

**Left:** spike session (workflow push now armed with granted scope + this commit's manifest line as the first live test of it).

**Verified:**

- `test_package.sh` delta executed here: baseline 11 PASS then abort at the missing-build-artifact `cp` (no `build/` in this sandbox); modified run 12 PASS then the same abort — exactly +1 (the new portable missing-file case passes; system side same shared `copy_required` path). Absolute 32-green re-measured in CI, which has real artifacts.
- `check_docs.sh --emit` regenerated `STATUS.md` (U-08 proof only); plain `--no-gate-run` green with zero failures; `sweep_stale.sh` clean.

**Not verifiable here:**

- The full 32-case packaging green (needs `build/` artifacts); CI's verdict on this commit, including whether the granted `workflows` scope accepts the live `build.yml` change (if rejected, the change is reverted from live and the pending marker extended).

**Docs touched:** `LICENSE`, `COPYING.ms-pl`, `COMPILED_AUDIT.md`, `STATUS.md`, `WORKLIST.md`, `SESSION_HANDOFF.md`, `IMPROVEMENT_LOG.md`, `PROJECT_VISION.md`, `README.md`, `FEASIBILITY_REVIEW.md`, `docs/planning/OFFLINE_BUILD_REVIEW.md`, `docs/planning/OWNER_DECISIONS.md`, `docs/planning/CSHARP_SHELL_PLAN.md`, `docs/release/RELEASE_PROCEDURE.md`, `reference_code/REFERENCE_MANIFEST.md`, `.gitignore`, `.github/workflows/build.yml`, `docs/ci/build.yml.proposed`, `working_code/gifscythe/scripts/package_common.sh`, `working_code/gifscythe/scripts/test_package.sh`.

---

## S18 follow-up — workflows scope granted, retry armed (2026-09-14)

**Changed:**

- Owner granted the `workflows` permission, unblocking pushes to `.github/workflows/`. Per the owner's "try it later": nothing pushed now; `docs/ci/PENDING_WORKFLOW_CHANGE.md` status flipped to granted-with-retry (marker stays until a retry succeeds, with a restore rule if rejection recurs), and the C# spike next-action now says to land the workflow change directly instead of staging it in `docs/ci/build.yml.proposed`.
- Also corrected an owner mental model in chat: MS-PL is not "GPLv3 with a different name" (weak/file-level copyleft vs strong/work-level copyleft; FSF lists them incompatible). Reference-only stance unchanged.

**Partial:** none — docs only, no code.

**Left:** the retry itself belongs to the spike session (needs the spike's CI job to exist first).

**Verified:**

- `working_code/gifscythe/scripts/check_docs.sh --emit` re-run (register unchanged), then `check_docs.sh --no-gate-run` green with zero failures; `scripts/sweep_stale.sh` clean.

**Not verifiable here:**

- Whether the granted scope actually works (the proof is the retry push, deliberately deferred); CI's verdict on this commit (not yet pushed at write time).

**Docs touched:** `docs/ci/PENDING_WORKFLOW_CHANGE.md`, `WORKLIST.md`, `SESSION_HANDOFF.md`, `IMPROVEMENT_LOG.md`.

---

## S18 follow-up — stills-to-animated scope + mission-amendment fix (2026-09-14)

**Changed:**

- Owner confirmed video↔animated endpoints (already plan §2.1) and asked for still-image collections (JPG/PNG) → GIF/APNG/WebP with a speed control plus per-frame timing — the phone-app GIF-maker flow, ezgif-maker-class (global "Delay time" + per-frame "Delay" in 1/100 s, matching the existing `delay_cs` units). Recorded as plan §2.1 item 3; video endpoints shift to item 4, editing to item 5.
- Fixed the mission-amendment wording the owner quoted back: it must cover photos too — *"photos and video only as conversion endpoints/inputs, never as the subject."* Still gated: no stills-import or video-endpoint code until `PROJECT_VISION.md` is amended.
- Engine note recorded: gifsicle reads GIF inputs only (vendored man page), so stills need a decode step — folded into the same FFmpeg-sidecar-or-platform-codecs TBD as video.

**Partial:** none — planning only, no code.

**Left:** unchanged from the S18 entry below (Phase 1 spike needs a Windows runner; vision amendment due before endpoint work).

**Verified:**

- `working_code/gifscythe/scripts/check_docs.sh --emit` re-run (register unchanged), then `check_docs.sh --no-gate-run` green with zero failures; `scripts/sweep_stale.sh` clean. New doc text kept gate-safe (existing repo paths only, no runtime counts, no audit-ID checkboxes).

**Not verifiable here:**

- CI's verdict on this commit (not yet pushed at write time); the spike still needs Windows.

**Docs touched:** `docs/planning/CSHARP_SHELL_PLAN.md`, `WORKLIST.md`, `SESSION_HANDOFF.md`, `IMPROVEMENT_LOG.md`.

---

## S18 — C# shell plan: Phase 0 decided, plan is WORKING (2026-09-14)

**Changed:**

- **New plan:** `docs/planning/CSHARP_SHELL_PLAN.md` — a C#/WPF exe shell driving the unchanged gifsicle subprocess, written because the owner forked ScreenToGif as a faster exe path and asked for a custom UI/UX. Answers the C/C++-vs-C# question (no technical clash: subprocess+argv boundary, already proven by the JS web client) and lays out six phases with exit gates.
- **Phase 0 signed (owner, same session):** OD-C1 = a (fork is reference-only — MS-PL source never lands in this GPLv3-intent tree), OD-C2 = c (phased commitment: sidecar through the Phase 1 spike + Phase 2 Core port, full commit at Phase 3 UI), OD-C3 = a (no recorder; converter+compressor focus, APNG/WebP promoted to planned scope, video allowed only as a conversion endpoint, ezgif-class editing later — mission amendment to `PROJECT_VISION.md` required before any video-endpoint work), OD-C4 = a (WPF), OD-C5 = a (archive the Qt GUI at cutover; the C++ CLI stays forever as the parity oracle).
- **Direction effect:** the OFFLINE_BUILD_REVIEW §4 decision ("stay C++/Qt through 1.0.0") is now superseded by the phased plan — Qt stays the shippable path only until the Phase 3 commit point.

**Partial:** none — planning session, no code.

**Left:** Phase 1 spike (needs a Windows runner with dotnet; not obtainable here); the `PROJECT_VISION.md` mission amendment (due before video-endpoint work, not now); OD-03…OD-15 untouched.

**Verified:**

- Docs gate: `working_code/gifscythe/scripts/check_docs.sh --emit` regenerated `STATUS.md` (header session/date only — no register rows changed), then `check_docs.sh --no-gate-run` is green with zero failures (`G6 SKIP`, the CI-accepted mode for doc runs; no full `verify_audit.sh` re-run for a docs-only change). `scripts/sweep_stale.sh` run alongside, clean.
- Gate-safety review of the new plan doc before commit: every backticked repo path in it exists in this checkout (G8); no bare harness/unit runtime counts (G9); no audit-ID checkboxes added to `WORKLIST.md` (G2).

**Not verifiable here:**

- The Phase 1 spike itself (no dotnet/Windows in this sandbox) and CI's verdict on this commit (not yet pushed at write time).

**Docs touched:** `docs/planning/CSHARP_SHELL_PLAN.md`, `WORKLIST.md`, `SESSION_HANDOFF.md`, `STATUS.md` (regenerated header), `IMPROVEMENT_LOG.md`.

---

## S17 continuation — sequential DS-11 / GS-210 / GS-204 / GS-203 (2026-09-13)

**Authorization:** owner requested highest→high confidence sequential work, medium/
low portions handed to another agent, followed by PR creation (not merge).

1. **DS-11 DONE:** leading-current-status S5 checks both OPEN-vs-closed and
   closed-vs-nonclosed, prefers explicit §5 references, ignores historical tails,
   and fails uncheckable claims/Python errors. Four current status references made
   explicit. **20 tests pass**; baseline falsely passes all **3** OPEN-vs-fixed variants.
2. **GS-210 PARTIAL:** both entry points validate all args before consuming files,
   tools, logs or hooks; unknown rc=2/help rc=0. **12 cases pass**, and all **12**
   fail against original scripts. Native build **296/0**; qmake-first selection and
   hardcoded `.pro` version are unchanged and handed off.
3. **GS-204 PARTIAL:** both package types share fresh private staging and required
   non-empty manifests. Explicit headless omits GUI, target extension selection
   avoids native/Windows filename mixing, portable Windows GUI requires a working
   deployer plus key runtime/plugin entries. **30 checks pass on Linux**; real
   engine/CLI artifact tests are isolated so CI's GUI package is preserved. Baseline
   system packager returns zero with no binaries. Synthetic Windows files/deployers
   do not prove runtime deployment, architecture, complete licensing or release SHA.
4. **GS-203 PARTIAL:** Qt-independent core and JS verifier require new or size/mtime-
   changed non-empty regular GIF87a/89a-signature outputs. CLI ordinary explicit-file
   runs and both web APIs use them; web serves the exact verified buffer. **Smoke
   40/40** includes **12** core assertions; **transport 67/67** on Linux, with three
   11-case ordinary-mode output-fixture groups. Original CLI fails all **9** missing/
   stale/bogus probes; original server fails **3** mode groups (six invalid signatures
   in each). Qt integration, stdout/info semantics, full decoding and rollback are
   not changed. Metadata granularity can conservatively reject identical rewrites.

**Handoff:** `docs/planning/SEQUENTIAL_WORK_HANDOFF.md` gives the three PARTIAL
findings' remaining work, environment and acceptance tests. Registers/worklist and
release blocker text updated without pretending Linux/synthetic proof closes Qt/
Windows work. Review and sync of merged **PR #21** (`df1dfd5`) covers its OD-01
triage, GS-201 stop-loss and G18/G16/P3b gates; header and PR ledger now agree.
No version, workflow, template or other owner-decision change. F3/F4 add Python
regressions to the local audit. Change review: **4 passed / 1 R1 flag / 0 skipped** — expected mandatory manual
review for edited check logic, not suppressed. S5 baseline mutations and all 20
regressions supply the proof; forced Python exit 44 produces FAIL in S5 and in
both new F3/F4 gates.

**Final executed local gates:** full audit **27 passed / 0 failed / 6 skipped**;
docs **23 passed / 0 failed / 2 skipped**; sweep **5/0/0**. Audit includes native
build/unit **296/0**, CLI smoke **40/40**, engine **5/5**, packaging **30/30**,
web parity **17** / validation **23** and transport **67/67**. Skips remain missing
Qt/CMake, declared workflow drift, remote CI and clean-Windows verification.
Register regenerated: **87 DONE · 8 PARTIAL · 27 OPEN · 0 UNTRIAGED · 122 total**.

---

## S17 continuation — GS-207 / P1-29 strict engine overrides (2026-09-13)

**Authorization:** owner approved the recommended GS-207 fix ("Let's go with that").
Core resolver now returns a typed path/source/error result and stops on invalid
non-empty GS_ENGINE. CLI print/run exit 1 naming the override; --engine remains
higher priority and its prospective print-mode contract is unchanged. Run source
logs go to stderr. Empty/unset preserves discovery, including CLI PATH fallback.
POSIX checks require a regular file and access(X_OK); Windows regular files defer
format/architecture errors to process launch, with no retry using another engine.
The GUI compatibility wrapper still returns a string, empty on invalid overrides;
no GUI source changed, but Qt/Windows runtime verification was not performed here.

Web resolution now returns path/source/error too. Invalid overrides yield 503 on
both APIs and a startup error naming GS_ENGINE (the static UI remains available).
Valid startup logs identify GS_ENGINE/release; release discovery ignores unusable
candidates when no override is set. Request-time resolution refuses fallback even
if an initially valid override disappears after startup.

**Executed:** build and unit suite 296 checks, 0 failures; CLI smoke **30/30**;
web command **17**, validation **23**, transport **63/63** on Linux. Nine added
CLI groups cover five invalid paths in print/run, absolute/relative space paths,
--engine priority and empty/PATH discovery. Ten web groups cover both endpoints,
actual subprocess counts, source logs, invalid/valid/empty/unset overrides and
removal after startup. The POSIX non-executable-file group is omitted on Windows;
Windows runtime not measured. Node syntax and diff whitespace checks pass.

**Baseline proof:** compiled the pre-fix CLI from HEAD in an isolated scratch tree
and ran the expanded smoke suite: 21 passing, 9 failing groups. All five invalid
GS_ENGINE cases wrongly return zero in print/run; the four valid-path/priority/
discovery groups additionally expose missing source logs. The pre-fix web server
fails 10 groups: missing/removed overrides fall back, directories/non-executable
files attempt spawn, and the source-log assertions are absent. Neither baseline
was committed; scratch trees/files removed.

**Final gates:** `verify_audit.sh` **25 passed, 0 failed, 6 skipped** (CMake/Qt
C6/C9/B unavailable; E9 pending workflow; remote CI and clean-Windows smoke not
run). Engine 5/5, packaging negatives and the full CLI/web suites reran green.
Docs gate **23 passed, 0 failed, 2 skipped**; change review **4/0/1**.

**Docs:** GS-207 marked DONE, STATUS header regenerated; worklist, audit §6/§13,
root/product/web README, handoff, next-session prompt and template facts updated.
Template stays SKELETON; no other finding or owner decision changed. No workflow,
version, release, push, PR or merge.

---

## S17 continuation — DS-13 / P1-32 optimize output signature (2026-09-13)

**Authorization:** owner approved proceeding with the recommended DS-13 fix.
`/optimize` checks the exact response buffer for GIF87a or GIF89a before sending
200. A non-empty buffer with an invalid signature gets JSON 422, exitCode 0,
named diagnostic stderr and the command. Factored the signature predicate so
explode-file verification uses the same rule; no second file read for optimize.
Missing/empty output and nonzero engine exits retain their prior diagnostics.
This is signature-only, not a full GIF decoder; GS-203 remains OPEN.

**Executed proof:** transport **53/53**, up from 42. Eleven new cases run a real
Node child that writes controlled output: text, PNG, truncated signature, wrong
version/suffix/case, valid GIF87a/GIF89a, missing/empty output and nonzero exit.
A test-only preload redirects marked spawns; production has no test hook. Existing
cases still run gifsicle. The pre-fix server fails exactly **6 invalid-signature
cases**, returning success where 422 is required. Temporary baseline files were
removed. Web command parity **17**, validation parity **23** remain passing.
Node syntax checks and `git diff --check` pass.

**Final gates:** `verify_audit.sh` **25 passed, 0 failed, 6 skipped** (C6/C9/B
require CMake/Qt; E9 pending workflow; remote CI and clean-Windows smoke not run).
`check_docs.sh` **23 passed, 0 failed, 2 skipped**; change review **4/0/1**.
The audit also reran the engine/CLI/unit build, engine 5/5, smoke 21/21,
packaging negatives and all three web suites successfully.

**Docs:** DS-13 marked DONE; STATUS header regenerated; worklist, audit §6/§13,
handoff, next-session prompt, root/web READMEs and template facts updated. Earlier
GS-202 proof counts remain historical; the live suite is now 53 check groups.
Template stays SKELETON. No workflow, version, release, push, PR or merge changes.

---

## S17 continuation — GS-202 / P0-6 web path containment (2026-09-13)

**Authorization:** owner said "Do that" to the recommended GS-202 security fix.
No other product decision or finding was included.

**Changed:** `web/run-paths.mjs` defines portable name admission (reject, not
sanitize), independent resolved-path containment, and conservative case/NFC
collision keys. `/run` validates every upload name before engine lookup, resolves
all output targets/prefixes before upload writes or engine launch, then executes
the locked plan. Batch target/target and target/source comparisons are no longer
case-sensitive. Neutral input paths and returned explode-frame paths also use the
guard. Spaces, Unicode, emoji and literal percent sequences remain supported;
client settings cannot replace server-owned input/output paths.

**Regression evidence:** expanded the existing CI-gated transport suite from 30 to
42 check groups, including 100 invalid-name requests across all modes, a Node preload
logging actual subprocess launches, two outside-request sentinel files, case/NFC
collision cases, valid-name runs and direct POSIX/Windows-drive/UNC guard assertions.
Tests isolate TMPDIR/TMP/TEMP in a disposable test root, so baseline escape attempts
cannot touch unrelated files. Cleanup assertions wait for asynchronous server cleanup.

The unmodified pre-fix server fails **7 security groups**: it launches the engine for
unsafe requests, overwrites the Auto/Batch and Explode sentinels outside its request
directory, and accepts case/NFC collisions. The fixed server passes **42/42** and
preserves both sentinels. Independently disabling the containment predicate fails
its direct guard group (the other 41 remain passing). Scratch mutations were removed.

**Other executed checks:** build + C++ unit 296 checks / 0 failures; engine 5/5;
CLI smoke 21/21; package negatives 9/9; web command 17, validation 23; N-07 sweep
regressions 14 tests; Node syntax checks and `git diff --check` pass. Windows path
semantics tested with Node path.win32 on Linux, not a real Windows GUI/runtime run.

**Final gates (executed after the local commit):** `verify_audit.sh` **25 passed,
0 failed, 6 skipped** in this non-Qt/non-CMake sandbox (C6/C9/B, pending E9,
remote CI and clean-Windows smoke skipped). `check_docs.sh` **23 passed,
0 failed, 2 skipped** (G6 full-toolchain measurement and pending G7).
`review_change.sh --report`: **4 passed, 0 failed, 1 skipped** (no gate script edit).

**Scope and docs:** GS-202 marked DONE; register header regenerated. Audit §6/§13,
worklist, handoff, next-session prompt, root/web README and template current-state
facts updated; template still SKELETON. Remaining GS-203/DS-13 output verification,
U-06 resource limits, licensing/release work and owner decisions are unchanged.
The containment guarantee assumes a trusted engine and private temp directory; it
is not a sandbox against arbitrary engine code or hostile local symlink writers.
No workflow edit, version bump, push, PR, or merge.

---

## S17 — N-07 / P2-15: standalone UNTRIAGED count check (2026-09-13)

**Selected work:** the owner asked to choose and immediately execute a job within current
capabilities. Chose documentation tooling, leaving product decisions OD-03…OD-15 untouched.

**Changed:** extended sweep S2 to check standalone numeric UNTRIAGED counts against the
STATUS.md generated counts line, not just a full four-state tally. Inline Markdown and
line wraps work; paragraph boundaries and token boundaries prevent unrelated matches.
Failures name file:line and reference count; malformed reference counts fail closed.
Historical corpus exclusions and `--report` semantics are unchanged. S4 is still a
fixed five-phrase list, not a general natural-language reversal detector. Unnumbered
claims remain a review responsibility; N-07 is closed for the scoped P2-15 count check.

**Executed evidence:** `python3 working_code/gifscythe/tests/test_sweep_stale.py`:
14 tests passed (including subcases for formatting and the existing full-tally check).
Tests invoke the real sweep in temporary Git repositories and assert no Markdown writes.
The stale standalone and changed-reference probes both fail against the pre-fix script:
it returns zero instead of one. The patched script rejects both. Live sweep: 5 passed,
0 failed, 0 skipped. `bash -n` and `git diff --check` pass.

**Review:** `review_change.sh --report` flags R1 because S2 check logic changed;
this is expected, not waived: the baseline-negative and patched-positive mutations
above supply its required proof. R2/R3/R4 pass. R5 obligations addressed in the handoff,
worklist, this log, and STATUS (hand-maintained N-07 row; header regenerated by `--emit`).
P2-15 now also has its missing §6 fix-order row in COMPILED_AUDIT.md.

No product source, CI workflow, release artifact, version, or owner decision was changed.
No push, PR, or merge performed. All work remains on `arena/01a09934-gifscythe`.

---

## S16 continuation — uncommitted-work hard rule (G18) + template content check (G16)  (2026-09-13)

**Changed:**

- **No GitHub patch** was in the asking message; none was applied this session.
  The S14 owner patch was adjudicated earlier and was not applied wholesale.

- **HARD RULE, mechanical:** every edit/write/delete is committed into the repo
  before merge **and** before the session can close. Uncommitted work is lost
  when the sandbox is cut off (second time). Do not wait to be reminded.
  - `check_docs.sh` gate **G18** FAILs on a dirty tree. Pre-push runs this
    script, so a dirty tree cannot be pushed. CI checkouts are clean, so G18
    PASSes there.
  - `pr_preflight.sh` **P3** FAILs create/merge on dirty; new **P3b** FAILs if
    HEAD has no upstream or is ahead of origin (unpushed commits are not in
    the repo).
  - Standing rule 6 is written in `SESSION_HANDOFF.md`, `WORKLIST.md` and
    `docs/release/RELEASE_PROCEDURE.md` (G13 now requires those files to
    mention **G18**). `docs/planning/NEXT_SESSION_PROMPT.md` recovery no
    longer hardcodes a check_docs triple; it requires 0 failed and names G18.

- **Merge-related checks run before merge**, not after. Rule 3 already pointed
  at `pr_preflight.sh --online`; WORKLIST rule 3 now matches, and the prompt
  says create **and** merge.

- **G16 inspects template content vs the state token** (session-start check).
  Slot placeholders in `web/WEB_PLAN_TEMPLATE.md` §1–§10 (not §0) must agree
  with the token: leftover `<date>`/`<owner>`/`<next>`/`<who>`/`<what>` /
  `<ids the draft names>` / `<open:` / "the owner's draft fills this in" =
  stay **SKELETON**; filled content + SKELETON = flip both lines to
  **WORKING PLAN** in the same commit (one-way). WORKING PLAN + leftover
  placeholders = fill them; never flip back. The gate never auto-edits.
  Current tree is still skeleton — both lines stay **SKELETON**.

- **SW-03** registered DONE in `STATUS.md` (hand-maintained).

**Partial:** none of this work. N-07 / P2-15 remains OPEN (detector gap).

**Left:** OD-03…OD-15 unanswered. Remaining 17 intake rows OPEN. No PR until
an explicit yes.

**Verified:**

- Mutation: dirty tree → `FAIL [G18]`; restore → PASS after commit.
- Mutation: `WORKING PLAN` token while §1–§10 still have placeholders →
  `FAIL [G16]`; restore → PASS (`SKELETON`, content skeleton).
- Mutation: strip §1–§10 placeholders while token stays `SKELETON` →
  `FAIL [G16]`; restore → PASS.
- Gate never edited the template state lines.

**Not verifiable here:**

- Whether a future sandbox cut-off still drops unpushed commits if the agent
  ignores P3b (the check cannot run after the sandbox is gone).
- Native Windows / CI for this commit (not yet pushed at write time).

**Docs touched:** `working_code/gifscythe/scripts/check_docs.sh`,
`working_code/gifscythe/scripts/pr_preflight.sh`, `STATUS.md`,
`SESSION_HANDOFF.md`, `WORKLIST.md`, `IMPROVEMENT_LOG.md`,
`docs/release/RELEASE_PROCEDURE.md`, `docs/planning/NEXT_SESSION_PROMPT.md`,
`docs/ci/README.md`, `web/WEB_PLAN_TEMPLATE.md`.

---

## S16 — `OD-02 = a` executed: CLI `--run` refuses Batch with no output (GS-201 / P0-5)  (2026-09-13)

**Changed:**

- **Landed S15 on this branch first.** Cherry-picked the S15 triage commit
  `5677612` (from `arena/01a09712-gifscythe`) onto `2542f1b` as `0e6e1a7`. That
  is the 18-row §6 mapping (`OD-01 = a`) plus `N-07` registered UNTRIAGED. This
  session then executed the only owner-authorized code change.

- **`OD-02 = a` / GS-201 / P0-5 stop-loss.** CLI `--run` with `mode = batch` and
  no `output` key now exits **2** with a named reason (`Batch with no output` /
  `in-place -b`) **before** engine locate. Print mode still prints (the builder
  stays a faithful `-b` mapping). Usage documents the refusal. The planner skip
  (`!s.output.empty() && mode != Explode`) is unchanged: Batch-with-output and
  Auto/Merge empty-output are out of scope. GUI and web never emit a single `-b`
  run (they already per-file Auto).

- **Regression.** `scripts/smoke_cli.sh` gained cases 17–18: `--run` rc=2 +
  source GIF `cmp`-identical + greppable named reason; print still emits `-b`.
  Suite **19 → 21** `ok()`.

- **Engine hazard confirmed here** (intake A.1 had not executed it): bundled
  `gifsicle -b -O3` rewrote `logo.gif` **8703 → 8637** bytes. The same file under
  the CLI stop-loss stayed `cmp`-identical.

- **`N-07` triaged, not implemented.** Mapped to **P2-15** (OPEN): fail when a
  current-state doc's quoted UNTRIAGED count disagrees with `STATUS.md`'s
  generated counts line, or restate S4 as the 5-phrase list it actually is.
  Docs-only; the sweep was not changed. Closing it by restating S4's existing
  comment would not have been a triage.

- **Stale leftover claims corrected** because they were live false after S15:
  `COMPILED_AUDIT.md` still said §13 was "not triaged" / "still not in the §6
  fix order"; `RELEASE_PROCEDURE.md` still listed GS-201 as an open blocker;
  current-state smoke quotes that would have gone 19/19 → 21/21.

**Partial:**

- None of GS-201. The authorized stop-loss is the whole finding. A per-file Auto
  orchestrator (full Batch redesign) is future work, not a missing half of this
  row.

**Left:**

- **N-07 / P2-15** OPEN (detector gap). Remaining 17 intake rows OPEN. Duplicate
  §6 **P2-5** left alone (S15). **OD-03…OD-15** unanswered. No PR until an
  explicit yes.

**Verified:**

- `./build.sh` — **296 checks, 0 failures**.
- `scripts/smoke_cli.sh` — **21 passed, 0 failed**.
- Direct engine `-b -O3` rewrite: 8703 → 8637 B.
- CLI `--run` on Batch-no-output: rc=2, named reason on stderr, source
  `cmp`-identical; print rc=0 and the line contains `-b`.

**Not verifiable here:**

- Native Windows / Wine re-run of the new CLI cases (not executed this session).
- gifsicle `-b -o` (Batch-with-output `--run`) — deliberately untested.
- GitHub Actions for this branch (not pushed). GUI/web Batch paths were
  review-confirmed as already per-file Auto, not re-run.

**Docs touched:** `STATUS.md` (GS-201 DONE, N-07 OPEN/P2-15, W-04/W-11 21/21,
re-emitted), `COMPILED_AUDIT.md` (§6 P2-15, §7 A12/A18, §13 leftovers, U-18
count), `WORKLIST.md`, `SESSION_HANDOFF.md`, `IMPROVEMENT_LOG.md`,
`docs/release/RELEASE_PROCEDURE.md`, `docs/planning/OWNER_DECISIONS.md`,
`web/WEB_PLAN_TEMPLATE.md`.

---

## S15 — PR #20 merged; `OD-01 = a` executed, all 18 intake findings triaged  (2026-09-13)

**Changed:**

- **PR #20 merged as `2542f1b`** (2026-09-13); post-merge `main` run **`34735692932`** is green on
  both jobs. That PR carried the `G10` and `S2` gate repairs, `pr_preflight.sh` step **P6**, the
  append-only PR ledger and `scripts/review_change.sh`. Per the ledger's maintenance rule the merge
  sha went into the ledger row and the header's **Docs synced through** line moved to PR #20 — and
  only after this write-up, which is the order **P6** enforces.

- **`OD-01 = a` executed: the 18-finding triage.** Every `UNTRIAGED` intake row now names a
  `COMPILED_AUDIT.md` §6 fix-order id and is `OPEN` in `STATUS.md`. At the moment of triage the
  register went `80 · 5 · 17 · 18 · 120` → **`80 · 5 · 35 · 0 · 120`** (emitted by
  `check_docs.sh --emit`, not edited by hand) — total unchanged at 120, so the triage moved rows
  and added none. It reads `80 · 5 · 35 · 1 · 121` now because S15 registered one finding of its
  own (**N-07**, below).

  **Four rows folded into actions that already specified the same fix.** Inventing a new id for
  each would have hidden that the work was already scoped, and left two places to update forever:

  | Finding | §6 id | Why it is the same work |
  |---|---|---|
  | `DS-06` | **P0-2** | P0-2 already prescribes `0` → bare `-j`, `-1` → no flag; the intake restated it as "`-1` now means 8 threads". Added the tri-state wording, unit test 28 and the settings comment. |
  | `DS-12` | **P1-13** | P1-13 is "settings string escaping … or reject unrepresentable values" — the whitespace loss is the same defect. |
  | `GS-208` | **P2-7** | P2-7 is "delete or CI-enforce `build.yml.proposed`"; the intake's remaining half of the release-red finding is exactly that commit. |
  | `DS-08` | **P3-5** | P3-5 is the CLI warning-policy doc task. Corrected while triaging: it says "add `--strict`", but `--strict` already exists at `src/cli/main.cpp:216-222`, so the row now describes the real remainder — the advisory exit-code contract. |

  **Fourteen got new ids**, tiered by the harm rather than by reviewer severity:

  | §6 id | Finding | Tier rationale |
  |---|---|---|
  | **P0-5** | `GS-201` | destroys user source GIFs; row says P0 candidate; `OD-02 = a` authorises the ~10-line stop-loss |
  | **P0-6** | `GS-202` | `../` escapes the web request temp dir; row says P0 candidate |
  | **P1-25** | `GS-203` | success claimed on exit 0 with no output check — P1-6/P1-19 cover web and Explode only |
  | **P1-26** | `GS-204` | packaging fail-open; P0-3 fixed the manifest path, these are the paths still open |
  | **P1-27** | `GS-205` | non-GIF inputs admitted; P1-9 fixes only the drop `||`→`&&` |
  | **P1-28** | `GS-206` | `long`→`int` narrowing plus four missing validation domains |
  | **P1-29** | `GS-207` | unusable `GS_ENGINE` silently falls back; P1-3 is PATH resolution only |
  | **P1-30** | `DS-07` | GUI spinner cannot express "unchanged"; must agree with P0-2 |
  | **P1-31** | `DS-09` | `threads < -1` accepted silently; pairs with P0-2 and P1-28 |
  | **P1-32** | `DS-13` | `/optimize` serves non-GIF bytes as `200 image/gif` |
  | **P2-12** | `GS-209` | one fixed glibc config for linux *and* mac engine builds |
  | **P2-13** | `GS-210` | build entry points accept mistyped options |
  | **P2-14** | `DS-11` | the mechanical narrative-vs-register gate S14 proposed but never built |
  | **P3-11** | `DS-10` | disposal 4..7 unreachable from the picker |

  **Triage scoped them; it fixed none of them.** Every one of the 18 stays an unchecked line in
  `WORKLIST.md`, now naming its §6 id.

- **Gate consequence:** with zero `UNTRIAGED` rows, **G12** no longer blocks a newer `## S<n>`
  heading — which is what allows this entry to exist at all. The S14-continuation entry above
  keeps its note explaining why it could not be filed as `## S15` at the time.

- **Registered `N-07` (new, `UNTRIAGED`, S15):** sweep **S4** matches only 5 hardcoded retired
  phrases (`demo only`, `not the product path`, `is a demo`, `demo/parity harness`,
  `not the product`), so it cannot catch a current-state doc that still says findings "stay
  `UNTRIAGED`" after a triage empties the register. Measured, not suspected: the triage left
  **4** such live claims (`SESSION_HANDOFF.md:121`, `WORKLIST.md:411`,
  `docs/planning/OWNER_DECISIONS.md:53`, `web/WEB_PLAN_TEMPLATE.md:98`) and the sweep reported
  `5 passed, 0 failed` both before and after they were corrected by hand. All four are fixed;
  the detector gap is the finding. This is also a correction to how S4 has been described in
  these docs — "a retired claim a decision reversed" overstates a fixed 5-phrase list.

**Found while triaging (not fixed):** `COMPILED_AUDIT.md` §6 has two rows numbered **`P2-5`**
(independent X/Y scale, and the web validation layer). Left alone — renumbering a tier that other
documents cite by id would break every existing reference for a cosmetic defect. New ids therefore
start at **P2-12**, not P2-5.

## S14 continuation — stale-claim sweep, PR preflight, owner-decision register, gate repairs  (2026-09-13)

*(This entry spans 2026-09-12 → 2026-09-13 and was deliberately **not** filed as `## S15` at the
time: gate **G12** fails on a session heading newer than any `UNTRIAGED` row, and the 18 intake
findings were still `UNTRIAGED` since S14. **S15 resolved that** by executing `OD-01 = a` — see the
entry above. The date is the newest of its day so that **G11** can see the log is current with the
code changed on 2026-09-13 — `check_docs.sh`, `sweep_stale.sh` and `pr_preflight.sh`.)*

**Changed:**

* **Added the stale-claim sweep** — `scripts/sweep_stale.sh` scans the same current-state `.md` set
  as `check_docs.sh` (via `git ls-files '*.md'`, minus the dated snapshots and the append-only log)
  for claims that can only be checked against *reality*, and names `file:line` + the fix; it never
  edits. Five rule groups, each mutation-tested (inject the staleness and the rule must FAIL):
  **S1** workflow copies vs the pending-change marker (class N-01); **S2** a quoted four-cell
  `DONE/PARTIAL/OPEN/UNTRIAGED` tally must equal `STATUS.md`'s counts line; **S3** volatile
  "release-red / is red / CI green / not yet merged" wording must carry evidence (run id, PR #,
  date, sha, session) or prescriptive wording in a +/-1-line window; **S4** retired
  "demo only / not the product path" claims must carry a supersession marker within +/-2 lines;
  **S5** a narrative `**Status:**` block claiming FIXED/CORRECTED/DONE/RESOLVED while its §5
  register row is not ✅/☑. A report-only **S6** surfaces pending/outstanding/TODO wording and
  never fails.
* **`check_docs.sh` gained gate G17** — runs the sweep, PASSes with the count of `PASS [S` lines,
  else FAILs and prints the sweep's `FAIL [S` + `action:` lines. The gate total is now 22 checks.
* **Added `scripts/pr_preflight.sh`** — the PR/merge companion (`--online`, `--body FILE`): P1
  `check_docs.sh`, P2 `sweep_stale.sh`, P3 dirty-tree guard, P4 repo/main-tip/latest-run/PR state
  (offline by default; SKIPs without `gh`), P5 PR body skeleton. Exits 1 on any failed check.
* **Added the owner-decision register + SkillOpt query** — `docs/planning/OWNER_DECISIONS.md`
  (`OD-01`…`OD-15`, options + recommendation + what each unblocks) and
  `docs/planning/SKILLOPT_INTEGRATION_QUERY.md` (incorporate microsoft/SkillOpt into this repo:
  verified facts, the three non-negotiable conditions, the four shapes), plus
  `docs/planning/NEXT_SESSION_PROMPT.md` for the copy-paste hand-off.
* **The sweep's first real catches, corrected in this session:** two undated "CI green" cells in
  `docs/planning/OFFLINE_BUILD_REVIEW.md` ("Windows CI green" → "green in S6"; "done, CI green" →
  "as of S6"), and the U-06/U-08 narrative `**Status:**` lines that claimed "FIXED (S8)" while §5
  marks them ◐ PARTIAL — both corrected to ◐ PARTIAL with the missing half named, and S5 now
  enforces it.

**Partial:**

* None new — U-06/U-08 remain ◐ PARTIAL as before (their narratives now match §5).

**Left:**

* The owner answers (`OD-01`…`OD-15`) and the SkillOpt integration await the owner (see the two new
  planning docs); the 18 intake findings remain untriaged.

**Verified:**

* `working_code/gifscythe/scripts/check_docs.sh` — **22 passed, 0 failed, 3 skipped** (skips: G6
  full-toolchain total, G7 declared-pending workflow, G9b unit-not-built; G17 runs the sweep).
* `working_code/gifscythe/scripts/sweep_stale.sh` — **5 passed, 0 failed, 0 skipped** after the two
  corrections above; each of S1–S5 re-checked by injecting the staleness and confirming the rule
  fails.
* `working_code/gifscythe/scripts/pr_preflight.sh` — offline run: P1/P2/P3 PASS, P4/P5 SKIP
  (no `--online`/`--body`), exit 0; `--online --body /tmp/pr_body.md` at PR time: P1–P5 all PASS.

**Merged:** PR #18 (`arena/01a096ec-gifscythe` → `main`) merged as `e32ed28`; **`main` run
`34713398377` is green on linux + windows** (the branch's PR run `34713246313` was green on both
jobs first).

**Post-merge sync (branch `arena/01a09712-gifscythe`, pushed as PR #20, tip `4c6311e`).** PR #19
recorded PR #18's merge but could not record its own, so the handoff header was one merge stale;
it now names **PR #19 merged as `43e3f96`** and **`main` run `34713800552` on `43e3f96`, `success`
on linux + windows** (both re-read from the GitHub API, not from memory). Two stale claims found
while re-running the previous session's verification script are corrected here: the
`SESSION_HANDOFF.md` TL;DR still quoted the S13 tally (`80/3/17/0`, `100 total`) while
`STATUS.md`'s generated counts line reads `80/5/17/18`, `120 total` — it is corrected and put on
one line so sweep rule **S2** (which only matches single-line four-cell tallies) now covers it; and
`COMPILED_AUDIT.md` §13 gained the automation note it was missing. This entry is filed inside the
S14-continuation entry rather than as `## S15` because gate **G12** fails on any new session
heading while the 18 intake rows are still `UNTRIAGED` since S14 — measured, not assumed: a
temporary `## S15` heading produced `20 passed, 2 failed` before being reverted.

**Owner's pre-rebuild patch, adjudicated hunk by hunk (not applied wholesale).** The owner supplied
the patch S14-continuation was *supposed* to produce, from the session that closed before pushing.
It is **not self-contained**: its prose documents creating five artefacts — `sweep_stale.sh`,
`pr_preflight.sh`, `OWNER_DECISIONS.md`, `SKILLOPT_INTEGRATION_QUERY.md`, `NEXT_SESSION_PROMPT.md`
(10/9/8/8/2 mentions) — and it contains **no `diff --git` header for any of them**. Its
`check_docs.sh` hunk adds a gate whose first act is `bad "G17" "scripts/sweep_stale.sh missing or
not executable"`, so applying the patch alone installs a gate that fails at once on a script the
patch never ships. It was therefore never the byte-exact fallback its covering note claimed.
Separately, `git apply --check` rejects it on 5 of its 9 files (`COMPILED_AUDIT.md`, `README.md`,
`SESSION_HANDOFF.md`, `STATUS.md`, `docs/planning/OFFLINE_BUILD_REVIEW.md`) — though that is a
strict-apply result: GNU `patch` accepts three more hunks *with fuzz*, including the
`README.md` duplicate-numbering bug and the obsolete ⚠️ block, so the per-hunk reasoning below is
what carries the decision, not the file-level reject. All 4 files where it *does* apply were also
left unapplied — 3 because applying them would reintroduce falsehoods or duplicates, and
`docs/ci/README.md` because the merged text already covers G16/G17 more precisely (it names the
three run sites and the one-line workflow drift). Its `IMPROVEMENT_LOG.md` and
`WORKLIST.md` hunks both describe the work as *"local commits, not pushed"* / *"a fresh session
must push them"* (it was pushed as PR #18 and merged); and its `check_docs.sh` hunk inserts a
**second gate G17** immediately after the existing one — verified by applying it to a scratch copy:
`G17` occurrences 3 → 6, with two `ok "G17"` lines at 1029 and 1051, so the sweep would run twice
per gate and the pass total would stop meaning anything. Adopted instead:
the patch's *placement* for the §13 automation note (before **Task registration**, where it belongs
in the reading order) merged with the corrected wording; its OD/SkillOpt pointer sentence; its
"Do not vendor anything before that answer" guardrail on `OD-15`; and its naming of what
`OD-01`/`OD-02` actually are, which the merged handoff only called "release blockers".
Rejected as buggy: its `README.md`
hunk numbers two list items `6.`; its `WORKLIST.md` hunk points at SkillOpt query "§6" for the
ordered task list when §6 is the constraints list and §7 is the task list; and its handoff line
claims "all 35 narrative status lines match the 52 register rows" — this file has **44** narrative
`**Status:**` lines and 52 register rows, so the count was already wrong when written. One real
defect the patch exposed and this sync fixes: the answer format was recorded as `OD-nn = a|b` in
three places while `OD-15` has four options `a`–`d`, so no literal `a|b` reply could answer it —
now `OD-nn = <letter>` in `SESSION_HANDOFF.md`, `WORKLIST.md` and
`docs/planning/OWNER_DECISIONS.md`.

**Two dead gate checks repaired (owner-authorized).** Neither was working:

* **G10 was matching nothing.** Its regex recognised `based on|base commit|base of|branched from|
  merge of PR #n` but not the handoff's `**Base:**` header form. Run over every tracked `.md`, the
  old regex hit **0 files** — the gate had passed vacuously since PR #16 rewrote the header from
  "based on `main` commit …" to `**Base:** \`main\` …`, i.e. through PRs #16–#20. The alternation
  now also matches `**Base:**` / `**Based on …:**` and is case-insensitive. Mutation-tested:
  injecting base `2176573` now gives `FAIL [G10] stale base commit - SESSION_HANDOFF.md names base
  2176573`; before the fix the same injection passed.
* **S2 exempted wrapped tallies "by design".** The exemption is how `SESSION_HANDOFF.md` kept
  quoting `80/3/17/0, 100 total` through PRs #16–#19. Each doc is now flattened before matching
  with a **bounded** gap (`<=40` non-digit chars) between cells, so a tally split over two lines is
  caught but a match cannot pair unrelated numbers across paragraphs; the trailing `<n> total` cell
  is checked too. Mutation-tested three ways: wrapped stale tally → FAIL; correct cells with a wrong
  total → FAIL; four correct-looking cells placed >40 chars apart → still PASS (no false positive).

**Owner decisions `OD-01 = a` and `OD-02 = a` recorded** in
`docs/planning/OWNER_DECISIONS.md`, with the count error corrected on the way: the option text said
"the 5 release-blockers" and the old recommendation named `GS-201`…`GS-204`, but
`docs/release/RELEASE_PROCEDURE.md` lists **6** open blockers of which only **4** are untriaged
intake (`GS-201`, `GS-204`, `GS-208`, `DS-06`). `OD-01`'s recommended **(b) was also mechanically
impossible** — G12 fails if *any* UNTRIAGED row outlives its session, so triaging only the blockers
would still block a new session entry. Both answers are recorded, **not executed**; the 18-row
triage and the `GS-201` code fix are the next session's first two tasks.

**Handoff sync is now a check, not a promise (`pr_preflight.sh` step P6).** The header gained a
machine-readable `**Docs synced through:** PR #n · branch X · merged as Y` line, and **P6**
(`--online`) compares it against the newest *merged* PR. If a merge landed after the last doc sync
it fails, names every unreviewed PR with its branch and title, and tells the session to read
`gh pr diff <n>` and write up what changed / what was fixed / what was implemented *before* moving
the line. Mutation-tested: with the line at #18 while #19 is merged it fails with exit 1 and lists
`unreviewed: PR #19`; with a wrong branch label but the right PR number it passes and prints a
mismatch note. The **PR number is the key and the branch only a cross-check** — measured, not
assumed: `arena/01a0968e-gifscythe` produced both #16 and #17, and `arena/01a096ec-gifscythe`
produced both #18 and #19, so a branch-name-only comparison would have passed through both skipped
syncs.

**Added `scripts/review_change.sh` — review a change instead of accepting it (owner request).**
The sweep checks whether docs still describe reality; nothing checked whether a *change* is any
good. `review_change.sh` (`--commit`/`--range`/`--patch`/`--pr`) reports five evidence-backed
checks and never edits: **R1** check-logic lines added/removed/edited (a check that is edited must
be re-proven), **R2** a matcher that matches *nothing* in the corpus, so its gate passes without
reading anything, **R3** an added line stating a count that can be measured here and disagrees,
**R4** a lost executable bit or file-mode change, **R5** the docs the change obliges you to update
(derived from what the diff touched and which docs name it). Rule: a flag with no measurement is
not printed.

Both R2 and R3 exist because of things that actually got through: PR #16 reworded the handoff
header and switched the base claim out of G10's reach, and G10 then passed vacuously for five PRs;
and a patch arrived asserting "all 35 narrative status lines match the 52 register rows" when the
file had 44. Verified against both:

* **R2 catches the G10 regression.** With the pre-fix alternation restored (case-sensitive, no
  `**Base:**` branch) the reviewer reports `FAIL [R2] vacuous matcher(s): G10 base-commit matcher
  (-E, as the gate invokes it) (0 matches across 21 docs)` — **while `check_docs.sh` itself still
  prints `PASS [G10]` and totals 22/0/3.** That divergence is the whole point.
* **R3 catches the false count.** A patch claiming 35 (and 99) narrative status lines yields
  `FAIL [R3] claims 35 narrative status lines, measured 44; claims 99 …, measured 44`.
* **R3 does not flag a count the line is correcting.** `IMPROVEMENT_LOG.md` quotes that same
  false claim in order to refute it, so R3 skips a line carrying a correction marker
  (`claim`, `assert`, `quote`, `refut`, `not true`, `incorrect`, `correcting`, `wrong`, …).
  **Known limitation:** a genuinely false claim that happens to contain one of those words is
  missed. The trade is deliberate — a reviewer that flags corrections gets ignored, and the
  bare-assertion case is the one that ships.

Two bugs in the reviewer were found by running it, and are fixed: the matcher was extracted with
"first `grep -oE '…' <<<"$content"` line", but `check_docs.sh` has three such lines, so it silently
probed the wrong regex and reported 1827 hits for a pattern that really has 1 — extraction is now
anchored on distinctive content and sanity-checked, degrading to "probe skipped" rather than
measuring something else; and the probe hardcoded `-iE` while the gate invokes `grep -oE`, which
made it report 1 hit for a matcher the gate could not use at all — it now reads the flag from the
gate line and prints it (`-E, as the gate invokes it`).

**Handoff header re-based on what a session can actually know.** It no longer asserts its own
merge sha, its own run ids, or "not yet pushed" — all of which are unknowable at write time and
went stale within one commit (the previous version cited runs `34717833397`/`34717874740` for
`4c6311e` while the tip was already `7f8ce8d`). It now carries the session, the branch, the PR
number once `gh pr create` returns it, the last merge, and one base sha that G10 checks. A new
**append-only PR ledger** replaces the prose "PR #17 was the previous merge" sentences: one row per
PR with its branch, merge sha and one-line summary, so a skipped or closed PR is visible (**#3** and
**#9** were closed without merging) and "what did PR #12 do" has an answer without `gh`.

**Not verifiable here:**

* Windows/macOS desktop behaviour (unchanged this session) and the intake's destructive
  reproductions (deliberately not executed). Everything else above is measured in this sandbox or
  read from the GitHub Actions runs named in **Merged**.

**Docs touched:** `STATUS.md` (SW-01/SW-02 + re-emitted counts), `COMPILED_AUDIT.md` (U-06/U-08
narrative), `SESSION_HANDOFF.md`, `WORKLIST.md`, `README.md`, `docs/ci/README.md`,
`docs/planning/OFFLINE_BUILD_REVIEW.md`, `docs/planning/OWNER_DECISIONS.md`,
`docs/planning/SKILLOPT_INTEGRATION_QUERY.md`, `docs/planning/NEXT_SESSION_PROMPT.md`,
`working_code/gifscythe/scripts/check_docs.sh`, `working_code/gifscythe/scripts/sweep_stale.sh`,
`working_code/gifscythe/scripts/pr_preflight.sh`.

---

## S14 — External reviews compiled; stale status claims corrected (docs only, no code fixes)  (2026-09-12)

**Changed:**

* **Added the external-review intake.** `docs/audit/EXTERNAL_REVIEW_INTAKE_2026-09-12.md` compiles
  the three reviews fetched this session — Max/GPT-class `GS-201…GS-210` (1 Critical / 4 High /
  4 Medium / 1 Low), DeepSeek `DS-06…DS-13` (renamed: its own `N-06…N-13` collided with this
  repo's existing N-series), and a Gemini deployment that returned an empty page. Each finding
  carries its evidence, impact, the reviewer's proposed solution and a re-check against
  `2d51347`. `COMPILED_AUDIT.md` §13 is the inbox. **Nothing was triaged and no finding was
  remediated** — the owner asked for a compiled intake to review first.
* **Corrected status claims that this session's verification proved false.** The
  `COMPILED_AUDIT.md` header named base `2176573` (the failing **G10** check) — it now names
  `main` `2d51347`; 35 narrative `**Status:**` lines in §2/§3/§4 still read OPEN for items the §5
  register marks fixed — each now cites its §5 row and the audit as filed follows after
  *"Original report:"*; the header carries a current-state banner (main was release-red at
  `2d51347`; fixed by this session's PR #16).
* **Two register states corrected DONE → PARTIAL** (with the missing half named):
  **U-06** — loopback bind landed, but the concurrency cap, per-client rate limit and engine-run
  bound named in the finding are still missing; **U-08** — the silent-skip is closed (both
  packagers hard-require the licence files), but the licence set itself is still incomplete (no
  full GPLv3 text, no Qt LGPL notices staged; owner decision pending). U-08 was removed from §6
  P0-3's Closes list, and `STATUS.md` was re-emitted.
* **`docs/ci/PENDING_WORKFLOW_CHANGE.md` rewritten** to describe the drift that actually remains:
  the S9 documentation-gate step is already live in `.github/workflows/build.yml`, and the only
  difference left is the Windows E2E temp-path fallback line.
* **`SESSION_HANDOFF.md`** header now states the real state: PR #15 **merged**, and the
  post-merge `main` run **RED** at the Linux documentation gate. **`WORKLIST.md`**: S14 section,
  the intake parked as a pending line, and the U-06 tick unticked (it is PARTIAL now).

**Registered (not triaged):**

* The 18 intake findings are now rows in `STATUS.md` (`UNTRIAGED`, reviewers' ids `GS-201…GS-210`
  and `DS-06…DS-13`), one pending line each in `WORKLIST.md`, plus release-blocking pointers in
  `docs/release/RELEASE_PROCEDURE.md`. Nothing was mapped into the §6 fix order, so nothing is
  scheduled — triage waits on the owner's review and the direction decision below.
* **Web plan template + direction decision:** `web/WEB_PLAN_TEMPLATE.md` (moved out of
  `docs/planning/` so it sits next to the code and is easy to find) holds the split plan — how the
  web app moves fast while the desktop/portable lanes are frozen to correctness-only, with the
  guardrails that stop the split from re-creating divergence bugs (`U-03`'s class) — and is built
  as a **template** so the owner's own draft can be refitted into it slot by slot without changing
  meaning (§0 rules). **Direction changed by the owner in S14:** `PROJECT_VISION.md` no longer
  scopes `web/` as "demo only / not the product path" — the web build is now a **supported product
  surface**, a self-hosted alternative to the `.exe`/portable build, with the offline-only
  (no cloud) promise unchanged. Desktop remains the 1.0.0 release artifact.

**Partial:**

* U-06 and U-08, as described above.

**Left:**

* The 18 intake findings are untriaged (review requested first): `COMPILED_AUDIT.md` §13.
* The workflow copy still differs from
  `.github/workflows/build.yml` (applying it needs a `workflows`-scoped token).

**Merged:** PR #16 (`arena/01a0968e-gifscythe` → `main`) merged as `629135a`; **`main` run
`34709202307` is green on linux + windows**, clearing the documentation-gate failure recorded in
`GS-208` (the stale base-commit line this session corrected).

**Verified:**

* **CI:** run `34707532582` at commit `ddc4194` on `arena/01a0968e-gifscythe` — **linux + windows
  green**, including the "Documentation status gate (STATUS.md register)" step that failed on the
  main tip in run `34705247115`.
* `working_code/gifscythe/scripts/check_docs.sh` in this clone after
  `working_code/gifscythe/scripts/bootstrap_hooks.sh`: **21 passed, 0 failed, 3 skipped** (G6
  SKIPs — no cmake/Qt6 in this sandbox, so the full-toolchain total is not measurable here); the
  G10 stale-base failure is gone and the re-emitted register reads 78 DONE / 5 PARTIAL / 17 OPEN /
  18 UNTRIAGED.
* **New gate G16 (web plan template state):** `web/WEB_PLAN_TEMPLATE.md` carries
  `**Template state:** SKELETON|WORKING PLAN` and `SESSION_HANDOFF.md` mirrors it; the gate fails if
  they disagree or name anything else, so the one-way flip at refit time cannot be half-applied.
  Mutation-tested both ways (disagreeing mirror FAILs, bad token FAILs) before shipping it.

**Not verifiable here:**

* The CI step log text (the Actions log download fails from this sandbox), so the exact failing
  sub-gate on run `34705247115` is inferred, not read; Windows/macOS behaviour; and the intake's
  destructive reproductions were deliberately not executed.

**Docs touched:** `COMPILED_AUDIT.md`, `STATUS.md`, `SESSION_HANDOFF.md`, `WORKLIST.md`,
`IMPROVEMENT_LOG.md`, `docs/ci/PENDING_WORKFLOW_CHANGE.md`,
`docs/audit/EXTERNAL_REVIEW_INTAKE_2026-09-12.md`.

---

## S13 — Product config leaves the vendored tree; U-10 reduced to workflow pinning  (2026-09-12)

**Changed:**

* Moved the product-owned native engine configuration from the former
  reference_code/gifsicle/config.h into
  `working_code/gifscythe/build_support/gifsicle/config.native.h`.
* `scripts/build_engine.sh` now stages that explicitly named config as
  `config.h` in a temporary include directory, uses it for native builds, and
  lets upstream `src/win32cfg.h` win for Windows builds. The temporary staging
  directory is removed on exit; the reference tree is never written.
* Updated `REFERENCE_MANIFEST.md` digests and provenance wording. U-10 remains
  PARTIAL only because CI hash-pinning still needs `workflows` permission.

**Partial:**

* U-10 CI hash-pinning, U-14 CI enforcement, U-09 release re-cut, U-12 GUI
  waits, clean-Windows/desktop probes, and owner decisions remain blocked or
  intentionally unstarted.

**Left:**

* Apply the pending workflow change with a `workflows`-scoped token; perform
  the release and clean-desktop evidence steps when their infrastructure is
  available. Do not relabel U-12 without a meaningful async GUI test.

**Verified:**

* `bash -n scripts/build_engine.sh && ./scripts/build_engine.sh` — native
  engine builds and reports `LCDF Gifsicle 1.96` with no reference config.
* `./build.sh` — 296 checks, 0 failures; `scripts/test_engine.sh` — 5/5;
  `scripts/smoke_cli.sh` — 19/19.
* The staged config is absent after the build, and `reference_code/gifsicle/`
  contains no `config.h`.

**Not verifiable here:**

* Windows cross-build/Wine and CI hash-pinning require the unavailable
  toolchains or a token with `workflows` scope.

**Docs touched:**

* `COMPILED_AUDIT.md`, `STATUS.md`, `README.md`, `PROJECT_VISION.md`,
  `SESSION_HANDOFF.md`, `WORKLIST.md`, `IMPROVEMENT_LOG.md`,
  `reference_code/REFERENCE_MANIFEST.md`, and `build_engine.sh`.

## S12 — Regression coverage expanded; post-merge documentation gate repaired  (2026-09-12)

**Changed:**

* **U-18 (P2-4) closed.** The CLI smoke suite now covers
  unknown and incomplete options, byte-pure binary stdout, PATH-only engine
  discovery from an isolated executable directory, and refusal of an output
  target equal to its input. Existing unit/package coverage already pins
  output planning, thread flags, and incomplete-package failure behavior.
* **Post-merge G10 repair.** `check_docs.sh` now accepts the current `main`
  merge tip or its first parent, including in a depth-1 checkout where parent
  objects are unavailable. G6 skips honestly when the full local toolchain is
  unavailable instead of comparing reduced audit totals with the full-toolchain
  historical headline.

**Partial:**

* U-10 and U-14 remain PARTIAL; U-09, U-12 and the remaining release/desktop
  items stay OPEN as recorded in `STATUS.md`.

**Left:**

* U-10 `config.h` relocation and workflow hash-pinning; U-09 release re-cut;
  U-12 async GUI waits; U-14 workflow enforcement; clean Windows and
  physical-desktop probes; owner decisions.

**Verified:**

* `./build.sh`: 296 checks, 0 failures.
* `scripts/test_engine.sh`: 5/5.
* `scripts/smoke_cli.sh`: 19/19.
* Web suites: 17 + 23 + 30; packaging negatives: 9/9.
* `scripts/check_docs.sh`: 20 passed, 0 failed, 2 skipped in this sandbox.
* GitHub Actions run `34688399245` and PR run `34688533670`: Linux and Windows
  successful for commit `91afbdd`.

**Not verifiable here:**

* This sandbox currently has no CMake, Qt6, MinGW, or Wine. The local audit
  therefore skips the GUI/CMake/Wine-dependent checks; the GitHub Linux/Windows
  matrix validates the submitted build.

**Docs touched:**

* `COMPILED_AUDIT.md`, `STATUS.md`, `README.md`, `PROJECT_VISION.md`,
  `SESSION_HANDOFF.md`, `WORKLIST.md`, `docs/release/RELEASE_PROCEDURE.md`,
  and this log.

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
* **N-05 (new finding, closed in-session).** While designing the U-41 web
  explode rule I probed the desktop path: `gifsicle -e a.gif b.gif -o p`
  exits **0**, explodes every input except the LAST as `<basename>.NNN` into
  the process CWD, and writes only the last input's frames under the prefix —
  and the GUI queued ALL inputs into one explode run (U-17's prefix check
  would even report the single leftover frame as "complete"). Refused the
  same session at every layer: `validate()` warning (C++ + byte-identical JS
  mirror, parity fixture), CLI `--run` rc=2 before any process starts (print
  mode keeps the documented warn-and-print policy), GUI warning dialog +
  "REFUSED" output summary; tests: unit block 35, smoke case 12 (refusal +
  print-policy lines), harness T7 subcase (engine never starts, no frames,
  summary honest), Wine rc=2 + zero frames. The web `/run` already refused
  multi-file explode by design.
* **N-06 (new finding, closed in-session).** The sandbox restarted late in
  the session and came back WITHOUT the apt-installed gawk — and suddenly G0
  failed on a tree that was byte-identical to a pushed, CI-green head. Root
  cause: the emitter truncates §5 proof notes at 150 via awk
  `length()`/`substr()`, which count CHARACTERS in gawk under a UTF-8 locale
  but BYTES in mawk; the U-17 note had an en-dash before the cut, so the
  gawk-emitted committed STATUS.md (`wine ...`) differed by one byte from
  mawk's (`win...`). The gate had been non-deterministic across sandboxes
  since S9 — it only ever bit when a truncated note contained multibyte text.
  Fixed in the checker, same session: `export LC_ALL=C` at the top of
  `check_docs.sh` (byte semantics under every awk, sort and grep), and the
  five S11 §5 notes reworded ASCII-only and under the 150 limit so no
  truncation can ever cut a multibyte character in half. `--emit` output
  verified byte-identical under mawk AND gawk before re-push.
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
* **Tests/fixtures added:** unit blocks 33 (ExplodeVerify) + 34 (splitter) +
  35 (N-05 validate) → runtime **296 checks** (source occurrences: unit 261,
  harness 250 `CHECK(` sites); `tests/fake_engine_exit0.cpp` + CMake target
  (static under MINGW); harness T7 extension (lying engine + N-05 refusal) →
  **324 runtime checks**; smoke **9→14**; web suites command **15→17**,
  validate **19→23**, transport **18→30**; verify_audit C9 gate + gate B now
  builds the fixture target.
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
* **PUSHED, PR #14 OPEN, CI GREEN.** Branch `arena/s11-gifscythe` pushed
  after the push-time gate run; **PR #14** opened against `main`. The FIRST
  CI run (`34671814580` on `f655987`) failed windows-only, and the failure
  was a real (cosmetic) portability finding in the new T7 assertion:
  `locate_engine()` returns NATIVE separators (backslashes) while
  `applicationDirPath()` uses forward slashes, so the full-path `contains()`
  check for the lying-engine status could only pass on POSIX — every U-17
  behavioral assertion passed on Windows CI untouched. Fixed in `88e15ea`
  by comparing the basename. The N-05 fix + a wrapped README count claim
  followed as `2f2eee6` / `74091b7`, and the N-06 emitter fix as `635c986`.
  **Final verdict: run `34685470992` on `635c986` — linux success + windows
  success** (every intermediate head was green too: `88e15ea` run
  `34672175363`, `74091b7` run `34677506828`; only the very first run
  `34671814580` on `f655987` failed, windows-only, on the T7 separator
  assertion). The windows job was the first NATIVE compilation of the S11
  Windows code (CreateProcessW, `_wopen`, the splitter) and ran the extended
  harness green. PR #14 is fully green at its head; merge remains the owner's
  call (rule 3 re-applies before merge).

**Verified (run in this sandbox):**

* `./build.sh` → **296 checks, 0 failures**; `test_engine.sh` 5/5;
  `smoke_cli.sh` **14/14**; `test_package.sh` 9/9.
* GUI offscreen harness (compiled AND run locally, Qt 6.4.2): **324 checks,
  0 failures** (T1–T20, T7 extended) — the new last-measured figure (306 @ S10).
* Web: `command.test.mjs` **17**, `validate.test.mjs` **23**,
  `transport.test.mjs` **30** (live server + real engine).
* `verify_audit.sh` → **28 PASS / 0 FAIL / 3 SKIP, exit 0** (skips: E9
  declared-pending workflow, CI-gated, clean-Windows); `check_docs.sh` →
  **21 passed, 0 failed, 1 skipped** (the skip is G7, same declared drift).
* Wine matrix (all executed): old CLI rc=1 + mojibake repro; new CLI é-conf
  rc=0 + output written; é-conf-path argv rc=0; é-`GS_ENGINE` rc=0; CJK child
  cmdline byte-exact (probe); unit exe green under Wine at **292 runtime checks (0 failures)** — the 4 POSIX-signal checks compile out on Windows; multi-input explode refused rc=2 with zero frames; explode
  real-engine `12 frame(s)` rc=0; lying engine rc=1 naming the prefix. Engine
  exe rebuilt via `build_engine.sh --windows`: `LCDF Gifsicle 1.96 (Windows)`.
* U-15 before/after repro executed both halves (read-only `src/` uid 65534;
  deleted-fallback copy build). U-10: clone + `diff -rq` + digests executed;
  `git status` proves the vendored trees were never modified.
* awk parity: `check_docs.sh`/`verify_audit.sh` behavior verified under mawk
  AND gawk (gawk apt-installed this session — the PR #13 CI-awk scar), and
  after N-06 the `--emit` output was re-verified BYTE-IDENTICAL under both
  (with `LC_ALL=C` in the gate, and again without gawk's UTF-8 locale
  advantages — the restart proved the mawk path the hard way).

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
