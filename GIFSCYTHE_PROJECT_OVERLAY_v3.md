# Gifscythe — Project-Specific Context Overlay

**Repository:** https://github.com/freeforall1932-design/gifscythe  
**Review snapshot:** 2026-10-13  
**Use with:** `UNIVERSAL_AGENT_CONTEXT_v4.md` (unified; supersedes the v3 lineage)  
**Scope:** This file contains Gifscythe-specific facts and next steps. The universal file defines the reusable engineering method. Recheck the current branch and latest CI state before treating any item below as current.

## 1. Verified live baseline (2026-10-13)

The checks below were made against GitHub's live API and files on 2026-10-13. They are a remote snapshot, not a statement about any local/uncommitted working tree.

- Default branch: `main`; current head: [`3dc49ce24838b636d6e4058ec7ad0a524108ee88`](https://github.com/freeforall1932-design/gifscythe/commit/3dc49ce24838b636d6e4058ec7ad0a524108ee88), merged 2026-10-09.
- Latest `main` Actions run returned by the live API: [37961880819](https://github.com/freeforall1932-design/gifscythe/actions/runs/37961880819), completed successfully on 2026-10-09 at that commit. No later `main` run was returned in the checked first-page query.
- GitHub issue/PR searches returned no open items at review time. Recheck after any branch/PR is created.
- `.github/workflows/build.yml` and `docs/ci/build.yml.proposed` are still byte-identical (blob SHA `d22de8aec9e514445409fcba36592f8d9a692ad7`). Preserve their parity on all edits.
- The active workflow references `actions/checkout@v4`, `actions/setup-dotnet@v4`, `actions/upload-artifact@v4`, and `actions/download-artifact@v4`. The exact `v4` action metadata declares `runs.using: node20`; those refs need migration because GitHub removed Node 20 from hosted Actions runners on 2026-09-23.
- Official first-party release metadata was checked. The Node 24 candidates as of this review are `actions/checkout@v7.0.1`, `actions/setup-dotnet@v6.0.0`, `actions/upload-artifact@v7.0.2`, and `actions/download-artifact@v8.0.2`; each selected tag's `action.yml` declares `node24`. Before changing them, review release notes/input compatibility and preserve artifact behavior. These are verified candidates, not yet applied or tested in this project.
- All four C# project files currently target `net9.0`. The CI setup step specifies `9.0.x`, and `csharp/spike` output paths explicitly use `net9.0`.
- Microsoft policy checked 2026-10-13 lists .NET 9 end of support as **2026-11-10**, while .NET 10 is LTS through **2028-11-14**. A migration should update all project target frameworks, SDK setup, output-path assumptions, and relevant current documentation together, then prove unit + real CLI parity in CI.

## 2. Immediate implementation priorities

### P0 — Migrate first-party Actions from Node 20 to Node 24

1. Re-read the exact live workflow and mirror on the branch being changed.
2. Review release notes and `action.yml` for the candidates listed in the live-baseline section; check inputs, artifact compatibility, and runner requirements.
3. Update both workflow copies in a dedicated branch, preserve byte-for-byte equality, run the existing YAML/docs/parity checks, and inspect the actual new Actions jobs/logs.
4. Keep the change isolated from the .NET target migration where practical, so a regression has a clear cause.
5. Do not treat a green historical run as proof of the new action versions; require a new run on the resulting commit.

Official evidence:
- [GitHub: Node 20 is no longer available in Actions](https://github.blog/changelog/2026-09-23-node-20-is-no-longer-available-in-github-actions/)
- [checkout v7.0.1](https://github.com/actions/checkout/releases/tag/v7.0.1)
- [setup-dotnet v6.0.0](https://github.com/actions/setup-dotnet/releases/tag/v6.0.0)
- [upload-artifact v7.0.2](https://github.com/actions/upload-artifact/releases/tag/v7.0.2)
- [download-artifact v8.0.2](https://github.com/actions/download-artifact/releases/tag/v8.0.2)

### P1 — Migrate the C# proof lane from .NET 9 to .NET 10 LTS

Scope at the checked snapshot: `csharp/Gifscythe.Core/Gifscythe.Core.csproj`, `csharp/Gifscythe.Core.Tests/Gifscythe.Core.Tests.csproj`, `csharp/spike/spike.csproj`, `csharp/testdoubles/FakeEngine/FakeEngine.csproj`, and the workflow's SDK/path references. Search current code and docs for `net9.0` / `.NET 9` before editing so active assumptions are not missed; preserve historical session notes as history rather than silently rewriting their past.

Required proof: compile/run all relevant projects in CI, keep `GS_REQUIRE_PROOF=1` enabled for parity, verify the fake-engine timeout/flood/failure tests, and confirm the built paths match the workflow's staging/execution commands. Review project and planning docs for current-version statements and update only statements that claim to describe current state.

Reference: [Microsoft .NET release/support policy](https://dotnet.microsoft.com/en-us/platform/support/policy).

### P2 — Determine whether the actual staged Windows portable artifact is smoke-tested

Inspect current jobs, artifact names/paths, and outputs. If no post-package runtime smoke exists, add a focused Windows job to download the exact artifact from that run in a fresh hosted runner, verify runtime/plugin files, run the staged CLI/engine on fixtures, attempt a meaningful offscreen GUI launch, and retain diagnostic output. This does not replace W-18 clean-machine smoke or W-19 real-desktop checks.

### P3 — Keep owner/manual acceptance checks open until evidenced

- **W-18:** run the actual portable artifact on a Windows machine without Qt, MinGW, or development tools.
- **W-19:** visible real-desktop GUI probes for behavior offscreen CI cannot fully establish.

Re-read the current task/status files before relying on IDs or state. The connector review established the live branch and selected files, but it did not inspect any local checkout's working tree.

## 3. C# port scope and sequence

The intended direction is Windows-first C#/WPF. Preserve the working behavior and contracts that matter, adapt them to Gifscythe’s target architecture, remove features that the owner does not want only after tracing dependencies, and add requested capabilities. Do not mechanically translate all C++/Qt files or copy the existing Qt GUI layout as the default WPF design.

During the inspected snapshot, `csharp/Gifscythe.Core/` contained `Settings.cs`, `CommandBuilder.cs`, `SettingsWriter.cs`, and `ExitCodes.cs`. The core test project had unit tests and a CLI-parity lane comparing C# argument construction with the actual C++ CLI. Verify the current state before relying on this inventory.

The then-documented missing native-logic ports were:

- `Validate.h`
- `OutputPlan.h`
- `OutputName.h`
- `ProcessRunner.h`
- `ExplodeVerify.h`
- the configuration reader
- integration of the shell’s process/error behavior with the shared exit-code contract

Suggested sequence, subject to the current `WORKLIST.md`/`STATUS.md`:

1. Stabilize current CI/action runtime and decide the .NET 10 migration.
2. Port validation and output planning with boundary tests and parity against C++ behavior.
3. Port naming/path handling and configuration reading; test Windows path rules, Unicode, quoting, collisions, and settings round-trips.
4. Port process execution and explode/verify behavior; test timeouts, process-tree termination, bounded output, cancellation, invalid/missing output, and preservation of previous good output.
5. Resolve the exit-code contract consistently across the spike, core, tests, and docs.
6. Decide unresolved result/status contracts explicitly before treating a proposal as approved.
7. Build the WPF shell after the useful core contracts are tested and the visual direction is reviewed.
8. Package the intended Windows deliverable and test the staged artifact.

The inspected docs marked **U-91** (spike use of the shared exit-code contract) and **W-32** (per-file result/status contract) as open. Recheck current state. `docs/planning/W32_RESULT_CONTRACT.md` was explicitly a proposal, not an approved specification.

## 4. UI must be researched and visually reviewed

The owner has said text-only attempts leave agents guessing and that the new UI must not stubbornly reuse the same layout. This is a firm process constraint for the C# port.

- Research and inspect actual visual references; do not infer appearance solely from README prose.
- Use screenshots and a vision-capable tool when available; be honest if a reference could not be visually inspected.
- Create a feature map of **keep / adapt / drop / owner decision** separately from the visual hierarchy.
- Treat any XnConvert-inspired Input/Actions/Output/Status concept in repository docs as a functional reference to review, not permission to copy XnConvert’s visual design verbatim.
- If design intent remains open, create two or three genuinely different inexpensive mockups and explicitly show the previews before implementing the full WPF UI.
- Preserve the engine/domain behavior while independently designing layout, navigation, controls, and UI state.
- Verify empty/running/completed/partial-failure/error/cancelled states, keyboard use, window resizing, high-DPI behavior, and truthful progress/status.

Review [docs/planning/OWNER_DECISIONS.md](https://github.com/freeforall1932-design/gifscythe/blob/main/docs/planning/OWNER_DECISIONS.md) for current decisions, including any outstanding design/asset questions. Do not mark a decision resolved without evidence of owner approval.

## 5. Exact Arena continuation prompt

> Read `UNIVERSAL_AGENT_CONTEXT_v4.md` first, then this project overlay. Verify the live branch head, local working tree if available, workflow mirror, current build configuration, open tasks, and most recent Actions run. Do not treat historical runs as proof of current health.
>
> Start with a dedicated Node 24 action-runtime update using the release candidates and metadata linked in this overlay. Recheck the branch head, keep `.github/workflows/build.yml` and `docs/ci/build.yml.proposed` synchronized and byte-identical, run all relevant gates, and verify a new real Actions run. Keep the separate .NET 10 migration reviewable: update all project TFMs, SDK selection, output paths and current-state documentation; then prove compile, unit, fake-engine, and C++ CLI-parity behavior in CI. Finally inspect whether the actual staged Windows artifact is executed after packaging; add only a focused package-smoke job if the gap is confirmed.
>
> For the C# port, use existing C++ implementation/tests as behavioral evidence where appropriate. Continue the currently missing core logic after verifying the latest worklist. Preserve the intended contracts and resolve owner decisions explicitly. Do not build the WPF shell by guessing or copying the current Qt UI. Research real visual references, inspect screenshots, create distinct preview mockups, and show them for review before committing to a substantially new interface.
>
> Report exact changed files, tests/commands actually executed, CI run IDs and outcomes, package/artifact verification, the visual references/previews inspected, and remaining owner/manual checks. Do not claim completion without evidence.

## 6. End-of-session report format

```text
Current branch/base commit:
Files inspected and changed:
Current CI run(s) and URLs/IDs:
Actions/toolchain/SDK versions:
Tests actually executed and outcomes:
Artifact package and checks performed:
.NET/core port status:
Visual references and previews actually reviewed:
Owner/manual checks still open:
Known limitations:
Next recommended action:
```

**No changes to the repository are implied by this context document.** Treat it as instructions for the next Arena session and re-verify the repository before acting.
