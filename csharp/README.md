# csharp/ — the C# shell (Phase 1+) — PARKED

**PARKED 2026-09-14 (S19, owner direction): no further work until 1.0.0
ships on C++17/Qt6. This tree stays as-is; the spike stays CI-run.**

Future home of `Gifscythe.Core` (settings/command/validate/output-planning
port) and the WPF shell. Today it holds only the Phase-1 spike:

- `spike/` — throwaway-allowed proof: hardcoded run, engine spawn, output
  verify, single-file publish. Run by CI, decided by the C# shell plan
  (`docs/planning/PLANNING.md` §2; formerly its own file until the S24
  consolidation).

Licence: Ms-PL like the rest of the first-party code (`LICENSE`,
`COPYING.ms-pl`). Copying rules for fork files: `docs/legal/README.md` §2.

## Phase-1 spike (throwaway-allowed) — PARKED

**PARKED 2026-09-14 (S19, owner direction): the exe stays C++17/Qt6, no
rewrite — this spike is inert until 1.0.0 ships on the current stack. No
further work here (no Phase 2 start, no edits beyond keeping this notice
true). The spike stays CI-run as-is; that is parking, not progress. Record:
`docs/planning/PLANNING.md` §2 decision table (`OD-C7 = park`).**

Smallest possible proof for the C# shell (Phase 1 of the plan): a C# console
app that builds one argv from a hardcoded settings object, spawns the
repo-built `gifsicle`, and verifies the output GIF.

Exit codes (all honest — non-zero unless a verified GIF was produced; note
these COLLIDE with the C++ CLI's code space — 3 = engine missing here vs
3 = strict-validation refusal there — which is registered as U-91/P3-17, to
be resolved by one shared exit-code contract when Phase 2 resumes):

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
not a shell bug. First green run: `34804350470` (2026-09-14, S18).

Deliberately not here (Phase 2+): settings parsing, validation, batch
planning, naming templates, engine discovery, full output verification.
Two port-time traps are registered as U-91 (S24 intake): `Stream.Read` may
under-fill the 6-byte magic probe (use `ReadExactly`), and `Quote()` prints
POSIX quoting on a Windows product (the desktop contract is MSVCRT quoting).
