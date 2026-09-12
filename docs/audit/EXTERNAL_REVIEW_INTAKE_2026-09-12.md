# Gifscythe — External review intake (compiled for review) — 2026-09-12

> **INTAKE ONLY. Nothing in this file has been fixed, triaged into the `U-nn`
> register, or acted on.** It exists so a later session can review the newly
> reported problems, risks and proposed solutions in one place, with the
> reviewer's own words next to an independent re-check against the shipped
> source. No repository code, script, workflow or status document was changed
> to produce it. Proposed reproductions are the reviewers'; destructive ones
> were deliberately **not** executed.

**Compiled by:** Arena session, 2026-09-12 (S14 intake; not a remediation session)
**Repository:** `freeforall1932-design/gifscythe`
**Reviewed revision:** `main` at `2d51347817f5cdb39334415a03bb5f2b543119dd` (local checkout, read-only reads)
**Method:** fetch the three review URLs, verify each cited symbol against the shipped tree,
confirm the live CI state through the GitHub API, reproduce the documentation gate locally
**Register impact:** none yet — `COMPILED_AUDIT.md` §5 / `STATUS.md` are untouched
(see §7 "How to fold this in later")

---

## 0. How to read this file

| Column / mark | Meaning |
|---|---|
| **GS-2nn** | Finding ID from the highest-ranked source (Max via OpenAI; the GS- series continues the compiled audit's own `A:GS-` namespacing). |
| **N-06…N-13** | Finding ID from the DeepSeek review. Kept distinct on purpose: no two reviewers are merged into one ID until triage. |
| **[V-SRC]** | Re-verified here by reading the shipped source at this revision; the code says what the reviewer says it says. |
| **[V-LIVE]** | Re-verified against live GitHub state (API), not prose. |
| **[V-GATE]** | Re-verified by running the repository's own checker locally. |
| **[V-PART]** | Partly confirmed — the static claim holds, the runtime consequence is not proven in this sandbox. |
| **[UNVERIFIED]** | Reviewer prose only; no cheap check was possible here. |
| **[CONFLICT]** | Two sources (or a source and the register) disagree — both views recorded. |

**Priority ranking requested by the owner** (applied to triage order only, not to whether a
finding is real — every finding below that says **[V-SRC]** is real at this revision):

1. **Max via OpenAI (the `GS-2nn` set)** — treated as the highest tier.
2. **DeepSeek (`N-06…N-13`)** — treated as below the GPT-class review.
3. **Gemini 3.8 flash high** — treated as below DeepSeek (bloated page size), and in practice
   it delivered **no content at all** (see §1).

---

## 1. Sources fetched

| # | Source | Reviewer (as labelled) | URL | What actually came back | Findings |
|---|---|---|---|---|---|
| 1 | Gemini 3.8 flash high | Gemini | `https://01a09675-9bfc-7781-b0f6-89fd5ca02f43.arena.site/` | **Empty application scaffold.** Both fetches returned the stock "Ready to build / Start prompting to build your app" page. No audit, no findings, nothing to extract. | **0** |
| 2 | DeepSeek v4 pro | DeepSeek | `https://01a09675-9bfc-7bd3-965c-9c135d6f02d0.arena.site/` | Full page, 3 chunks: an 8-finding review plus an audit-reconciliation table over `U-01…U-52`. | **8** |
| 3 | Max via OpenAI ("presumably GPT 6 Astra") | Max / GPT-class | `https://01a09675-2e39-7377-a362-5eddec16075f.arena.site/` | Landing page ("Codelens"); the full report was supplied by the owner as text rather than fetchable per-finding routes (`/findings` → "Not found"). The landed page matches the supplied text: same 10 findings, same severities, same file references. | **10** |

**Action needed from the owner on source 1:** the Gemini page served an empty scaffold, so no
Gemini findings exist to compile. If that review produced content, it was not published to that
deployment — a working URL or export is needed before it can be ranked against the other two.

**Capture caveat:** DeepSeek's page does not pin a commit. Every line reference below was
re-checked at `2d51347`, so where the two differ, the line numbers here are the authoritative
ones for this revision (differences are called out per finding).

---

## 2. Headline

* **Ten findings** in the highest-tier review (1 Critical, 4 High, 4 Medium, 1 Low) and
  **eight** in the DeepSeek review (1 High, 3 Medium, 2 Low, 2 Info) — **18 new items**, none of
  which is a `U-nn` row today.
* **One of them is a data-loss path that the existing planner does not cover:** the CLI maps the
  product's "Batch" mode onto the engine's in-place `-b`, and because the output planner is
  skipped when no `output` key is set, `--run` can rewrite the user's source GIFs (**GS-201**,
  [V-SRC]).
* **The web `/run` endpoint uses client-supplied upload names to build on-disk output paths**, so
  a name containing `../` escapes the per-request temp directory (**GS-202**, [V-SRC]).
* **"Exit 0 + non-empty" is still treated as success outside Explode**: the CLI verifies only
  Explode frames, and the desktop and web paths check existence/size only — so a stale or
  non-GIF file can be reported as a successful run (**GS-203** [V-SRC]; the same class at the
  web `/optimize` endpoint is **N-13** [V-SRC]).
* **Current main is release-red [V-LIVE]:**
  Actions run `34705247115` (the merge of PR #15) failed: the **Linux documentation gate**
  failed, and every Linux step after it — web parity, GUI offscreen tests, packaging, artifact
  upload — was skipped; Windows passed. This is a recurrence of the class the doc gate exists to
  prevent, and it is contradicted by `SESSION_HANDOFF.md` line 4.
* **Local reproduction of the gate [V-GATE]:** `check_docs.sh --no-gate-run` on this revision
  reports **18 passed, 2 failed, 3 skipped**. The failures are **G10** (a stale base-commit
  claim in `COMPILED_AUDIT.md` — it names `2176573`, while the accepted bases are
  `2d51347`/`53a6eda`) and **G15** (this fresh clone's `core.hooksPath` is not `.githooks`).
  G10 is the probable CI failure; the exact CI sub-gate could not be confirmed because the job
  log download from this sandbox fails at the transport level (§6).
* **The compiled audit contradicts itself [V-SRC]:** `COMPILED_AUDIT.md` §3 still marks
  A-03/GS-003 and A-10/GS-010 as `⬜ OPEN` while §5 marks U-04 and U-23 `✅ FIXED (S8)` — the
  DeepSeek **N-11** observation, confirmed.
* **Threads sentinel [V-SRC]:** the product can no longer express "do not touch threading".
  `threads` defaults to `-1`, and the builder emits a bare `-j` for every value `<= 0`; upstream
  defaults to `thread_count = 0` (single-threaded) and a bare `-j` selects
  `GIFSICLE_DEFAULT_THREAD_COUNT = 8`. A conf with no `threads` key therefore silently runs
  8-way parallel (**N-06**; the unit test pins the conflation).

---

## 3. New findings — highest-tier review (`GS-2nn`)

### 3.1 GS-201 — Critical — CLI Batch mode overwrites every source file in place

* **Class:** new class (not covered by `U-01…U-52`)
* **Where:** `working_code/gifscythe/src/cli/main.cpp`, `working_code/gifscythe/src/core/GifsicleCommand.h`
* **Verification:** **[V-SRC]**

**Problem.** The product describes Batch as "optimize each file separately", but the CLI forwards
`Mode::Batch` as `gifsicle -b`. Upstream defines `-b` as *modify each input in place*. With no
`output` key the CLI's output planner is skipped entirely, so `--run` can replace the user's
source GIFs and still return success. The desktop GUI and the web runner both implement Batch as
one Auto command per planned output — the CLI is the only layer where Batch means "destroy the
inputs".

**Evidence (as re-verified at this revision).**

* `GifsicleCommand.h:93` — `case Mode::Batch: add(args_, "-b"); break;`
* `GifsicleCommand.h:230` appends the inputs, `GifsicleCommand.h:233` appends `-o <file>` **only
  when output is non-empty** — so a Batch conf with no `output` produces `-b in1.gif in2.gif …`.
* `main.cpp:284` — `if (!s.output.empty() && s.mode != gs::Mode::Explode) { plan_outputs(...); }`
  — the safety plan that refuses source-overwriting runs is unreachable when `output` is empty.
* `main.cpp:325-326` — the run is executed and only Explode output is verified.
* The parity test pins the behaviour without executing it:
  `tests/test_gifsicle_command.cpp:263-268` asserts `-b` is emitted.
* Upstream contract: `reference_code/gifsicle/gifsicle.1:190` —
  *"Modify each GIF input in place by reading and writing to the same filename."*

**Risk.** Silent, irreversible destruction of user source GIFs from a documented code path; the
tool reports success. Worse than U-01's original collision because there is no output key to
validate against.

**Proposed solution (reviewer).** Do not map product Batch directly to `-b`. Extract a shared
batch orchestrator that derives and validates one output per input and runs each job in Auto
mode, exactly like the GUI and web server. Until that exists, make CLI `--run` refuse
`Mode::Batch` with exit 2. If in-place editing is ever wanted, expose it as a separate, explicit
`--in-place` contract.

**Intake note.** The "refuse with exit 2" step is the minimal stop-loss and can land before the
orchestrator. Any fix must add a test that *executes* the path against a disposable copy — the
existing test only inspects argv, which is how this survived the U-01 remediation.

---

### 3.2 GS-202 — High — The web `/run` endpoint builds filesystem paths from upload names

* **Class:** new class (U-41 added `/run` after the earlier audits)
* **Where:** `web/server.mjs`
* **Verification:** **[V-SRC]**

**Problem.** Input *bytes* are staged under neutral names (`in0.gif`, `in1.gif`, …), but the
client-controlled `files[].name` is still reused to derive output paths. A name containing `../`
produces a target outside the request temp directory. Batch collision checks compare raw strings
case-sensitively, so case-only differences can also collapse on Windows and silently return two
references to the last output.

**Evidence (as re-verified at this revision).**

* `web/server.mjs:285-288` — `stemOf(name)` strips only after the **last dot**; it does not strip
  path separators, so `../../outside.gif` → stem `../../outside`.
* `web/server.mjs:426` — `targets = files.map((f) => \`${stemOf(f.name)}_opt.gif\`)`.
* `web/server.mjs:481` (batch), `:493-494` (explode prefix), `:523` (auto) — each does
  `join(dir, <name-derived target>)`, and `join` resolves `..` above the temp dir.
* `web/server.mjs:430-434` — the collision check is `seen.has(targets[i])` on raw strings: no
  host-aware/case-folded comparison.
* The comment immediately above the upload loop claims "nothing from the request ever becomes a
  path here" — the targets below it are exactly that.
* The only containment check in the file is for static assets (`web/server.mjs:138`); `/run`
  outputs have none.

**Risk.** Arbitrary-path file write with a server-chosen suffix, under a process that may run as
the developer; on Windows, silently collapsed batch outputs. Reachable from a single crafted
upload name.

**Proposed solution (reviewer).** Treat upload names as display metadata only. Generate every
on-disk input, output and prefix from server-owned IDs. Sanitize the *download* name in the
response separately. Require each resolved target to start with `resolvedTempDir + path.sep`.
Use host-aware canonical comparison for collisions, including Windows case folding.

**Intake note.** Cheap interim hardening that does not change behaviour: reject any upload name
containing `/` or `\\` before planning, and apply the resolved-target containment assertion at
the three `join` sites. Both are testable with the existing web transport suite.

---

### 3.3 GS-203 — High — Ordinary runs can report success without producing a new valid GIF

* **Class:** new class (the "exit 0 is not proof" fix stopped at Explode)
* **Where:** `working_code/gifscythe/src/cli/main.cpp`, `working_code/gifscythe/src/qtui/MainWindow.cpp`, `web/server.mjs`
* **Verification:** **[V-SRC]**

**Problem.** Post-run verification exists only for Explode. Auto, Merge and direct Batch return
exit 0 unchanged even when nothing was written. The desktop and web paths check only existence
and non-zero size, so a stale pre-existing output — or a text file — is accepted as a successful
GIF. None of the ordinary paths checks the `GIF87a`/`GIF89a` magic, and none proves *this run*
created or changed the target.

**Evidence (as re-verified at this revision).**

* `src/cli/main.cpp:325-326` — `int rc = gs::run_argv(full_argv); if (rc == 0 && s.mode == gs::Mode::Explode) verify_explode_frames(...);`
* `src/qtui/MainWindow.cpp:986` and `:1147-1148` — `if (!fi.exists() || fi.size() == 0)` is the
  whole postcondition.
* `web/server.mjs:465-466` — `if (!st || st.size === 0)` is the whole postcondition in `runOne`.
* A suitable non-executed fixture already ships: `working_code/gifscythe/tests/fake_engine_exit0.cpp`
  (used by `working_code/gifscythe/scripts/verify_audit.sh:145`), so the reproduction is cheap.

**Risk.** A lying, crashed-mid-write or misconfigured engine is reported as success; a user can
be shown (or served) a stale or non-GIF artefact with a success status. This is the product's
core "honest success" promise applied to the paths that were not yet covered.

**Proposed solution (reviewer).** Add a shared `SingleOutputVerify` helper: snapshot the target
before launch, and after exit 0 require a newly created or changed, non-empty file with a
`GIF87a`/`GIF89a` header. Use it in CLI and GUI, mirror it in Node. Writing to a temporary
sibling plus a validated atomic replacement would additionally preserve an older output if the
engine dies mid-write.

**Intake note.** Overlaps **N-13** (web `/optimize` has no magic check at all). One helper with
three call sites closes both; do not fix them as two independent patches.

---

### 3.4 GS-204 — High — Packaging remediation remains fail-open outside its happy path

* **Class:** incomplete fix (the unclosed part of A:GS-002/GS-007 and U-02/U-08/U-14)
* **Where:** `working_code/gifscythe/scripts/package_system.sh`, `working_code/gifscythe/scripts/package_portable.sh`, `working_code/gifscythe/scripts/test_package.sh`
* **Verification:** **[V-SRC]**

**Problem.** The system packager still copies every binary and license conditionally, never
clears its destination, and always prints success. The portable packager stages into the final
directory (not an atomic temp), silently skips Qt deployment when `windeployqt` is absent, and
selects the first native/`.exe` candidate without proving a single platform. The negative test
suite covers portable packaging only and omits wrong-platform and missing-deployer cases.

**Evidence (as re-verified at this revision).**

* `working_code/gifscythe/scripts/package_system.sh:9` — `mkdir -p "$out"` (stale files survive);
  `:23`, `:26-30` — conditional `cp` for engine/CLI/GUI and licenses; `:39` — unconditional
  `printf 'System-dependent package created: …'`.
* `working_code/gifscythe/scripts/package_portable.sh:46-47` — `rm -rf`/`mkdir` on the **final**
  path; `:53-81` — candidate list mixes native and `.exe` layouts; `:100-104` — `windeployqt`
  runs only `if command -v windeployqt`, so a Windows GUI package can be produced with no Qt
  runtime (a windeployqt *failure* is fatal; its *absence* is not).
* `working_code/gifscythe/scripts/test_package.sh:3` — its own header says it is the negative
  suite for `package_portable.sh`; the system packager has no negative coverage.
* `LICENSE` is a short "intended" notice, not the complete GPLv3/LGPL notice set that Audit A
  asked for.

**Risk.** A "successful" package that is incomplete, mixed-platform, or cannot start on a clean
machine — the exact failure the U-02 remediation was meant to eliminate, still reachable outside
the covered happy path.

**Proposed solution (reviewer).** Replace both scripts with one manifest-driven stager
parameterised by target platform and package type. Stage into `mktemp`, require one
platform-consistent engine/CLI/GUI set, require the Windows deployer and runtime manifest,
require complete legal material, run the packaged CLI, inspect binary formats/dependencies, then
atomically replace the final directory. Extend the negative tests to both package types.

**Intake note.** Status documents currently present packaging as closed for portable only; this
finding is about the *scope* of the claim, not a regression.

---

### 3.5 GS-205 — Medium — Non-GIF inputs still enter the desktop queue through supported UI paths

* **Class:** incomplete fix (U-13 closed the boolean typo only)
* **Where:** `working_code/gifscythe/src/qtui/MainWindow.cpp`
* **Verification:** **[V-SRC]**

**Problem.** Admission is not centralised. The Add dialog offers `All files (*)` and
`appendInputs` accepts every selected path; drag-and-drop accepts an existing *directory* named
`*.gif` because it checks `exists()`, not `isFile()`. Neither path verifies the GIF magic before
Run becomes available.

**Evidence (as re-verified at this revision).**

* `src/qtui/MainWindow.cpp:405` — `getOpenFileNames(..., "GIF files (*.gif);;All files (*)")`,
  then `:406` `appendInputs(files)`.
* `src/qtui/MainWindow.cpp:358-406` — `appendInputs` appends without validation.
* `src/qtui/MainWindow.cpp:415` — `f.endsWith(".gif", Qt::CaseInsensitive) && QFileInfo::exists(f)`
  — no `isFile()`, no magic check.

**Risk.** Wrong files enter the queue and are discovered only when the engine rejects them;
queue size, run enablement and (later) batch planning operate on non-GIF inputs.

**Proposed solution (reviewer).** One `admitInputs()` used by picker, drop and future sources:
require an existing, readable regular file, check the first six bytes for `GIF87a`/`GIF89a`,
normalise for dedupe, and report rejected items. Keep a final pre-run recheck for files changed
after admission.

---

### 3.6 GS-206 — Medium — Large and invalid numeric settings can silently become valid actions

* **Class:** incomplete fix (A:GS-011 asked for overflow, NaN/Inf, thread, loop and gamma coverage)
* **Where:** `working_code/gifscythe/src/core/SettingsIO.h`, `working_code/gifscythe/src/core/Validate.h`
* **Verification:** **[V-PART]** — the narrowing and the missing validations are source-confirmed;
  the exact wrapped values were not executed here.

**Problem.** Values are parsed as `long` and narrowed to `int` with no range check, so on LP64 a
value above `INT_MAX` can be accepted and then narrow implementation-defined into something
validation considers legitimate. Validation still ignores loop counts below `-1`, negative
threads, gamma finiteness/range, and allowed method names, so typos can vanish or select a
different operation.

**Evidence (as re-verified at this revision).**

* `src/core/SettingsIO.h:132` (`need_long`), `:187`, `:191`, `:195`, `:199`, `:204`, `:208`,
  `:226` — every integer field goes through `static_cast<int>(tmp)` with no bounds check.
* `src/core/Validate.h` covers `colors`, `disposal`, `optimize`, `lossy`, `delay`, `info`,
  multi-input explode, crop and resize geometry — and nothing for `loopcount`, `threads`, `gamma`,
  `dither`/color/resize method names.
* `src/core/GifsicleCommand.h:226` — `threads <= 0` emits a bare `-j` (see N-06), so a negative
  threads value that validation ignores becomes "auto" instead of a refusal.

**Risk.** A settings file that looks valid produces a different operation than written; scripted
and GUI runs diverge from the documented contract.

**Proposed solution (reviewer).** Parse directly into the destination width with
`std::from_chars` and reject outside `INT_MIN..INT_MAX` before assignment. Define domains for
every sentinel-bearing integer, require finite gamma and scale, and validate string enums against
engine-truth tables. Keep the JS mirror (`web/validate.mjs`) synchronised.

---

### 3.7 GS-207 — Medium — An invalid `GS_ENGINE` override silently falls back to another engine

* **Class:** incomplete fix (the strict-override half of A:GS-004 was never closed)
* **Where:** `working_code/gifscythe/src/core/EngineLocator.h`, `web/server.mjs`
* **Verification:** **[V-SRC]**

**Problem.** `GS_ENGINE` is documented as an override, but an unusable value is merely the *first
candidate*; the locator continues to packaged, development and `PATH` engines. The web server
repeats the pattern. Automation that intends to test one specific engine can mistype the path and
successfully run a different binary.

**Evidence (as re-verified at this revision).**

* `src/core/EngineLocator.h:100-140` — `GS_ENGINE` is pushed as candidate 0, then packaged/dev/
  CWD candidates are appended, then `find_on_path(base)`; the loop returns the first executable
  match with no signal about which candidate won or that the override was ignored.
* `web/server.mjs` `findEngine()` — `if (process.env.GS_ENGINE) { try { await access(...); return …; } catch {} }`,
  then it scans release versions: an invalid override is swallowed.

**Risk.** False confidence in engine-specific testing and release validation; the tool reports
success while running a different engine than specified.

**Proposed solution (reviewer).** Return a typed `EngineResolution` carrying source and error. A
non-empty `--engine` or `GS_ENGINE` must be strict: if unusable, stop and name it. Discover only
when no override was supplied. Log the selected source at GUI and web startup.

---

### 3.8 GS-208 — High — Current main is red while status documents claim a green open PR

* **Class:** regression (recurrence of N-01/U-27 after the mechanism intended to prevent it)
* **Where:** `.github/workflows/build.yml`, `SESSION_HANDOFF.md`, `docs/ci/PENDING_WORKFLOW_CHANGE.md`, `COMPILED_AUDIT.md`
* **Verification:** **[V-LIVE] + [V-GATE]**

**Problem.** Current `main` is the merge of PR #15, but `SESSION_HANDOFF.md` still describes
PR #15 as open and quotes a green branch run; the live main run failed in the Linux
Documentation status gate, skipping every later Linux step. The pending-workflow marker still
describes a change that is already present in the live workflow.

**Evidence (as re-verified this session).**

* Live API: `main` = `2d51347817f5cdb39334415a03bb5f2b543119dd`; run `34705247115` (push, main) —
  job **linux failed** at step "Documentation status gate (STATUS.md register)"; all Linux steps
  after it are skipped; job **windows succeeded**.
* `SESSION_HANDOFF.md:4` — "**PR #15** open against `main`, CI green on commit `c105f3c`".
* `.github/workflows/build.yml:35-37` — the documentation gate step is present and runs
  `./scripts/check_docs.sh --no-gate-run`.
* `docs/ci/PENDING_WORKFLOW_CHANGE.md:28` — "the PREVIOUS pending change is already applied"; the
  gate agrees, reporting G7 as a declared-pending SKIP even though `build.yml` and
  `docs/ci/build.yml.proposed` differ.
* Local reproduction `[V-GATE]`: `working_code/gifscythe/scripts/check_docs.sh --no-gate-run` →
  **18 passed, 2 failed, 3 skipped**, failing **G10** (stale base-commit claim: `COMPILED_AUDIT.md`
  names `2176573`; accepted bases are `2d51347` / merge first parent `53a6eda`) and **G15**
  (fresh clone: `core.hooksPath` is not `.githooks`).
* **Probable CI root cause:** G10, matching the local failure. G15 is a local-clone artefact —
  CI runs `build.sh`, which bootstraps the hook. G6 and G7 are SKIP under `--no-gate-run`. This
  is **not** confirmed: the CI job log could not be downloaded from this sandbox (§6).

**Risk.** Release-red main treated as green; the next session starts from corrupted context and
the pending-workflow marker keeps a fixed change in limbo.

**Proposed solution (reviewer).** Treat main as release-red. Reproduce
`check_docs.sh --no-gate-run` from a fresh depth-1 checkout, fix the failing sub-gate, then update
status from the merged commit. Sync the workflow copies, remove the stale marker if no functional
change remains, regenerate `STATUS.md`, and rerun both jobs. Do not claim green until the
exact-SHA run passes.

**Intake note.** Two concrete, verifiable sub-tasks fall out of the local run: (a) the base-commit
line in `COMPILED_AUDIT.md` is stale and fails G10 whenever a full-history clone is used — note
that this contradicts the gate's own comment that a shallow CI checkout makes G10 SKIP, so the
CI failure cause needs to be read off the runner, not assumed; (b) the pending-workflow marker
should be resolved either way, since it currently suppresses G7 by declaration.

---

### 3.9 GS-209 — Medium — The native Linux/mac engine build still uses a fixed Linux/glibc ABI config

* **Class:** incomplete fix (S13 moved the file; it did not make the values target-detected)
* **Where:** `working_code/gifscythe/build_support/gifsicle/config.native.h`, `working_code/gifscythe/scripts/build_engine.sh`
* **Verification:** **[V-SRC]**

**Problem.** Moving `config.h` out of the vendored tree fixed ownership, not target detection. The
build script advertises a native Linux/mac build, while the configuration hardcodes Linux/gcc
headers, glibc `random()`, 64-bit type sizes, SIMD support and `gettimeofday` details. Nothing
selects values for the active native target.

**Evidence (as re-verified at this revision).**

* `working_code/gifscythe/scripts/build_engine.sh:6-7` — usage: "build native (linux/mac) gifsicle".
* `working_code/gifscythe/build_support/gifsicle/config.native.h` — "Headers available on
  Linux/gcc"; `#define RANDOM random` ("Random function on glibc"); fixed type sizes
  (`SIZEOF_UNSIGNED_LONG 8`, `SIZEOF_VOID_P 8`); `HAVE_SIMD 1`; `GETTIMEOFDAY_PROTO 2`.
* `working_code/gifscythe/scripts/build_engine.sh:31-35` — target selection is only
  `--windows` vs anything-else-is-native; no configure/CMake feature checks run.

**Risk.** Advertised macOS/musl/32-bit builds fail to compile or misbehave at runtime
(gifsicle's `main()` static-asserts `sizeof(unsigned long)`).

**Proposed solution (reviewer).** Generate `config.h` in the build tree from upstream configure
or CMake feature checks per supported target; add macOS/musl CI before advertising them.
Otherwise narrow the documentation to x86_64 glibc Linux instead of shipping guessed macros.

---

### 3.10 GS-210 — Low — Primary build entry points silently accept mistyped options

* **Class:** new
* **Where:** `working_code/gifscythe/build.sh`, `working_code/gifscythe/scripts/build_engine.sh`, `working_code/gifscythe/gifscythe.pro`
* **Verification:** **[V-SRC]**

**Problem.** `build.sh`'s argument loop has no default branch, and `build_engine.sh` treats every
first argument other than `--windows` as native. A typo produces the wrong target with exit 0.
The GUI dispatcher tries qmake before the documented preferred CMake, and the qmake project file
hardcodes its version.

**Evidence (as re-verified at this revision).**

* `working_code/gifscythe/build.sh:30-34` — `case` handles `--gui|--all` and `--windows` only;
  unknown arguments are ignored.
* `working_code/gifscythe/scripts/build_engine.sh:31-35` — `if [[ "${1:-}" == "--windows" ]] …`
  then falls through to native: `--windwos` builds native.
* `working_code/gifscythe/build.sh:157` — `if build_gui_qmake || build_gui_cmake;` (qmake first).
* `working_code/gifscythe/gifscythe.pro:17` — `VERSION = 0.1.0`.

**Risk.** Wrong-target builds that look successful; a secondary GUI build path that drifts from
the release path.

**Proposed solution (reviewer).** Strict option parsers with help and unknown-option exit 2. Make
CMake the sole release build (or try it first and test qmake separately). Derive qmake metadata
from `VERSION.md`, or remove the unmaintained secondary path.

---

## 4. New findings — DeepSeek review (`N-06…N-13`)

DeepSeek delivered eight findings plus a reconciliation of the existing register. All eight were
re-checked against `2d51347`; every one holds. Paraphrase of its own framing, then the detail.

### 4.1 N-06 — High — Threads sentinel collision: `-1` "unset" now means the same as `0` "Auto"

* **Where:** `working_code/gifscythe/src/core/GifsicleCommand.h`, `working_code/gifscythe/src/core/GifsicleSettings.h`, `working_code/gifscythe/tests/test_gifsicle_command.cpp`
* **Verification:** **[V-SRC]**

**Problem.** Emitting a bare `-j` for every `threads <= 0` (the U-03 fix) removed the meaning of
the `-1` "unset" sentinel. A conf with no `threads` key now silently runs 8 threads instead of the
engine's single-threaded default. There is no longer any way to express "emit no `-j` at all".

**Evidence (re-verified).** `src/core/GifsicleCommand.h:226` — `if (s.threads > 0) … else add(args_, "-j");`;
`src/core/GifsicleSettings.h:75` — `int threads = -1; // -j; <=0 = auto`;
upstream `reference_code/gifsicle/src/gifsicle.c:39` — `int thread_count = 0;` (default),
`:38` — `const int GIFSICLE_DEFAULT_THREAD_COUNT = 8;`, `:1893` — bare `-j` sets 8.
The conflation is pinned by `tests/test_gifsicle_command.cpp:355-365`
(`CHECK(def.threads == -1); CHECK(has(GifsicleCommand(def).args(), "-j"));`).

**Risk.** A conf written to leave threading untouched changes runtime behaviour (up to 8 threads,
more memory, different merge determinism). The settings comment contradicts the implementation.

**Proposed solution (reviewer).** Restore three distinct states: `threads < 0` → emit nothing
(engine default, single-thread); `threads == 0` → bare `-j` (explicit Auto); `threads > 0` →
`-jN`. Update the unit test to assert `-1` emits no flag, keep `0 → -j`, and re-align the
`GifsicleSettings.h` comment to "<0 = no flag; 0 = auto; >0 = -jN".

**Intake note.** Interacts with **N-09** (`threads < -1` is unvalidated) and **GS-206**
(numeric domains). Decide the tri-state once, in one place, then make validation, GUI and tests
agree.

---

### 4.2 N-07 — Medium — GUI Threads spinner cannot express the "no flag" state

* **Where:** `working_code/gifscythe/src/qtui/SettingsPanel.cpp`
* **Verification:** **[V-SRC]**

**Problem.** The spinner has range `0..64` with "Auto" at 0 (reviewer cites ~`:345-350`; actual
`:346-352`), and the panel unconditionally writes the value, so the GUI can only ever emit `-j`
or `-jN`. Combined with N-06, every GUI run forces threading on.

**Evidence (re-verified).** `src/qtui/SettingsPanel.cpp:349-352` — `setRange(0, 64)`,
`setValue(0)`, `setSpecialValueText("Auto")`; `:594` — `s.threads = threadsSpin_->value();`.

**Risk.** The UI and the settings model disagree about the control's meaning; the default GUI run
is 8-way parallel even when the user changed nothing.

**Proposed solution (reviewer).** Either map the spinner minimum to `-1` ("Unchanged") with `0`
as "Auto" and `>=1` explicit N, or document clearly that the GUI is always explicit about
threading. The requirement is that `-1` and `0` keep distinct meanings end to end.

---

### 4.3 N-08 — Low — CLI print mode returns 0 even when validation warns (without `--strict`)

* **Where:** `working_code/gifscythe/src/cli/main.cpp`
* **Verification:** **[V-SRC]**

**Problem.** In print mode without `--strict`, `validate()` warnings are printed and `main`
returns 0, so a script cannot distinguish "valid" from "warned" without parsing stderr.

**Evidence (re-verified).** `src/cli/main.cpp:274-277` — after printing warnings,
`if (!do_run) { note("# (use --run to execute)"); return 0; }`; strict refusal is opt-in at
`:216-222` (exit 3).

**Risk.** Automation that treats exit 0 as "settings are sound" silently replicates the
warn-then-proceed policy.

**Proposed solution (reviewer).** Document (or log) that non-strict print/run modes are advisory;
optionally emit a greppable `# N warning(s)` summary line in print mode. No code change strictly
required.

---

### 4.4 N-09 — Info — `threads < -1` is accepted and re-interpreted as "auto"

* **Where:** `working_code/gifscythe/src/core/SettingsIO.h`, `working_code/gifscythe/src/core/Validate.h`, `working_code/gifscythe/tests/test_gifsicle_command.cpp`
* **Verification:** **[V-SRC]**

**Problem.** `threads` is loaded with no lower bound; a conf value such as `-7` passes parsing,
then hits the `else add("-j")` branch and silently becomes 8 threads instead of being
refused/warned. The same silent-degradation class the audit otherwise worked to remove.

**Evidence (re-verified).** `src/core/SettingsIO.h:202-204` — `need_long(&tmp)` then
`static_cast<int>(tmp)`; `src/core/Validate.h` has no `threads` rule at all;
`tests/test_gifsicle_command.cpp:362-368` asserts only the `>0` cases.

**Risk.** Same as N-06, reachable from a hand-written conf.

**Proposed solution (reviewer).** In `Validate.h` (and `web/validate.mjs`), warn when
`threads < -1`; keep `-1` = unchanged, `0` = auto, `>=1` = N; add a unit assertion for `-7`.

---

### 4.5 N-10 — Info — Disposal `4..7` and the `-1` sentinel are unreachable from the desktop UI

* **Where:** `working_code/gifscythe/src/qtui/SettingsPanel.cpp`, `web/validate.mjs`
* **Verification:** **[V-SRC]**

**Problem.** The desktop combo offers `-1, 0, 1, 2, 3` while the engine and validation accept
`0..7`; the code comments acknowledge the cap. The web validator allows `0..7`. Feature-parity
gap, no data risk.

**Evidence (re-verified).** `src/qtui/SettingsPanel.cpp:329-339` — five items only;
`web/validate.mjs:22-25` — `disposal` allowed `-1` or `0..7`.

**Proposed solution (reviewer).** Add items 4..7 (the restore-to-background variants) to match
`web/validate.mjs` and the engine's full range, or document the cap in the UI tooltip.

---

### 4.6 N-11 — Medium — The compiled audit's inline `§3`/`§4` statuses lag its own `§5` register

* **Where:** `COMPILED_AUDIT.md`
* **Verification:** **[V-SRC]**

**Problem.** The auto-emitted `§5` register correctly marks U-03, U-04, U-05, U-13, U-23, U-32,
U-40, U-48, U-49 and others as fixed with executed proof, while the narrative `§3`/`§4` sections
still carry `⬜ OPEN` markers for the same items. A reviewer who follows the file's own "start
here" advice will chase closed findings — and may "un-fix" verified behaviour.

**Evidence (re-verified).** `COMPILED_AUDIT.md` §3 A-03/GS-003: "Status: ⬜ **OPEN** — confirmed by
execution (C:U-04)" while its §5 row `U-04` reads "✅ FIXED (S8) — `--run` commentary moved to
stderr". Same shape for §3 A-10/GS-010 ("⬜ OPEN") vs §5 `U-23` ("✅ FIXED (S8) — strict arg
parser, rc=2").

**Risk.** Duplicate work; accidental re-opening of verified fixes; the master audit loses its
authority as the single source of truth.

**Proposed solution (reviewer).** Make `§3`/`§4` rows cite their `§5` row (or regenerate them
from the register), and add a documentation-gate check that fails when a narrative row claims
OPEN while its register row claims FIXED.

**Intake note.** This is the same "stale status view" family as **GS-208**; the two should be
fixed in one pass, not two.

---

### 4.7 N-12 — Low — The line-based settings format silently loses leading/trailing whitespace

* **Where:** `working_code/gifscythe/src/core/SettingsIO.h`
* **Verification:** **[V-SRC]**

**Problem.** The loader splits on the first `=` (values containing `=` are fine) but trims the
value, and the encoder does not escape whitespace, so `comment = " padded "` loses its padding on
a save→load round trip. The header documents this as a known limitation of the line format; no
rejection path exists, so the value is silently altered.

**Evidence (re-verified).** `src/core/SettingsIO.h:82-86` — the comment admits the loss and defers
a quoted format; `:87-92` — `encode_line_value` folds only CR/LF; `:282-283` — `trim()` on key and
value at load.

**Proposed solution (reviewer).** Either reject unrepresentable values at save time with a
warning, or introduce a minimal `"…"` quoting convention for values that start/end with
whitespace, with a format-version bump and a JS mirror update.

---

### 4.8 N-13 — Medium — Web `/optimize` trusts rc=0 + non-empty and never checks GIF magic

* **Where:** `web/server.mjs`
* **Verification:** **[V-SRC]**

**Problem.** The single-file endpoint checks only that the output exists and is non-empty (the
U-24 fix) and does not run the `isGifMagic()` check that the explode path uses. A misconfigured or
lying engine that echoes an error into the file is served as `200 image/gif`.

**Evidence (re-verified).** `web/server.mjs:229-234` — `if (!outBytes || outBytes.length === 0)`
→ 422, otherwise a `200` with `Content-Type: image/gif`; `isGifMagic()` is defined at
`web/server.mjs:290-303` and used only for the explode frame listing (`:336`).

**Proposed solution (reviewer).** Reuse `isGifMagic()` on the returned buffer before answering
200; return 422 with a clear message on mismatch; add a transport regression with a non-empty,
non-GIF engine output.

**Intake note.** Same workstream as **GS-203**: one postcondition helper (size + magic + changed
since snapshot) applied to CLI, GUI, `/optimize` and `/run`.

---

## 5. Reconciliation — what the DeepSeek review adds to the register view

DeepSeek also re-verified the existing 52-row register against source, resolving every
disagreement in favour of the shipped code. Its conclusions, with my re-check where it matters:

| Bucket | Rows | DeepSeek's verdict | Intake view |
|---|---|---|---|
| Confirmed fixed in code | U-01, U-02, U-03*, U-04, U-05, U-07, U-11, U-13, U-15, U-16, U-17, U-18, U-23, U-32, U-48, U-49, U-50, U-51, U-52 (and U-06 bind only) | present and correct | **Agrees.** Spot-checked U-01 (`OutputPlan` used by CLI + GUI), U-04 (stderr routing), U-05 (real PATH search), U-17 (explode verification), U-23 (strict parser, exit 2), U-32 (128+signum). |
| Genuinely still open | U-08 (licence set), U-09 (release re-cut), U-12 (synchronous GUI waits) | owner/build-action dependent | **Agrees**; the highest-tier review proposes the same sequencing (§6). |
| Partially addressed | U-10 (provenance/config), U-14 (CI gaps), U-06 (web concurrency cap) | remaining halves named | **Agrees**; GS-209 sharpens U-10's residual into "target detection, not just CI hash-pinning". |
| Corrected by this review | U-03* | the fix overshot into N-06 | **Confirmed** — see §4.1. |

**Where the two reviews disagree, and which view this intake records:**

1. **U-01's closure.** DeepSeek marks U-01 fixed (true for *planned* outputs: `OutputPlan` refuses
   target == source and duplicate targets). The highest-tier review adds that the planner is
   skipped when `output` is empty, so CLI Batch still has an uncovered destructive path
   (**GS-201**). These are consistent: the planner is correct, its *coverage* is not complete.
   Recorded as: U-01 stays fixed; GS-201 is a new class.
2. **U-13's closure.** DeepSeek marks U-13 fixed (the drop filter now requires `.gif` AND
   existence). The highest-tier review shows the picker and `appendInputs` still admit anything,
   and `exists()` is not `isFile()` (**GS-205**). Recorded as: the typo is fixed, the finding
   (input validation) is not.
3. **U-06.** DeepSeek's register row says fixed (loopback bind) and its "still open" list keeps the
   concurrency/rate-limit remainder. No conflict — just make sure a later triage does not read the
   "fixed" cell as covering the remainder.
4. **Audit self-consistency.** DeepSeek's N-11 (narrative vs register) and the highest-tier
   review's GS-208 (handoff says green, main is red) are the same failure mode at two different
   files. Neither reviewer's fix alone closes it.

---

## 6. Proposed solutions for work that is already open (from the highest-tier review)

These are not new findings — they are the reviewer's proposed remedies for rows that are already
`OPEN`/`PARTIAL` in `STATUS.md`. Recorded here because they are actionable proposals:

* **U-12 — Remove the five GUI-thread waits.** Model start, terminate, kill escalation and close
  as a `QProcess`/`QTimer` state machine. Add a delayed-start fixture and event-loop tick
  assertions *before* changing cancellation semantics.
* **U-09 — Re-cut release evidence from one exact SHA.** Only after critical/high issues and the
  red main are closed: tag the reviewed commit, attach artifacts from its green run, publish
  checksums, and run the clean-Windows procedure against that exact zip.
* **U-10 / U-14 — Make provenance and release gates machine-enforced.** Pin upstream tree digests,
  action versions and `aqtinstall` inputs in CI; run non-duplicated audit gates or equivalent
  targeted checks; fail on undeclared status drift.
* **A:GS-007 — Finish the legal distribution manifest.** Choose an actual first-party grant and
  ship the complete applicable GPL/LGPL texts, Qt notices, and source/relink information in every
  package. Get legal review before 1.0.0.
* **W-18 / W-19 — Complete real-machine evidence.** After an exact-SHA re-cut, run clean Windows
  without dev tools and the three physical desktop probes; record OS build, artifact digest,
  screenshots, commands and results.

---

## 7. How to fold this in later (triaged, not done here)

1. **Decide first: are `GS-201`, `GS-202`, `GS-203`, `GS-208` release-blockers?** The reviewers
   rank GS-201 Critical and the others High; the register currently contains nothing for them.
2. **Triage IDs.** A natural mapping is to append the 18 items to `COMPILED_AUDIT.md` §5 as
   `U-53…U-70` with the usual evidence cells, keeping `GS-2nn`/`N-nn` as the source column. That
   edit is what makes them real: §5 is the detail, and the register in `STATUS.md` is generated
   from it by `working_code/gifscythe/scripts/check_docs.sh --emit`.
3. **Re-emit and re-check.** After any §5 edit, run `check_docs.sh --emit` (rewrites `STATUS.md`)
   and then `check_docs.sh` until green; the pre-push hook enforces the same thing. Note that G10
   is already failing on this revision, so the gate is red before this intake is folded in.
4. **Do not treat this file as a register.** It is a dated intake snapshot; if it is folded into
   `COMPILED_AUDIT.md` §5, this file becomes evidence for those rows, not a second source of
   truth.

## 8. Verification limits (what this compilation does **not** claim)

* **No destructive reproduction was run.** GS-201's in-place overwrite, GS-202's traversal and the
  packaging reproductions are as unexecuted here as they were in the source review.
* **CI log text was not retrievable** from this sandbox: the Actions log download endpoint failed
  at the transport level (`EOF`), so the failing CI sub-gate is inferred from step-level status
  plus a local re-run, not read from the failing step's output.
* **The web findings are static-review confirmed, not exercised against a live server** in this
  intake.
* **GS-206's wrap behaviour** (`long` → `int` on LP64) is source-confirmed as *unguarded*, but the
  concrete wrapped values were not executed.
* **Cross-platform claims** (Windows case-folding collisions, macOS/musl engine builds) were not
  executed; they follow from the platform rules the reviewers cite.
* **The Gemini source is empty** — if it contained findings, they are missing from this
  compilation and must be re-supplied.
* **No code, script, workflow or status document was modified** to produce this file. Where it
  cites line numbers, they are for revision `2d51347817f5cdb39334415a03bb5f2b543119dd`.

---

*End of external review intake, 2026-09-12. Awaiting review; nothing actioned.*
