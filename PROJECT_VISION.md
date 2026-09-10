# Project Vision

**Product name:** Gifscythe — a GIF / APNG / WebP animation tool  
**Version:** 0.1.0 (in development). See `working_code/gifscythe/VERSION.md`.  
**Status:** Feasibility decided (`FEASIBILITY_REVIEW.md`). Engine, control layer,
CLI, and GUI implemented; P0/P1 honesty fixes landed and were verified with
evidence (audit §6); the XNConvert-style UI retrofit (tabs/controls/preview)
landed 2026-09-07 (S4b). The portable Windows build is **CI-green and merged**
(PR #5 → `0ad1ff5`; main runs #23/#24 green on both jobs). **2026-09-09
(S5/S6):** direction pinned to **offline-only** and the language **stays
C++17/Qt6 through 1.0.0**; the `web/` build is a demo only. **2026-09-10
(S7):** the remaining sandbox-codeable Phase-1 items landed — **GUI settings
persistence** (remember-me between sessions), **queue reorder**, **naming
templates** (`{name}_opt.gif` default), and the **release-procedure doc**;
harness reached 243 checks there. **2026-09-10 (S8):** the audit register was
worked down — **31 findings closed with executed proof** (both release blockers
included) and the one-command gate is green at **23 PASS / 0 FAIL / 5 SKIP,
exit 0** (E9 SKIPs while the CI workflow change awaits a `workflows`-scoped
token; 24/0/4 once applied).
Note the S8 sandbox had no cmake/Qt6, so 243 is the last *measured* harness
figure and the GUI edits since then are CI-verified only. Remaining: the
clean-VM / desktop smoke (C4/D3/D4,
B5/B6/B14 — `docs/ci/CLEAN_WINDOWS_SMOKE.md`) plus the owner decisions
(two-way CLI, version) remain before **1.0.0**. See `WORKLIST.md`,
`COMPILED_AUDIT.md`, and `docs/planning/OFFLINE_BUILD_REVIEW.md`.

## Mission
A portable, click-and-use desktop app for **animated (moving) images only** —
**GIF, APNG, and WebP**. It should let a *normal person* edit, optimize, and
convert animated images without ever seeing "a pile of code scrap."

## Audience
Casual / non-technical users who just want it to work, **plus** power users who
want the full gifsicle terminal control underneath.

## UX references
- **"Feels & vibe" of XNConvert** for the overall layout and ease of use.
- **Feature-richness of eZgif** for GIF-style options + a "Convert / other"
  branch (convert between formats, explode, merge, reorder frames, and so on).

## UI approach (owner decision)
- Use **Caesium's UI/UX files as the base** — **not** a full copy/paste.
- Reuse their **open-source / public** icon and style assets; Caesium has no
  exclusive trademark on those (the XNConvert-style modeling applies, **logo
  excluded**).
- Model the layout/feel on **XNConvert** for ease of use.

## Hard scope constraints
- **Exclusively** animated GIF, APNG, and WebP. Not photos, not video —
  only "moving picture" / GIF-type features.
- Portable, click-and-run (no installer, no admin). Windows-first.
- **Offline-only** (2026-09-09): no server, no auto-update, no telemetry.
- Retain gifsicle terminal-level control for power users (live command pane;
  argv execution — never shell injection).
- gifsicle stays a **subprocess** (GPL v2-only engine vs GPLv3 UI).

## Format support plan
- **GIF:** gifsicle (existing, native) — current focus through 1.0.0.
- **APNG + animated WebP:** **bucket-list / future task** — only after the
  GIF UI/UX retrofit is complete and 1.0.0 ships. See `WORKLIST.md`.

## Progress snapshot (2026-09-10)
| Area | State |
|------|--------|
| GIF engine subprocess | Done (native + Windows config path) |
| Settings → argv control layer | Done |
| CLI driver + tests | Done (honest exits, smoke suite) |
| GUI | **Retrofit done (S4b) + polish done (S7):** Input/Actions/Output tabs, ~30 engine-truth controls, debounced async before/after preview, batch output folder, **settings persistence between sessions, queue reorder, `{name}` naming templates** — 243-check offscreen harness green (T1–T16) *in the S7 sandbox*; S8 added **T17** and rewrote **T8**, and those are **CI-verified only** |
| Audit remediation | **S8 (2026-09-10):** 31 of the 52 registered findings closed with executed proof — see `docs/audit/REMEDIATION_2026-09-10.md` |
| Silent-failure class bugs | Fixed in code; re-verify via `COMPILED_AUDIT.md` §6 |
| Offline-only direction + language decision | **Decided (S6)** — stay C++17/Qt6 through 1.0.0; see `docs/planning/OFFLINE_BUILD_REVIEW.md` |
| Settings persistence (offline "remember me") | **Done (S7)** — SettingsIO-backed; `docs/release/RELEASE_PROCEDURE.md` documents the release flow |
| Web build | **Demo only** (`web/`, server-side POC + JS⇄C++ command parity); not the product path |
| 1.0.0 | **Not yet** — clean-VM smoke + desktop probes + owner decisions (two-way CLI, version) remain |
