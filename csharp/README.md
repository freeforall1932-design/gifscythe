# csharp/ — the C# shell (Phase 2 in progress)

**Resumed 2026-10-08 (S38) on the owner's direction** — *"im planning to pursue
windows first and that wpf .net"*. The S19 park (`OD-C7 = park`, "no further
work until 1.0.0 ships on C++17/Qt6") was the owner's call and is the owner's to
reverse; `docs/planning/OWNER_DECISIONS.md` records the reversal. The plan
itself is `docs/planning/PLANNING.md` §2.

Licence: Ms-PL like the rest of the first-party code (`LICENSE`,
`COPYING.ms-pl`). Copying rules for fork files: `docs/legal/README.md` §2.

## What is here

- `Gifscythe.Core/` — the port of the Qt-independent control layer
  (`working_code/gifscythe/src/core/*.h` + `web/command.mjs`). **Phase 2 is
  deliberately core-first:** no window exists until the settings, the command
  builder, the conf writer and the exit-code contract agree with the C++ CLI,
  case for case. Ported so far:
  - `Settings.cs` — `GifsicleSettings.h`: the same field names, the same
    sentinels (threads `-1`/`0`/`N`; loopcount `-2`/`-1`/`0`/`N`; delay in
    1/100 s), so the C#, C++ and JS layers stay one contract.
  - `CommandBuilder.cs` — `GifsicleCommand.h`: identical option order, identical
    ranges and identical "emit nothing" branches, with the audit id of each rule
    kept in the comment (U-03/P0-2 threads, U-63 loopcount, U-13/U-48 comments,
    U-42 scale, U-60/U-61 literal tokens…). NOT ported yet: `Validate.h`,
    `OutputPlan.h`, `OutputName.h`, `ProcessRunner.h`, `ExplodeVerify.h` and the
    settings *reader*.
  - `SettingsWriter.cs` — `saveSettingsLines()` / `encode_line_value()`: the
    U-51 newline fold and the DS-12 quoting rules, byte-compatible with the JS
    writer the parity test already feeds to the C++ reader.
  - `ExitCodes.cs` — the ONE exit-code contract `U-91` / `P3-17` demands
    (0 ok · 1 engine/path/output failure · 2 usage or unsafe target · 3 --strict
    refusal, the CLI's documented codes). The Phase-1 spike's own codes (3 =
    engine missing, 4 = engine failed, 5 = invalid output) mapped the same
    numbers to different meanings; the shell's failure kinds now map onto the
    shared contract by test. **U-91 stays OPEN** until the spike itself is
    pointed at this class.
- `Gifscythe.Core.Tests/` — the check runner. House style, not xunit: a CHECK
  counter and a non-zero exit, exactly like
  `working_code/gifscythe/tests/test_gifsicle_command.cpp`, and no package to
  restore. Two lanes:
  - **unit** — always; the audit rules above, pinned case by case.
  - **parity** — the same settings written to a conf and handed to the **real
    C++ `gifscythe-cli`** in print mode; the argv tokens behind the command line
    it prints must equal the C# builder's `Build()` output (compared token by
    token, with path-separator flavour the only tolerated difference; the
    display line itself is compared byte-for-byte wherever the platform echoes
    no separators back). `GS_CLI` points at a built CLI, `GS_PARITY_ENGINE` at the
    engine token to compare past (default `/opt/gifsicle`), and
    `GS_REQUIRE_PROOF=1` turns "no CLI" into a FAILURE instead of a skip — a
    skipped proof must never read as a passed one.
  - Run: `dotnet run --project csharp/Gifscythe.Core.Tests -c Release`.
- `spike/` — the Phase-1 record, unchanged and still CI-run: 117 lines proving
  the toolchain, honest subprocess semantics (distinct exit codes),
  Unicode-path behaviour and a self-contained single-file publish. Its exit
  codes are the *legacy* shape described above.

## Where it runs

`.github/workflows/build.yml`, job **`csharp-spike`** (windows-latest, .NET 9,
`needs: windows`): it downloads the `gifscythe-windows` artifact, builds the
spike, runs `Gifscythe.Core.Tests` against `gifscythe-cli.exe` from that artifact
(new in S38), then stages the engine and runs the spike's own scenarios.
A failing step there is a valid, decision-grade outcome; the job is not
allowed to pass by skipping the parity lane.

**No .NET SDK exists in the agent sandboxes** (`dotnet: command not found`), so
the compile and the parity verdict are CI's to give, exactly as the Qt harness
was before `build_qt6_local.sh` existed. Local syntax is checked with a C#
grammar before a push, but that is not a build and is not claimed as one.

## Spoken to in the docs

- Plan and phases: `docs/planning/PLANNING.md` §2.
- Owner decisions (park, resume, video scope): `docs/planning/OWNER_DECISIONS.md`.
- Copying ScreenToGif fork files (licence-compatible, checklist):
  `docs/legal/README.md` §2.
