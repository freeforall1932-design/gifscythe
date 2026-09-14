# C# Shell Plan — a C#/WPF exe for Gifscythe alongside (then instead of) the Qt GUI

**State:** `WORKING PLAN` (Phase 0 signed 2026-09-14, S18 — all five
decisions answered, record in §8; the C++-through-1.0.0 direction is now
superseded per OD-C2(c), phased commit)
**Date:** 2026-09-14 · **Author:** arena agent (planning session with owner)
**Context:** owner forked `NickeManarin/ScreenToGif` →
`freeforall1932-design/ScreenToGif-fork` as a faster path to a Windows exe,
with a custom UI/UX for Gifscythe. This plan answers *"do C/C++ and C# clash?"*
and lays out the migration in phases with gates.

Related: `OFFLINE_BUILD_REVIEW.md` §4 (current decision: **stay C++17/Qt6
through 1.0.0** — this plan proposes reversing that, consciously),
`PROJECT_VISION.md` (mission + hard constraints, unchanged),
`docs/ci/CLEAN_WINDOWS_SMOKE.md` (reused for the C# exe in Phase 4).

---

## 1. Do C/C++ and C# clash?

**Technically: no. Philosophically: yes — and that difference is the point.**

### 1.1 No technical clash (three proven boundaries)

| Boundary | How | Fits Gifscythe? |
|---|---|---|
| **Subprocess + argv** | C# `Process.Start(gifsicle.exe, args)`, read exit code | **Yes — this is the plan.** The engine is already a black box behind argv. |
| P/Invoke | C# calls exported C functions in a DLL | Possible, unnecessary. Adds FFI risk for zero gain. |
| C++/CLI bridge | Mixed-mode assembly gluing C++ and .NET | No. Heaviest option, worst packaging story. |

Precedent inside this repo: `web/server.mjs` is **JavaScript** driving the
exact same C engine through the same argv contract, parity-tested against the
real C++ CLI. A C# shell is just the *third client* of that contract
(C++ Qt → JS web → C# WPF). The engine never knows or cares what language
spawned it. There is no "clash" because there is no shared memory, no shared
build, no shared runtime — only a command line.

### 1.2 The philosophical difference (your "game vs software" point) is real

You described it accurately:

- **C/C++ culture = build the engine yourself.** Manual memory, manual
  strings/paths, hand-rolled UI widgets, `windeployqt` DLL hunting. Maximum
  control, like writing a game engine. Every feature pays an infrastructure
  tax before it pays product value. That is why the Qt GUI took S4b+S7+S10
  and 324 harness checks to get right.
- **C# culture = compose from the ecosystem.** The framework + NuGet give you
  windowing, dialogs, data binding, async/await, JSON, settings, and
  single-file publishing. You write *product* code, not infrastructure. That
  is "software you can deliver fast."

**The rule this plan follows:** dependencies for *plumbing*, hand-rolled for
*correctness*. The fork gives you windows, dialogs, and export flow for
free — but Gifscythe's honesty rules (planned batch targets, collision
refusal, output/frame verification, exact argv semantics) must be **ported
faithfully and parity-tested**, not replaced by a NuGet package that
"probably does the same thing." The audit history in this repo exists because
silent-failure bugs hide exactly there.

### 1.3 What the fork actually gives you (scope warning)

ScreenToGif is a **screen/webcam/board recorder + frame editor + exporter**
(C#/WPF, .NET 9, Windows-only). Gifski is only one of its five GIF export
paths. Gifscythe's mission is an **optimizer/editor for animated images** —
recording is *not* in scope (`PROJECT_VISION.md`: "Exclusively animated GIF,
APNG, and WebP").

| Fork part | Adopt? | Why |
|---|---|---|
| WPF project setup, packaging, settings, updater patterns | **Yes** | Pure exe velocity, no scope cost |
| Export pipeline shape (presets, progress, cancel) | **Yes, adapted** | Maps onto gifsicle runs |
| Frame-editor UX ideas | **Yes, ideas only** | Informs the custom UI/UX |
| Screen/webcam/board **recorder** | **No (Phase 0 decision)** | New product, new scope; explicitly out unless the owner redefines the mission |
| Gifski/FFmpeg encoder internals | **No** | Gifscythe's engine is gifsicle; two engine families = two audit surfaces |

### 1.4 License flag (must resolve in Phase 0)

- ScreenToGif is licensed **MS-PL** (`LICENSE.txt`, `Directory.Build.props`
  confirms `MS-PL`). MS-PL §3(D): source distribution only under MS-PL.
- Gifscythe's intent: UI **GPLv3**, engine **GPLv2** (`LICENSE`,
  `PROJECT_VISION.md`).
- MS-PL source files copied into a GPLv3 tree are a license conflict
  (this is not legal advice — but it is a real, known incompatibility).

**Decided 2026-09-14 (OD-C1, option a):** the fork is **reference-only**,
exactly like `reference_code/` treats third-party sources today — read it, learn its
patterns, write fresh C# here. No fork source file is copied into this repo.
This keeps the GPL boundary and the audit trail clean.

---

## 2. Goal and non-goals

**Goal:** a portable, click-and-run Windows `.exe` built in C#/WPF with a
custom Gifscythe UI/UX, driving the *unchanged* gifsicle engine subprocess
with the *same* argv/validation/honesty semantics as the Qt GUI and the web
build — shippable as 1.0.0.

**Non-goals (unless re-decided):**
- No engine change (gifsicle stays C, stays a subprocess, stays GPLv2).
- No recorder (screen/webcam/board capture).
- No cloud, no telemetry, no auto-update (offline-only promise stands).
- No change to `web/` (it becomes the parity oracle's sibling, not a rewrite
  target).

### 2.1 Scope refinement (decided 2026-09-14, OD-C3)

Owner: no recorder; focus is an **XNConvert-style converter + compressor**
with **ezgif-like richness**. Planned product scope, in ship order:

1. **GIF MVP** (Phases 1–4): optimizer/compressor at Qt-GUI parity.
2. **APNG + WebP** (promoted from bucket-list to planned): same
   converter/compressor treatment as GIF, after the GIF shell ships.
3. **Video ↔ animated-picture conversion**: video formats allowed strictly
   as **conversion endpoints** (video→GIF/APNG/WebP import, animated→video
   export). No video editing, no timeline, no capture — the product's
   subject stays animated pictures. Engine story TBD (likely an FFmpeg
   sidecar subprocess, same argv/subprocess pattern; needs its own
   feasibility + license note — FFmpeg is LGPL/GPL depending on build).
4. **Editing features** (ezgif-class: crop/resize/rotate/reverse/text-style
   frame ops): after conversion/compression is solid; each op lands as a
   verifiable engine argv, same honesty bar.

**Mission-amendment note:** `PROJECT_VISION.md` currently says "Not photos,
not video." Item 3 narrows that to *"video only as a conversion endpoint,
never as the subject."* The vision doc must be amended (with this rationale)
before any video-endpoint work starts — no code until the words change.

**Hard constraints carried over:** portable/no-installer/no-admin,
Windows-first, animated-images-only, live one-way command pane, argv
execution (never shell injection), engine-missing and Unicode-path honesty
(the U-07 class of bugs must not regress).

---

## 3. Target architecture

```
Custom WPF UI (XAML, Gifscythe design)      <-- your UI/UX, fresh code here
        |
Gifscythe.Core (C# class library)           <-- port of src/core/ + command.mjs:
        |                                      Settings, command builder,
        |                                      Validate, OutputPlan/OutputName,
        |                                      ProcessRunner, ExplodeVerify
        | argv array, Process.Start (never a shell)
        v
gifsicle.exe (unchanged C engine, subprocess)
```

Parity chain (nothing drifts silently):

```
gifscythe-cli (C++, print mode)  <--- the oracle, kept and built
        ^byte-identical argv^                ^same (field,value,reason)^
Gifscythe.Core.Tests (C#)  <------>  web/test/*.mjs (already exist)
```

The C# test project mirrors what `web/test/command.test.mjs` and
`validate.test.mjs` already do: run the real C++ CLI and compare. This is
how the repo's "verify-before-trust" standard survives the language change.

Packaging: `dotnet publish -r win-x64 --self-contained` → single-file exe +
`gifsicle.exe` sidecar in one portable folder. Self-contained keeps the
portable promise (no "install .NET 9 Desktop Runtime" step, unlike upstream
ScreenToGif).

---

## 4. Phases

### Phase 0 — Decisions (owner, this week; no code)

| ID | Decision | Options | Recommendation |
|---|---|---|---|
| OD-C1 | License stance | (a) reference-only / (b) resolve + copy | **DECIDED: (a)** 2026-09-14 |
| OD-C2 | Reverse "C++ through 1.0.0"? | (a) full commitment / (b) sidecar / (c) phased (sidecar→commit at Phase 3) | **DECIDED: (c) phased** 2026-09-14 — rationale in §8 |
| OD-C3 | Recorder in scope? | (a) no / (b) yes | **DECIDED: (a)** 2026-09-14 — plus scope refinement, see §2.1 |
| OD-C4 | UI framework | (a) WPF / (b) Avalonia / (c) WinUI 3 | **DECIDED: (a) WPF** 2026-09-14 |
| OD-C5 | Qt GUI fate | (a) archive / (b) maintain both | **DECIDED: (a) archive** 2026-09-14 (C++ CLI stays as parity oracle) |

Exit gate: this doc moves `DRAFT` → `WORKING PLAN`, decision log appended
to `IMPROVEMENT_LOG.md`, `WORKLIST.md` gains the Phase-1 board.

### Phase 1 — Spike (time-boxed, 1 session; throwaway-allowed)

Smallest possible proof: a C# console app that builds one argv from a
hardcoded settings object, spawns the repo-built `gifsicle`, verifies the
output GIF signature, and publishes as a self-contained single file.
Success criteria: runs on stock Windows with no SDK/runtime installed,
Unicode path (`é`, CJK — the U-07 case) passes, exit codes honest
(engine-missing ≠ engine-failed ≠ invalid-output).
**Spike failing is a valid outcome** — it would re-confirm the C++ decision
with evidence instead of opinion.

### Phase 2 — `Gifscythe.Core` port (the careful phase)

Port, in order: `GifsicleSettings` → command builder (`GifsicleCommand.h`)
→ `Validate.h` → `OutputName/OutputPlan` → `ProcessRunner` →
`ExplodeVerify/OutputVerify` → `SettingsIO`. Each lands with C# tests that
assert byte/triple parity against `gifscythe-cli` (same fixtures as
`web/test/`). **No UI in this phase.** Exit gate: `dotnet test` green + a
parity report (C# vs C++ vs JS, three-way agreement).

### Phase 3 — WPF shell MVP (the fast phase — this is where C# pays off)

 fresh XAML UI in the Gifscythe custom design (not the fork's look):
Input/Actions/Output tabs at Qt-GUI parity (~30 controls), queue + reorder,
live command pane, before/after animated preview, naming templates, batch
folder, settings persistence. Fork patterns reused (dialogs, progress/cancel,
export flow) but reskinned. Exit gate: side-by-side checklist vs the Qt GUI
harness behaviors (T1–T20 categories) — every honest refusal the Qt GUI
makes, the WPF shell makes too.

### Phase 4 — Packaging + CI + clean-Windows smoke

`dotnet publish` single-file + portable zip script (sibling to
`package_portable.sh`), CI windows job (`dotnet build/test/publish`),
then `docs/ci/CLEAN_WINDOWS_SMOKE.md` executed against the C# artifact
(C4/D3/D4, B5/B6/B14 probes). Exit gate: green CI + signed-off smoke sheet.

### Phase 5 — Cutover to 1.0.0

Owner version decision, release re-cut with the C# exe, docs roll-up
(`STATUS.md`/`WORKLIST.md`/`PROJECT_VISION.md` updated via `check_docs.sh`
flow, Qt GUI archived per OD-C5), and a final three-client parity run
(Qt-last-green vs WPF vs web).

---

## 5. Risks

| Risk | Mitigation |
|---|---|
| License commingling (MS-PL vs GPLv3) | Reference-only default (OD-C1); no fork file lands here without resolution |
| Scope creep (recorder, gifski engine, APNG/WebP) | §2 non-goals + OD-C3; any addition re-opens Phase 0 |
| Parity drift (C# shell silently differs from engine truth) | Phase-2 tests run the real C++ CLI as oracle; same fixture set as `web/test/` |
| Re-introducing fixed audit bugs (U-01 overwrite, U-07 Unicode, U-17 unverified explode…) | Phase-3 exit gate replays the harness categories; audit IDs cited per control |
| .NET runtime breaks "portable" promise | Self-contained publish (Phase 1 proves it); size noted, not a blocker |
| Two GUIs maintained forever | OD-C5 decided up front, executed in Phase 5 |

---

## 6. What stays untouched (and why)

- **`reference_code/gifsicle/` + the C engine build** — the product's most
  verified asset; rebuilding it in C# would be pure risk.
- **`working_code/gifscythe/src/cli/`** — becomes the permanent parity
  oracle for both JS and C# clients.
- **`web/`** — second client and second parity suite; proves the argv
  contract is language-agnostic. Untouched except to document the third
  client.
- **The docs gate** (`check_docs.sh`, `STATUS.md`, four states) — the
  migration is tracked through it, not around it.

---

## 7. Immediate next actions (after owner reads this)

1. Owner answers OD-C1…OD-C5 (chat is fine; agent records them here).
2. If green: run Phase 1 spike (needs a Windows runner or Wine + dotnet —
   CI windows job is the honest proof).
3. Promote this file to `WORKING PLAN` and open the Phase-2 work board.

## 8. Phase-0 decision record (S18, 2026-09-14)

| ID | Decision | Rationale (one line) |
|---|---|---|
| OD-C1 = a | Fork is reference-only | MS-PL source (§1.4) never enters this GPLv3-intent tree; patterns only. |
| OD-C2 = c | Phased commitment | Sidecar through Phase 1–2 (spike + Core port, Qt oracle alive), full commit at Phase 3 (UI built once, in WPF; Qt frozen). |
| OD-C3 = a | No recorder (+ §2.1 scope) | Converter+compressor focus; APNG/WebP planned; video endpoints only; editing later. |
| OD-C4 = a | WPF | Matches the fork, best portable single-file story, mature. |
| OD-C5 = a | Archive Qt at cutover | One GUI to maintain; C++ CLI stays as the permanent parity oracle. |

Why phased (c) over full commitment (a) or permanent sidecar (b):

- **(a) full commitment** is fastest on the calendar (single target, no
  duplicate work, solo focus) but bets 1.0.0 on an unproven stack and
  discards verified Qt behavior before the replacement has earned trust; a
  late WPF wall means an expensive context reload.
- **(b) permanent sidecar** keeps a release fallback but taxes every behavior
  change twice, splits solo focus, and risks two-GUI limbo where the C#
  shell never gets trusted enough to promote.
- **(c) phased** takes the cheap proof first (spike + parity-tested Core,
  with the Qt oracle still alive to check against) and commits exactly when
  UI work starts — the point where building twice would begin to hurt. A
  failing spike, or failing Core parity, re-confirms the C++/Qt direction
  with evidence instead of opinion.

With Phase 0 signed, this file is `WORKING PLAN`, and the OFFLINE_BUILD_REVIEW
§4 decision ("stay C++/Qt through 1.0.0") is superseded on the phased terms
above: Qt remains the shippable path until the Phase 3 commit point, then
archives per OD-C5.

*End of working plan. Next: the Phase 1 spike (§4) on a Windows runner.*
