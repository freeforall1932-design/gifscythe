# Legal — the one place licensing lives

**Status of this project: UNLICENSED (2026-10-26, owner decision).**

Gifscythe is unfinished. Nothing here is a grant, an offer, or a promise: no
first-party licence is in force, and the published `legal/LICENSE.md-pl.proposed`-style notices that
used to sit in the repository root are **withdrawn** and kept in this folder only
so the history is readable. The owner will choose a licence explicitly, and only
after the program is demonstrably working end to end. Until then:

- treat the first-party code as **all rights reserved**;
- do not cite this repository as licensed for reuse;
- do not add licence text, licence sections or licence reasoning to any other
  document — this folder is the only place it belongs.

`LICENSE.md-pl.proposed` is the Ms-PL notice that was in force from S18
(2026-09-14) until this decision. It is **not** active; it is kept as the
proposal most likely to be adopted when licensing is settled.

## Third-party components (these texts must travel with any build)

| File | What it covers | Why it ships |
|---|---|---|
| `legal/COPYING.gifsicle` | gifsicle 1.96 | GPL v2-only. It is run as a **separate subprocess**, never linked, so it is not part of this program's own licensing question. Its text ships with every package and every wasm build. |
| `legal/COPYING.lgplv3` | Qt 6 | LGPL v3. The GUI links Qt dynamically; the text ships and Qt's own notice is generated into the package as `QT_NOTICE.txt`. |
| `legal/COPYING.gplv3` | Qt | The companion GPL text the LGPL v3 asks for. |
| `legal/COPYING.ms-pl` | Ms-PL | The full text of `LICENSE.md-pl.proposed`, kept beside it. |

These four are obligations that exist **now**, regardless of the first-party
decision above: they are about the components, not about us.

## The context that used to be scattered across the docs

Kept here so no other file has to mention any of it.

1. **Why Ms-PL was chosen (S18, `OD-09 = b`).** The ScreenToGif fork this
   project studies is Ms-PL, so its files can be copied and adapted with notices
   retained instead of rewritten. The earlier GPLv3 intent existed only to match
   a Caesium-derived UX base that turned out to contribute nothing (zero files,
   no foreign copyright headers), so relicensing was the owner's call alone on a
   sole-author tree.
2. **Why the engine boundary matters.** gifsicle is GPL v2-only. Running it as a
   subprocess keeps it at arm's length from the GUI's own terms. Anything that
   embeds the engine in-process (the `web/wasm/` experiment) changes that
   question and is therefore **not shippable** while the first-party licence is
   undecided.
3. **The FFmpeg sidecar.** FFmpeg builds are LGPL or GPL depending on
   configuration; the libx264/libx265 builds needed for MP4 are GPL. If the
   sidecar lands, it is a **third separately-licensed piece** run as a
   subprocess, with its own text and source offer in the package. Bundling the
   binary does not change this program's own licensing question.
4. **The superseded notices.** Published releases before S18 carried a GPLv3
   intent, and the one public pre-release predates the Ms-PL notice entirely.
   Both are superseded by this file; nothing published needs to be recalled for
   that reason (the pre-release is already marked superseded on its own page).

## Mechanical footprint (paths that name these files)

Because the texts moved out of the root on 2026-10-26, these are the only places
that reference them:

- `working_code/gifscythe/scripts/package_common.sh` — required-file list and the
  generated package README;
- `working_code/gifscythe/scripts/test_package.sh` — packaging fixtures;
- `working_code/gifscythe/scripts/verify_audit.sh` — the packaged-file checks
  (`D1`/`D2`);
- `.github/workflows/build.yml` and its byte-identical copy
  `docs/ci/build.yml.proposed` — the artifact manifest lists;
- `web/wasm/build_wasm.sh` — the engine text staged into the wasm build.

Nothing else in the tree should mention licensing. If something does, it is a
bug: move the sentence here.
