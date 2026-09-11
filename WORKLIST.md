# Worklist

**Version:** 0.1.0 · **Status register:** `STATUS.md` · **Audit detail:** `COMPILED_AUDIT.md`

> ## ⚑ Start with `STATUS.md`, not this board
> **`STATUS.md`** is the single status register: one row per tracked item in
> exactly one of four states — **DONE / PARTIAL / OPEN / UNTRIAGED** — with a
> generated header that says how much of everything the repo knows about is
> done. It answers *"what is left?"* without reading a 52-row register.
>
> This file is the **human task board**: what to pick up next, in order. It is
> not the status source of truth any more, and it must not contradict
> `STATUS.md` (`check_docs.sh` gate **G2** fails if a ticked box here maps to a
> non-DONE row there).
>
> Per-finding evidence stays in **`COMPILED_AUDIT.md`** §5 and, for the S8
> remediation, **`docs/audit/REMEDIATION_2026-09-10.md`**.

## The rules that keep the docs true (NEW in S9 — do not skip these)

These live here, in `SESSION_HANDOFF.md` and in
`docs/release/RELEASE_PROCEDURE.md` because rules only stick if a new session
reads them in a file, not in a conversation.

1. **Every session ends by updating the docs** — `STATUS.md`,
   `SESSION_HANDOFF.md`, `WORKLIST.md`, `IMPROVEMENT_LOG.md` — **then runs
   `check_docs.sh` until green.** Stale docs are corrupted input for the next
   session, not a cosmetic problem.
2. **A new finding is recorded in the same session it is found:** `UNTRIAGED` in
   `STATUS.md` **and** a pending `- [ ]` line in the "Found this session" list
   below. It may not wait for a later audit pass. Gate **G12** fails if an
   `UNTRIAGED` row outlives the session that found it.
3. **Before `gh pr create`, and again before `gh pr merge`:** run
   `working_code/gifscythe/scripts/check_docs.sh`, fix every failure, re-run
   until green. Do not create or merge with a failing doc check, and **do not
   ask whether to run it**.
4. **It is also enforced mechanically.** `.githooks/pre-push` blocks a red push.
   Git does not copy hooks on clone, so run
   `working_code/gifscythe/scripts/bootstrap_hooks.sh` once per clone —
   `build.sh` does it for you. Verify: `git config core.hooksPath` → `.githooks`.
5. **`IMPROVEMENT_LOG.md` entries use the template** (`Changed / Partial / Left /
   Verified / Not verifiable here / Docs touched`). The **`Not verifiable here`**
   line is mandatory and must never be omitted or softened.

## Found this session — pending lines (rule 2)

Every `UNTRIAGED` row in `STATUS.md` must have a matching line here.

- [ ] **N-03** — `docs/screenshots/*.png` (3 shots) claim to show the S7 UI; S8
      changed `src/qtui/` afterwards, nothing links to them, and this sandbox has
      no Qt6 to regenerate them. Decide: re-shoot on a Qt machine and link them
      from `README.md`, or mark them historical. **UNTRIAGED — scope it.**

## Direction decisions (2026-09-09 — see `docs/planning/OFFLINE_BUILD_REVIEW.md`)

- **Offline-only.** No server, no auto-update, no telemetry. The `web/` build
  is a demo / command-parity harness, not the product path.
- **Language: stay C++17 + Qt6 Widgets through 1.0.0** (already offline,
  portable, CI-verified). Revisit only if a documented trigger fires — then
  spike **Rust + Tauri**. Comparison matrix in the offline review doc.

## Current status

### Foundation (done)
- [x] P0 project setup and reference/working separation
- [x] GIF engine build and verification (`release/<ver>/gifsicle`, identity 1.96)
- [x] Qt-independent command/settings control layer
- [x] CLI driver + unit tests + integration smoke (`smoke_cli.sh`)
- [x] Qt6 GUI MVP scaffold (Batch default, mode combo, async run, queue, DnD)
- [x] Portable and system-dependent packaging scripts (`.exe` probe + licenses)
- [x] Linux GitHub Actions path with Qt6 + artifacts (recipe present)
- [x] Root LICENSE / COPYING.gifsicle / `.gitignore` / `.gitattributes`

### Review remediation (implemented 2026-09-07 — re-verify via §6)
- [x] **P0 silent-failure fixes** — CLI exit codes, argv exec, CMake INTERFACE,
      GUI batch default, output validation, SettingsIO safety, Windows `win32cfg.h`
- [x] **P1 honesty work** — single version source, EngineLocator, queue UX,
      live pane sync, `save_settings`/`validate`, packaging, `shell_quote`,
      Settings-by-value
- [x] Smoke suite + engine tests scripts; CI recipe proposed in `docs/ci/build.yml.proposed`
- [x] Apply `docs/ci/build.yml.proposed` → `.github/workflows/build.yml` — applied
      by maintainer in `821a310`; S4 hardened both copies (static-link CLI/tests,
      Ninja generator, native Windows E2E smoke, GUI offscreen steps).
- [x] Merged compiled audit (S1+S2+S3) → `COMPILED_AUDIT.md`

### Gates before more features
- [x] **Verify fixes** (`COMPILED_AUDIT.md` §6) — one-command rerun:
      `working_code/gifscythe/scripts/verify_audit.sh`, currently
      **25 PASS / 0 FAIL / 5 SKIP, exit 0** (skips: cmake, Qt6, E9
      declared-pending workflow, CI-gated, clean-Windows)
- [x] **Documentation gate** (NEW S9) — `scripts/check_docs.sh` emits and
      enforces `STATUS.md`; wired into `verify_audit.sh` as **F1/F2**, into CI
      (pending — see `docs/ci/PENDING_WORKFLOW_CHANGE.md`) and into
      `.githooks/pre-push`
- [x] Windows engine + CLI **proven under Wine**: `gifsicle.exe` runs
      (`1.96 (Windows)`), CLI E2E with `C:\` paths + spaces + honest exit 1;
      found & fixed two Windows-only bugs (engine `-I.` recipe, `_spawnvp`
      space-splitting → CreateProcessA + quoting; static-linked exes)
- [x] **Valid push token** — token #3 worked (2026-09-07); PR #5 merged
      into `main` (merge `0ad1ff5`)
- [x] Windows GitHub Actions job **green** with downloadable artifact
      (main run #23: windows ✅ + linux ✅; `gifscythe-windows` 27.8 MB)
- [ ] Clean-machine portable smoke (esp. Windows + `windeployqt`) — from CI
      artifact after C2 (C4/D3/D4). **Blocked by U-09**
- [ ] One-time real-desktop GUI probes: B5 (kill engine mid-run), B6 physical
      drag-drop, B14 engine-missing GUI variant

### Audit remediation S8 (2026-09-10) — 31 of 52 register findings closed
> Evidence per finding: **`docs/audit/REMEDIATION_2026-09-10.md`**.
> Roll-up: **`STATUS.md`**. Tally: 31 fixed outright (21 batch 1 + 10 batch 2) ·
> 3 partial · 15 open · 1 fixed earlier by S7 · 2 register rows corrected = **52**.

- [x] Both release blockers: **U-01** (`src/core/OutputPlan.h`) · **U-02**
      (fail-closed packager + `scripts/test_package.sh`)
- [x] Engine-truth guards: **U-03** threads `-j` · **U-22** resize geometry ·
      **U-11** strict booleans · **U-33** half-specified `-p`
- [x] CLI honesty: **U-04** stdout purity · **U-05** PATH engine search ·
      **U-23** strict arg parser
- [x] Settings serializer: **U-51** `encode_line_value()` (9 sites + JS mirror)
- [x] Process layer: **U-32** `128+WTERMSIG` · **U-31** `-lstdc++fs` probe
- [x] Output names: **U-21** `src/core/OutputName.h` (`NameRules` Host/Win/Posix)
- [x] Web demo: **U-06** loopback default · **U-24** honest rc=0 · **U-25/U-29**
      Scale default + Touch · **U-26** version sort · **U-30** validation layer ·
      **U-49/U-50** transport · **U-46/U-52** request ownership + URL revoke
- [x] Process hygiene: **U-38** SKIP not FAIL · **U-39** workflow-drift guard
      **E9** · **U-44** `docs/archive/` · **U-43** · **U-48** empty comment
- [x] Three web suites now gate the JS copies — **W1** command (14) ·
      **W2** validation (19) · **W3** transport (17)
- [x] **CI confirms `src/qtui/` and harness T8/T17** — PR #11 run `34471563229`:
      **linux pass 1m14s, windows pass 2m56s**
- [ ] **U-40** — CLI prints `validate()` warnings and runs anyway while the GUI
      refuses. Intentional, but undocumented at the point of use (P3-5)
- [ ] Qt-only findings still need a Qt machine to *change*: U-12, U-15, U-16,
      U-17, U-34, U-35, U-36, U-37, U-45, U-47.
- [ ] Windows-only: **U-07** ANSI process APIs (needs a real Windows run).
- [ ] **U-09** re-cut the release from *this* SHA.

### Session S9 (2026-09-10) — status-tracking system
- [x] **N-01** — the pending-workflow marker was left behind after the maintainer
      applied that change in `190d030`; every doc still quoted **23/0/5** while
      the real gate run was **24/0/4**. Marker rewritten to describe the *new*
      pending change; gate **G6** now measures the real number and compares.
- [x] **N-02** — `web/README.md` documented the pre-U-06 bind address
      (`0.0.0.0`); now states `127.0.0.1` + `GS_WEB_HOST`.
- [ ] **N-03** — see "Found this session" above. **UNTRIAGED.**

### GIF UI/UX → 1.0.0  (S4b retrofit 2026-09-07 + S7 polish 2026-09-10 — harness T1–T16 last *measured* at 243 runtime checks in the S7 sandbox; S8 added T17 + rewrote T8, **both CI-green on PR #11**)
- [x] Input / Actions / Output tab flow (XNConvert feel) — QTabWidget + Preview
      pane in splitter; bottom live pane/progress/status bar kept
- [x] Before/after preview (debounced 1200 ms, fully async, seq-guarded;
      honest captions; savings readout; refuses in Explode mode)
- [x] File size/count display (row sizes + total label), output-folder actions
      (batch folder + Open folder via QDesktopServices)
- [x] Free-form naming templates — **S7**: Output-tab `Name template`, default
      `{name}_opt.gif` renders exactly the historical auto-name (E4 unchanged);
      separators stripped, `.gif` appended, constant-template multi-file
      collision REFUSED with dialog (S3-25, harness T16)
- [x] Expose remaining `GifsicleSettings` controls (~30 widgets, engine-truth
      value lists; harness T11 asserts each control → exact flag)
- [x] Queue reorder (move up/down) — **S7**: list+model stay index-aligned,
      selection follows, merge order = queue order (S3-9, harness T15)
- [ ] Optional: two-way CLI pane (`gs::parse_args`) **or** keep one-way forever
      (UI label now says one-way explicitly) — owner decision
- [x] **Persist GUI settings between sessions** — **S7**: SettingsIO-backed
      load/save (ctor/closeEvent) at `AppConfigLocation/gifscythe.conf`,
      override `GS_SETTINGS_PATH`; GUI keys `batch_dir`/`name_template`;
      queue + Save-as deliberately NOT persisted; warnings surfaced
      (S6 gap / audit row 15, harness T14)
- [x] Document release procedure — **S7**: `docs/release/RELEASE_PROCEDURE.md`
- [ ] Bump `VERSION.md` → **1.0.0** only after Windows CI green + desktop
      probes + the optional items above are decided (owner may take 0.2.0
      first per the minor-bump rule)

## Next actions (ordered)

1. **Scope N-03** (the screenshot question) — it is the only `UNTRIAGED` row and
   rule 2 says it may not survive the session boundary.
2. **U-09** — re-cut release artifacts from the tagged SHA (the banked zip
   predates S7; its notes pin `d3544b1`). Needs a tag + `gh release`.
3. **Apply the pending CI change** — needs a token with the `workflows` scope.
   Steps + the follow-up doc-number refresh are in
   `docs/ci/PENDING_WORKFLOW_CHANGE.md`.
4. Clean-VM `windeployqt` smoke from the `gifscythe-windows` artifact (C4/D3/D4)
   — checklist: `docs/ci/CLEAN_WINDOWS_SMOKE.md`. **Blocked by U-09.**
5. One-time real-desktop GUI probes: B5 (kill engine mid-run), B6 physical
   drag-drop, B14 engine-missing GUI variant.
6. Owner decisions: two-way CLI pane **or** keep one-way forever; version
   (0.2.0 for the S7 feature set per the minor-bump rule, vs straight 1.0.0
   once 4+5 are green). Release how-to: `docs/release/RELEASE_PROCEDURE.md`.
   Audit release criterion: *no Critical/High open, package-negative tests green,
   clean-Windows smoke against the exact tagged SHA.*
7. The 15 `OPEN` audit findings — see `STATUS.md`; all except U-09 are Qt-only or
   Windows-only paths this sandbox cannot compile.
8. WebP/APNG stay blocked until all of the above ships.

## Deferred bucket list — after GIF `1.0.0`

Tracked as **D-01…D-08** in `STATUS.md`. All `OPEN`: known, scoped, not started,
and deliberately not started until GIF 1.0.0 ships.

- Common RGBA animation frame model (timing, disposal, blend, alpha, canvas, loop).
- Animated WebP (libwebp AnimDecoder/AnimEncoder).
- APNG (libpng/zlib with APNG support).
- GIF ⇄ APNG ⇄ WebP convert, explode, merge, reorder, loop controls.
- Frame editor, text/watermark overlays, presets, richer previews.
- Optional: logging framework, i18n, dark mode, system tray (see COMPILED_AUDIT S3 P2/P3).
- **Web (not the product path):** client-side `gifsicle.wasm` + web UI
  (`docs/web/WEB_FEASIBILITY.md` Option 4); `web/` server demo already exists.
- **Language migration (only if a trigger fires):** Rust + Tauri spike —
  see `docs/planning/OFFLINE_BUILD_REVIEW.md` §4.

## Build commands

```bash
cd working_code/gifscythe
./build.sh                 # engine + CLI + unit tests (also bootstraps git hooks)
./scripts/test_engine.sh
./scripts/smoke_cli.sh
./scripts/test_package.sh  # packaging negative suite
./scripts/check_docs.sh    # documentation gate — must be green before any PR
./scripts/check_docs.sh --emit   # regenerate STATUS.md from the repo
./scripts/verify_audit.sh  # whole COMPILED_AUDIT §6 suite + the doc gate (F1/F2)
./scripts/package_portable.sh
./scripts/package_system.sh
./scripts/build_engine.sh --windows   # needs mingw-w64
./scripts/bootstrap_hooks.sh          # make .githooks/pre-push live in this clone
# GUI harness (needs Qt6): cmake -S . -B build-cmake && cmake --build build-cmake
#   && QT_QPA_PLATFORM=offscreen ./build-cmake/test_gui_offscreen

# Web demo (offline-unrelated; parity harness only)
node web/server.mjs 8000           # from the repo root; binds 127.0.0.1
node web/test/command.test.mjs     # JS ⇄ C++ command parity (14 fixtures)
node web/test/validate.test.mjs    # JS ⇄ C++ validation parity (19 fixtures)
node web/test/transport.test.mjs   # live-server transport net (17 cases)
```

## Do not

- Bump to 1.0.0 as a placeholder.
- Add WebP/APNG before GIF UI is stable.
- “Fix” `--loopcount=0`, `-O0`, crop plus-form, or gamma sentinel (verified correct).
- Link gifsicle into the GUI binary (keep subprocess for GPL v2-only vs GPLv3).
- Edit `reference_code/` (read-only).
- Hand-edit the generated block in `STATUS.md` — run `check_docs.sh --emit`.
- Push, open or merge a PR with a red `check_docs.sh`, or bypass the pre-push
  hook with `--no-verify`.
- Reintroduce the removed `scripts/build_gifsicle.sh` shim.
