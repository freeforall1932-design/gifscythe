# Release procedure — how to cut a Gifscythe release

**Added:** 2026-09-10 (S7) — closes the WORKLIST "Document release procedure"
item. **Offline-only product:** a release is a *downloadable portable folder*
— no installer, no auto-update, no telemetry, no server step.

Audience: the maintainer (or an agent session) preparing a snapshot or a
version release. Follow top to bottom; every gate must be green **with
evidence recorded** (the project rule: `COMPILED_AUDIT.md` §6 — never mark
anything done without evidence).

---

## 0. Decide what you are cutting

| Type | When | Version |
|---|---|---|
| **Snapshot** | Any time CI is green; for testing/archival | no version bump; Release named `snapshot-YYYY-MM-DD` |
| **Patch (0.x.y+1)** | Bug fixes only | bump `working_code/gifscythe/VERSION.md` |
| **Minor (0.x+1.0)** | New non-breaking feature (e.g. the S7 persistence/reorder/templates work) | bump `VERSION.md` |
| **1.0.0** | **ONLY** when the UI/UX task is genuinely done: C4/D3/D4 clean-Windows smoke green + desktop probes B5/B6/B14 done + owner decision | bump `VERSION.md` |

Version rules live in `working_code/gifscythe/VERSION.md` — it is the **single
source of truth**; `build.sh` syncs it into the committed
`src/core/version.h` fallback (`GS_VERSION`), and CMake generates its own
copy inside the build tree from `build_support/version.h.in` without ever
writing into `src/` (S11, audit U-15 — gate C9 enforces). Never hardcode the
version anywhere else. The **engine keeps
upstream identity 1.96** regardless of the product version (audit A10).

## 1. Pre-flight (local, ~10 minutes)

From `working_code/gifscythe/`:

```bash
./build.sh                    # engine + CLI + unit tests      -> ALL TESTS PASSED
./scripts/test_engine.sh      # engine pipeline                -> 5/5
./scripts/smoke_cli.sh        # CLI integration                -> 19/19
./scripts/test_package.sh     # packaging negative suite       -> 0 failed
./scripts/check_docs.sh       # documentation status gate      -> 0 failed
./scripts/verify_audit.sh     # whole COMPILED_AUDIT §6 suite  -> 0 FAIL
# GUI harness (needs Qt6):
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=ON
cmake --build build-cmake -j2
QT_QPA_PLATFORM=offscreen ./build-cmake/test_gui_offscreen   # -> 0 failures
# Web parity harness (demo, must still mirror the core command layer):
node ../../web/test/command.test.mjs                         # -> ALL PASSED
```

Expected counts as of S12 (2026-09-12): unit suite **296 checks, 0 failures**
(the runtime counter, not the 261 `CHECK(` source sites), GUI harness **324
runtime checks** *(measured in the S11 sandbox, which had Qt 6.4.2 — re-run it
on a Qt machine before trusting the number; the file now holds 250 `CHECK(`
source sites, which is a different quantity)*, smoke **19/19**, web **17 + 23 +
30**, `verify_audit.sh` **28 PASS / 0 FAIL / 3 SKIP, exit 0** (skips are the
declared-pending workflow change and the CI-gated + clean-Windows items; a
toolchain-less sandbox additionally skips C6/C7*/C9/B). If a count changed,
update the docs in the same PR — stale counts are treated as a finding, and
`check_docs.sh` gates **G6/G9** now fail the build over them instead of leaving
it to review.

Also verify before packaging:

- **The documentation gate is green.** `./scripts/check_docs.sh` must report
  `0 failed`. If `STATUS.md` has drifted, run `./scripts/check_docs.sh --emit`,
  inspect the diff, and commit it. Do not release with a red doc gate and do not
  hand-edit the generated block to make it pass.
- `git status` clean; `.github/workflows/build.yml` and
  `docs/ci/build.yml.proposed` are **byte-identical** (`diff` them) — unless
  `docs/ci/PENDING_WORKFLOW_CHANGE.md` exists, which declares a workflow change
  waiting on a token with the `workflows` scope. Apply it before releasing, and
  **delete that marker in the same commit** — S9 found it left behind after the
  previous change had already landed, with every doc still quoting the old gate
  numbers (`STATUS.md` N-01).
- Engine identity: `release/<ver>/gifsicle --version` → `LCDF Gifsicle 1.96`.

## 2. Bump the version (if not a snapshot)

1. Edit `working_code/gifscythe/VERSION.md` → `Current version: X.Y.Z`.
2. Rebuild (`./build.sh`) — `src/core/version.h` regenerates; commit it with
   the bump (it is the synced fallback for direct g++ builds).
3. Sanity: `./build/gifscythe-cli` banner and the GUI window title (harness
   T1 asserts the title contains `GS_VERSION`) both show the new version.

## 3. CI build (both OSes)

Push the branch/PR and wait for **both** GitHub Actions jobs green
(`.github/workflows/build.yml`):

- **linux** — engine, CLI, unit tests, engine/CLI smoke, GUI offscreen
  harness, portable package; uploads artifact `gifscythe-linux`.
- **windows** — MinGW engine (`win32cfg.h` recipe), static-linked CLI/tests,
  native E2E smoke (spaces in paths + honest exit codes), GUI via CMake+Ninja,
  `windeployqt` runtime, engine staged beside the GUI; uploads artifact
  `gifscythe-windows`.

Record the run numbers/IDs in the release notes (evidence rule).

## 4. Package

On the machine that built (or from CI artifacts):

```bash
./scripts/package_portable.sh   # release/<ver>/Gifscythe/          (self-contained)
./scripts/package_system.sh     # release/<ver>/Gifscythe-system/   (no Qt runtime)
```

Every package **must** contain (the portable packager enforces/probes these):

- the engine (`gifsicle` / `gifsicle.exe`) beside the app binaries —
  `EngineLocator` finds it there first after `GS_ENGINE`;
- `gifscythe-cli` and (when Qt was present) the `gifscythe` GUI;
- on Windows: the `windeployqt` runtime for the GUI **and** the offscreen
  platform plugin if the harness will run there;
- licenses: `LICENSE` (UI, GPLv3 intent), `COPYING.gifsicle` (engine, GPL
  v2-only — always shipped next to the engine), `VERSION.md`, `README.md`,
  generated `README.txt`.

Zip the folder(s): `gifscythe-<ver>-windows.zip`, `gifscythe-<ver>-linux.zip`.
Record each zip's **sha256** in the release notes.

## 5. Publish (GitHub)

Artifacts from Actions runs **expire after 14 days** — binaries are *banked*
on Releases instead (policy since 2026-09-07):

1. Create a GitHub **Release**:
   - snapshots: tag/name `snapshot-YYYY-MM-DD`;
   - versions: tag `vX.Y.Z` (annotated) and the same as the Release name.
2. Attach the zips (+ sha256 lines in the body).
3. Release notes must state: version, what changed (link the PR/commits),
   the CI run IDs proving green, the harness/unit/smoke counts from §1, and
   known gaps (e.g. clean-Windows smoke status).
4. Never attach anything built from `reference_code/` other than the engine
   binary itself and its `COPYING` (reference trees are read-only source
   material, not shippables).

## 6. Post-publish verification

- **Clean-Windows smoke (gates C4/D3/D4)** — run
  `docs/ci/CLEAN_WINDOWS_SMOKE.md` against the *published* zip on a machine
  with no Qt/MinGW/dev tools. Required before any **1.0.0** claim; strongly
  recommended for every Windows-facing snapshot.
- Desktop probes (one-time, real GUI): B5 kill-engine-mid-run, B6 physical
  drag-drop, B14 engine-missing variant.
- Settings-persistence spot check (S7): start the GUI, change a few Actions
  controls + batch folder + name template, close, relaunch → state restored;
  `%APPDATA%\Gifscythe\gifscythe.conf` (Windows) / `~/.config/.../gifscythe.conf`
  (Linux) exists and is a readable `key = value` file; delete it → clean
  defaults, no error.
- Download-and-run one zip on a second machine/profile (portable promise:
  no installer, no admin, offline).

## 7. Record the evidence — and close the docs loop

**Every session ends by updating the docs, then running `check_docs.sh` until
green.** For a release that means, in this order:

1. `STATUS.md` — re-emit it (`./scripts/check_docs.sh --emit`) so the register
   and its generated counts match the repo. A release changes states; the
   register must say so.
2. `COMPILED_AUDIT.md` §6: tick/annotate the items this release proves. If a
   finding closed, its §5 row's status marker is what `STATUS.md` is generated
   from — update it there, not in `STATUS.md`.
3. `IMPROVEMENT_LOG.md`: add the release entry (newest on top) using the
   standard template — `Changed / Partial / Left / Verified / Not verifiable
   here / Docs touched`. The **`Not verifiable here`** line is mandatory.
4. `SESSION_HANDOFF.md` + `WORKLIST.md`: update current state and gates.
5. `PROJECT_VISION.md` progress snapshot if a milestone changed.
6. `./scripts/check_docs.sh` → **0 failed**. Then, and only then, push.

Anything discovered while cutting the release goes in **the same session**:
`UNTRIAGED` in `STATUS.md` plus a pending `- [ ]` line in `WORKLIST.md`. A
release is not a reason to defer recording a finding.

---

### Rollback

Releases are append-only artifacts — a bad release is handled by publishing
the previous zip as the recommended download and opening a fix-forward PR.
Never delete a published Release (evidence chain); mark it *pre-release* or
note the defect in its body.
