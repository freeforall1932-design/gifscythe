# Planning — direction reviews, parked tracks, handoffs (consolidated S24)

**Consolidated 2026-09-17 (S24) on owner instruction** from five planning docs:
`OFFLINE_BUILD_REVIEW.md` (§1 here), `CSHARP_SHELL_PLAN.md` (§2),
`SKILLOPT_INTEGRATION_QUERY.md` (§3), `SEQUENTIAL_WORK_HANDOFF.md` (§4) and
`NEXT_SESSION_PROMPT.md` (§5). Decision-relevant content is kept; prose is
condensed; the originals' full text is in git history at `3c67e14`
(`git show 3c67e14:docs/planning/<file>`). The live register is `STATUS.md`;
owner questions live in `docs/planning/OWNER_DECISIONS.md` (kept separate — it
is the active decision register).

---

## 1. Offline-only build review (2026-09-09, S5/S6) — direction of record

**Verdicts.** Offline-only is feasible and already true (local engine
subprocess, nothing in the cloud, one portable folder). **Language: stay
C++17 + Qt6 Widgets through 1.0.0** — the only stack already built, CI-verified
end-to-end (linux + windows + offscreen harness), with native animated-GIF
preview (QMovie) and the GPLv2 subprocess boundary implemented. Weighted
comparison (reuse ×3, portable ×3, GPL boundary ×3, offline ×3, preview ×2,
velocity ×2, size ×2, native feel ×2, multi-format ×2): C++/Qt ~258, Rust+Tauri
~205, Go+Wails ~199, Electron ~174, Python/PySide6 ~161, Flutter ~153. Tauri's
size win erases itself against the portable promise once the WebView2
fixed-version runtime (~120 MB+) is bundled; Electron is portable but ~150 MB+.
**S18 note:** the first-party UI licence is Ms-PL (`OD-09 = b`; rationale:
`docs/legal/README.md`); the subprocess-separation design is unchanged.
**S14 amendment:** the `web/` build is a supported, self-hosted product surface
(not "demo only" as first scoped here) — `web/WEB_PLAN_TEMPLATE.md` §1.

**Why the subprocess decides a lot:** gifsicle is GPL v2-only; keeping it a
separate process keeps the licence boundary clean in any language. A rewrite
buys less than usual here — the hard part (argv spawning + honest exit codes)
is trivial everywhere; the cost of a rewrite is discarding verified behavior.

**Revisit the language only on a documented trigger** (this is register row
`D-08`, trigger-based): (a) bundle must drop under ~15 MB → prototype Rust +
Tauri; (b) UI must be web-tech → Tauri or Electron; (c) must run in a browser →
the wasm track (`web/wasm/`, additive, licence-blocked by `OD-16`). The 2026-09-16
external repo review argued trigger (b) already fired because `web/` exists; that
is an owner call, not a session call — the standing decisions (S19: exe stays
C++17/Qt6 through 1.0.0; `OD-C7 = park`; D-08 trigger-based) are unchanged until
the owner re-decides. Do not fold a migration into 1.0.0.

**Roadmap.** Phase 0 (done): engine + control layer + CLI + Qt GUI + packaging +
CI. Phase 1 → 1.0.0: clean-Windows smoke (C4/D3/D4 — `docs/ci/README.md`),
desktop probes B5/B6/B14, settings persistence (landed S7), owner version
decision; no WebP/APNG before this ships. Phase 2 → multi-format: common RGBA
frame model, APNG/WebP as separate subprocess helpers. Phase 3 → migration only
if a trigger fires, spiked separately. Known gaps noted at the time: no
icon/logo (`assets/` empty, cosmetic); the dated archive snapshots still mention
the removed `scripts/build_gifsicle.sh` shim (historical, left as-is by policy).

## 2. C# shell plan — RESUMED (S38, `OD-C7 = resume`; parked at S19)

**State: RESUMED 2026-10-08 (S38) by the owner** — *"im planning to pursue
windows first and that wpf .net"* — reversing S19's park (`OD-C7 = park`). The
resume point was this section, and **Phase 2 has started the way the plan
demanded: the control layer first, the C++ CLI as the oracle, no window yet.**
`csharp/Gifscythe.Core/` holds the settings, the command builder, the conf writer
and the one exit-code contract (`U-91`/`P3-17`); `csharp/Gifscythe.Core.Tests/`
runs a unit lane plus a **parity lane** that hands the same settings to the real
`gifscythe-cli` in print mode and compares the argv tokens it would exec (token
by token, separator flavour the one tolerated difference) as a step of the
existing `csharp-spike` CI job. Still to port before Phase 3: `Validate.h`, `OutputPlan.h`, `OutputName.h`,
`ProcessRunner.h`, `ExplodeVerify.h`, the conf reader — and then the WPF shell
itself. **There is no .NET SDK in the agent sandboxes, so CI is the only
compiler**; nothing in this lane may be called working before its run id is
named — the first verdict is in: CI run `37813051817` (2026-10-08, head
`cc94ed1`) is green on all six jobs, the parity step included. `csharp/README.md`
carries the tree notice.

**UI requirement (owner, 2026-10-08):** the shell must have the XNConvert
four-tab shape — Input (explorer-like file management: drag and drop, filter,
sort), Actions (the formatting controls, adapted to this repo's scope), Output
(per-format settings once APNG/WebP land, destination folder) and a **Status tab**
(per-file processing log with the size change, plus fail/success totals). Today's
Qt surface has Input/Actions/Output/Guide plus an activity log and an output
summary; the Status tab exists in neither surface yet, so it is new work, not a
port. **Video/FFmpeg (`OD-20 = a`, answered by the owner 2026-10-08):** an
FFmpeg sidecar for video → GIF is **approved as a conversion endpoint** and
`PROJECT_VISION.md` now carries the adopted amendment — decode in via one
subprocess sidecar, gifsicle still does all the GIF work, no editing/timeline/
capture, its own licence note, and not on the 1.0.0 critical path. **What still
gates the code:** `U-90`/`P3-19` — the deferred rows that own this scope by name
must exist (and name their preconditions) before a line of it is written.

**Goal (when resumed):** a portable, click-and-run Windows `.exe` in C#/WPF
with a custom Gifscythe UI/UX, driving the *unchanged* gifsicle subprocess with
the *same* argv/validation/honesty semantics as the Qt GUI and web build.
Non-goals: no engine change, no recorder, no cloud/telemetry/auto-update, no
`web/` rewrite (it becomes the parity oracle's sibling).

**Architecture:** WPF UI → `Gifscythe.Core` class library (port of `src/core/`
+ `web/command.mjs`: Settings, command builder, Validate, OutputPlan/OutputName,
ProcessRunner, ExplodeVerify) → `gifsicle.exe` via argv array + `Process.Start`
(never a shell). Parity chain: `gifscythe-cli` (C++ print mode) stays the
permanent oracle; C# tests mirror `web/test/command.test.mjs` /
`validate.test.mjs` (run the real C++ CLI and compare). Packaging:
`dotnet publish -r win-x64 --self-contained` single-file exe + `gifsicle.exe`
sidecar (keeps the portable promise; no runtime install).

**Phases.** 0 decisions (done, record below) → 1 spike (**GREEN** 2026-09-14,
run `34804350470`: all 9 steps + single-file publish + published-run success;
é+space native pass, CJK fails honestly per the engine-ACP residual; exit codes
0/2/3/4/5 exact) → 2 `Gifscythe.Core` port, test-first, three-way parity report
(C#/C++/JS), no UI → 3 WPF shell MVP at Qt-GUI parity (~30 controls, tabs,
queue, live pane, preview, templates, persistence), exit gate = every honest
refusal the Qt GUI makes, the shell makes too → 4 packaging + CI + clean-Windows
smoke → 5 cutover: owner version decision, Qt GUI archived (`OD-C5`), final
three-client parity run.

**Scope refinement (`OD-C3 = a`, 2026-09-14), ship order:** (1) GIF MVP;
(2) APNG + WebP (promoted to planned, after the GIF shell ships); (3) still-image
collections → animated (ezgif-maker-class, owner request 2026-09-14: global speed
+ per-frame delay in 1/100 s + reorder); (4) video ↔ animated-picture conversion
strictly as endpoints (no editing/timeline/capture); (5) ezgif-class frame ops.
**Mission-amendment precondition — MET 2026-10-08 (`OD-20 = a`).** Items 3–4
narrow the vision's "Not photos, not video" to *conversion endpoints only*, and
the owner adopted exactly that amendment on 2026-10-08: `PROJECT_VISION.md` now
says photos and video are **inputs to a conversion endpoint, never the subject**
(one FFmpeg sidecar, decode-only, argv subprocess, its own licence note, not on
the 1.0.0 path). The "words change first" rule is satisfied. Engine precondition
still stands: gifsicle reads GIF inputs only, so stills and video need a decode
step — the FFmpeg sidecar, with **its licence identified at build time**
(`docs/legal/README.md` §5). **The scope is registered as `U-90`, still OPEN for
a different reason:** the vision blocker is gone, but the deferred rows that own
this work by name (`P3-19`) do not exist yet, and a session must not read the
amendment as pre-approved work.

**Phase-0 decision record (S18):** OD-C1 = a (fork reference-only — superseded
same day by OD-C6) · OD-C2 = c (phased: sidecar through Phase 2, commit at
Phase 3 — cheap proof first, commit exactly when building twice would hurt) ·
OD-C3 = a (no recorder + scope above) · OD-C4 = a (WPF) · OD-C5 = a (archive Qt
at cutover; C++ CLI stays the oracle) · OD-C6 = b (UI relicensed Ms-PL; fork
files copyable with notices per `docs/legal/README.md`) · OD-C7 = park (S19).

**Risks (with mitigations):** parity drift → Phase-2 oracle tests; re-introducing
fixed audit bugs → Phase-3 replays harness categories with audit ids per control;
scope creep → non-goals + OD-C3 (any addition re-opens Phase 0); .NET runtime vs
portable → self-contained publish (spike-proven). **Port-time traps recorded by
the 2026-09-16 external review (`U-91`):** the spike's `Stream.Read` may
under-fill the 6-byte magic probe (use `ReadExactly`), its `Quote()` is
POSIX-style on a Windows product (port the MSVCRT quoting contract), and its
exit-code comments collide with the CLI's (3 = engine-missing vs 3 = strict
refusal) — fix all three against one shared exit-code contract when Phase 2
resumes. **Untouched by any of this:** `reference_code/`, `src/cli/` (the
oracle), `web/` (second parity client), the docs gate.

## 3. SkillOpt integration query — awaiting `OD-15`

**The ask:** incorporate microsoft/SkillOpt INTO this repo (available inside the
checkout) so a future session can *train a natural-language skill* against this
repo's own ground truth. Nothing is built until the owner answers `OD-15`
(shape A pinned submodule [recommended] / B vendored pinned copy / C venv-pip
wrapper / D skip). **Do not vendor, submodule or pip-install before that answer.**

**Verified facts:** upstream `microsoft/SkillOpt`, MIT (© 2026 Microsoft),
Python (`pyproject.toml` + `requirements.txt`), 531 commits, page
`microsoft.github.io/SkillOpt`, paper `arXiv:2605.23904`; a text-space optimizer
that trains reusable skills for frozen agents via trajectory-driven edits gated
by a scored validation loop, deployed as `best_skill.md`; entry points
`microsoft/SkillOpt/scripts/train.py` / `eval_only.py`; backends
`claude_code_exec`, `codex_exec`, OpenAI-compatible/vLLM; needs API credentials
+ network. (Upstream paths are cited with the `microsoft/SkillOpt/` prefix on
purpose — gate G8 fails a backticked path whose first segment is a repo
directory that does not exist here.)

**Three non-negotiable conditions:** (1) quarantined from the product and every
shipped package (never inside `working_code/` build/packaging output — else
GS-204/U-08 are violated by shipping a Python trainer + MIT notice inside a
Qt/C++ bundle); (2) optional and inert — no build step, CI job or gate may
depend on it; (3) MIT notice preserved in whatever subtree holds it. Plus: no
credentials in the repo; a subtree README naming provenance + the OD-15
decision; never cite an upstream path as if it were ours.

**First experiment candidate (recorded, not started):** the *doc-sweep skill* —
detect the same "true-when-written, false-when-read" staleness that
`working_code/gifscythe/scripts/sweep_stale.sh` detects mechanically. Ideal
first target: scorable ground truth (the sweep's mutation tests) and checkable
output (compare the skill against the sweep on the same corpus). Register it as
its own `WORKLIST.md` item, not as part of the SkillOpt task. **Open questions:**
Q1 repo-wide vs throwaway branch · Q2 backend + who provides credentials ·
Q3 is doc-sweep the right first target · Q4 submodule vs vendored vs wrapper.

## 4. Sequential work handoff (S17) — remaining Qt/Windows/build-tool work

Owner authorization (2026-09-13) was: highest→high-confidence work by one
agent, medium/low-confidence portions handed off, PR but no merge/release/
version/workflow/owner-decision changes. `STATUS.md` is authoritative; this
section supplies execution detail for the three PARTIAL handoffs.

| Finding | Already implemented/proved | Remaining work / required environment |
|---|---|---|
| **GS-210 / P2-13 — PARTIAL** | Both build scripts parse every argument before side effects; unknown rc=2, help rc=0; 12 isolated cases pass (originals fail all 12); native build passes | Qt6 + CMake/qmake: choose and test CMake-first/only dispatch, remove the `.pro` version drift using the product source of truth, exercise missing/broken tools and clean output trees. Do not assume the qmake-first path was changed |
| **GS-204 / P1-26 — PARTIAL** | Both packagers share `package_common.sh`, private staging, required non-empty manifests, explicit target extensions, strict headless scope; 36 Linux checks incl. real native artifacts; Windows CI packages + asserts the manifest (run 34812043127, S20) | Dependency/architecture inspection, clean-machine GUI startup + encode, system-package test with documented installed Qt. Keep the U-09 release/artifact provenance blocker (U-08 licensing closed S19) |
| **GS-203 / P1-25 — PARTIAL** | Core `OutputVerify.h` + JS `output-verify.mjs`: new-or-changed, non-empty regular file with exact GIF87a/89a prefix; wired into CLI explicit outputs and web `/optimize`+`/run` (serve the verified buffer) | Qt-equipped agent: integrate the core helper into `MainWindow` single/queue/Merge lifecycles — capture before launch, verify only after exit zero, keep busy/cancel/error state honest. Test no-write, stale-valid, text/PNG/short output, valid refresh, source preservation, cancellation. Offscreen + real-desktop probes. Do not silently buffer binary stdout or break `--info` |

**Contract limits (not hidden completion claims):** GIF magic is not full
decoding; size/mtime snapshots are conservative within timestamp granularity and
prove nothing against an adversarial local process; the CLI reports failed
postconditions but does not roll back; GS-202 assumes a trusted engine and a
private web temp dir; GUI integration is not done; Windows packaging tests use
fixtures, not executed Windows binaries. **Repeatable local checks** (from the
repo root): `python3 working_code/gifscythe/tests/test_sweep_stale.py` ·
`python3 working_code/gifscythe/tests/test_build_options.py` ·
`working_code/gifscythe/build.sh` · `scripts/smoke_cli.sh` ·
`scripts/test_package.sh` · the eight `node web/test/*.test.mjs` suites ·
`scripts/verify_audit.sh` · then commit before the clean-tree gates
(`check_docs.sh --no-gate-run`, `pr_preflight.sh --online`). Local skips are
never platform proof — re-check remote CI for the pushed SHA.

## 5. Next-session prompt (copy-paste hand-off)

Regenerated S24. The authoritative hand-off is `SESSION_HANDOFF.md` (read its
header block first); the register is `STATUS.md`; decisions are
`docs/planning/OWNER_DECISIONS.md`. This block holds no state.

```
CONTINUATION — gifscythe (freeforall1932-design/gifscythe), after S24.

1. RECOVERY (in order, from the repo root):
   working_code/gifscythe/scripts/bootstrap_hooks.sh      # if G15 says core.hooksPath != .githooks
   working_code/gifscythe/scripts/check_docs.sh           # must be 0 failed; G18 FAIL = commit NOW
   working_code/gifscythe/scripts/sweep_stale.sh          # must be 0 failed
   working_code/gifscythe/scripts/review_change.sh --pr N # review before accepting; every flag has evidence
   working_code/gifscythe/scripts/pr_preflight.sh --online --body /tmp/pr_body.md
   #  before PR create AND again before merge. G16: inspect web/WEB_PLAN_TEMPLATE.md §1-§10 -
   #  leftover <placeholders> = stay SKELETON; filled = flip both state lines (one-way).

2. STATE: START HERE -> STATUS.md (single register); COMPILED_AUDIT.md §5 is the detail
   (v4: 96 U-nn rows). Never hand-edit STATUS.md's generated block - check_docs.sh --emit.
   S24 incorporated the four 2026-09-16 external reviews (§20): U-77/U-79/U-80 fixed by
   PR #30, U-82 fixed in S24, 16 new OPEN rows U-78, U-81, U-83..U-96 with §6 ids
   (P1-44..P1-46, P2-18..P2-22, P3-13..P3-19). DO §19 BEFORE TRUSTING ANY FIXED ROW:
   re-run the §17.1 validation order, failing test FIRST, hunt new pits (§19.3 pairings,
   incl. the S24 addition U-22<->U-62). Windows-only rows (U-55/U-70/U-71) cannot close
   on Linux - PARTIAL with the exact remaining proof, never DONE. Highest-priority open
   rows: U-59 (P0-7 data loss), U-09 + U-95 (release re-cut + pre-relicence asset note),
   GS-203 GUI half (P1-25), U-58 (P1-38), U-78/U-87 (P1-44 web numeric honesty).

3. SKILLOPT: awaiting OD-15 - see docs/planning/PLANNING.md §3. Do not vendor/submodule/
   pip-install before the answer.

4. DECISIONS: docs/planning/OWNER_DECISIONS.md - answered so far: OD-01=a, OD-02=a,
   OD-09=b, OD-11=a, OD-12=a, OD-17=a (and OD-08 resolved in substance S24: the workflow
   copies are re-synced and the pending marker is gone). Remaining: OD-03..OD-07, OD-10,
   OD-13..OD-16, OD-18. OD-16 blocks web/wasm shippable.

5. STANDING CONSTRAINTS: HARD RULE - commit every edit/write/delete before merge AND
   before session close (G18/P3/P3b; the sandbox cut-off lost work twice). Never merge a
   PR without an explicit yes. Docs consolidation (S24): the dated snapshots and the four
   external review files are folded into COMPILED_AUDIT.md / docs/archive/AUDIT_HISTORY.md -
   originals live in git history at 3c67e14; do not recreate scattered copies (G17/S2
   class). Keep every gate green; never ask the owner for tokens. CI workflow edits ARE pushable by
   an agent session (verified S34): edit .github/workflows/build.yml and its byte copy
   docs/ci/build.yml.proposed in one commit and push - do not park them as proposals.
```

---

## 6. Planning report — owner upload 2026-09-29 (snapshot S31), reconciled S32

**Provenance.** The owner uploaded `GIFSCYTHE_PLANNING_REPORT.md` to the repo root
(`13d95a7`, "Add files via upload") carrying its own instruction — *"paste it into
`docs/planning/` and edit freely."* Folded into this file per that instruction and
per the S24 consolidation rule (planning content extends this file; no scattered
root copies — the original text is in git history at `13d95a7`). Read alongside §1
as the current **direction of record**; nothing here commits the owner to anything.

**S32 reconciliation — three places where the snapshot had already been overtaken
by the repo it was written from** (it read the stale WORKLIST/IMPROVEMENT_LOG tails
that S32 corrected):

1. **§6.1 item 1 was DONE before the report was written.** U-59/P0-7 landed end to
   end — CLI/core S28 (`5bbfb83`), GUI S30 (`4a8e353`), and the Explode hole the
   report names as "the same failure class" (the external audit's top HIGH) was
   closed by S31 in PR #6: the CLI now writes frames under a partial prefix and
   promotes them only after verification (mutation-tested smoke case), and the
   GUI's cancel honesty is asserted by harness T21. **Do not redo it.** The
   report's item 1 was written against the "Known hole: N-10 — treat it as open"
   tail that S32 deleted from `SESSION_HANDOFF.md` / `WORKLIST.md` /
   `IMPROVEMENT_LOG.md`.
2. **The ordered list therefore starts at the report's item 2** (release re-cut),
   and gains the audit remainder the report predates: **N-18**'s remaining
   line-by-line reads (`src/core/SettingsIO.h`, `src/qtui/SettingsPanel.cpp`,
   `tests/test_gifsicle_command.cpp`), **N-25** (rate-limit map), **N-26** (CI
   linux flake — it also threatens the W-18 "real green CI run" precondition; **S34: closed** —
   it was the doc gate, not a flake, and a fully green CI run now exists).
3. **Sources corrections:** the report's Sources list cited a
   docs/web/WEB_FEASIBILITY.md file that does not exist in this repo (that
   content has lived in §1 since the S24 consolidation), and "the two
   external-audit files at repo root" are now folded into `COMPILED_AUDIT.md` §21
   (originals deleted S32, texts at `8d30614`).

### 6.1 EXE track — finish, don't rewrite

**Decision:** stay on C++17 + Qt6. A rewrite (Tauri, or the C# shell)
discards ~250 already-verified tests and CI for a marginal ergonomics gain.
*(S38: the C# shell is resumed, but as an ADDITIVE lane behind the CLI
oracle — the EXE track still finishes on C++17/Qt6, and nothing here re-opens
this decision.)*
`OFFLINE_BUILD_REVIEW.md`'s own weighted scoring already settled this
(C++/Qt6 ≈258 vs Tauri ≈205 vs Electron ≈174) — nothing since has changed
that math. *(S32: that scoring now lives in §1; the original file is the S24
consolidation's source 1.)*

**Ordered remaining work** (per the repo's own "what remains before 1.0.0"
list — do these in order, not in parallel, since later items assume earlier
ones are done). ~~1. Finish the GUI's partial-write safety (U-59/P0-7)~~ —
**done, see the reconciliation above; the list starts at 2:**

2. Release re-cut: U-09/P0-4 + the U-95 release-notes edit (owner action first:
   mark the pre-relicence Release superseded/pre-release, never delete).
3. Clean-Windows smoke test (W-18) — re-point at a real green CI run (see
   N-26 — closed S34; run 36967608254 is a fully green run), then actually run it on real hardware (a CI artifact is not the same
   as a clean-machine run).
4. Desktop probes (W-19): kill-engine-mid-run, physical drag-drop,
   engine-missing GUI state. `DESKTOP_PROBES.md` has the procedures.
5. GS-203's GUI half, remaining Qt/platform rows.
6. The register/docs rows: P2-21/U-88, P2-22/U-89, then P3-17/U-91,
   P3-18/U-96, P3-19/U-90.
7. The audit remainder: N-18 line-by-line reads (N-25 and N-26 are closed).
8. Owner decisions: OD-16 (see §6.3), OD-18, the version-bump call.

**Already done, don't redo:** Qt LGPL notice (U-08, closed S19). U-59/P0-7
including the Explode hole (S28/S30/S31). The C# lane's Phase 1 (spike) runs in
CI and Phase 2 (`Gifscythe.Core` + the parity lane) started 2026-10-08
(`OD-C7 = resume`, S38) — additive, CI-only so far, and not on the 1.0.0
critical path: nothing in it should displace the list above.

### 6.2 Web track — minimal wasm, additive, not a replacement

**Decision:** the existing Node-based `web/` server build stays exactly as
it is — untouched, still the officially shipped web path — while a
client-side wasm build is developed *alongside* it as an unproven,
additive track (`web/wasm/`). Nothing gets deleted until the wasm build is
actually proven.

Why not just harden the Node server instead: it needs Node installed and a
manually-started process, which isn't the "no server, double-click and use"
experience the whole project is going for. Wasm is the only path to that,
but it has to earn its way to shippable, not be assumed there.

Why not delete the server now and commit fully to wasm: no `.wasm` binary
has ever actually been produced or run, by any agent, in any session so far
(see §6.3). Deleting a working, hardened thing to bet on something untested
is the least stable option on the table, not the boldest one.

### 6.3 The wasm task — build it, or borrow it, but prove it either way

This is the one piece of the whole plan that's still genuinely open. Two
honest paths; pick one (or run both and compare):

#### Option A — build gifsicle.wasm from scratch (existing scaffold)

The scaffold already exists (`web/wasm/`: `build_wasm.sh`, `config.wasm.h`,
`glue_harness.mjs`, `prove_wasm.mjs`, `wasm.js`, `index.html`) and reuses
`web/command.mjs` + `web/validate.mjs` verbatim. What's missing is the
actual compile: every attempt so far — across at least two separate agent
sandboxes, mine included — has been blocked at the exact same point:
Emscripten's toolchain download comes from
`storage.googleapis.com/webassembly/emscripten-releases-builds/`, and that
domain is unreachable from every sandbox tried so far. This is an
environment/network limitation, not a code problem — the gifsicle source is
straightforward single-process C with no `fork()`, so it should compile
cleanly once a real toolchain is reachable.

**Task:** on a machine with real, unrestricted internet access (a personal
machine, or a CI runner without an egress allowlist), run
`web/wasm/build_wasm.sh`, then `prove_wasm.mjs` against a real file. This
cannot be done from inside the kind of sandboxed agent environment this
project has been built in so far — it needs to be someone's actual machine,
once.

*(S34 correction: the compile itself is not blocked by the sandbox after all —
`python -m ziglang cc -target wasm32-wasi` built the engine from the repo's sources and
the result ran under Node's WASI in an agent sandbox. That is not the emcc build and
promotes nothing; see N-32.)*

**Byte parity note:** this path preserves the project's existing
verification approach (compare wasm output byte-for-byte against the native
1.96 oracle already captured), since it's compiling the exact same vendored
source.

*(S34 correction: this bar cannot pass for any build whose libc differs from the oracle's —
`qsort` orders equal keys differently and `random()` (the dither seed) differs between glibc
and musl; a wasm32-wasi build matched a musl-native build in 9 of 9 cases and the glibc
oracle in 3 of 9 (re-run: `scripts/libc_parity/libc_parity.py --check`). Tracked as N-32 and DECIDED in S34: the bar is a **same-libc native oracle** — the wasm32-wasi build must equal a musl-native build of the same sources byte for byte (`libc_parity.py --bar`, run in CI: 9/9; `--bar --against glibc`, the old bar, fails 6 of 9), and `prove_wasm.mjs --oracle` holds an emcc build to the same bar once someone can build one.)*

#### Option B — adopt an existing pre-built implementation

Gifsicle has already been compiled to WebAssembly by other open-source
projects, and at least one installs cleanly in a network-restricted sandbox
(tested directly in the session that wrote the report):

| Package | Target | Notes |
|---|---|---|
| [`gifsicle-wasm-browser`](https://github.com/renzhezhilu/gifsicle-wasm-browser) (npm) | Browser | Single ~336KB bundled file, no separate `.wasm` fetch, `gifsicle.run({input, command})` API accepting real CLI-style argv. **Installed successfully via `npm install` in that sandbox** — the GCS wall does not block npm. Restores **gifsicle 1.92** behavior — not the project's vendored 1.96, so it will not byte-match the existing oracle; a new correctness baseline would be needed. Could not fully execute it in a plain Node test there (it appears to assume real browser Worker semantics) — untested past install. |
| [`@wasm-codecs/gifsicle`](https://www.npmjs.com/package/@wasm-codecs/gifsicle) (npm) | Node.js only | Clean `encode(buffer, options)` API, MIT license, part of the `cyrilwanner/wasm-codecs` monorepo. No browser build yet per its own README — would need porting for a client-side page. |
| [`gifsicle-bin`](https://pypi.org/project/gifsicle-bin/) (PyPI) | Python wheel + WASM/JS | Explicitly built for "frontend developers... without needing a backend." Worth a closer look if Option A stays blocked. |

**Task:** if Option A stays blocked for longer than the owner is willing to wait,
spike `gifsicle-wasm-browser` in a real browser (not Node) against the same
`logo.gif` oracle used elsewhere in the project, and decide whether a
1.92-vs-1.96 behavioral diff is acceptable for a first shippable web build.

**Licensing note — applies to both options equally:** adopting someone
else's compiled binary does not change the OD-16 question at all. It's
still GPLv2 engine code running in-process alongside Ms-PL first-party
code; the origin of the `.wasm` file is irrelevant to that analysis. Don't
let "we didn't compile it ourselves" read as "so the license question
doesn't apply here."

#### Automatic crash/failure diagnostics (applies to whichever option ships)

Requested explicitly: when someone finally runs this with real internet
access, a crash or failure should be self-diagnosing, not something that
needs a live debugging session to explain. Concretely:

- Wrap wasm module instantiation and every `run()` call in try/catch.
  Never let a failure surface as a silent no-op or a generic "something
  went wrong."
- Capture and log, on any failure: the exact error/exception, the wasm
  module's own stdout/stderr (Emscripten exposes this via `print`/
  `printErr` callbacks — route both into the log, don't drop them), the
  exact command/argv that was attempted, and the environment (Node version
  or browser user-agent).
- Write that log somewhere durable and visible — a downloadable `.log`
  file, or an on-page "show error details" panel — not just the console.
- Fail loud at load time too: if the `.wasm` file itself fails to fetch or
  instantiate, say so immediately and specifically, rather than letting the
  first symptom be a confusing downstream error from code that assumed the
  module loaded.
- Treat this the same way the rest of the project treats "tests that can't
  fail" (the external audit's Priority 6) — a diagnostic path that can't
  actually produce a log on a real failure isn't done.

### 6.4 Open decisions this report doesn't make for the owner

- **OD-16** (`docs/legal/README.md` §3–4): may `web/wasm/` ship with the
  GPLv2 engine in-process alongside Ms-PL code? Currently answered
  "(a) no, until counsel actually answers" — that's a real, load-bearing
  answer, not a placeholder. Needs an actual owner/counsel decision before
  either wasm option above is shippable, regardless of which one is built.
- **OD-18** and the version-bump call — unresolved, not urgent relative to
  the list in §6.1.
- **Option A vs Option B** (§6.3) — genuinely the owner's call; this report
  lays out the tradeoff rather than picking one, since "less work now" and
  "less work overall" point in different directions here.

### 6.5 Sources (as written at S31, corrected S32)

Repo docs referenced: `README.md`, `PROJECT_VISION.md`, `STATUS.md`,
`WORKLIST.md`, `SESSION_HANDOFF.md`, `docs/planning/OWNER_DECISIONS.md`,
`docs/planning/PLANNING.md`, `docs/legal/README.md`,
`web/WEB_PLAN_TEMPLATE.md`, `.github/workflows/build.yml`, and the two
   external-audit files at repo root ("space bunny review", independent
   read-only review via the public GitHub API — since folded into
   `COMPILED_AUDIT.md` §21; originals deleted S32). *(S32: the original text
   also cited a docs/web/WEB_FEASIBILITY.md file, which does not exist in this
   repo — the feasibility direction has lived in §1 since the S24
   consolidation. Mention without backticks on purpose: it is a non-path.)*

External packages referenced: `gifsicle-wasm-browser`, `@wasm-codecs/
gifsicle`, `gifsicle-bin` (links in §6.3).
