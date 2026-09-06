# Improvement Log

Chronological log of decisions and changes. **Newest at the top.**

---

## 2026-09-06 — GUI MVP and packaging follow-up

- Improved the Qt GUI with an animation queue, optimization and lossy controls,
  output selection, generated command preview, status messages, and process
  error handling.
- Added portable and system-dependent packaging scripts.
- Fixed clean engine builds by creating the versioned release directory.
- Added SettingsIO parser coverage to the Qt-independent tests.
- Added GitHub Actions CI. Linux passes with Qt6; Windows currently fails and is
  the next investigation target.
- Confirmed WebP/APNG remain deferred until the GIF UI/UX retrofit is complete.

---

## 2026-09-06
- **Created** `FEASIBILITY_REVIEW.md` — feasibility verdict for the 4 product
  asks (Caesium GUI base, XNConvert-like ease-of-use + terminal control,
  portable click-and-run, exclusive GIF/APNG/WebP like eZgif).
- **Created** the project doc set for the next session: `PROJECT_VISION.md`,
  `SESSION_HANDOFF.md`, `IMPROVEMENT_LOG.md`, `WORKLIST.md`.
- **Resolved Blocker 1 (UI base):** use Caesium's UI/UX files as the base (not a
  full copy), modeled on XNConvert's "feels & vibe". Icons/styles are
  open-source/public with no exclusive trademark (logo excluded).
- **Deferred Blocker 2 (WebP/APNG):** moved to the bucket list, to be done only
  after the pending UI/UX retrofit task.
- **Separated reference from working code:** created `reference_code/` (read-only
  source material) and `working_code/` (the product). Moved gifsicle trees +
  Caesium bundle into `reference_code/`. Auto-fetched `gifsicle-upstream`
  (kohler/gifsicle master) and `caesium-source` (Lymphatus/caesium-image-compressor
  UI) from GitHub.
- **Named the product `gifscythe`** and set the version scheme: v0.1.0 →
  1.0.0–1.9.9 (finished GIF product) → 2.0.0–3.0.0 (WebP + APNG). Created
  `working_code/gifscythe/` skeleton + `VERSION.md`.
- **Built the gifsicle engine natively** (P0): hand-wrote `config.h` in
  `reference_code/gifsicle/` (no autotools in sandbox); created
  `scripts/build_gifsicle.sh`; engine lives in `release/0.1.0/gifsicle`.
  Verified via `scripts/test_engine.sh` (info/optimize/lossy/resize/explode).
- **Built the engine control layer** (P1): `src/core/` with `GifsicleSettings`,
  `GifsicleCommand` (argv + live CLI string), `SettingsIO`; `src/cli/main.cpp`
  → `gifscythe-cli` (prints or runs the command). Unit + integration tests pass.
  Fixed optional-value gifsicle options to attached form (`--lossy=N`, `-O3`,
  `-j4`, `--loopcount=0`).
- **Constraint discovered:** cannot compile the Qt6 GUI or Windows `gifsicle.exe`
  in this sandbox — Qt not installed, apt offline, Qt mirrors fail SSL. Only
  GitHub HTTPS is reliable. These builds must happen on a machine/CI with the
  toolchains.
- **Reviewed the authored diff** (engine control layer, CLI driver, Qt GUI
  scaffold, build scripts): C++ compiles clean with `-Wall -Wextra -pedantic`;
  shell scripts pass `bash -n`. External clones (`caesium-source/`,
  `gifsicle-upstream/`) are treated as reference only and gitignored.
- **Created + merged PR** for the P0/P1 work (engine + control layer + Qt GUI
  scaffold + one-command build). Committed the authored work and repo
  reorganization to `main`.
