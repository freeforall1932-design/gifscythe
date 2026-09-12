# Reference Code Manifest

This folder holds **read-only source material** — reference code we adapt, bundle,
or consult. **Never edit or ship these directly.** All edits happen in
`../working_code/`.

## What's here and where it came from

| Folder | What it is | Source / origin | Retrieved |
|---|---|---|---|
| `gifsicle/` | gifsicle source (GPL v2-only) — the tree `scripts/build_engine.sh` compiles. **Provenance VERIFIED 2026-09-12 (S11):** byte-identical to upstream `kohler/gifsicle` master commit `07f5c4c3de1306156e1d8f33e62971d4664c8f7` (5 commits after the `v1.96` tag) in every shared file, **minus four upstream CI dotfiles** (`.appveyor.yml`, `.github/`, `.gitignore`, `.travis.yml`). The product-owned native config is staged from `working_code/gifscythe/build_support/gifsicle/config.native.h`; it is no longer inside this tree (S13). Hashes below. | `https://github.com/kohler/gifsicle.git` @ `07f5c4c3` | Verified against a fresh clone 2026-09-12; config relocation verified 2026-09-12 |
| `gifsicle-nested-1.96/` | **Pristine gifsicle v1.96** — verified 2026-09-12 (S11) byte-identical to upstream tag `v1.96` (commit `a08e0f6686d467bb8b9e4715b1f1835f12984fb0`) in every shared file, minus the same four CI dotfiles, with NO local additions. The three files that differ from `gifsicle/` (`src/gifsicle.c`, `src/gifsicle.h`, `src/Makefile.w32`) plus the missing `test/012-framechange.testie` are exactly the upstream post-1.96 delta, not local edits. | `https://github.com/kohler/gifsicle.git` @ `v1.96` | Verified against a fresh clone 2026-09-12 |
| `gifsicle-upstream/` | Fresh FULL clone of upstream gifsicle used for the 2026-09-12 verification; `07f5c4c3` checked out. Gitignored (re-fetch as needed: `git clone https://github.com/kohler/gifsicle.git reference_code/gifsicle-upstream`). | `https://github.com/kohler/gifsicle.git` | Auto-fetched (GitHub) 2026-09-12 |
| `caesium-source/` | Caesium **UI** source (GPLv3) — the UI/UX base we adapt. Commit `867c7d5ce6efec599b87cd773fbe659bd5d1263f`. | `https://github.com/Lymphatus/caesium-image-compressor.git` | Auto-fetched (GitHub) |
| `caesium-bin/` | Caesium 2.8.5 **Windows binary bundle** (Qt6 runtime: Qt6*.dll, platforms/, imageformats/ incl. `qgif.dll` + `qwebp.dll`). Used as the reference for the **portable Qt runtime** pattern. | Was already in this repo (bundled `.exe` + DLLs). **Untracked 2026-09-07** (74 MB of third-party binaries; `.gitignore`d) — re-fetch from the Caesium GitHub releases (`Lymphatus/caesium-image-compressor` 2.8.5 Windows bundle) if needed. | Untracked (gitignored) |

## Verified identity of `gifsicle/` (audit U-10 / A-09, provenance half — S11, 2026-09-12)

Method: fresh full clone of `kohler/gifsicle`; `diff -rq` of each snapshot tree
against the checked-out upstream commit; `sha256sum` per file. Reproduce with:

```bash
cd reference_code
git clone https://github.com/kohler/gifsicle.git gifsicle-upstream   # if absent
git -C gifsicle-upstream checkout 07f5c4c3de1306156e1d8f33e62971d4664c8f7d
diff -rq --exclude=.git gifsicle gifsicle-upstream
# expected: only the four CI dotfiles (upstream-only); product config is outside this tree
cd gifsicle && find . -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum | sha256sum
```

| Tree | Files | sha256 of the sorted per-file sha256 list ("list digest") |
|---|---|---|
| `gifsicle/` @ upstream `07f5c4c3` | 56 | `f4cfd32cbb05e11b0b8d354513a7fefafac6e12477a60f1985c7d555c07903f7` |
| `gifsicle-nested-1.96/` @ upstream tag `v1.96` (`a08e0f66`) | 55 | `2e067fcf13c501f4f89aab1c4e89b440df0dd98811f51afd009d2928593b3a65b` |
| product-owned `working_code/gifscythe/build_support/gifsicle/config.native.h` | 1 | `e5dc1ac6a26398f861cd8059cacba84b9384acda8610b71c303ccfe62f415dda` |

The four files that carry the post-1.96 upstream delta were hashed individually
and match upstream `07f5c4c3` exactly:

| File | sha256 (local == upstream) |
|---|---|
| `src/gifsicle.c` | `2c37ac258183e29d5b9f4cb0b27472ce830b9c895e50d3b40f1b66383198602a` |
| `src/gifsicle.h` | `c49942cf41414667497a540ee5b689f6698f797d8197102786cc48e9723b7b77` |
| `src/Makefile.w32` | `1d77d0aae466c5f1fda97a6916e30516c774ed5e4f106713591e456b0f6b2a64` |
| `test/012-framechange.testie` | `e08bcc484ae50e11479f8af8b9dffbb292b3e4e5037481397bb0b9c2b836116a` |

**Resolved question (was open in the previous manifest):**
`src/gifsicle.h:346` `FRAME_SELECTION_MODE_MASK 0x1F` (used at
`src/gifsicle.c:432`, `frames_done |= 1 << mode` at `:521`) **is upstream code,
not a local edit** — it arrived in upstream commit `9efcc14` ("Fix #186"), one
of the 5 commits between `v1.96` and `07f5c4c3`. `test/012-framechange.testie`
is upstream's own test from `ed5b018` ("Add a frame-change test").

## Notes
- **Auto-fetched** items (network worked): `gifsicle-upstream`, `caesium-source`.
- If a future reference **cannot** be auto-fetched, it must be **manually
  uploaded** here; append it to this manifest and mark `Retrieved: manually
  uploaded`.
- **The remaining U-10/A-09 gap** (provenance and config relocation are now
  closed):
  1. **CI hash-pinning is proposal-only:** the CI token has no `workflows`
     scope, so a digest-check step cannot be pushed to
     `.github/workflows/build.yml` (see `docs/ci/PENDING_WORKFLOW_CHANGE.md`).
- Product build configuration is now explicitly owned by
  `working_code/gifscythe/build_support/gifsicle/config.native.h`; the build
  script stages it as `config.h` in a temporary include directory and removes
  that staging directory on exit. The upstream `reference_code/gifsicle/`
  snapshot no longer carries a local file.
- `gifsicle-nested-1.96/` is the pristine `v1.96` comparison tree (frame-
  selection behaviour predates upstream fix #186 there). It is not the build
  input.
- The three **Auto-fetched** rows above (`gifsicle-upstream/`, `caesium-source/`,
  `caesium-bin/`) are gitignored and therefore **absent from a fresh checkout**;
  re-fetch them before relying on this manifest.
- The nested `.git` directories of earlier shallow clones were removed; this
  folder is a plain read-only snapshot, and provenance is documented here
  instead. (`gifsicle-upstream/` keeps its `.git` — it is gitignored scratch
  for re-verification.)
