# Phase-1 spike (throwaway-allowed)

Smallest possible proof for the C# shell (`docs/planning/CSHARP_SHELL_PLAN.md`
Phase 1): a C# console app that builds one argv from a hardcoded settings
object, spawns the repo-built `gifsicle`, and verifies the output GIF.

Exit codes (all honest — non-zero unless a verified GIF was produced):

| Code | Meaning |
|---|---|
| 0 | ok (prints `in -> out` byte counts) |
| 2 | usage / caller error (bad args, missing input — engine never starts) |
| 3 | engine missing |
| 4 | engine failed (non-zero exit, start failure, timeout) |
| 5 | engine exited 0 but output is missing/empty/not a GIF |

Run by the `csharp-spike` CI job (`.github/workflows/build.yml`): happy path,
é + space paths, every failure code (the exit-5 cases use `true` as a
lying engine that exits 0 without writing), then a self-contained single-file
publish that must be one `.exe` and must run. CJK paths fail honestly
(rc≠0) — the engine-ACP residual documented in `src/core/WinUnicode.h`,
not a shell bug.

Deliberately not here (Phase 2+): settings parsing, validation, batch
planning, naming templates, engine discovery, full output verification.
