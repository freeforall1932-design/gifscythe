# Verification of the 2026-09-27 "space bunny" audit

**Question asked:** is the codebase audit true?
**Verdict:** **Yes, substantially — 20 of 22 checkable claims confirmed, some verbatim to the line.**
**One mechanism is wrong** (and the real behaviour is *worse* than reported).
**One supporting claim is refuted** (it does not affect the finding's validity).
**The audit's single biggest open question — `web/server.mjs` — is now answered, and it confirms the finding.**

---

## 0. Method — why this verification is valid

The audit was done with **no local checkout** (no network from its shell; every file read through the
GitHub raw endpoint against `main` @ `957c143`). It repeatedly flagged that as its main weakness:
no build, no execution, and line numbers only where it could count them by hand.

This session has a real checkout. Before trusting any of it, I established that **the tree the audit
read is the tree I have**:

```
git fetch origin 957c143c2cd0a792f85b952bb96f1494409b01bc
git diff --stat 957c143 8d30614
  incomplete space bunny review.txt | 260 ++++
  space bunny review 2026-09-27.md  | 1052 ++++
  2 files changed, 1312 insertions(+)
```

**The only difference between the audit's target tree and my working tree is the two review files
themselves.** Every claim is therefore checkable against identical bytes.

Where the audit could only *read* code, I could also **execute** it (Node 22 is available, so the
`web/` findings were tested empirically, not just read).

*Caveat on local git checks:* the sandbox clone is shallow (`depth=1`, one commit locally), so local
ancestry tests are meaningless. On the remote, history is intact: 24 commits, `7c035fd` (PR #4) →
`957c143` (PR #5 merge) → `8d30614` ("Add files via upload", current `main`). Zero open PRs.

---

## 1. Confirmed findings — claim by claim

### N-01 [HIGH] Explode is exempt from the U-59 partial guard — **CONFIRMED, end to end**

| Audit claim | Verified against | Result |
|---|---|---|
| GUI exempts Explode | `MainWindow.cpp:897-905` | **verbatim match** |
| CLI exempts Explode | `cli/main.cpp:602-603` | **verbatim match** |
| `ExplodeVerify.h` only verifies, never protects | grep for `rename\|partial\|redirect\|truncat` in `ExplodeVerify.h` | **zero hits** — snapshot (`:65`) + diff (`:130`) only |
| gifsicle truncates frames in place | `gifsicle.c:1141` → `:1043` → `:969` | **CONFIRMED**: `merge_and_write_frames()` → `write_stream()` → `fopen(output_name, "wb")` |
| T7 never re-runs Explode over existing frames, never cancels during Explode | `test_gui_offscreen.cpp:592-660` | **CONFIRMED** — fresh `QTemporaryDir`, happy path + multi-input refusal + lying-engine case only |
| `cancelRun()` discards the (empty) partial and reports success | `MainWindow.cpp:939-955` | **CONFIRMED** |

The full mechanism holds: `settings.mode != gs::Mode::Explode` skips `pendingPartial_` entirely, so
`cancelRun()`'s `gs::discard_partial(pendingPartial_...)` discards an empty string and prints
"Cancelled." while gifsicle has already `fopen(..., "wb")`-truncated frame *k* over the previous good
frame. The audit's scenario is real. **This is the one item I would treat as ship-blocking.**

### N-02 [HIGH] `B1-B15` gate collapses 20 cases into one bit — **CONFIRMED**

`scripts/verify_audit.sh:149-151`, verbatim:

```sh
     QT_QPA_PLATFORM=offscreen "$work/gui/test_gui_offscreen" 2>/dev/null | tail -1 | grep -q "ALL GUI TESTS PASSED"; then
    ok "B1-B15" "offscreen GUI harness green (batch/merge/explode/cancel/close/dedupe/live pane)"
  else bad "B1-B15" "GUI harness failed"; fi
```

`tail -1` discards everything but the banner. **Improvement on the audit's fix:** the harness *already*
prints the counts the audit wanted to add — `test_gui_offscreen.cpp:1731` does
`std::printf("==> %d checks, %d failures\n", g_checks, g_failures)`. The fix is a **one-line change to
`verify_audit.sh`** (grep that line instead of `tail -1`); no harness edit is needed. Harness has 20
`T`-blocks (T1–T20) and 356 `CHECK`/`CHECK_MSG` call sites, so a floor is meaningful.

### N-03 [MEDIUM] E3 defeated by the word "never" — **CONFIRMED** (`verify_audit.sh:200-204`)
### N-04 [LOW] E7 checks a Markdown literal — **CONFIRMED** (`verify_audit.sh:206`)
### N-05 [LOW] `DOC_GATE_CHECKS=2` hand-maintained — **CONFIRMED** (`verify_audit.sh:247`)
### N-06 [LOW] gate id `A1` emitted twice — **CONFIRMED** (lines `30` and `35`)

### N-07 [MEDIUM] validator coerces, builder drops — **CONFIRMED EMPIRICALLY, and reachable over HTTP**

The audit proved the module seam by reading both files. I ran it:

| input | `buildArgs` output | result |
|---|---|---|
| `disposal: "5"` (string) | `["a.gif","-o","o.gif"]` | **flag silently dropped** |
| `disposal: 5` (number) | `["--disposal","5",...]` | applied |
| `loopcount: "-2"` (string) | `["a.gif","-o","o.gif"]` | **no loop flag at all** |
| `loopcount: -2` (number) | `["--no-loopcount",...]` | applied |
| `color_count: "32"` | dropped | **silent** |
| `optimize_level: "3"` | dropped | **silent** |
| `lossy: "40"` | dropped | **silent** |
| `threads: "4"` | `["-j4",...]` | works by `>` coercion — **exactly as the audit predicted** |

`command.mjs:52` and `validate.mjs:16-22` are verbatim as quoted.

**The audit's one UNVERIFIED point is now settled.** It wrote: *"If `server.mjs` normalises types
before calling `buildArgs`, this is unreachable via HTTP. That is a 2-minute grep and it is the first
thing to check — it decides whether this is MEDIUM or LOW."*

**It does not normalise.** `server.mjs` calls `validate({ ...settings, ... })` on a throwaway spread
(lines `443`, `738`), keeps only the returned issue array (`validate()` ends `return w;`), and then
passes the **raw** `settings` object — straight out of `JSON.parse` — into `buildArgs` at lines `504`,
`861` and `871`. Coerced values are computed and thrown away.

> **MEDIUM stands.** `curl -d '{"settings":{"disposal":"5"}}'` returns HTTP 200, a real GIF, and a
> setting that was never applied.

### Tracked rows re-derived from source — all **CONFIRMED**

| Row | Evidence | Verdict |
|---|---|---|
| U-71 | `ProcessRunner.h:123` `return static_cast<int>(code) & 0xff;` | verbatim |
| U-70 | `ensureEngine()` `:343`,`:350` use `u8path_compat`; `startPreview()` `:1151` does **not** | verbatim |
| U-72 | `cancelRun()` `:943-946`: `cancelling_=true → kill → waitForFinished(3000) → cancelling_=false` | verbatim; T9's `g_dialogs.empty()` (`:832`) indeed cannot exercise it |
| U-55 | `OutputPlan.h:64-68` bytewise `tolower` under `_WIN32` | verbatim |
| U-12 | five blocking sites, exact order: `:172` `waitForFinished(2000)`, `:855` `waitForStarted(5000)`, `:928` `waitForStarted(5000)`, `:945` `waitForFinished(3000)`, `:1140` `previewProcess_->waitForFinished(1000)` | verbatim |

`MainWindow.h`'s header comment ("Never blocks the UI thread (S3-7 rule)") is present verbatim and is
indeed the mirror image of the code. The audit's five named sites are exactly right.

### Smaller claims — **CONFIRMED**

- **T4 threshold** (`:499`): `CHECK_MSG(clickMs < 3000, "Run click returns fast (no waitForFinished block)")` —
  verbatim, and `runCommand()` uses `waitForStarted(5000)`, so the assertion is tighter than the code's budget.
- **OutputPlan.h stale rationale** (`:29-33`): verbatim, and it cites `docs/audit/FIX_PICK_2026-09-10.md §1.4`
  — **the `docs/audit/ directory that citation names does not exist`** (`docs/` = archive, ci, legal, planning, release, screenshots).
- **CLI snapshot asymmetry:** `cli/main.cpp:572` snapshots the **target**, `:645` verifies the
  **partial**; `OutputVerify.h:45-46` is the "unchanged" clause. Conservative direction, as stated.
- **Phase 5 containment:** `run-paths.mjs` is 45 lines; `assertContainedPath` at 31–39, throw at 36,
  name bans at 10, NFC+casefold at 24, trailing-dot at 13, scope disclosure at 29–30. **Every line
  number the audit cited is exact.**
- **Parity suite not vacuous:** executes the real binary both directions. **No fixture sets `info`** —
  the string `info` does not appear anywhere in `command.test.mjs`.
- **Positive result:** `CMakeLists.txt:89` `add_dependencies(test_gui_offscreen fake_engine_exit0 fake_engine_partial_failure)` — the audit's own suspicion refuted by the code.
- **Incomplete review's F3:** `.github/workflows/build.yml` and `docs/ci/build.yml.proposed` share blob
  `2978885391691e54a2f19dd17de5f2e2c9976f22` — byte-identical. **CONFIRMED.**

---

## 2. Where the audit is wrong

### ❌ A. `--gamma=0` — the mechanism is wrong, and reality is worse

The audit claims: *"An empty gamma field is `\"\"`, and `\"\" >= 0` is **true** in JavaScript, so an
empty field emits `--gamma=0`."* The premise (`"" >= 0` is true) is correct. **The conclusion is not.**

`command.mjs:93` calls `fmtDouble(s.gamma)`, and `fmtDouble` (`:37`) does `parseFloat(x.toPrecision(10))`.
A string has no `.toPrecision`. Executed:

```
gamma ""          THROWS: TypeError: x.toPrecision is not a function
gamma undefined   ["a.gif","-o","o.gif"]          (no flag — fine)
gamma 0           ["--gamma=0","a.gif","-o","o.gif"]
```

So an empty gamma does **not** silently change the image — it **throws an uncaught `TypeError`** inside
the request path. That is a failed/hung request (or a crashed process, depending on the handler), not a
wrong picture. **Severity should go up, not down.** Reachable: `validate.mjs` has **no numeric gamma
rule at all** (only the `gamma_str` shape check at `:81-91`), so `{"settings":{"gamma":""}}` validates
clean and then blows up in `buildArgs`.

The audit's *scope* call is still right: `app.js` never populates `gamma` (no `gamma` in the file), so
the shipped browser UI is unaffected. Direct `POST /run` clients are the exposure.

### ❌ B. "STATUS.md cites B1-B15 as proof for U-34/U-47/U-16/U-01/U-45" — refuted

`grep "B1-B15" STATUS.md` returns **zero hits**. `B1-B15` appears nowhere in the register. Those rows
cite per-test ids instead:

| Row | Real proof citation |
|---|---|
| U-34 | `T20` |
| U-47 | `T20` |
| U-16 | `unit test 32 + T19 no-stray check` |
| U-45 | `T18` |
| U-01 | `src/core/OutputPlan.h` + CLI/GUI planning |

So the audit's "one deleted case silently invalidates five DONE rows at once" is not supported as
written.

**N-02 is nonetheless still true and still the highest-value fix.** The register's rows depend on
T18/T19/T20 *existing*; the gate only depends on a banner being printed. Delete T19 and `B1-B15`
stays green while `U-16`'s cited proof points at a test that is gone. The failure is real; only the
audit's citation of the register was wrong.

---

## 3. Handoff claims (from the earlier, quota-truncated review)

| Claim | Verdict | Evidence |
|---|---|---|
| "U-59 / P0-7 GUI implementation is on the **open PR #5** branch" | **FALSE** | PR #5 = *"Add seeded real-engine settings oracle (U-94 / P2-18)"*, **merged** 2026-09-26. Open PRs: **zero**. |
| Handoff baseline `7c035fd` is stale | **CONFIRMED, and worse now** | `main` is `8d30614` — two steps on (`957c143` PR #5 merge, then a direct "Add files via upload"). |
| Is the U-59 work actually on `main`? | **YES** — just not via PR #5 | commits `4a8e353` "Protect GUI outputs with verified partial promotion", `9de2607`, `94c95df`. The handoff names the wrong PR, not the wrong state. |

This is the audit's most practically useful correction: an S31 reader following the handoff to PR #5
would review the oracle and never look at the P0 data-loss code. **The U-59 row in `STATUS.md` is
`DONE` (S30)**, so nothing was silently lost — only mis-routed.

---

## 4. Accuracy of the audit's self-reported limits

Its coverage ledger estimated line counts from blob sizes. Actual: `server.mjs` 975 (est. ~1,050),
`check_docs.sh` 1,102 (est. ~1,500), `MainWindow.cpp` 1,336 (est. ~1,400), `cli/main.cpp` 661 (est. ~730),
`test_gui_offscreen.cpp` 1,738 (est. ~1,700). All close; none material to any finding.

Its honesty holds up where it matters: it marked `server.mjs` and `check_docs.sh` as unread rather
than guessing (Phase 5/6 "unanswered", not "clean"), and it refused to invent line numbers. **22 of 62
files DEEP, 39 PENDING** is an accurate and prominently-stated limit — this is a partial audit that
says so, not a full one pretending otherwise.

---

## 5. Bottom line for the next session

**Trust the audit, with two edits:**

1. **N-01 (Explode has no partial guard) is real and confirmed on every link of the chain** — it is the
   one item here worth treating as ship-blocking. The audit's minimal fix (honest status text) is the
   right interim; full frame-level promotion is the real fix.
2. **N-07's severity is MEDIUM, confirmed** (I closed its open question — `server.mjs` does not
   normalise), **and its gamma sub-case is worse than described**: an empty gamma throws, it does not
   emit `--gamma=0`.
3. **N-02 is the highest-value single change**, and it is a one-liner — the harness already prints the
   counts. Drop the audit's "five DONE rows" justification; use "T18/T19/T20 can be deleted while the
   gate stays green" instead, which is true.
4. **Fix the handoff's PR #5 bullet before anything else** — it currently points the next reviewer at
   the wrong work.

Not done here: no code was changed, nothing committed. The 39 PENDING files (`check_docs.sh`,
`smoke_cli.sh`, `SettingsIO.h`, `GifsicleCommand.h`, T15–T20, the remaining web suites) are still
unaudited, as is the missing `docs/audit/FIX_PICK_2026-09-10.md decision record`.
