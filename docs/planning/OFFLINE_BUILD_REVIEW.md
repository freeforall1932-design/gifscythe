# Offline-only build review — feasibility, language choice, and plan

**Date:** 2026-09-09 · **Author:** arena agent (S5/S6) · **Product:** Gifscythe 0.1.0

This review answers three questions:

1. Is an **offline-only** Gifscythe feasible (portable, no installer, no network)?
2. Which **language/runtime** is it best built on?
3. What is the **plan**, and does the plan hold up under review?

It supersedes the "web" thread from `docs/web/WEB_FEASIBILITY.md` for the
**product** direction: the web server build is a demo, not the offline path.
*(Superseded 2026-09-12, S14: the owner made the web server build a supported, self-hosted
product alternative to the desktop/portable build — the desktop/offline path above is unchanged.
See `web/WEB_PLAN_TEMPLATE.md` §1.)*

---

## 1. Verdict at a glance

| Question | Answer |
|---|---|
| Offline-only feasible? | **Yes — already true today.** The desktop app shells out to a local `gifsicle` binary, stores nothing in the cloud, and packages to a single portable folder. Offline-only *removes* constraints (no server, no auto-update) rather than adding them. |
| Best language to build on | **Stay with C++17 + Qt6 Widgets for 1.0.0.** It is already built, verified (143-check offscreen harness, Windows CI green), and natively plays animated GIF previews. If a future rewrite is wanted for a smaller/more-modern stack, **Rust + Tauri** is the best alternative — at the cost of bundling a WebView2 runtime for truly portable offline Windows. |
| Plan | Finish 1.0.0 on the current stack; treat any language migration as a *separate, later* decision with an explicit rewrite-cost budget. |

---

## 2. What "offline-only" means for this codebase

Hard constraints from `PROJECT_VISION.md` + `WORKLIST.md`:

- Animated images only (GIF now, APNG + WebP later).
- **Portable, click-and-run, no installer, no admin.** Windows-first.
- gifsicle stays a **separate subprocess** (GPL v2-only engine vs GPLv3 UI).
- Retain gifsicle terminal-level control (live one-way command pane).

Implications of "offline-only":

| Implication | Effect |
|---|---|
| No server, no cloud | The `web/` server-side build (`docs/web/WEB_FEASIBILITY.md` Option 3) was **not** the product path; the S14 owner decision makes it a supported, **self-hosted** product alternative (still no cloud service — the offline-only promise stands). See `web/WEB_PLAN_TEMPLATE.md` §1. |
| No auto-update | The Phase-3 "auto-update flow" bucket item drops out; releases are downloaded manually (already the model — portable folder + GitHub artifacts/Release). |
| Everything local | Engine, settings, previews, temp files are all local (already true). One gap: **the GUI does not yet persist settings between sessions** (see §6). |
| Preview must not phone home | `QMovie` plays the local file — no network. ✅ |

---

## 3. Language/runtime comparison (scored for *this* project)

Criteria, weighted for a portable offline GIF tool that shells out to a C engine:

| Criterion (weight) | C++17 + Qt6 Widgets | Rust + Tauri | Go + Wails | Electron | Flutter | Python + PySide6 |
|---|---|---|---|---|---|---|
| Already implemented (×3) | **10** (done, CI green) | 2 (rewrite) | 2 (rewrite) | 3 (reuses web POC JS) | 1 (rewrite) | 4 (rewrite, slower) |
| Portable/no-installer Windows (×3) | **9** (windeployqt folder) | 7 (needs WebView2 runtime) | 7 (needs WebView2 runtime) | **10** (bundles Chromium) | 8 | 5 (PyInstaller fragile) |
| Animated-GIF before/after preview (×2) | **9** (QMovie, native) | 6 (browser `<img>`, fine) | 6 (WebView2) | **9** (browser) | 5 (plugin needed) | 6 (QMovie via Qt) |
| Subprocess engine + GPL boundary (×3) | **10** (QProcess argv) | 9 (`std::process`) | 9 (`os/exec`) | 8 (`child_process`) | 7 | 8 |
| Bundle size (×2) | 6 (~30–40 MB) | **9** (~10 MB + WebView2) | **9** | 3 (~150 MB+) | 8 | 4 |
| Offline (no network at runtime) (×3) | **10** | **10** | **10** | **10** | **10** | **10** |
| Dev velocity for UI polish (×2) | 6 | 8 (web UI, reuse `command.mjs`) | 7 | 8 | 6 | 8 |
| Native Windows feel / tooling (×2) | 8 | 7 | 7 | 6 | 6 | 5 |
| Long-term multi-format (APNG/WebP) fit (×2) | 8 (link libs as subprocess helpers) | 8 | 8 | 7 | 6 | 7 |
| **Weighted total** | **~258** | ~205 | ~199 | ~174 | ~153 | ~161 |

Notes on the table:

- **C++/Qt wins on reuse** — the control layer (`src/core/`), the GUI, the
  143-check harness, both build systems (cmake + qmake), packaging, and CI are
  done. Any other language throws that away and re-earns it.
- **Tauri/Go+Wails are the credible "smaller app" alternatives**, but on
  Windows they ride on **WebView2**. Win10/11 ship it via Edge, but a
  *portable, offline, no-installer* promise needs the WebView2 **fixed-version
  runtime** bundled (~120 MB+), which largely erases their size advantage.
- **Electron** is the only web-UI option that is *guaranteed* portable with no
  system dependency (it ships its own Chromium), at ~150 MB.
- **Flutter/Python** add a new runtime and toolchain for no clear win here.

### Why the engine subprocess decides a lot

gifsicle is GPL **v2-only**; the UI may be GPLv3. Keeping gifsicle as a
separate process (the current design) is what keeps the license boundary
clean. Any language must therefore be good at **spawning a process with an
argv array and capturing exit codes** — C++/Qt (`QProcess`), Rust, Go, Node,
Python all do this trivially. It does **not** need FFI to C, which is why
"rewrite in Rust" buys less here than it would for a library-heavy app.

---

## 4. Decision

**Keep C++17 + Qt6 Widgets through 1.0.0.** Rationale:

1. It is the only stack that is *already* offline-only, portable, and
   CI-verified end-to-end (linux + windows, offscreen GUI harness).
2. The dominant cost of a rewrite is discarding verified behavior, not writing
   new code. The remaining 1.0.0 work is small (clean-Windows smoke, desktop
   probes, optional polish) — not a reason to switch languages.
3. QMovie gives native animated-GIF previews (before/after) for free; every
   alternative must hand-roll or rely on browser rendering.
4. The GPL subprocess boundary is already implemented and correct.

**Revisit the language only if** one of these becomes a real requirement:

- **Target bundle size** must drop under ~15 MB → prototype **Rust + Tauri**
  (accept the WebView2 fixed-runtime bundling, or verify Evergreen presence on
  the target machines).
- **UI must be web-tech** (so a browser UI is also the desktop UI) →
  **Tauri** (small) or **Electron** (most portable, heaviest).
- **Must also run in a browser** → compile gifsicle to WASM and reuse
  `web/command.mjs` + the web UI client-side (see the web review); this is an
  *additive* build, not a language change of the desktop app.

---

## 5. Plan (offline-first roadmap)

**Phase 0 — current (0.1.0).** Engine subprocess, control layer, CLI, Qt GUI
(tabs + ~30 controls + async preview), portable + system packaging, CI
linux+windows green. *(Done.)*

**Phase 1 — finish 1.0.0 on the current stack (unchanged, re-confirmed).**
1. Clean-Windows smoke from the CI artifact (`docs/ci/CLEAN_WINDOWS_SMOKE.md`,
   gates C4/D3/D4).
2. One-time desktop probes: B5 (kill engine mid-run), B6 (physical drag-drop),
   B14 (engine-missing GUI variant).
3. Optional polish before freeze: free-form `{name}` naming templates, queue
   reorder, release-procedure doc.
4. **Add: persist GUI settings between sessions** (see §6 gap) — a natural
   offline-app expectation, cheap via the existing `SettingsIO`.
5. Owner version decision (0.2.0 vs 1.0.0). No WebP/APNG before this ships.

**Phase 2 — multi-format (APNG + WebP), still offline.** Common RGBA frame
model; APNG via libpng/zlib, WebP via libwebp — as **separate subprocess
helpers** (keeps the license boundary) or in-process for the *new* codecs
only. Unchanged from the deferred bucket list.

**Phase 3 — only-if-needed language migration (separate decision).** If §4's
triggers fire, spike **Tauri** with the existing `command.mjs` + a Rust sidecar
that spawns gifsicle; port the harness checks one-for-one before committing to
the migration. Do **not** fold this into 1.0.0.

---

## 6. Gaps found while writing this review (to carry into the next session)

- **GUI settings are not persisted.** `SettingsIO.h` round-trips settings to
  disk, and the CLI reads a conf file, but the GUI neither loads nor saves
  `Settings` between sessions. For an offline desktop app this is the most
  visible missing "remember me" feature. → add to Phase 1 (item 4).
- **No icon/logo** yet (`assets/` is empty) — cosmetic, pre-1.0.0.
- **The `web/` server POC was scoped as a demo**, not the product path *(superseded 2026-09-12,
  S14: the owner made the web build a supported, self-hosted product surface — see
  `web/WEB_PLAN_TEMPLATE.md` §1)*; it stays as the browser UI and command-layer parity harness.
- The two dated review snapshots (now `docs/archive/gifscythe-comprehensive-review.md`
  and `docs/archive/gifscythe-final-code-review.md`) still reference
  `scripts/build_gifsicle.sh`;
  they are historical and left as-is by policy.

---

## 7. Self-review of this plan

- **Assumption checked:** "offline-only adds nothing" — true for runtime, but
  it *does* remove auto-update and the server-side web option, and it raises
  the settings-persistence expectation. Captured in §2/§6.
- **Risk underweighted?** The biggest real risk remains the **clean-Windows
  windeployqt smoke** (a missing DLL would break the "portable" promise). It is
  already the top Phase-1 gate.
- **Language table honesty:** weighted scores are directional, not precise;
  the decisive factor is the reuse column, which is objective (the code and CI
  exist). If the owner weighs "future UI velocity" far above "already done",
  the table should be re-weighted toward Tauri.
- **Missing decision the owner must make:** none blocking — the language
  question is answered (stay C++/Qt), and the migration trigger is explicit.
- **Not covered (out of scope):** online sync/cloud, auto-update, telemetry —
  all intentionally excluded by "offline-only".

**Conclusion:** feasible and already largely true; **keep C++17/Qt6**; finish
1.0.0; treat Tauri as an explicit, separately-funded option only if a trigger
fires.
