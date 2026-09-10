# Remediation — 2026-09-10 · what was fixed and the proof it was fixed

**Branch:** `arena/01a08a10-gifscythe` · **Tree:** `a55a68d` ·
**Register:** `docs/audit/CONSOLIDATED_AUDIT_2026-09-10.md` (U-01…U-44) ·
**Pick rationale:** `docs/audit/FIX_PICK_2026-09-10.md`

**Batch 1 — 21 findings closed outright, 3 closed in part (U-10 / U-14 / U-18),
2 register rows corrected (U-19 / U-20).**
**Batch 2 — 10 more closed (U-21, U-30, U-31, U-32, U-44, U-46, U-49, U-50,
U-51, U-52), see §2b.** Every "after" below was produced by running the code in
this sandbox, not by reading it. Where a fix could only be compiled by CI (Qt),
that is stated instead of implied.

---

## 0. Register tally (counted from `COMPILED_AUDIT.md`, not estimated)

| Status | Count | IDs |
|---|---|---|
| ✅ **Fixed outright in S8** | **31** | U-01, U-02, U-03, U-04, U-05, U-06, U-08, U-11, U-13, U-21, U-22, U-23, U-24, U-25, U-26, U-28, U-29, U-30, U-31, U-32, U-33, U-38, U-39, U-43, U-44, U-46, U-48, U-49, U-50, U-51, U-52 |
| 🔶 **Partially fixed** | 3 | U-10 (wording), U-14 (targeted CI gates, not the whole script), U-18 |
| ⬜ **Still open** | 15 | U-07, U-09, U-12, U-15, U-16, U-17, U-34, U-35, U-36, U-37, U-40, U-41, U-42, U-45, U-47 |
| ✅ **Fixed earlier (by S7, verified again here)** | 1 | U-27 |
| ↩️ **Register row corrected** (was not a defect as written) | 2 | U-19, U-20 |
| **Total register** | **52** | U-01…U-52 |

31 + 3 + 15 + 1 + 2 = **52**. Of the 31 S8 fixes, **21 landed in batch 1** and
**10 in batch 2** (§2b).

---

## 0. Gate status

| Check | Before | After |
|---|---|---|
| `./build.sh` (engine + CLI + unit tests) | `ALL TESTS PASSED` (no count printed) | **`211 checks, 0 failures`** — counter added; tests 21–30 |
| `scripts/test_engine.sh` | 5 passed, 0 failed | **5 passed, 0 failed** |
| `scripts/smoke_cli.sh` | 7 passed, 0 failed | **7 passed, 0 failed** |
| `scripts/test_package.sh` | *did not exist* | **9 passed, 0 failed** (new) |
| `scripts/verify_audit.sh` | **19 passed, 1 FAILED (C6), 3 skipped — exit 1** | **23 passed, 0 failed, 5 skipped — exit 0** (E9 SKIPs: the CI workflow change needs a `workflows`-scoped token — 24/0/4 once applied) |
| `node web/test/command.test.mjs` | 13 PASS | **all PASS, 14 fixtures** (caught the U-03 divergence mid-change) |
| `node web/test/validate.test.mjs` | *did not exist* | **19 PASS** — JS `validate()` vs the real C++ `validate()` (new) |
| `node web/test/transport.test.mjs` | *did not exist* | **17 PASS** — live server: percent-encoding, latin1 headers, 422 path (new) |
| GUI offscreen harness | 243 checks (T1–T16), CI only | **+T17** (T1–T17), CI only — see §3 |

*`verify_audit.sh` went 21 → 24 because batch 2 added gates **W1**, **W2** and
**W3** (the three web suites). All three were also added to the CI linux job,
which previously ran **none** of them.*

**Mutation-tested.** Each new core guard was broken on purpose and the unit
suite re-run, to prove the tests actually fail when the fix is removed:

| Mutation | Result |
|---|---|
| revert `-j` "Auto" mapping (U-03) | 3 FAIL |
| remove target-equals-source guard (U-01) | 9 FAIL |
| remove duplicate-target guard (U-01) | 3 FAIL |
| restore lenient `parse_bool` (U-11) | 7 FAIL |
| delete resize validation (U-22) | 4 FAIL |
| allow half-specified `-p` (U-33) | 3 FAIL |
| *all restored* | **211 checks, 0 failures** |
| E3 guard: exempt `//` comment lines (U-30 follow-up) | real `system()`, real `/bin/sh` literal and a trailing `// spawns sh -c` **all still FAIL**; a pure comment line correctly PASSes |
| re-add the double `decodeURIComponent` (U-49) | **4 FAIL** — exactly the four percent-encoding cases |
| drop `encodeURIComponent` on the header (U-50) | **6 FAIL** — both non-ASCII cases *and* the four percent cases, whose values no longer round-trip |
| remove the `validate()` call (U-30) | **3 FAIL** — exactly the three out-of-range cases |
| *all restored* | **17 PASS**, `server.mjs` byte-identical to the pre-mutation copy |

---

## 1. Critical

### U-01 — batch auto-naming silently destroyed user data — **FIXED**

New Qt-independent core module **`src/core/OutputPlan.h`**: callers hand over the
(input, output) pairs they intend to write; `gs::plan_outputs()` refuses

* a target that is **any** queued input (`PlanIssueKind::TargetsSource`),
* two inputs mapping to **one** target (`DuplicateTarget`),
* an empty target (`EmptyTarget`),

and *reports* (does not refuse) targets already on disk, because re-running an
optimize legitimately replaces the previous result. Path comparison goes through
`std::filesystem::weakly_canonical` so `./a.gif`, `a.gif` and an absolute path to
the same file cannot slip past; on Windows the key is additionally case- and
separator-folded.

**Before** (verified earlier this session, `gifsicle self.gif -o self.gif`):
rc=0, source replaced in place, md5 `5ccf9df7…` → `5e2d1d87…`.

**After** (real CLI, real engine):

```
### U-01 (a) target == source
  before: 8703 B  md5=5ccf9df782db
ERROR: refusing to run — the planned output is not safe:
Refusing to overwrite a source file: "/tmp/proof/solo.gif" is input #1 of this run. …
  rc=2 -> after: 8703 B  md5=5ccf9df782db          ← source untouched

### U-01 (c) merge welded over its own input
Refusing to overwrite a source file: "/tmp/proof/d1/hero.gif" is input #1 of this run. …
  rc=2
```

Call sites, both now planning **before the first process starts**:

* `src/cli/main.cpp` — refuses with rc=2, and prints `NOTE: output already exists
  and will be replaced: …` when a re-run replaces a previous result.
* `src/qtui/MainWindow.cpp` — `planBatch()` / `plannedBatchTargets()` replace the
  per-file re-derivation at the old `:756` / `:872`; the summary label states the
  refusal **before** Run is clicked; the single-target modes (Merge/Auto) are
  planned too. Explode is exempt and the code says why: its `output` is a prefix
  and the engine appends `.000`/`.001`, so it cannot land on an input.

**Deliberately not done:** temp-sibling + rename. It would only protect a
*previous* output from a crashed engine, and it would put a staging path in the
live command pane, breaking the "the pane shows the command that runs" contract
that harness T1/T16 assert. The two vectors that actually destroy data are closed
by planning. Recorded here so the omission is a decision, not an oversight.

### U-02 — packager reported success for a release with no app — **FIXED**

`scripts/package_portable.sh` rewritten to fail closed: staging dir wiped first,
engine + CLI + GUI all **required**, `windeployqt` failure fatal, license set
asserted, and a final pass re-stats every required file for existence *and*
non-emptiness. The GUI is required unless the caller says
`--engine-cli-only`, and the headless package then says so in its own
`README.txt`.

**Before:** `Note: Qt GUI not found…` → `Portable package created` → **exit 0**,
folder with no `gifscythe` binary.
**After:** `scripts/test_package.sh` → **9 passed, 0 failed**, including

```
PASS: no GUI + default mode fails closed and says why
PASS: missing CLI is fatal
PASS: missing license set is fatal
PASS: staging dir is wiped before packaging
PASS: unknown packager argument is rejected
```

### U-08 — license set could ship incomplete — **FIXED** with U-02

`LICENSE` (or root `COPYING`) and a gifsicle GPL v2 text are now both mandatory;
the `if [[ -f ]]` guards that let a GPL-v2 engine ship with no license are gone.
Negative test 5 removes them from the repo root and asserts a non-zero exit.

---

## 2. High / Medium

### U-03 — threads "Auto" ran single-threaded — **FIXED**

`GifsicleCommand.h`: `threads > 0 → -jN`, otherwise **bare `-j`**. Single-threaded
stays expressible as `-j1` (the spinner range is 0…64). Mirrored in
`web/command.mjs` — the parity harness failed 4 fixtures the moment the C++ side
changed, which is exactly what it is for.

```
t0.conf (threads unset) -> -j  /tmp/proof/solo.gif -o /tmp/proof/t.gif
t1.conf (threads = 0)   -> -j  /tmp/proof/solo.gif -o /tmp/proof/t.gif
t2.conf (threads = 4)   -> -j4 /tmp/proof/solo.gif -o /tmp/proof/t.gif
```

`SettingsIO.h` / `command.mjs` now serialise `threads` for every value `>= 0` so
`0` ("Auto") survives a round trip instead of collapsing to the `-1` default.

### U-04 — `--run` with no output corrupted its own stdout — **FIXED**

All `#` commentary goes to **stderr** in `--run` mode; stdout belongs to the
engine alone. Print mode still puts the command line on stdout — that *is* its
documented output.

**Before:** 8887 B starting `GIF89a` and ending `# -> exit code 0`;
`gifsicle --info` → *warning: trailing garbage after GIF ignored*.
**After:** 8671 B, trailer `\0 ;`, and

```
$ gifsicle --info stdout.gif
* stdout.gif 12 images            ← no warning
```

### U-05 — documented PATH engine fallback was dead code — **FIXED**

`EngineLocator.h` gains a real `find_on_path()` (platform separator, executable
probe, absolute result); `locate_engine()` returns **""** when nothing is found
instead of a bare basename that then failed the caller's own probe with a
confusing message.

```
$ PATH=/tmp/proof/bin:$PATH ./cli p.conf --run
/tmp/proof/bin/gifsicle  <- engine resolved from PATH     rc=0

$ env -u GS_ENGINE PATH=/usr/bin:/bin ./cli p.conf --run
ERROR: engine not found
       Searched GS_ENGINE, the folders next to this executable,
       the release/ trees, and PATH.                        rc=1
```

### U-23 — unknown CLI arguments silently ignored — **FIXED**

Strict parser: unknown argument → `ERROR: unknown argument '--rnu'` + usage on
stderr, **rc=2**; `--engine` with no value → rc=2; `-h/--help/--version` added.
Previously `--rnu` gave rc=0 with an empty stderr.

### U-22 — `validate()` skipped resize geometry — **FIXED**

```
WARNING: resize=0x0: one of width and height must be > 0 (the engine refuses 0x0)
WARNING: scale=0.000000x1.000000: both scale factors must be > 0; 0 makes the
         engine skip the resize silently
```

Rules were derived by probing the bundled 1.96 engine, not guessed:
`--resize-fit 0x0` → rc=1 but `40x0` → rc=0; `--resize-width 0` → rc=1;
`--scale 0x0` → rc=1; **`--scale 0x1` → rc=0 and silently does nothing** (output
stays 60×132), which is why a zero factor is warned rather than passed through.

### U-11 — malformed booleans degraded silently — **FIXED**

`parse_bool_strict()` accepts `1/0, true/false, yes/no, on/off` (any case) and
warns on anything else, leaving the field **unchanged**:

```
WARNING: settings key 'unoptimize' value 'maybe': not a boolean (expected
         true/false, yes/no, on/off or 1/0); left unchanged
WARNING: settings key 'careful' value 'Trueish': not a boolean …
```

### U-33 — half-specified `-p` — **FIXED**

`load_settings()` now tracks which halves appeared; a lone `position_x` (or `_y`)
clears `has_position` and warns, instead of emitting `-p 12,0` and relocating
every frame to row 0.

### U-13 — drag-and-drop accepted any existing file — **FIXED** *(CI-compiled)*

`MainWindow.cpp:426` `|| QFileInfo::exists(f)` → `&&`. **This one had a
dependency the audit did not mention:** harness **T8** reached the failed-run
path by dropping a *nonexistent* `ghost.gif`, relying on the very bug being
fixed. T8 was rewritten to drop a file that exists but is not a GIF (the engine
answers `file not in GIF format`, rc=1 — verified), which is a stronger test of
failure honesty, and two new checks pin the filter itself.

### U-06 — web demo bound 0.0.0.0 with no auth — **FIXED**

Binds `127.0.0.1` by default; `GS_WEB_HOST=0.0.0.0` opts in explicitly. Verified:
`Gifscythe web server on http://127.0.0.1:8124` plus a loopback-only notice.

### U-24 — web server returned 500 for "rc=0 but no output" — **FIXED**

Output is stat-ed after a zero exit. Verified by pointing `GS_ENGINE` at a stub
that exits 0 and writes nothing:

```
HTTP/1.1 422 Unprocessable Entity
{"ok":false,"exitCode":0,"stderr":"the engine exited 0 but produced no output file", …}
```

### U-26 — web engine discovery sorted versions lexicographically — **FIXED**

Numeric per-component compare. Verified by creating `release/0.9.0/` and
`release/0.10.0/`: the server selected **`0.10.0`** (a lexicographic sort picks
`0.9.0`). The probe directories were removed afterwards.

### U-25 / U-29 — web Scale default and missing Touch — **FIXED**

`web/index.html`: Scale % default 50 → **100** (matching the desktop's
`SettingsPanel.cpp`), and the resize dropdown gains **Touch**, which
`command.mjs` already implemented.

### U-14 — green CI enforced none of the release-gate claims — **PARTIALLY FIXED**

Added to `.github/workflows/build.yml` (linux job) and mirrored byte-for-byte into
`docs/ci/build.yml.proposed`:

* `Packaging negative tests (incomplete packages must fail)` → `test_package.sh`
* `Assert package manifest` → fails the build unless `gifsicle`,
  `gifscythe-cli`, `gifscythe`, `LICENSE`, `COPYING.gifsicle` and `README.txt` are
  all present **and non-empty**

**Not done, deliberately:** running the whole `verify_audit.sh` inside CI. It
re-runs `build.sh`, `test_engine.sh`, `smoke_cli.sh` and the GUI harness — four
steps CI already has — roughly doubling the job for no new coverage. The two
gates above are the part that was actually missing. `verify_audit.sh` stays the
one-command local runner, and its headline number is now reproducible (U-38).

### U-38 — `verify_audit.sh` FAILed instead of SKIPping without cmake — **FIXED**

C6 is now guarded by `command -v cmake`, exactly like the `[B]` block 13 lines
below it always was. `19 passed, 1 failed, 3 skipped, exit 1` →
**`21 passed, 0 failed, 4 skipped, exit 0`** in this sandbox.

### U-39 — proposed-workflow copy is a drift magnet — **FIXED**

New `E9` check in `verify_audit.sh` diffs the live workflow against
`docs/ci/build.yml.proposed` and FAILs on drift.

### U-48 — empty comments emitted a bare `--comment` — **FIXED**

`add()` drops empty strings, so a `comment = ` line in a conf pushed `--comment`
with **no operand**, and the flag then swallowed the next argument as its text.
Verified: `comments = {"", "real one"}` produced

```
argv: [--comment --comment 'real one' -j a.gif -o o.gif]
```

— the real comment lost, the command mangled. Now skipped in
`GifsicleCommand.h` **and** mirrored in `web/command.mjs`, with unit test 27 and a
new JS⇄C++ parity fixture. Mutation-tested: removing the guard → 3 FAIL.

### U-10 — manifest called a patched tree "identical to upstream" — **WORDING FIXED**

`reference_code/REFERENCE_MANIFEST.md` no longer claims `gifsicle/` is identical
to upstream master; it now lists the observed deltas and marks the provenance
question open. **The underlying question is not answered** — that needs a fresh
clone of `kohler/gifsicle` at the pinned `07f5c4c3`, and this sandbox has no
network access to GitHub for clones.

---

## 2b. Batch 2 — ten more findings closed with executed proof

### U-32 — a signalled child reported exit code 1 — **FIXED**

`run_argv()` returned `1` for a child killed by a signal, so "cancelled" was
indistinguishable from "the program failed". It now returns the shell
convention `128 + WTERMSIG(status)`.

**Proof** — unit test 28, `211 checks, 0 failures`:

| Child | Before | After |
|---|---|---|
| `kill -TERM $$` | `1` | **`143`** (128+15) |
| `kill -KILL $$` | `1` | **`137`** (128+9) |
| `exit 3` | `3` | `3` (unchanged — not in the 128+ range) |
| missing binary | `127` | `127` (unchanged, still distinct) |

The test silences fd 2 around these four calls: `run_argv` correctly reports a
signalled child on stderr, and printing three `ERROR:` lines during a **green**
run is misleading.

### U-21 — output-name sanitisation was POSIX-only — **FIXED**

New **`src/core/OutputName.h`**: `sanitize_output_name()`,
`is_windows_reserved_device_name()` and a `NameRules` enum
(`Host` / `Windows` / `Posix`). `MainWindow::renderedOutputName()` now delegates
to it instead of keeping its own copy.

Making the rule set a **parameter** is what makes this verifiable here: the
Windows rules run under Linux g++ and are unit-tested, so the Windows behaviour
is no longer an untested claim even though no Windows machine is available.

**Proof** — unit test 29, 30 assertions:

| Input | Posix | Windows |
|---|---|---|
| `../../evil` | `evil` | `evil` |
| `a<b>c.gif` | `a<b>c.gif` (unchanged) | **`abc.gif`** |
| `trail...   ` | `trail` | `trail` |
| `CON` | `CON` | **`_CON`** |
| `LPT9.gif` | `LPT9.gif` | **`_LPT9.gif`** |
| `console.gif` | unchanged | unchanged (prefix match is not enough) |
| `<>:"\|?*` | — | `""` |
| non-ASCII (e.g. `作品`) | passthrough | passthrough |

Separators are stripped in **both** flavours on **every** platform — that
preserves the GUI's existing POSIX behaviour. Only the character filter, the
trailing-dot trim and the reserved-name defusing are Windows-conditional.

### U-51 — a settings value could inject a new key — **FIXED**

**Reproduced before fixing.** A comment containing a newline plus `mode = merge`
was written verbatim; the loader split on newlines and read the second line as a
real key. Reloading that file gave `mode = MERGE (HIJACKED)`.

New `encode_line_value()` in `SettingsIO.h`, applied at **all 9** string write
sites, and mirrored in `web/command.mjs` so both serializers agree.

**After:** the file holds `comment = hi mode = merge` on **one** line; reload
gives `mode = auto`, and `info` / `optimize` are not hijacked either. Unit
test 30 asserts one line per key and that plain values pass through untouched.

*Known remaining limit, documented at `encode_line_value`:* leading/trailing
whitespace inside a value is still lost. Fixing that needs a quoted format,
which would break every existing `.conf` — not a silent trade to make.

Two self-inflicted bugs were caught while mirroring this into JS and are worth
recording: a `const` helper inserted mid-function sits **below** its first uses
(temporal-dead-zone `ReferenceError`, invisible to `node --check`), and the JS
loop variable is `input`, not `i`, so a regex mirror silently missed one of the
nine sites. Both were found by actually invoking the function.

### U-49 / U-50 — the web transport corrupted valid settings — **FIXED**

Both reproduced against a live server, both re-verified after:

| Request | Before | After |
|---|---|---|
| `{"comments":["100%"]}` | **HTTP 400** `bad settings JSON` (double `decodeURIComponent`) | **HTTP 200**, 8679 B, `GIF89a` |
| `{"comments":["作品"]}` | **HTTP 500** `Invalid character in header content ["X-Gifscythe-Command"]` — a *successful* engine run thrown away | **HTTP 200**, 8681 B; header is pure ASCII and decodes back to `… --comment '作品' -O2 -j …` |

### U-46 / U-52 — stale response and leaked object URL — **FIXED**

`app.js`: a `requestGen` counter is checked after the fetch, after the blob read,
in `catch` and in `finally`, so a slow earlier request can no longer populate
the preview of a newer one. `beforeUrl` is now tracked and revoked on
replacement alongside `afterUrl`. Browser-only, so the proof is `node --check`
plus code review, not a rendered assertion.

### U-31 — `build.sh` never linked `-lstdc++fs` — **FIXED**

`build.sh` now writes `build/.fs_probe.cpp`, tries to link it with and without
`-lstdc++fs`, and sets `$STDCXXFS` accordingly. On the g++ 12 here the flag
stays empty (as it should) and no probe artefact survives the run — both
verified. The fix itself can only be *exercised* on g++ ≤ 8, which is not
available here; that part is unproven and stated as such.

### U-30 — the web server had no validation layer — **FIXED**

New **`web/validate.mjs`** mirrors `src/core/Validate.h`; `server.mjs` answers
**422** with an `issues[]` list. `app.js` already renders `err.error`, so the
message reaches the user unchanged.

The mirror is not taken on trust — new **`web/test/validate.test.mjs`** (19
fixtures) serializes each settings object, runs the **real** `gifscythe-cli`,
and requires the `(field, value, reason)` triples to match the JS validator
exactly: same set, same order, same wording. Writing it found four genuine
disagreements:

1. `execFileSync` returns only **stdout**, so on a rc=0 run the piped stderr was
   discarded and the C++ side looked silent. Switched to `spawnSync`.
2. `scale` was rendered `0x1` in JS but `0.000000x1.000000` in C++
   (`std::to_string` is `%f`). JS now uses `toFixed(6)`.
3. **`delay < 0` and `lossy < 0` never survive a conf round-trip** — *both*
   writers drop them (`SettingsIO.h:321,334`, `command.mjs:203,214`) because
   `-1` means "unset". The C++ **reader** does accept them (`need_long` has no
   lower bound), so those two fixtures drive the C++ side from raw conf text.
   This is a real property of the format, now written down, not papered over.
4. One fixture's raw conf was missing its `input` line — my error, fixed.

**Strongest before/after.** `{"resize_kind":"scale","scale_x":0,"scale_y":1}`:

* **Before:** HTTP 200 with a GIF that was never resized. Proven: `gifsicle
  --scale 0x1` exits **0** and its output is **byte-identical to `--scale 1x1`**
  (`cmp` reports no difference), geometry still 60x132 — while `--scale 0.5x0.5`
  gives 30x66. The zero factor is silently treated as 1.
* **After:** `HTTP 422` — `scale=0.000000x1.000000: both scale factors must be
  > 0; 0 makes the engine skip the resize silently`.

Also verified live: `optimize_level:9` → 422, `color_count:900` → 422,
`crop` with height 0 → 422, and sane settings → 200 / 8671 B.

### U-44 — dated review snapshots at repo root — **FIXED**

`git mv` to `docs/archive/` (history preserved). The three prose references in
`IMPROVEMENT_LOG.md`, `docs/planning/OFFLINE_BUILD_REVIEW.md` and
`docs/audit/POST_S7_AUDIT.md` were updated, and the README layout block lists
the new directory. Verified there were no path links to break — only prose.

### Transport regression net — register P2-8 / P2-9 / P2-10 — **NEW**

U-49 and U-50 were fixed, but the *proof* was a handful of one-off curl calls.
New **`web/test/transport.test.mjs`** turns that into a gate: it starts the real
server on an ephemeral port and pushes 17 cases through the actual
`POST /optimize?settings=` path — literal `%`, `%20`, `%22`, `%2540`, a plus
sign, CJK, emoji, an embedded newline, a comment that looks like a flag, three
out-of-range settings, two malformed JSON bodies and an empty upload.

Each success asserts three things: the status code, that the command the server
actually ran still contains the exact value sent, and — because GIF comment
extensions store their text verbatim — that the value is present in the
**returned GIF bytes**, so "the engine received it" is checked rather than
assumed.

Writing it corrected two of my own claims:

* `shellQuote` leaves a value with no shell metacharacters unquoted, so
  `--comment --help` is correct, not a bug — gifsicle consumes the operand
  positionally (`gifsicle --info` reports `comment --help`). Verified, and the
  C++ builder emits identical argv.
* My first harness **crashed** on the U-50 mutation with `URIError: URI
  malformed` instead of failing the one case, because `decodeURIComponent` was
  called on a mangled header. It reported "1 FAIL", which I nearly recorded as a
  clean detection. The helper now falls back to the raw header and each case is
  individually wrapped, so a bad case cannot mask the rest. Re-run under that
  mutation: **6 clean FAILs**, suite runs to completion.

Mutation-tested in all three directions (see §0). This closes P2-8, P2-9 and
P2-10, which asked for exactly this round-trip coverage.

### The CI change could not be pushed — recorded, not hidden

Both workflow files were edited together and verified byte-identical. The push
was then rejected:

```
! [remote rejected] arena/01a08a10-gifscythe -> arena/01a08a10-gifscythe
  (refusing to allow a GitHub App to create or update workflow
   `.github/workflows/build.yml` without `workflows` permission)
```

So the live `.github/workflows/build.yml` was reverted to base and the change
now lives in `docs/ci/build.yml.proposed` plus a new
**`docs/ci/PENDING_WORKFLOW_CHANGE.md`** that states what is waiting, why, and
the exact `cp` + `git rm` + push to apply it. This is precisely the situation
`docs/ci/README.md` says the proposed copy exists for (*"so the recipe survives
even when workflow pushes are blocked"*).

**E9 was extended rather than deleted.** Undeclared drift still fails; drift
*declared* by that marker file is a SKIP. Mutation-tested in all four states:

| State | E9 |
|---|---|
| files identical, no marker | **PASS** |
| files differ, marker present | **SKIP** (declared) |
| files differ, **no** marker | **FAIL** (undeclared drift) |
| files identical, marker present | PASS (marker is simply redundant) |

Consequence for the headline number: `verify_audit.sh` is now
**23 passed, 0 failed, 5 skipped, exit 0**, and becomes **24/0/4** the moment a
maintainer applies the pending change and deletes the marker.

### Guard regression caught by re-running the gate

Batch 2's own comment in `ProcessRunner.h` — "(no /bin/sh, no cmd.exe)." —
tripped `verify_audit.sh` **E3**, turning a green run into
**22 passed, 1 failed**. E3 is line-based, and the exemption words
(`NEVER`, `no shell`) were on the *previous* line. Fixed both ways: the comment
now carries its exemption on the same line, and E3 exempts `//` / `*` comment
lines, since nothing can execute inside one. Mutation-tested in all four
directions (see §0) so the guard is not weakened into uselessness. Re-run:
**23 passed, 0 failed, 4 skipped, exit 0.**

---

## 3. Fixed but CI-compiled only (Qt)

**UPDATE — CI is now green on this branch.** PR #11, run `34471563229`:
**linux pass (1m14s)** and **windows pass (2m56s)**. The linux step runs
`QT_QPA_PLATFORM=offscreen ./build-cmake/test_gui_offscreen` under
`set -euo pipefail`, and the harness `return 1`s on any failure, so a green job
**is** a green harness — including the T8 rewrite and the new T17, which had
never been compiled before this run. Windows passing additionally covers the
`NameRules::Host` compile path and the `CreateProcessA` process layer.

What this run did **not** cover: the three web suites, because the workflow
change adding them is still pending (`docs/ci/PENDING_WORKFLOW_CHANGE.md`). All
three were run locally instead. The exact harness check count was not
retrievable from this sandbox — the Actions log host
(`results-receiver.actions.githubusercontent.com`) is unreachable — so **243
remains the last measured number** and should not be quoted as current.

This sandbox has **no cmake and no Qt6** (`apt-get update` →
`Acquire (13: Permission denied)`, uid 1001), so `src/qtui/` and the offscreen
harness could not be compiled here. At the time of writing these changes were
written and reviewed but **not locally compiled** — CI was the first compiler
they met, and per the update above it accepted them on both platforms. The
table is kept as the record of what was at stake:

| Change | File |
|---|---|
| U-01 batch + single-target planning, `batchTargets_` member | `MainWindow.h/.cpp` |
| U-01 summary label states the refusal before Run | `MainWindow.cpp` |
| U-13 drop filter `\|\|` → `&&` + T8 rewrite + 2 new filter checks | `MainWindow.cpp`, harness |
| U-28 missing `return` after the "could not start engine" dialog | `MainWindow.cpp` |
| U-43 `Batch (1 files) → X … X` → `Batch (1 file) → X` | `MainWindow.cpp` |
| **T17** — duplicate target refused, self-target refused, distinct targets run, shared batch folder re-refuses; sources byte-size-checked before/after | harness |

Risk controls applied: brace/paren balance checked on every edited file; all new
Qt API uses cross-checked against existing call patterns in the same files
(`QStringList::value`, `.arg(qsizetype)`, `QDir::mkpath`, `QFileInfo::size`);
`plannedBatchTargets()` uses a range-based loop to avoid an `int` vs `qsizetype`
comparison; `OutputPlan.h` added to `gifscythe.pro` HEADERS for the qmake path.

---

## 3b. Documentation consistency sweep

Current-state docs had drifted from the gates during S8, so every one was
re-read and corrected rather than left to disagree with the register:

| Doc | What was stale | Now |
|---|---|---|
| `README.md` (root) | "243 checks (T1–T16)" and "21 PASS" presented as current; S8 described as batch 1 only | 31 findings, current gate numbers, harness count labelled as the **S7 measurement** |
| `SESSION_HANDOFF.md` | header said *"all run locally, linux + Qt 6.4"* — false for this sandbox; "green locally (Qt 6.4)"; "close U-01/U-02/U-03" as still to do; U-38 listed as an open portability problem | header split into ✅ run-here vs ⏳ cannot-run-here; S7 claims labelled as S7's sandbox; a full **"What S8 did"** section added |
| `WORKLIST.md` | still-open list included U-21/U-30/U-31/U-32/U-44 | new S8 board + the 15 rows that are genuinely open |
| `PROJECT_VISION.md` | "harness at 243 checks" with no measurement caveat | S8 summary + caveat that 243 is the last *measured* figure |
| `docs/release/RELEASE_PROCEDURE.md` | "243 checks, 21 PASS / 0 FAIL / 2 SKIP" | 24/0/4 + "re-run on a Qt machine before trusting the number" |
| `working_code/gifscythe/README.md`, `docs/ci/README.md` | same two stale numbers | corrected |
| `web/README.md`, `docs/web/WEB_FEASIBILITY.md` | listed one web test | all three, with what each proves |

**A number this repo should stop confusing:** the harness holds **226 `CHECK(`
sites in source** but its last **runtime** count was **243**. Those are not in
conflict — `CHECK` increments a counter at runtime and several sites sit inside
loops. The unit suite shows the same effect: 196 source sites, **211** runtime
checks. Quote the runtime number and say where it was measured.

## 4. Register corrections

### U-19 — **not a data bug; downgraded to a wording nit**

The row claims a real `save_settings → load_settings` round trip returns
`crop_w`/`crop_h`/`position` as **0**. Run against the current tree with the
parent toggles **on**:

```
reload: crop_w 200 → 200   crop_h 150 → 150
        position 12,7 → 12,7  (has_position 1 → 1)
        scale 0.5,0.5 → 0.5,0.5    load warnings: 0
```

Nothing is lost, and unit test 26 now pins it. The original repro started from
`crop=false, has_position=false`, where *not* serialising the children is correct
— those fields are inert. What survives is only the wording: `SESSION_HANDOFF.md`
calls `readFrom()` the "exact inverse" of `writeInto()` without stating the
toggle carve-out. Both places are now corrected.

### U-20 — **doc overstatement, corrected**

Reproduced: a real GUI-saved conf (no `input` key) → `WARNING: input=: at least
one input file is required`, rc=0. The load-warning half of the original claim
holds (GUI state keys are tolerated silently — unit test 20). The "no warnings at
all" phrasing is removed from `SESSION_HANDOFF.md` and `IMPROVEMENT_LOG.md`.

### U-27 — already fixed before this session; left as-is.

---

## 5. Still open

**15 register rows remain ⬜ OPEN** (matching the §0 tally exactly), plus U-10
which is closed only in wording.

| ID | Why it is not done here |
|---|---|
| **U-07** Windows ANSI process APIs | needs a real Windows run to verify |
| **U-09** re-cut release artifacts from the tagged SHA | needs a tag + `gh release`; not a code change |
| **U-10** provenance of `FRAME_SELECTION_MODE_MASK` | needs a fresh `kohler/gifsicle` clone at `07f5c4c3` |
| **U-12** five UI-thread `waitFor*` calls | needs Qt to verify any change is safe |
| **U-15 / U-16** non-atomic settings write | Qt-only path |
| **U-17** Explode never verifies a frame was written | Qt-only path |
| **U-34 / U-35 / U-36 / U-37** preview temp leak, `setBusy` re-check, third parser, silent persistence | Qt-only paths |
| **U-40** CLI warns and runs on out-of-range values | intentional CLI/GUI split; now documented at the point of use |
| **U-41 / U-42** web POC scope | documented-as-POC gaps, not defects |
| **U-45 / U-47** | Qt-only paths |

**Closed by batch 2 but with a residual caveat, so they are listed here too:**

| ID | Fixed | Residual caveat |
|---|---|---|
| **U-21** | ✅ rule set implemented and unit-tested | the Windows *rules* are proven on Linux via the `NameRules` parameter; `NameRules::Host` resolving to `Windows` **on a real Windows build** is still CI/unverified |
| **U-30** | ✅ `web/validate.mjs` + 422 + 19 parity fixtures | the parity test drives the C++ side through `gifscythe-cli` print mode, not through the Qt widgets |
| **U-31** | ✅ link probe in `build.sh` | only g++ 12 is available here, so the flag was probed **and correctly left empty**; the g++ ≤ 8 branch has not been exercised |
| **U-32** | ✅ `128+WTERMSIG` | POSIX only; the Windows `run_argv` path is unchanged and ties to U-07 |

**Not attempted, deliberately:** U-51's leading/trailing-whitespace loss
(needs a breaking quoted `.conf` format), and the U-14 trade-off already
recorded above (CI runs two targeted gates, not the whole `verify_audit.sh`).
