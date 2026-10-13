# Universal Agent Context — Remote Builds, CI, and Reference-Driven Porting

**Purpose:** Reusable context for an AI coding agent working in Arena Agent Mode or another repository-connected coding harness. Use this to move heavy work from a limited agent sandbox to GitHub Actions, and to keep ports/UI work grounded in source evidence and visual references rather than guesses.

**Reviewed:** 2026-10-12  
**This file is a method, not a project specification.** The agent must inspect the current repository and adapt it. Do not assume Qt, CMake, C#, a browser application, or any particular product is present.

---

## 1. Mission and working principles

The goal is a reliable engineering loop in which the agent makes focused changes, GitHub Actions provisions the real build environment, CI provides objective feedback, and the owner reviews both the patch and the deliverable.

Use these principles throughout:

1. **Inspect before changing.** Read the real source, build configuration, tests, CI, licensing, and the current task register. Do not infer the application stack from an old conversation, a README alone, or a file name.
2. **Use the right execution environment.** If the agent sandbox lacks a compiler, SDK, GUI runtime, device, or system library, do not pretend a syntax check is a real build. Delegate the check to a suitable CI runner or state what cannot be verified.
3. **Extend before adding.** Prefer improving an existing workflow over creating a second workflow that repeats the same work. Add jobs only for a distinct platform, environment, verification gap, artifact consumer test, or costly task that deserves an independent gate.
4. **Test behavior, not just files.** A workflow that parses is not proof a project builds. A successful compile is not proof tests passed. A passing test suite is not proof a portable package works on a clean user machine.
5. **Do not fake success.** Never swallow non-zero exits, turn required tests into skips, upload an empty artifact as success, or write “verified” without naming the command/run and outcome.
6. **Preserve the owner’s actual intent.** Read the latest user request carefully. Distinguish mandatory requirements, preferences, explicitly unwanted features, and open decisions. If an old design conflicts with a newer direct request, record the conflict and follow the newest explicit direction within the project’s constraints.
7. **Keep evidence and uncertainty visible.** Mark unimplemented, untested, simulated, and owner-only checks as such. Do not silently convert a proposal into an approved requirement.

## 2. Arena Agent Mode: where to run the work

When the target GitHub repository is already connected to Arena Agent Mode, the normal path is to continue inside Arena. A second agent or a separate IDE is optional, not a prerequisite.

Recommended loop:

1. Open a session with the correct repository connected and verify the intended branch/base.
2. Ask the agent to inspect and summarize the repository before editing. Upload this context file if the session needs durable instructions.
3. Have it make one cohesive change on the working branch, run any available local checks, and push/prepare the pull request using the existing integration.
4. Inspect the GitHub Actions checks and the actual diff. Ask the agent to diagnose the real failed step, not to guess from the workflow text.
5. Review the produced artifact if there is one. Keep manual, hardware, and real-desktop checks separate from CI-only evidence.

Arena’s current documentation says a connected Agent Mode session works in a repository copy, can commit/push and create a pull request, and exposes a diff and checks view. It also notes one pull request per chat session; after the PR is merged or closed, new changes in that session may no longer be pushable. Therefore, finish/push the intended work before merging, and start a fresh connected session for subsequent work where necessary.

If the product UI is important, **explicitly request a visual preview**. Arena documentation says preview can be requested and that Agent Mode supports image uploads, web search, and a sandbox/bash environment. Actual access to a specific app, screenshot, or executable still depends on what the session can reach and what the user provides.

References:
- https://help.arena.ai/articles/1655691990-how-to-use-coding-in-agent-mode
- https://help.arena.ai/articles/5432423882-how-to-use-agent-mode

## 3. Repository reconnaissance checklist

Before writing or changing CI, establish the following facts from the repository itself.

### Application and build

- What is the actual product and intended deliverable?
- What is the top-level build entry point: `CMakeLists.txt`, CMake presets, `.sln`/`.csproj`, `package.json`, `Cargo.toml`, `pyproject.toml`, Makefile, custom scripts, or something else?
- Which language(s), SDKs, compilers, generators, runtime libraries, and external dependencies are used?
- What versions are required or already pinned? Check lockfiles, `global.json`, presets, toolchain files, CI configuration, and code requirements.
- Which OSes and architectures are genuine product targets, rather than just convenient test environments?
- Which GUI modules, plugins, assets, external executables, codecs, drivers, or native runtime DLLs must be deployed?
- Which source tree is canonical? Are there mirrors/templates of workflow or config files that must remain synchronized?

### Existing validation

- What jobs/workflows already run on push, pull request, manual dispatch, or release tag?
- Which tests actually execute, and which are skipped or only simulated?
- What test runner, test names, expected outputs, fixture requirements, and failure codes are used?
- Is packaging already configured? What exactly is the real output path?
- Is the actual package tested after packaging, or does CI only test the build tree?
- Are there open issue/audit registers, handoffs, owner decisions, or “known limitations” documents that define current truth?
- Are the relevant build/action versions still supported? Check current official release notes rather than assuming an old major tag is current.

### Deliver a short inspection summary before edits

State the detected stack, existing build command, relevant CI jobs, existing tests, package path, biggest verified gap, and proposed smallest change. If a fact cannot be established, say it is unknown instead of inventing it.

## 4. Decide whether to modify an existing workflow or add a new one

**Default: extend the existing workflow.** Do not add a new workflow just because the current workflow has many steps or because the agent sandbox lacks dependencies.

Add a new job or workflow only when it creates a meaningful boundary, for example:

- a different host OS/compiler/SDK that cannot be validated in the existing job;
- a separate clean-runner consumer test of the actual packaged artifact;
- a manual/release-only process with different permissions;
- a long-running test or costly integration task that should be independently scheduled;
- a test requiring a device, GPU, licensed SDK, or controlled environment unavailable to the normal build;
- a distinct security or supply-chain gate.

Avoid adding broad matrices before a representative target works. Start with one authoritative build, then expand only to supported targets that matter to users.

A workflow may support `push`, `pull_request`, and/or `workflow_dispatch`; pick the triggers that suit the repository and avoid running an expensive full matrix for every irrelevant file change unless that behavior is deliberate.

## 5. Requirements for a useful GitHub Actions workflow

Every project’s workflow should be adapted to its real stack, but should generally have these properties:

- **Clean runner:** no dependency on state existing only in the agent sandbox or owner’s machine.
- **Correct runner:** choose the OS/compiler/SDK needed by the target. A Linux build does not prove a Windows GUI package works.
- **Explicit versions:** select SDK and dependency versions deliberately. Recheck official action releases and runner-image lifecycles before implementation; dates and current major versions in a context document will age.
- **Least privilege:** begin with read-only token permissions for build-only jobs. Grant additional permissions only to jobs that actually need them. Do not expose secrets to untrusted pull-request code. Do not add `pull_request_target` or write permissions without a reviewed security reason.
- **Failure honesty:** set the workflow to fail on real configure/build/test/package failures. Do not use `continue-on-error` for required checks. If diagnostics are best-effort, label them diagnostic-only and ensure the primary step still determines the result.
- **Timeouts:** set reasonable job/step timeouts, especially around hung subprocesses, integration tests, or GUI tests.
- **Concurrency:** cancel superseded work where appropriate, without accidentally cancelling release/mainline proof runs that must complete.
- **Cache carefully:** use a cache only when it helps and its key captures toolchain/configuration inputs. A cache must not hide dependency drift or become a requirement for a clean build.
- **Readable failures:** emit useful compiler/test failure lines and failing test names. Preserve the full diagnostic log where appropriate, but do not make every failure require downloading a huge build directory.
- **Artifacts:** upload intentional deliverables and test reports, not the entire workspace by default. Use an explicit artifact name, appropriate retention, and an error if an expected output is missing.
- **Document proof:** state what was built, the target OS/architecture, SDK versions, commands, tests, artifact path, run URL/ID, and what remains unverified.

For stronger supply-chain protection, consider pinning third-party actions to full commit SHAs with a release comment, following the repository’s security policy. If the project uses version tags instead, keep them current and review upgrades rather than silently copying versions from this file.

Official references:
- GitHub Actions runner labels and lifecycle: https://github.com/actions/runner-images
- Runner selection: https://docs.github.com/en/actions/how-tos/write-workflows/choose-where-workflows-run/choose-the-runner-for-a-job
- Qt installer action: https://github.com/jurplel/install-qt-action
- Qt CMake integration: https://doc.qt.io/qt-6/cmake-making-qt-available.html

## 6. Verification ladder: do not confuse these levels

Report each level distinctly. Do not claim a higher level merely because a lower level passed.

| Level | What it proves | What it does **not** prove |
|---|---|---|
| Source/static check | Formatting, types, syntax, lint, or source invariants as explicitly tested | Successful native compilation or runtime behavior |
| Compile/build | The relevant target compiles on the named runner/toolchain | Tests passed, packaging is complete, or it works on another OS |
| Unit/regression tests | The named cases passed against the tested code | Uncovered behavior or GUI experience |
| Integration/E2E tests | Components interacted in the tested scenario | All real-user setups or all platforms |
| Headless GUI test | Tested GUI contracts work with the declared offscreen setup | Physical input, visible rendering, OS dialogs, or desktop behavior that the harness cannot exercise |
| Package manifest | Required files are present and non-empty | DLL/plugin completeness or that the program launches |
| Packaged-artifact smoke | The actual staged/archive deliverable launches and performs selected actions in a fresh job | A genuinely clean consumer machine or full visual usability |
| Clean-machine/real-device test | The package ran on the specified real device/clean OS setup | Unchecked environments or other hardware configurations |

A successful compile is necessary but insufficient for “portable” or “ready to release”. Test the artifact after packaging, separately from the developer build directory, when feasible.

## 7. Heavy native builds: Qt 6, CMake, C/C++, and similar toolchains

When the agent sandbox lacks Qt/CMake or the required compiler, use the real repository’s CI build rather than asking the agent to install every dependency into its sandbox.

### Required inspection

- Read the root build file, CMake presets/toolchain file, source CMake files, and existing build scripts.
- Determine exact Qt version, Qt modules, compiler ABI, architecture, generator, and target OS. Do not install a random Qt version or mix MSVC Qt with MinGW binaries.
- Decide whether a system package is sufficient or a versioned SDK install action is required.
- Identify Qt platform plugins, image-format plugins, codecs, third-party DLLs, and non-Qt runtime dependencies.
- Discover actual build and packaging output paths by reading the scripts or measuring one real build.

### Basic CI shape

1. Checkout.
2. Provision the required compiler, CMake/Ninja/Make, and exact Qt SDK/modules.
3. Configure using the repository’s actual command and generator.
4. Build the intended target(s).
5. Run native unit tests.
6. Run integration/CLI tests and GUI offscreen tests if supported.
7. Package for the real end-user target.
8. Assert package contents and required license/notice files.
9. Run selected tests against the packaged output, not just the build-tree executable.
10. Upload the actual deliverable, plus concise useful test reports.

Use `jurplel/install-qt-action` only if its selected Qt version/host/target/architecture/modules match the project, or use exact system packages where suitable. Qt is usually provisioned as a prebuilt SDK; do not FetchContent-build Qt from source as the default solution.

On Windows, compile and deploy with a consistent toolchain. For a Qt desktop app, use the relevant deployment mechanism (for example, `windeployqt` when appropriate), include platform plugins and application assets, then launch the staged package in a new runner with development-tool paths removed as much as practical. This improves packaging confidence but is still not the same as proving behavior on a truly clean user machine.

A dedicated packaged-smoke job is useful when the existing job builds and checks a manifest but never runs the resulting product from its staged package. It is not a reason to duplicate the whole build matrix.

## 8. C#/.NET and native-interop projects

For a C# application, discover the real `TargetFramework`/`TargetFrameworks`, `global.json`, `.sln`/`.csproj`, runtime identifier, WPF/WinForms/Avalonia dependencies, native process boundaries, and packaging method.

A common .NET CI lane should:

1. Set up the chosen supported .NET SDK.
2. Restore deterministically when a lockfile/locked-mode policy exists.
3. Build the real solution/project in Release.
4. Run unit and integration tests.
5. Test the actual native dependency or CLI boundary where relevant.
6. Publish/package with the required runtime assets.
7. Upload the resulting installer/archive or self-contained package.
8. Execute selected checks against the published result.

Use a Windows runner for Windows-only WPF behavior. A Linux compile of a portable class library does not validate a WPF shell. If the .NET version is near end of support, evaluate upgrading before the application grows, with tests covering the migration.

### Porting existing logic without inheriting an unwanted UI

Separate the port into three contracts:

- **Behavior contract:** settings, validation, algorithm choices, file I/O, command arguments, error handling, exit codes, cancellation, and output verification.
- **Integration contract:** interfaces/adapters to the owner’s existing engine, domain model, CLI, storage, services, or other codebase.
- **Visual/interaction contract:** layout, navigation, component hierarchy, states, workflows, keyboard/mouse/touch behavior, and accessibility.

Do not translate every file mechanically. For each useful behavior, identify source evidence, define the expected contract, adapt it to the target architecture, and write a test. Reuse public interfaces or concepts when sensible; avoid copying irrelevant implementation details. Make an explicit feature matrix: **keep / adapt / drop / owner decision**. Trace dependencies before deleting code, and verify that removed features are not still called or needed by tests.

When an existing implementation is a behavioral oracle, compare the new implementation against it with golden tests, characterization tests, or black-box integration tests. If it is a separate executable, invoke it in the test harness and compare outputs or observable behavior. Keep the reference oracle isolated from the new UI when possible.

## 9. Reference-driven UI: research and inspect visuals, do not guess

This section is mandatory whenever the user asks for a particular visual result, port, redesign, or feature inspired by another application.

### Before implementing UI

1. **Identify the actual reference.** Use the user-provided application, official website, screenshots, documentation, public source, or a named authorized binary. If none is supplied, research candidate references and clearly label assumptions.
2. **Use visuals as evidence.** Search for real screenshots, open the relevant image/page, inspect the visible layout, and use an image/vision-capable tool when available. A prose description is not a substitute for a screenshot. Do not say that an image was examined if only text was read.
3. **Inspect states, not just the home screen.** Look for normal/empty/loading/running/success/error states, dialogs, settings, progress, status/history, keyboard behavior, and responsive or high-DPI layouts. Use the real app where access and permission allow it.
4. **Create a visual inventory.** Record screen/section hierarchy, spacing, alignment, density, typography, icon style, color tokens, controls, table structure, and interaction rules. Distinguish observed facts from guesses.
5. **Separate inspiration from copying.** Extract interaction principles and useful behavior; do not automatically clone the reference’s branding, assets, layout, or source code. Check license/permission before reusing code or assets.
6. **Offer concrete alternatives.** Before implementing an expensive UI, create two or three genuinely different low-cost wireframes/mockups/HTML previews if design intent is ambiguous. Label their differences and ask the user to choose or revise. Do not repeatedly default to the current application’s layout just because it already exists.
7. **Ask for preview explicitly.** Request a screenshot/preview and show the user the actual generated UI. Do not substitute a description of what the UI might look like for a preview.
8. **Validate against the selected reference and requirements.** Capture screenshots of the new implementation at the same window size/state, compare layout and hierarchy, and record remaining differences. Use screenshot tests when stable enough, but do not overfit to pixels if responsive behavior matters.

If the user says “do not reuse the same UI”, preserve the working behavior and domain logic while changing the visual/interaction structure according to their requirements. Treat the old UI as evidence of features, not the default design to copy.

### Visual acceptance checklist

- Does the screen hierarchy match the user’s desired workflow, rather than just the old application?
- Are primary actions obvious and secondary settings discoverable?
- Are file queues, progress, per-item results, errors, and cancellation truthful?
- Are states visible: empty, processing, success, partial failure, cancellation, and no output?
- Does the UI avoid misleading byte counts, false 100% progress, stale previews, and “success” on incomplete output?
- Are spacing, contrast, text sizing, keyboard navigation, and window resizing usable?
- Was the proposed UI shown for review before a substantial implementation when user preference was unresolved?

## 10. Reverse engineering and repository references

When reference sources or a compiled application are involved:

- Use repository source and official documentation first when available.
- For authorized .NET assemblies, a decompiler such as ILSpy can help map types and methods; decompiled code is evidence to verify, not guaranteed original source.
- For native binaries, tools such as IDA or Ghidra and their automation interfaces may help when they are available and licensed.
- For Unity IL2CPP games, a specialized tool such as Cpp2IL may be relevant; it is not a general C# desktop-app porting tool.
- Game-modding agent guides and universal-modder knowledge bases are relevant to game modding, live testing, asset workflows, and game-specific tooling; they do not replace the build system of a general application.
- PS5 executable porters are specialized binary interoperability projects, not generic examples for a WPF interface or ordinary C# code migration.
- Check each project’s license, dependency licenses, and restrictions before reusing code/assets. Keep attribution and notices as required. Do not copy proprietary material merely because it can be decompiled. Only inspect targets the user has rights and permission to analyze.

The repository list below is a set of leads, not a set of dependencies that must all be installed:

- REA — https://github.com/morluto/rea — a promising evidence-oriented investigation tool; its README describes analysis of native binaries, JavaScript/Electron applications, .NET assemblies, and websites.
- ILSpy — https://github.com/icsharpcode/ILSpy — .NET assembly browser/decompiler; MIT license according to its README.
- IDA MCP — https://github.com/HexRaysSA/ida-mcp — IDA-based binary-analysis interface; only useful if IDA and a suitable target are available.
- Ghidra MCP — https://github.com/bethington/ghidra-mcp — Ghidra automation; substantial tooling, relevant for reverse engineering, not basic app builds.
- Cpp2IL — https://github.com/SamboyCoding/Cpp2IL — specifically useful for Unity IL2CPP investigation; inspect its development/stability state before adopting it.
- Universal Modder — https://github.com/rehan-remade/universal-modder — AI-assisted game-modding tools and knowledge, especially relevant if the product is actually a modding tool.
- AI Game Modding Guides — https://github.com/trevaintdead/ai-game-modding-guides — methodology and templates for agent-driven game modding; linked projects have their own licenses.
- PortPS5 — https://github.com/ymatrix1/PortPS5 — specialized PS5 binary porting work; GPL-2.0-only per the project spec surfaced during review.
- AnyPS5 — https://github.com/boykopovar/AnyPS5 — specialized PS5 executable relinking/interoperability; GPL-2.0-only per its README.

Do not add all these as plugins or package dependencies. Select one tool only if it addresses a demonstrated evidence gap, and first confirm current maintenance, requirements, license, and integration effort.

## 11. Recommended task workflow

### Stage A — Inspect and baseline

- Identify branch/base, clean/dirty working tree, actual code paths, current tests, and current CI runs.
- Run available quick checks without installing an entire stack unnecessarily.
- Write a short baseline: what currently builds, what fails, what is missing, and what cannot be measured here.

### Stage B — Make a narrow plan

For each item, record:

- observed defect or user requirement;
- source evidence;
- intended behavior and non-goals;
- implementation files;
- regression test and failure mode it should catch;
- target CI job/toolchain;
- package/runtime consequence;
- owner decision required, if any.

### Stage C — Implement test-first where feasible

- Reproduce the defect or create the test before the fix where possible.
- Make the smallest correct change.
- Keep tests that fail on the broken behavior and pass on the corrected behavior.
- Do not rewrite unrelated UI or architecture while fixing CI.
- For design work, agree the visual direction using a real preview before applying it across multiple screens.

### Stage D — Run local and remote verification

- Run all relevant local checks the sandbox can truly execute.
- Push the branch and let Actions run against the real SDK/compiler/platform.
- Open the failing job/step and get its actual logs/annotations.
- Fix root causes, not just the assertion or workflow gate. Rerun required checks.
- Download and inspect the actual build artifact; verify its contents and, where feasible, run it from a fresh runner job.

### Stage E — Review, document, and stop honestly

- Review the full diff for unintended changes, secrets, license changes, and unwanted UI reuse.
- Synchronize duplicate workflow templates/configuration if the repo requires it, then run its parity gate.
- Update current-state docs, issue registers, and handoff notes if the repo requires them.
- Commit all intended edits before the session ends; don’t leave important changes only in the sandbox.
- Report unverified owner-only checks and next action. Don’t claim “done” merely because CI is green if acceptance criteria remain open.

## 12. Reusable prompt to give the connected coding agent

> **Inspect this repository first and adapt this context to its real architecture.** I want the agent sandbox to focus on source changes and analysis, while GitHub Actions performs resource-intensive dependency installation, compilation, testing and packaging whenever that is the right execution environment.
>
> Before editing, identify the actual stack, build entry point, target platforms, SDK/compiler versions, dependencies, existing workflows and mirrors, existing tests, and real package output. Read the latest handoff/issues/decisions if they exist. Do not assume this repo is Qt, CMake, C#, web, or any other stack until you verify it.
>
> Reuse and improve existing CI rather than adding a duplicate workflow. Add only the smallest missing job(s) needed to prove a real gap—for example, a clean-runner test of the actual package if it is currently only compiled and checked by manifest. Check current official GitHub Action versions and runtime compatibility. Use least-privilege permissions, safe PR handling, timeouts, reliable failures, and intentional artifacts. Do not suppress errors or let required evidence become a skip.
>
> If the task involves porting or redesigning a product, separate behavioral logic from integration and UI. Inspect the user’s actual requirements and reference screenshots/apps. Do not guess a UI from prose, clone the existing layout by default, or reproduce an old visual design unless I explicitly ask. Research and inspect actual visuals using available image/vision tools. Produce a few distinct low-cost mockups when design intent is unclear and explicitly show a preview for review before building the full interface. Preserve the useful behavior, adapt it to this repository’s architecture, remove only features confirmed unwanted after tracing dependencies, and test behavioral parity against an existing implementation where appropriate. Respect source-code and asset licenses.
>
> Make a narrow change, run local checks that are genuinely available, then rely on real CI results for missing compilers/SDKs/platforms. Read actual failing logs, fix the root cause, and rerun. Report changed files, commands, toolchain versions, actual CI run IDs/results, exact artifact links, visual previews when applicable, limitations, and remaining owner/manual checks. Never claim success without executed proof.

## 13. Report template after a session

```text
Repository / branch / base commit:
Task and non-goals:
Inspected source/build/CI files:
Changed files:
Local checks run and results:
Remote CI run URL/ID and target toolchains:
Tests that actually ran:
Tests skipped, and why:
Artifact name/path/link and package verification:
Visual reference(s) inspected and preview reviewed (if applicable):
License/security considerations:
Known limitations and owner-only tests:
Next single action:
```

## 14. Time-sensitive notes

This file was reviewed on 2026-10-12. The GitHub Actions ecosystem, JavaScript action runtimes, runner images, .NET support policy, Qt SDKs, and third-party repositories can change. Before changing workflow versions or committing a tool as a dependency, verify the current official release, runtime compatibility, maintenance status, and license.
