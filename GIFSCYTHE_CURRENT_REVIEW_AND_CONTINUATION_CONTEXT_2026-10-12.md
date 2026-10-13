# Gifscythe — Current Review and Continuation Context

**Repository:** https://github.com/freeforall1932-design/gifscythe  
**Reviewed:** 2026-10-12  
**Purpose:** Project-specific companion to the universal guide `UNIVERSAL_AGENT_CONTEXT_v4.md` (the earlier `UNIVERSAL_AGENT_CI_AND_REFERENCE_DRIVEN_PORTING.md` is absorbed into it). Use this for the current Gifscythe repository only. Recheck the real tip and latest Actions run before making claims about current health.

---

## 1. Direct answers

### Do I need another agent outside Arena?

No. Continue in Arena Agent Mode with this GitHub repository connected. Upload this file and the universal context as supporting context if useful. The main problem is not that Arena cannot manage GitHub changes; it is that some work requires toolchains and evidence unavailable in its sandbox, and the agent must be instructed to use the real CI and inspect visual references rather than guess.

### Should we add another Qt/CMake workflow?

Not as the first move. The current `.github/workflows/build.yml` already provisions Qt 6 on both Linux and Windows, compiles the GUI, runs tests, packages the Windows build, asserts the package manifest, and uploads artifacts. Improve the existing workflow and add only a missing verification stage that catches something it does not currently exercise.

### What needs attention first?

1. Update and validate old JavaScript GitHub Actions versions, because the workflow still uses v4 actions whose definitions specify Node 20.
2. Plan the C# projects’ move from .NET 9 to .NET 10 LTS while the port is still small, and run the existing CLI-parity tests afterward.
3. Add a focused packaged-artifact smoke test if confirmed absent, separate from the current compile/offscreen tests.
4. Keep real clean-machine and real-desktop GUI checks explicitly open until they are actually run.
5. Continue the C# core port and define/review the UI visually before building the WPF shell.

No source or workflow files have been modified as part of this review.

---

## 2. What already exists in CI

The inspected live workflow is `.github/workflows/build.yml` (1163 lines in the inspected snapshot). Its mirror, `docs/ci/build.yml.proposed`, was fetched and compared during review and was **byte-identical** at that time. The project documentation says their parity is enforced; preserve that rule when editing.

Existing build lanes include `gate`, `docs`, `linux`, `portability`, `windows`, and `csharp-spike`. The current workflow already covers:

- **Linux/Qt:** Ubuntu 24.04, `qt6-base-dev`, `./build.sh --all`, CLI/engine checks, web/JS parity suites, packaging tests and manifest assertions.
- **Windows/Qt:** Windows hosted runner, Qt 6.7.3 + MinGW installed through aqt, native Windows engine and CLI/unit tests, CMake with Ninja/MinGW, GUI build, `windeployqt`, offscreen GUI tests, and portable package manifest assertion.
- **Portability:** checks such as the 32-bit-`long` test and the wasm/native libc parity bar.
- **C# spike/core:** Windows hosted runner, .NET SDK setup, the phase-1 process-running spike, and `Gifscythe.Core` unit + CLI-parity checks that compare the C# argument builder with the real C++ CLI from the Windows artifact.
- **Artifacts:** `gifscythe-windows` currently retains artifacts for 14 days. The workflow uploads build outputs and release staging folders; permanent releases are handled separately.

The Qt-free local build path remains useful:

```bash
cd working_code/gifscythe
./build.sh
```

It builds the engine, CLI, and Qt-free unit tests. The full Qt desktop build is explicitly requested with:

```bash
./build.sh --all
```

Use CI for the full Qt/CMake build if the current Arena environment lacks that stack. The repository also has `working_code/gifscythe/scripts/build_qt6_local.sh` for a local Qt fallback when a local reproduction is genuinely useful. Its script source states that it builds Qt 6.8.3 modules/tools, the offscreen plugin, and qgif from source; it is not necessary for every fix.

Sources in the repository:
- [Workflow](https://github.com/freeforall1932-design/gifscythe/blob/main/.github/workflows/build.yml)
- [Workflow mirror](https://github.com/freeforall1932-design/gifscythe/blob/main/docs/ci/build.yml.proposed)
- [CI documentation](https://github.com/freeforall1932-design/gifscythe/blob/main/docs/ci/README.md)
- [Build script](https://github.com/freeforall1932-design/gifscythe/blob/main/working_code/gifscythe/build.sh)
- [Local Qt builder](https://github.com/freeforall1932-design/gifscythe/blob/main/working_code/gifscythe/scripts/build_qt6_local.sh)

---

## 3. Priority issue: modernize GitHub Actions runtimes

The live and mirrored workflows use:

- `actions/checkout@v4`
- `actions/setup-dotnet@v4`
- `actions/upload-artifact@v4`
- `actions/download-artifact@v4`

The action metadata for these v4 refs declares `runs.using: node20`. GitHub announced that Node 20 was retired on **2026-09-23** and recommends using current action versions that support Node 24.

As of this review, the observed official releases are:

| Action | Current workflow | Candidate modern release observed 2026-10-12 |
|---|---|---|
| `actions/checkout` | `@v4` | `@v7.0.1` |
| `actions/setup-dotnet` | `@v4` | `@v6.0.0` |
| `actions/upload-artifact` | `@v4` | `@v7.0.2` |
| `actions/download-artifact` | `@v4` | `@v8.0.2` |

**Treat the table as a researched candidate set, not an untested automatic replacement.** Verify releases and compatibility at implementation time. Consider full commit-SHA pinning with release comments if desired by the project’s supply-chain policy.

Required migration process:

1. Update the live workflow and mirror in the same change.
2. Confirm they remain byte-identical.
3. Check every action used by the workflow, not only the four listed above, for runtime support.
4. Run YAML/parity and documentation gates.
5. Push and inspect an actual Actions run; verify all jobs, artifact upload/download, and the C# lane. Do not carry forward an earlier green claim as proof of this upgraded workflow.

This is a workflow maintenance task; it does **not** mean the application needs a second build pipeline. It also does not automatically explain every past red CI run—the actual current run log must still be inspected.

Official references:
- Node 20 retirement: https://github.blog/changelog/2026-09-23-node-20-is-no-longer-available-in-github-actions/
- Checkout releases: https://github.com/actions/checkout/releases
- Setup .NET releases: https://github.com/actions/setup-dotnet/releases
- Upload-artifact releases: https://github.com/actions/upload-artifact/releases
- Download-artifact: https://github.com/marketplace/actions/download-a-build-artifact

---

## 4. Priority issue: .NET 9 is approaching end of support

The C# projects and `csharp-spike` CI lane currently use `net9.0` / .NET 9. Microsoft’s current support information lists **November 10, 2026** as .NET 9’s end-of-support date, while .NET 10 LTS is supported through November 2028.

Recommended action: upgrade the core, tests, spike, fake-engine project, and CI SDK selection to .NET 10 when compatibility checks are ready. Do this while the WPF shell is still unbuilt, rather than after the migration has grown. Verify each project’s `TargetFramework`, test projects, any `global.json`, publish settings, and native process behavior; then use the real Windows CI run as proof. Avoid changing the framework and an unrelated behavior contract in one unreviewable patch.

Official source: https://learn.microsoft.com/dotnet/core/releases-and-support

---

## 5. What the current workflow verifies — and the remaining gap

The Windows job already compiles the Qt GUI and executes the offscreen harness before packaging. It then stages the portable folder and asserts expected files exist. The package-negative-test suite explicitly admits that its fake `windeployqt` fixture **does not prove real Windows DLL completeness or runtime compatibility**.

A useful targeted addition—after the action upgrades are stable—is a separate job such as `portable-smoke` that:

1. Depends on the successful Windows build/package job.
2. Downloads the artifact produced by that exact run.
3. Uses only the actual staged `release/<version>/Gifscythe` directory, not the compile tree.
4. Runs from a separate Windows hosted runner with the build SDK/tool directories removed from `PATH` where practical.
5. Exercises the packaged engine and CLI against generated real GIF fixtures.
6. Launches the packaged GUI with a supported offscreen mode if the package’s plugins make that possible, checks process exit/diagnostics, and verifies that the deployed Qt/runtime files are being used from the staged package.
7. Uploads concise logs and fails if a required packaged file or runtime test is missing.

This tests the package as a consumer artifact and catches problems not covered by a file manifest. It still does **not** replace a physical clean-machine test: GitHub-hosted Windows runners are not identical to a consumer’s clean installation, and offscreen GUI tests cannot verify all desktop interactions.

Do not add the job until checking its exact dependency paths and how to launch the package. Start small, report the evidence it genuinely provides, and never label it “clean machine verified”.

---

## 6. Explicitly open user/desktop verification

The current `STATUS.md` registers these two owner-only checks as OPEN:

- **W-18 — clean-machine portable smoke.** Requires running the actual release artifact on a Windows machine without Qt, MinGW, or development tools.
- **W-19 — real-desktop GUI probes.** Requires visible desktop testing for behaviors an offscreen harness cannot establish, such as interacting with GUI dialogs, physical drag/drop, or cancellation behavior under the real desktop environment.

The in-repo run-sheet is [docs/ci/WINDOWS_TEST_RUNSHEET.md](https://github.com/freeforall1932-design/gifscythe/blob/main/docs/ci/WINDOWS_TEST_RUNSHEET.md). A new CI job can reduce package risk but does not close W-18/W-19. Keep those rows open until the specified real-machine evidence is returned and recorded.

Do not call the product completely release-ready while these acceptance checks remain open.

---

## 7. C# port: preserve behavior, redesign the UI intentionally

The owner’s direction is Windows-first C#/WPF. The port should reuse the working behavior of Gifscythe’s own engine/control layer, adapt it to the C# architecture, remove features that do not belong in the intended product, and add the features the owner actually requests.

**Do not use the current Qt GUI as a default visual blueprint merely because it exists.** Preserve domain logic and functional coverage, but treat the UI as a separate design problem. The owner has specifically asked for a reference-driven, visual process instead of another text-only guess.

### Existing C# state at the inspected snapshot

`csharp/Gifscythe.Core/` (then targeting `net9.0`, zero NuGet dependencies) already contains:

- `Settings.cs` — shared settings representation;
- `CommandBuilder.cs` — C# counterpart of the C++ command builder;
- `SettingsWriter.cs` — configuration writer with existing quoting/newline behavior;
- `ExitCodes.cs` — intended shared exit-code contract.

`csharp/Gifscythe.Core.Tests/` has a check-counter test runner with a unit lane and a parity lane. The parity lane passes equivalent settings to the real C++ CLI in print mode and compares argv tokens. CI enables `GS_REQUIRE_PROOF=1` so a missing CLI cannot silently turn the parity test into a skip.

### Logic still not ported (inspect before editing)

The current project notes list these missing C++/Qt-independent ports:

- `Validate.h`
- `OutputPlan.h`
- `OutputName.h`
- `ProcessRunner.h`
- `ExplodeVerify.h`
- the configuration reader
- integration of the C# shell’s process/error handling with the shared exit-code contract

The C# WPF shell and its Windows packaging are not yet implemented. Check the current working tree/branch before treating the list as current truth.

### Critical behavior contract still to resolve

The current docs identify **U-91** as OPEN until the phase-1 spike itself uses the shared `Gifscythe.Core.ExitCodes` contract. The spike has historical exit codes whose numbers/meanings differ from the intended core contract. Do not silently “fix” those by editing tests or remapping unrelated behavior; align the spike, shared core, tests, and documentation deliberately.

The current register also has **W-32** OPEN: the per-file results/Status model. Its proposed document is [docs/planning/W32_RESULT_CONTRACT.md](https://github.com/freeforall1932-design/gifscythe/blob/main/docs/planning/W32_RESULT_CONTRACT.md). That document is explicitly **PROPOSED, not approved and not implemented**. It covers per-operation inputs/outputs, byte counts, named failure reasons, six states, and test cases. Do not quietly treat its five proposed columns or any open questions as owner-approved.

### Recommended core-first sequence

1. Reconfirm `.NET 10` migration plan and get current CI checks green.
2. Port `Validate.h` and `OutputPlan.h` as currently suggested by the worklist, with table-driven boundary tests and parity against the existing C++ implementation.
3. Port `OutputName.h` and the configuration reader; test Windows-specific path rules, settings round-trips, Unicode, quoting, and collisions.
4. Port `ProcessRunner.h` and `ExplodeVerify.h`; test timeout, process-tree termination, bounded output, invalid/missing output, preservation of previous good output, and honest cancellation/error behavior.
5. Resolve U-91 by making the spike use the one agreed exit-code contract and proving the result in CI.
6. Build the WPF shell only after the relevant logic contracts are tested and the UI direction has been reviewed visually.
7. Package as the actual intended Windows deliverable and test the staged package in CI.

Use the C++ CLI and tests as behavioral oracles where appropriate. Keep the underlying Gifscythe engine/process boundary intact unless the owner explicitly changes scope. Do not duplicate code blindly when an adapter or a testable shared contract is a better fit.

---

## 8. UI design instructions: research, vision, and user review first

The user has directly stated that repeated attempts based on text-only descriptions have left agents guessing, and they do not want the agent stubbornly to reuse the same UI design. This should be treated as a real development constraint, not a stylistic aside.

For the WPF work:

1. Read the current user prompt and distinguish the user’s actual requested result from prior layouts or inferred preferences.
2. Research the chosen reference product, then inspect genuine screenshots/images. Do not claim to have examined screenshots if only README prose was read.
3. If the user supplied screenshots or a visual reference, use those actual images. Otherwise, search for legitimate reference screenshots, compare more than one view/state, and cite/link the pages used for design conclusions.
4. Extract reusable interaction principles, information hierarchy, controls, and behavior. Do not automatically copy brand assets, exact styling, proprietary source, or the old Qt layout.
5. Write a short visual spec and feature keep/adapt/drop table. Distinguish observed details from inferred details.
6. Create **two or three genuinely different low-cost mockups** (for example, WPF/XAML prototype views or HTML previews if faster) when the requested visual direction is not fully pinned down. Show the preview explicitly and obtain owner direction before making the full interface.
7. Keep the new UI aligned with the owner’s needs and existing domain logic, not the shape that is easiest for the agent to describe. Don’t keep the old arrangement solely to avoid designing something new.
8. Test keyboard navigation, resizing, high-DPI behavior, empty/running/completed/error states, and screenshots of the implemented UI at comparable window sizes.

The repository currently has a draft describing an XnConvert-inspired Input / Actions / Output / Status structure. Use it as a source of functionality and navigation requirements to review—not permission to copy XnConvert’s visual design verbatim. Confirm any unresolved owner wording/asset decisions in `docs/planning/OWNER_DECISIONS.md`, including OD-21 and OD-22.

### Proposed design/product workflow

- **Research board:** reference screenshots and links, labelled by screen/state, with what is observed and what is inferred.
- **Feature map:** keep/adapt/drop/owner-decision per feature; map each kept feature to existing engine/core logic and tests.
- **Visual proposal:** two or three distinct layouts, with an explicit description of the trade-offs.
- **Approval checkpoint:** owner selects/adjusts a preview before major WPF implementation.
- **Implementation:** port behavior and add only requested new scope.
- **Validation:** compare actual WPF screenshots and interaction outcomes against the accepted proposal.

---

## 9. Assessment of the saved repositories

These repositories are research leads, not automatic dependencies. Which ones help depends on whether the task is porting a compiled .NET application, analyzing a native binary, or building game-modding functionality.

| Repository | What it is useful for | Relevance here / caution |
|---|---|---|
| [morluto/rea](https://github.com/morluto/rea) | Evidence-oriented investigation across native binaries, JavaScript/Electron applications, .NET assemblies, and websites; returns findings with evidence/limitations according to its README. MIT listed in README. | **Best lead to investigate first** if the reference is an installed/compiled application and you need to understand what it does rather than guess from prose. It does not replace visual screenshots or user approval. Check requirements/current maintenance before installing. |
| [icsharpcode/ILSpy](https://github.com/icsharpcode/ILSpy) | .NET assembly browser and decompiler; MIT license in README. | Useful for authorized .NET binaries to discover types, resources, methods, and possible XAML-related implementation. Decompiled output may differ from original source; use it as evidence and preserve license/permission boundaries. |
| [HexRaysSA/ida-mcp](https://github.com/HexRaysSA/ida-mcp) | MCP interface for IDA native-binary analysis. README lists IDA 9.4+ / idalib requirements. MIT license. | Only add if the reference requires native reverse engineering and a suitable IDA installation/license is available. Not needed to build ordinary WPF. |
| [bethington/ghidra-mcp](https://github.com/bethington/ghidra-mcp) | Ghidra automation with GUI/headless tooling and CI/build instructions. Apache-2.0. | Useful for native reverse engineering. More infrastructure and tooling surface than a normal app port needs. |
| [SamboyCoding/Cpp2IL](https://github.com/SamboyCoding/Cpp2IL) | Analysis/reconstruction of Unity IL2CPP output; the repository README describes its development branch as work in progress. | Use only if the reference is a Unity IL2CPP program. Not a general .NET decompiler or a generic WPF porting library. |
| [rehan-remade/universal-modder](https://github.com/rehan-remade/universal-modder) | AI-assisted game-modding tools, skills, and shared knowledge base; MIT listed in README. | Useful when this product is specifically a game-modding workflow, especially for tool-assisted live testing and mod-specific assets. Less relevant to a GIF desktop utility by itself. |
| [trevaintdead/ai-game-modding-guides](https://github.com/trevaintdead/ai-game-modding-guides) | Agent workflows, troubleshooting guides, templates, and game-modding project methodology. The README says the guides are MIT but linked projects retain their own licenses. | Useful for structuring agent-led game-mod tasks and documenting live testing, not a substitute for this repository’s compiler or UI spec. |
| [ymatrix1/PortPS5](https://github.com/ymatrix1/PortPS5) | Specialized PS5 executable relinking/porting work; GPL-2.0-only per the repo’s spec. | Not a generic C# desktop port example. Only relevant to its target binary/graphics interoperability domain. Do not copy code into a different-licensed product without a deliberate license review. |
| [boykopovar/AnyPS5](https://github.com/boykopovar/AnyPS5) | Specialized PS5 executable relinking for Linux/Windows; GPL-2.0-only in README. | Same caution: not a reusable generic C# UI/logic framework. Relevant only for the specialized PS5 interoperability topic. |

**Recommendation:** inspect REA and ILSpy documentation first if the reference really is a compiled .NET application. If Gifscythe’s own C++ source is the behavior oracle, use the existing C++ source and CLI tests first—reverse-engineering tools add no value when source is already available. Use IDA/Ghidra only for a demonstrated native-binary evidence gap, and game-modding/PS5 projects only if the product scope actually matches them.

---

## 10. Immediate Arena task prompt

Copy this into the already-connected Arena session, or upload the file and give the shorter instruction to follow it. No external AI agent is required.

> Read `GIFSCYTHE_CURRENT_REVIEW_AND_CONTINUATION_CONTEXT_2026-10-12.md` and `UNIVERSAL_AGENT_CONTEXT_v4.md` carefully. Start by verifying the actual branch/base and current workflow/run status—do not assume the historical green runs prove current Actions compatibility.
>
> First, inspect and modernize the current GitHub Actions action versions that still declare Node 20. Update `.github/workflows/build.yml` and `docs/ci/build.yml.proposed` together, preserve byte-identical workflow copies, run all repository documentation/YAML/parity gates, and verify the real Actions results. Keep the change focused; do not add a duplicate Qt workflow.
>
> Then evaluate upgrading the C# projects and `csharp-spike` lane from .NET 9 to .NET 10 LTS, using the existing tests and CLI-parity lane as proof. Review current CI outputs and identify any additional real failure before changing code.
>
> Before any new CI job, verify whether the current pipeline actually executes the portable package as a consumer artifact after packaging. If it only validates a manifest, propose and implement a narrow separate-runner `portable-smoke` check against the staged package, with named assertions and useful failures. Do not claim this replaces W-18 or W-19 real-machine checks.
>
> For the C# port, keep behavioral logic separate from the WPF layout. Continue from the documented missing core ports and shared exit-code decision; preserve the C++ CLI as a test oracle where useful. Do not build a WPF shell by copying the Qt GUI’s current arrangement or guessing from prose. Research and inspect actual visual references, use image/vision-capable tools, create distinct mockups, explicitly show a preview, and wait for direction if a material layout decision is unresolved. Keep the per-file Status contract labeled as proposed until the owner approves it.
>
> Report the changed files, tested toolchain/action versions, CI run IDs and outcomes, package/artifact verification, screenshot/mockup links if applicable, and remaining unverified owner-only tests. Do not state anything passed without executed evidence.

---

## 11. Research sources checked on 2026-10-12

- Arena Agent Mode repository workflow and preview: https://help.arena.ai/articles/1655691990-how-to-use-coding-in-agent-mode
- Arena Agent Mode upload/workspace behavior: https://help.arena.ai/articles/5432423882-how-to-use-agent-mode
- GitHub Actions Node 20 retirement: https://github.blog/changelog/2026-09-23-node-20-is-no-longer-available-in-github-actions/
- Official action releases: https://github.com/actions/checkout/releases · https://github.com/actions/setup-dotnet/releases · https://github.com/actions/upload-artifact/releases · https://github.com/marketplace/actions/download-a-build-artifact
- .NET releases and support: https://learn.microsoft.com/dotnet/core/releases-and-support
- Qt build configuration: https://doc.qt.io/qt-6/cmake-making-qt-available.html
- Qt installer action options: https://github.com/jurplel/install-qt-action

This review did not change the repository or execute a new GitHub Actions run. It inspected repository files and current published tool documentation; all proposed migrations and additional checks must be verified by the next actual CI run.
