# Worklist

Task board for the GIF/APNG/WebP animation tool. See `PROJECT_VISION.md` for the
vision, `FEASIBILITY_REVIEW.md` for the full analysis and **recommended path
(P0–P3)**, and `README.md` for the **reference vs. working code** separation.

## Repo separation (done this session)
- `reference_code/` = source material (gifsicle, caesium-source, caesium-bin) —
  **read-only, never ship as-is.**
- `working_code/gifscythe/` = the product (**v0.1.0** skeleton).
- Versioning: 0.1.0 → 1.0.0–1.9.9 (finished GIF product) → 2.0.0–3.0.0
  (WebP + APNG). See `working_code/gifscythe/VERSION.md`. **Never call it 1.0.0
  until the UI/UX task is done.**

## Current status (aligned to the recommended path)
- [ ] **Pending** — **P0 + P1** (project setup + GIF GUI MVP = the UI/UX retrofit)
- [ ] **Bucket list** — **P2 + P3** (WebP/APNG codec + converter, then eZgif
      richness); only after the pending retrofit is done
- Reference separation done; product skeleton at v0.1.0.

---

## Pending (do these next)

### P0 — Project setup
- [x] Separate `reference_code/` from `working_code/`; store canonical gifsicle
      + Caesium source/bin as read-only reference.
- [x] Fetch gifsicle upstream + Caesium source from GitHub automatically.
- [x] Create `working_code/gifscythe` skeleton at **v0.1.0**.
- [x] Canonical gifsicle tree confirmed = upstream master (`reference_code/gifsicle`).
- [x] Build a working gifsicle engine (**native** Linux) via
      `scripts/build_gifsicle.sh`, placed in `release/0.1.0/`. Wrote `config.h`
      by hand (no autotools in sandbox).
- [ ] **Windows** cross-compile `gifsicle.exe` into `release/0.1.0/` — blocked
      locally: no mingw and apt is offline. Do on a machine/CI with mingw-w64
      (`scripts/build_gifsicle.sh --windows`).

### P1 — GIF GUI MVP (proves asks 1/2/3)
> **OWNER DECISION:** use Caesium's UI/UX files as the base — **NOT** a full
> copy. Model the "feels & vibe" on XNConvert. Icons/styles are open/public
> (no trademark; logo excluded). Caesium source lives in
> `reference_code/caesium-source/`.

- [x] **Engine control layer** (Qt-independent, `src/core/`): `GifsicleSettings`
      struct = all UI controls; `GifsicleCommand` builds the exact argv + live
      CLI string; `SettingsIO` load/save as flat `key = value`.
- [x] **Live raw-CLI pane** feature: `gifscythe-cli` prints the exact command it
      will run (the "show me the command" pane). Verified.
- [x] **Emit + run** pipeline: settings → command → run against engine →
      valid GIF output. Verified with unit tests + integration test.
- [x] Command builder fixed so optional-value gifsicle options use attached
      form (`--lossy=N`, `-O3`, `-j4`, `--loopcount=0`).
- [x] **One-command build** `build.sh`: builds engine + CLI and runs tests;
      add `--all`/`--gui` to build the Qt GUI (skips gracefully if Qt absent).
- [x] **Scaffolded Qt6 GUI shell** at `src/qtui/` (`MainWindow` + `main.cpp` +
      `gifscythe.pro`), wired to the core layer (file picker, live command pane,
      run via QProcess).
- [ ] Build/verify the **Qt6 GUI shell** — **BLOCKED locally**: Qt not installed,
      apt offline, Qt mirrors fail SSL handshake. Needs a machine/CI with Qt6
      (`./build.sh --all` or `qmake6 gifscythe.pro && make`).
- [ ] Model the layout/feel on XNConvert (ease of use).
- [ ] Ship portable (`release/<version>/`): gifscythe app + Qt6 runtime +
      `gifsicle.exe`, no installer.
- [ ] Finish UI/UX task, then bump **1.0.0** (finished GIF product).

**Build constraint (sandbox):** only GitHub HTTPS reliable; apt is offline.
`gcc`/`make` + hand-written `config.h` build the engine natively. Qt GUI &
Windows `gifsicle.exe` must be built where the toolchains are installed.

---

## Bucket list / future (deferred by owner decision)
> WebP + APNG are "moving picture" formats. **Owner decision:** do them **only
> after** the pending UI/UX retrofit (P0 + P1) is complete. Bump to **2.0.0**
> when these land.

### P2 — Convert branch (unlocks ask 4)
- [ ] Codec layer: **libwebp** (`WebPAnimDecoder` / `WebPAnimEncoder`) for
      animated WebP; **libpng (APNG-capable, or the `libpng-apng` patch) + zlib**
      for APNG.
- [ ] Introduce a common RGBA frame model (delay, disposal, blend, loop,
      transparency) so every edit works across GIF/APNG/WebP.
- [ ] GIF ⇄ APNG ⇄ WebP conversion, explode, merge, reorder, loop, transparency.

### P3 — eZgif richness (polish)
- [ ] Frame editor.
- [ ] Overlay text / watermark.
- [ ] Presets, "show me the command" toggle.
- [ ] Optional: keyboard-first power users, auto-update.

---

## Done
- [x] Feasibility review (`FEASIBILITY_REVIEW.md`).
- [x] Doc set (`PROJECT_VISION`, `SESSION_HANDOFF`, `IMPROVEMENT_LOG`, `WORKLIST`).
- [x] Owner decisions captured (Caesium UI base; WebP/APNG deferred).
- [x] Reference/working separation + versioning scheme (v0.1.0).
- [x] P0 engine + P1 control layer + Qt GUI scaffold + one-command build —
      merged to main via PR.
- [x] External reference clones (`caesium-source/`, `gifsicle-upstream/`) are
      gitignored; provenance in `REFERENCE_MANIFEST.md`.

## Note on reference clones in git
`reference_code/caesium-source/` and `reference_code/gifsicle-upstream/` are
**not committed** (gitignored) — they are externally-fetched reference material,
re-fetchable from GitHub. Commit SHAs are in `reference_code/REFERENCE_MANIFEST.md`.
The committed reference tree is `reference_code/gifsicle/` (== upstream master).
