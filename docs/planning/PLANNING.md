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

## 2. C# shell plan — PARKED (S19, `OD-C7 = park`)

**State:** parked 2026-09-14 by owner direction — the exe stays C++17/Qt6, no
rewrite, until 1.0.0 ships on the current stack. The Phase-1 spike stays
CI-run and inert; this section is the designated **resume point** after 1.0.0
(a park, not a cancellation). `csharp/README.md` carries the tree notice.

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
**Mission-amendment precondition:** `PROJECT_VISION.md` says "Not photos, not
video" — items 3–4 narrow that to *conversion endpoints only*, and **the vision
doc must be amended before any stills/video work starts** (no code until the
words change). Engine precondition: gifsicle reads GIF inputs only, so stills and
video need a decode step (FFmpeg sidecar candidate — its own feasibility +
LGPL/GPL licence note required). This scope is registered as `U-90` (it must not
be read as pre-approved just because it lives in a parked plan).

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
   class). Keep every gate green; never ask the owner for tokens.
```
