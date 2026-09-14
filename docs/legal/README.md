# Legal — licence single source of truth

**Current state (S19, 2026-09-14):** first-party code is **Ms-PL**
(`COPYING.ms-pl`); the gifsicle engine is a **GPL v2-only** subprocess
(`COPYING.gifsicle`); Qt is **LGPL v3** (`COPYING.lgplv3` + companion
`COPYING.gplv3`, `QT_NOTICE.txt` in GUI packages). The UI carries no GPL
code and no Caesium content. The experimental `web/wasm/` track has its
own open licence question (`WASM_LICENSE_QUESTION.md`, `OD-16`) and is
not shippable until it is answered.

Rule: **licence rationale lives in this folder.** Everywhere else only *points
here* (one line) or *lists files* (packagers, manifests). If any doc outside
this folder disagrees with it, this folder + `LICENSE` win — fix the other
doc, not this one. Dated history (old log entries, dated reviews, audit
evidence) keeps its original wording as evidence; it is superseded, not
edited.

Files:

- `WHY_MSPL.md` — why Ms-PL was chosen (`OD-09 = b`, `OD-C6`): context,
  rejected alternatives, accepted costs, the one-way-door warning, what
  reopens it.
- `COPYING_RULES.md` — operational checklist for copying ScreenToGif fork
  files compliantly (notices, log entries, tripwires).
- `WASM_LICENSE_QUESTION.md` — the open Ms-PL/GPLv2 in-process question
  for the experimental `web/wasm/` track, decided by `OD-16`.

Maintenance contract — to change the licence again, touch exactly two sets:

1. **This folder** — rewrite the rationale + rules (the *why* and *how*).
2. **The mechanical set** (file names + grant text, no prose reasons):
   `LICENSE`, `COPYING.*`, `working_code/gifscythe/scripts/package_common.sh`
   (required-file lines), `working_code/gifscythe/scripts/test_package.sh`
   (fixtures), `.github/workflows/build.yml` + `docs/ci/build.yml.proposed`
   (manifest list), `STATUS.md` rows `W-08`/`U-08`/`GS-204` (via the normal
   audit flow), `docs/release/RELEASE_PROCEDURE.md` (file lists).

Everything else points here and needs no edit.
