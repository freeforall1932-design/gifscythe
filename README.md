# Gifscythe — work-in-progress (GIF / APNG / WebP animation tool)

**Current product version:** 0.1.0 (see `working_code/gifscythe/VERSION.md`)
**Status:** engine + control layer + CLI + Qt GUI + web app shippable as a
pre-release; **GIF is the only format implemented today** — APNG/WebP are
formally deferred to 2.0.0+ (roadmap below).

**Where state lives (read these, not prose):** **`STATUS.md`** is the single
generated status register — one row per tracked item, four states, counts you
can trust because a gate re-derives them. `COMPILED_AUDIT.md` (v4) is the
evidence behind every finding, ten independent reviews merged. This README
deliberately carries no session narrative any more: the S4–S24 history is in
`IMPROVEMENT_LOG.md`, and the current hand-off is `SESSION_HANDOFF.md`. The
2026-09-17 consolidation (S24) folded the dated doc snapshots together — see
`docs/archive/AUDIT_HISTORY.md` for the audit-history index.

**Honesty summary (as of S24, 2026-09-17):** the desktop/CLI/web surfaces are
CI-verified on linux + windows (main green at `3c67e14`, run 35112077599,
2026-09-16). What has NEVER been proven: the portable Windows bundle on a real
clean machine (W-18) and three physical-desktop GUI behaviors (W-19) — Wine and
the offscreen harness are emulation/CI signals, not Windows proof. One
registered data-loss row is still open (U-59/P0-7: cancel can truncate a file
over a previous good output), and the one published Release
(`snapshot-2026-09-07`) is stale (U-09) and predates the Ms-PL relicence
(U-95) — treat it as a historical artifact, not a current build.

## Quick start

```bash
cd working_code/gifscythe
./build.sh                  # engine + CLI + unit tests
./scripts/test_engine.sh
./scripts/smoke_cli.sh
./build/gifscythe-cli examples/animation.conf          # print command
./build/gifscythe-cli examples/animation.conf --run    # run engine
./build.sh --all            # also Qt6 GUI (fails honestly if Qt missing)
./scripts/check_docs.sh     # documentation gate — green before any PR
```

Web app (self-hosted product alternative): `node web/server.mjs 8000` from the
repo root, then open http://localhost:8000 — see `web/README.md`.

## Screenshots (S10 refresh, offscreen Qt 6.4.2 — see docs/screenshots/README.md)

| Input tab — queue, reorder, before/after preview | Actions tab — full control surface | Output tab — template, batch folder, summary |
|---|---|---|
| ![Input tab](docs/screenshots/shot_input_tab.png) | ![Actions tab](docs/screenshots/shot_actions_tab.png) | ![Output tab](docs/screenshots/shot_output_tab.png) |

## Repo layout (post-S24 consolidation: one file per topic)

```
gifscythe/                        (repo root)
  STATUS.md                       THE status register (4 states, generated)
  PROJECT_VISION.md               mission + hard constraints + architecture/flag map
  WORKLIST.md                     human task board + the status rules
  SESSION_HANDOFF.md              notes for the next session (start-at-top header)
  IMPROVEMENT_LOG.md              decision/change log (append-only, newest first)
  COMPILED_AUDIT.md               master audit register v4 (start here for reviews)
  LICENSE / COPYING.ms-pl / COPYING.gifsicle / COPYING.lgplv3 / COPYING.gplv3
                                  license notices (Ms-PL UI + GPLv2 engine + LGPLv3 Qt)
  README.md                       this file
  docs/
    archive/AUDIT_HISTORY.md      condensed index of the seven dated audit snapshots
    ci/README.md                  workflow status + clean-Windows smoke + desktop probes
    legal/README.md               licence single source of truth (Ms-PL why, copying
                                  rules, the wasm OD-16 question)
    planning/OWNER_DECISIONS.md   open owner questions (OD-01..OD-18)
    planning/PLANNING.md          direction review, parked C# plan, SkillOpt query,
                                  sequential-work handoff, next-session prompt
    release/RELEASE_PROCEDURE.md  how to cut snapshots/releases
    screenshots/                  UI shots + re-shoot recipe
  .githooks/pre-push              blocks a push with a red documentation gate
  web/                            web app (product surface) + eight JS⇄C++ suites
    WEB_PLAN_TEMPLATE.md          web-surface plan template (state: SKELETON, gate G16)
    wasm/                         experimental in-process track — NOT SHIPPABLE (OD-16)
  csharp/                         C# shell — PARKED S19 until 1.0.0 ships on C++/Qt6
    spike/                        Phase-1 spike (throwaway-allowed; CI-run; inert)
  reference_code/                 SOURCE MATERIAL — do not edit, do not ship
    gifsicle/                     gifsicle @ upstream 07f5c4c3 (build input)
    gifsicle-nested-1.96/         pristine v1.96 provenance baseline (see manifest)
    REFERENCE_MANIFEST.md         digests + reproduce recipe for both trees
  working_code/                   THE PRODUCT — edit & ship this
    gifscythe/                    the app (v0.1.0)
```

## What is reference vs. working
- **reference_code/** — unmodified source material we reference, adapt, or bundle
  into releases. **Never edit** these; treat them as read-only imports. The one
  exception: `reference_code/REFERENCE_MANIFEST.md` is the provenance record.
- **working_code/** — our actual product. All edits happen here.
- Large auto-fetched trees (`gifsicle-upstream`) are gitignored; re-fetch or see
  `reference_code/REFERENCE_MANIFEST.md` (its Caesium rows are retired S18).

## Docs for reviewers / next session
0. **`STATUS.md`** — the single status register. Start here. Run
   `working_code/gifscythe/scripts/check_docs.sh` to verify it is current.
1. **`COMPILED_AUDIT.md`** — findings, evidence, §6 fix order, §19 the
   review-before-trust ask, §20 the 2026-09-16 external-review intake.
2. **`SESSION_HANDOFF.md`** — current state + constraints (header block first).
3. **`WORKLIST.md`** — the task board + the six status rules.
4. **`docs/planning/PLANNING.md`** — direction of record (offline-only, stay
   C++17/Qt6 through 1.0.0), the parked C# plan, the SkillOpt query, the
   sequential-work handoff, and the copy-paste next-session prompt.
5. **`web/WEB_PLAN_TEMPLATE.md`** — the web surface is a **product alternative**
   (self-hosted, S14): split rules, phases and the slots owner drafts are
   refitted into. Background: `web/README.md` §History.
6. **`docs/release/RELEASE_PROCEDURE.md`** — how to cut a snapshot or version
   release (gates, packaging, publishing, post-publish verification).
7. **`docs/planning/OWNER_DECISIONS.md`** — the open owner questions
   (`OD-01`…`OD-18`), each with options + a recommendation and what it unblocks.
8. **`docs/archive/AUDIT_HISTORY.md`** — the condensed index of every dated
   audit snapshot this repo has produced (full texts in git history).

## Versions
The *product* version lives in `working_code/gifscythe/VERSION.md`; `build.sh`
syncs it into the committed `src/core/version.h` fallback, while CMake
generates its own copy inside the build tree only (S11, audit U-15).
0.1.0 (now) → 1.0.0–1.9.9 (finished GIF product) → 2.0.0–3.0.0 (WebP + APNG).
**Never call it 1.0.0 until the UI/UX task is done.**

## License note
gifsicle remains a **separate subprocess** (GPL v2-only). The first-party
UI/control layer is Ms-PL (`LICENSE`, full text `COPYING.ms-pl`); the engine text
is `COPYING.gifsicle`; Qt is LGPLv3 (`COPYING.lgplv3` + companion `COPYING.gplv3`,
plus a generated `QT_NOTICE.txt` in GUI packages). Rationale + copying rules:
`docs/legal/README.md`.
