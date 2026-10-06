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
| `caesium-source/` | **RETIRED S18** (`OD-09 = b`): was the planned Caesium UI-source base (GPLv3); never incorporated, do not re-fetch. | n/a (retired) | Retired 2026-09-14 |
| `caesium-bin/` | Caesium 2.8.5 **Windows binary bundle** (Qt6 runtime: Qt6*.dll, platforms/, imageformats/ incl. `qgif.dll` + `qwebp.dll`). Used as the reference for the **portable Qt runtime** pattern. | Was already in this repo (bundled `.exe` + DLLs). **Untracked 2026-09-07** (74 MB of third-party binaries). **RETIRED S18** (`OD-09 = b`): do not re-fetch. | Retired 2026-09-14 |

## Verified identity of `gifsicle/` (audit U-10 / A-09, provenance half — S11, 2026-09-12)

Method: fresh full clone of `kohler/gifsicle`; `diff -rq` of each snapshot tree
against the checked-out upstream commit; `sha256sum` per file. Reproduce with:

```bash
# The pin: fast, no network, what CI runs.
working_code/gifscythe/scripts/verify_reference_pins.sh

# Re-deriving the provenance itself (network, ~1 min):
cd reference_code
git clone https://github.com/kohler/gifsicle.git gifsicle-upstream   # if absent
git -C gifsicle-upstream checkout 07f5c4c3de1306156e1d8f33e62971d4664c8f7d
diff -rq --exclude=.git gifsicle gifsicle-upstream
# expected: the four CI dotfiles, logo.gif, logo1.gif (upstream-only, see S37
# correction #1), PLUS src/Makefile.bcc and src/Makefile.w32 (line endings) -
# and src/Makefile.w32 also differs in content (S37 correction #2).
```

**Pinned and machine-enforced since S37**: `working_code/gifscythe/scripts/verify_reference_pins.sh` checks every file below by sha256 against `reference_code/REFERENCE_PINS.txt` (108 files across the three sets) and runs in CI; `--emit` re-pins deliberately. The list digests are kept for cross-checking against the S11 measurement.

| Tree | Files (S37) | sha256 of the sorted per-file sha256 list ("list digest", S11 — see the drift note) |
|---|---|---|
| `gifsicle/` @ upstream `07f5c4c3` | **54** (S11 recorded 56) | `f4cfd32cbb05e11b0b8d354513a7fefafac6e12477a60f1985c7d555c07903f7` |
| `gifsicle-nested-1.96/` @ upstream tag `v1.96` (`a08e0f66`) | **53** (S11 recorded 55) | `2e067fcf13c501f4f89aab1c4e89b440df0dd98811f51afd009d2928593b3a65b` |
| product-owned `working_code/gifscythe/build_support/gifsicle/config.native.h` | 1 | `e5dc1ac6a26398f861cd8059cacba84b9384acda8610b71c303ccfe62f415dda` |

## S37 correction: what "identical to upstream" does and does not mean

Re-measured 2026-10-07 against a fresh full clone of `kohler/gifsicle`, checked out at
both pinned commits. Two statements above were wrong, and the third needed a caveat:

1. **File counts are two lower than S11 recorded, in BOTH trees** (54 vs 56, 53 vs 55). The
difference is upstream's own `logo.gif` and `logo1.gif` — image files deleted repo-wide by
the no-images policy (N-36); the harness rebuilds what it needs from text fixtures via
`scripts/fixtures.sh`. Together with the four upstream CI dotfiles (`appveyor.yml` and `travis.yml` at the
upstream root, `gitignore`, and upstream's own GitHub workflow file) those are the six files
this repo deliberately does not carry — upstream-side paths, none of them present here, so
this document does not name them as repo files. Every recorded list digest above was therefore
computed over a file set that no longer exists; they are history, not a live pin.

2. **`src/Makefile.w32` is NOT upstream-identical, and never was.** It carries a local
functional edit: `kcolor.obj` added to `GIFSICLE_OBJS` (line 27) and `GIFDIFF_OBJS`
(line 29); upstream's `.w32` makefile omits it (`kcolor.c` is in upstream's Unix build).
The S11 per-file hash recorded for that file — `1d77d0aa…` — is **today's file put back to
CRLF** (`sed 's/$/\r/'`), i.e. the hash of the edited file, presented as "local ==
upstream". The true upstream hash at `07f5c4c3` is
`273730cbfd63c8b5e2658592b06c9d8af6841338fb35133d8991a2ac26c4d803` (CRLF) /
`b4bfda4f4dfb25faafdfbe2445ab6bfbd6eb16274da68b9abfb68b228fedb85f` (LF). The file is
unused by this repo's builds (Windows uses CMake + `win32cfg.h`; Linux uses the staged
config), so the edit is inert here — but it is a local edit, and it is now pinned as one.

3. **Line endings.** Both trees are LF where upstream is CRLF in `src/Makefile.bcc` and
`src/Makefile.w32` — a consequence of the 2026-09-22 repo re-creation returning through a
GitHub web upload. Byte-identity is therefore claimed **modulo those two files**, with #2
being the only content difference. `src/Makefile.bcc` is upstream content, LF endings
(verified: `diff --strip-trailing-cr` is empty against upstream).

Everything else was re-verified byte-for-byte in S37: **52 of 54** files in `gifsicle/`
and **51 of 53** in the nested tree match upstream exactly (`cmp`), and the nested tree's
`Makefile.w32` matches upstream `v1.96` modulo line endings (i.e. it does NOT carry the
`kcolor.obj` edit — the edit is specific to the build-input tree).

The four files that differ from the nested `v1.96` tree were hashed individually. Three
match upstream `07f5c4c3` exactly; `src/Makefile.w32` does not (see S37 correction #2) —
the S11 hash printed for it is the CRLF form of the locally edited file, so it is listed
below with its measured basis instead:

| File | sha256 (local == upstream) |
|---|---|
| `src/gifsicle.c` | `2c37ac258183e29d5b9f4cb0b27472ce830b9c895e50d3b40f1b66383198602a` |
| `src/gifsicle.h` | `c49942cf41414667497a540ee5b689f6698f797d8197102786cc48e9723b7b77` |
| `src/Makefile.w32` | `1d77d0aa…` was RECORDED in S11 but is the local file's CRLF form, not upstream's. Local (LF): `85cdd1d60537a335492c1c3c8b0df6e42ff7fbc6bc68e52ddd02f55f24585b67`; upstream `07f5c4c3` (CRLF): `273730cb…`; local `src/Makefile.bcc` (EOL-only diff): `253cdd515b9148e2e865eeed55a2b53b0f221ce63c2da51ede5d0c912f483ec6` |
| `test/012-framechange.testie` | `e08bcc484ae50e11479f8af8b9dffbb292b3e4e5037481397bb0b9c2b836116a` |

**Resolved question (was open in the previous manifest):**
`src/gifsicle.h:346` `FRAME_SELECTION_MODE_MASK 0x1F` (used at
`src/gifsicle.c:432`, `frames_done |= 1 << mode` at `:521`) **is upstream code,
not a local edit** — it arrived in upstream commit `9efcc14` ("Fix #186"), one
of the 5 commits between `v1.96` and `07f5c4c3`. `test/012-framechange.testie`
is upstream's own test from `ed5b018` ("Add a frame-change test").

## Notes
- **Auto-fetched** items (network worked): `gifsicle-upstream`, `caesium-source` (retired S18).
- If a future reference **cannot** be auto-fetched, it must be **manually
  uploaded** here; append it to this manifest and mark `Retrieved: manually
  uploaded`.
- **The U-10/A-09 gap is CLOSED (S37, 2026-10-07).** Provenance, config
  relocation and CI hash-pinning are all in place:
  1. **CI hash-pinning is IMPLEMENTED.** `scripts/verify_reference_pins.sh`
     verifies all 108 files of the three sets against
     `reference_code/REFERENCE_PINS.txt` and runs as a CI step in
     `.github/workflows/build.yml` (mirrored byte-identically to
     `docs/ci/build.yml.proposed`). RED was executed for all three drift
     classes before the commit: one byte changed in `src/optimize.c` → FAIL
     naming the file and both hashes; a vendored file deleted → FAIL naming it;
     a file added to a tree → FAIL naming it; restore → PASS.
- Product build configuration is now explicitly owned by
  `working_code/gifscythe/build_support/gifsicle/config.native.h`; the build
  script stages it as `config.h` in a temporary include directory and removes
  that staging directory on exit. The upstream `reference_code/gifsicle/`
  snapshot no longer carries a local file.
- `gifsicle-nested-1.96/` is the pristine `v1.96` comparison tree (frame-
  selection behaviour predates upstream fix #186 there). It is not the build
  input.
- `gifsicle-upstream/` is gitignored and therefore **absent from a fresh checkout**;
  re-fetch it before relying on this manifest. The two Caesium rows above are
  retired (S18) — their absence is permanent policy, not a fetch-away state.
- The nested `.git` directories of earlier shallow clones were removed; this
  folder is a plain read-only snapshot, and provenance is documented here
  instead. (`gifsicle-upstream/` keeps its `.git` — it is gitignored scratch
  for re-verification.)
