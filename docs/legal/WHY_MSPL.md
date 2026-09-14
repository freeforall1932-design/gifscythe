# Why the first-party code is Ms-PL (`OD-09 = b`, `OD-C6` — S18, 2026-09-14)

## The decision

Owner relicensed all first-party code (GUI, control layer, CLI, web client,
future C# shell) from "intended GPLv3" to **Ms-PL**. Engine untouched
(GPLv2-only subprocess). Caesium base dropped at the same time.

## Context

- The ScreenToGif reference fork (`freeforall1932-design/ScreenToGif-fork`)
  is **Ms-PL** (its `LICENSE.txt`). Ms-PL is FSF-listed as GPL-incompatible
  weak copyleft:
  https://www.gnu.org/licenses/license-list.en.html
- The UI's GPLv3 intent existed *only* to match a Caesium-derived UX base.
- Verification before the relicense found the Caesium base was vapor: zero
  Caesium files in the product tree, empty `assets/` + `resources/`, no
  foreign copyright headers in shipped code (`working_code/gifscythe/src/`,
  `working_code/gifscythe/tests/`, `web/`). So the GPLv3 justification
  referenced a base that was never incorporated — and no third-party
  permission was needed to relicense. Sole-author tree.
- The engine boundary (subprocess, never linked) works identically under any
  UI licence, so the engine imposed no constraint either way.

## Why Ms-PL won

1. **Velocity.** Licences match the fork, so fork files (editor windows,
   export pipeline, settings infrastructure) can be copied and adapted with
   notices retained instead of rewritten from patterns. This is the
   maximum-speed path for the WPF shell (Phase 3 of the C# plan).
2. **The GPLv3 reason evaporated.** Its whole justification was Caesium;
   Caesium contributed nothing. Keeping GPLv3 would have preserved a
   constraint for a base that never arrived.
3. **Clean execution.** Sole-author tree + subprocess engine = relicense by
   owner fiat, no negotiations, no dual-licence debt.

## Rejected alternatives

- **Stay GPLv3 + reference-only (was OD-C1 = a).** Viable and safe — kept as
  the fallback right up to the relicense. Rejected for speed: every fork
  behaviour would be reimplemented instead of adapted.
- **Permission letter from upstream.** Would need the upstream author *plus*
  every substantial contributor (1,394 commits) for full-tree coverage, or a
  file-by-file grant. Moot after the relicense.
- **Dual-licence our own code (GPLv3 + Ms-PL).** Helps downstream choice,
  fixes nothing upstream: the fork files still couldn't enter a GPL tree.
- **Relicense the fork.** Impossible — it isn't our copyright.

## Accepted costs

1. **Strong copyleft lost.** Ms-PL is weak: someone may fork, improve
   privately, and ship binaries owing nothing back. GPLv3 would have
   forbidden that. The owner priced this scenario and accepted it.
2. **Caesium door closed.** GPLv3 assets can never enter an Ms-PL tree.
   Replaced by policy: system fonts + MIT/Apache-licensed icon sets only
   (`PROJECT_VISION.md` UI approach).
3. **Ecosystem signal.** Ms-PL is OSI-approved and FSF-free, but the FSF
   urges against it over the GPL incompatibility, and it has no "or later"
   upgrade path. Contributors will pause and google; this file is the answer
   they find.
4. **Qt LGPL notices still open.** Unrelated to the relicense, still the
   U-08 remainder — tracked, not forgotten.

## One-way-door warning

The relicense commit itself is reversible (sole-author code). It becomes
effectively permanent the moment the **first fork file lands** — that file is
Nicke Manarin's, Ms-PL forever, and a pure-GPL tree could never contain it
again. Rule: every copied file is named in `IMPROVEMENT_LOG.md` with its
source commit (see `COPYING_RULES.md`), so the door's state is always
auditable.

## What reopens this decision

- Routine copying does NOT reopen it — follow `COPYING_RULES.md`.
- A full revisit needs a trigger this file doesn't anticipate (e.g. a hard
  requirement to absorb GPL-licensed code) + a new owner decision + a rewrite
  of this folder. Deliberately hard: licence churn is worse than either
  licence.

## Verification appendix (S18)

- Copyright-header sweep over shipped code: no foreign headers.
- `assets/` + `resources/`: `.gitkeep` only.
- Caesium on disk: absent (reference rows retired, ignore lines removed).
- FSF incompatibility note: quoted from the FSF licence list (URL above),
  not interpreted here. Nothing in this folder is legal advice.
