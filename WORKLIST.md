# Worklist

**Version:** 0.1.0 · **Status register:** `STATUS.md` · **Audit detail:** `COMPILED_AUDIT.md`

> ## ⚑ Start with `STATUS.md`, not this board
> **`STATUS.md`** is the single status register: one row per tracked item in
> exactly one of four states — **DONE / PARTIAL / OPEN / UNTRIAGED** — with a
> generated header that says how much of everything the repo knows about is
> done. It answers *"what is left?"* without reading the 97-row audit register.
>
> This file is the **human task board**: what to pick up next, in order. It is
> not the status source of truth, and it must not contradict `STATUS.md`
> (`check_docs.sh` gate **G2** fails if a ticked box here maps to a non-DONE
> `U-nn` row there).
>
> Per-finding evidence stays in **`COMPILED_AUDIT.md`** §5 (the S8 remediation
> record is condensed in `docs/archive/AUDIT_HISTORY.md` entry 6). Session
> history is **`IMPROVEMENT_LOG.md`** — this board no longer duplicates it
> (S24 consolidation).

## The rules that keep the docs true (S9 — do not skip these)

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
   `working_code/gifscythe/scripts/pr_preflight.sh --online` (it runs
   `check_docs.sh` and `sweep_stale.sh`, and prints the repo/run/PR state),
   fix every failure, re-run until green. Do not create or merge with a
   failing check, and **do not ask whether to run it**. Anything labelled
   "after merge" is done **before** merging.
4. **It is also enforced mechanically.** `.githooks/pre-push` blocks a red push.
   Git does not copy hooks on clone, so run
   `working_code/gifscythe/scripts/bootstrap_hooks.sh` once per clone —
   `build.sh` does it for you. Verify: `git config core.hooksPath` → `.githooks`.
5. **`IMPROVEMENT_LOG.md` entries use the template** (`Changed / Partial / Left /
   Verified / Not verifiable here / Docs touched`). The **`Not verifiable here`**
   line is mandatory and must never be omitted or softened.
6. **HARD RULE — commit every edit/write/delete before merge AND before the
   session can close.** Uncommitted work is lost when the sandbox is cut off
   (it happened twice). Do not wait to be reminded. Gate **G18** fails
   `check_docs.sh` on a dirty tree; `pr_preflight.sh` **P3** fails create/merge
   on dirty and **P3b** fails if HEAD is ahead of origin. Push after you
   commit. At every new-session start, inspect `web/WEB_PLAN_TEMPLATE.md`
   §1–§10: leftover `<placeholders>` = stay **SKELETON**; filled content =
   flip both **G16** lines to **WORKING PLAN** in the same commit (one-way).
   The gate never auto-edits.

## Found this session — pending lines (rule 2)

Every new finding gets a matching line here. Nothing is `UNTRIAGED` right now:
the S24 intake was triaged in the same session (each row carries its §6 id).

- [x] **U-97** (found AND fixed in S27 — rule 2's strong form): the 2026-09-22
      zip re-creation of the GitHub repo lost the root license set
      (`COPYING.ms-pl` / `COPYING.lgplv3` / `COPYING.gplv3` / `COPYING.gifsicle`),
      so both packagers fail closed — that is where CI windows "Package
      portable" died on all three runs of the new main (executed local repro:
      `ERROR: COPYING.ms-pl missing or empty`, exit 1). Restored from canonical
      sources (URLs + sha256 digests in the S27 log entry); repro flipped to
      `Package created` exit 0; the repair PR's CI packaging steps are the
      platform proof.

- [x] **External review intake 2026-09-16 (four uploaded files) — incorporated
      and closed out in S24.** All 54 external findings dispositioned in
      `COMPILED_AUDIT.md` §20 (20 new register rows — 4 already fixed, 16
      OPEN and scoped; the rest already-tracked, already-fixed or refuted with
      evidence). The four root files were deleted after incorporation; full
      texts live in git history at `3c67e14`. Fixed at/before intake (no new
      work implied): **U-77**, **U-79**, **U-80** (PR #30) and **U-82** (S24).
      The new OPEN rows, one line each, in §6 order:
      - [x] **U-78 + U-87** → **P1-44** — **DONE (implemented S25 / PR #32,
            proved + closed S26).** Web numeric honesty: the validate.mjs
            finite-number gate (a wrong type is a named 422, never 200 ok:true
            with the setting silently dropped) + `numOrNull()` so an emptied
            field means OMIT, not `Number("")`→0. S26 added the three fixture
            batches the row asked for and mutation-tested them: wrong-type
            pinned against the REAL CLI for seven integer keys (its counterpart
            is the conf parser's `not an integer` + `--strict` rc=3), explicit
            zero pinned as parity, empty-vs-zero pinned over HTTP (transport
            72 → 79 cases).
      - [ ] **U-95** → **P1-45** — the published Release predates the Ms-PL
            relicence: owner release-notes edit (mark superseded/pre-release,
            never delete), then the P0-4 re-cut + tag-triggered asset gate.
      - [ ] **U-81** → **P1-46** — explode frame verification ignores the
            stream-output/--info exemptions the ordinary verifier documents;
            `output = -` explode downgrades an honest run to rc=1.
      - [ ] **U-94** → **P2-18** — oracle-fuzz gate: sweep the settings space
            against the REAL engine (both mirrors can be wrong together —
            U-62/U-63/N-05 proved it); seeded, offline, committed matrix.
      - [ ] **U-92** → **P2-19** — web upload admission: GIF magic on the
            decoded buffer + strict base64 (GS-205's web twin).
      - [ ] **U-93** → **P2-20** — web transport bounds: capped stderr echo,
            output envelope, favicon 404.
      - [ ] **U-88** → **P2-21** — register mechanics: derived P-id state +
            harness:/desktop: proof markers + derived release-bar counters.
      - [ ] **U-89** → **P2-22** — doc-machine cost: --json/digest instead of
            hand-typed counts, truncation revisit, gate freeze until P0/P1 empty.
      - [ ] **U-85** → **P3-13** — PORT validation (named error + exit 2, not
            a raw RangeError stack).
      - [ ] **U-84** → **P3-14** — /optimize reads the body before discovering
            the engine (413 contract parity with /run).
      - [ ] **U-83** → **P3-15** — pin single-input batch+output engine
            semantics with a smoke case (or refuse it until pinned).
      - [ ] **U-86** → **P3-16** — three comment-vs-behavior mismatches
            (collision message, expand_home bare-~, run() double-resolve).
      - [ ] **U-91** → **P3-17** — one shared exit-code contract (CLI vs C#
            spike collide on 3) + the spike's ReadExactly/quoting port traps.
      - [ ] **U-96** → **P3-18** — stemOf dotfile/extensionless parity vs Qt
            completeBaseName (probe + one shared helper + fixtures).
      - [ ] **U-90** → **P3-19** — register the parked-plan scope (stills →
            animated, video endpoints) as gated rows + the PROJECT_VISION
            amendment proposal; no code until the owner adopts the words.
- [x] **CI/infra (S14 → resolved S24):** the documentation status gate is LIVE
      in the CI linux job (maintainer-applied); the last declared drift was the
      proposed copy lagging the maintainer's live cygpath fix, so S24 re-synced
      `docs/ci/build.yml.proposed` to the live line and deleted the pending
      marker in one commit — E9/G7/S1 enforce byte-equality again, and
      W-30/R-03/GS-208 closed with it.
- [x] **Older session findings** (N-03 screenshots, N-04 MinGW paths, N-05
      multi-input explode, N-06 awk-locale emitter, N-07 count sweep, N-08 G11
      date rule, N-09 settings-panel sentinels) — all closed; detail in
      `STATUS.md` + `IMPROVEMENT_LOG.md`.

## Direction decisions (2026-09-09 → S20; detail in `docs/planning/PLANNING.md` §1)

- **Offline-only.** No cloud, no auto-update, no telemetry. **Amended S14
  (owner):** the `web/` build is a **supported, self-hosted product surface**
  (loopback default, `GS_WEB_HOST` for LAN). Plan + split rules:
  `web/WEB_PLAN_TEMPLATE.md`.
- **Language: stay C++17 + Qt6 Widgets through 1.0.0**; revisit only on a
  documented trigger (`STATUS.md` D-08) — then spike Rust + Tauri. **Reaffirmed
  S19 (owner):** exe stays C++17/Qt6; the C# shell is parked (`OD-C7 = park`,
  resume point `docs/planning/PLANNING.md` §2). S24 note: an external review
  argues the web-tech trigger already fired — that is an owner re-decision
  (recorded in `COMPILED_AUDIT.md` §20.4), not a session call.
- **Windows-only ship (S20, `OD-17 = a`):** Windows exe + web app ship; Linux
  is the CI/sandbox test battery and ships nothing.

## The road to 1.0.0 (ordered; states live in `STATUS.md`)

1. **U-59 / P0-7** — cancel/failure must never leave a truncated file over a
   pre-existing good output (temp+rename on verified success). The only
   registered data-loss row; fix before anything else. **S28: the CLI/core half
   landed test-first** — the engine writes `<target>.gs-partial` and the target
   is renamed onto only after verification, so a cancel, a signal or a refusal
   leaves the previous bytes intact (smoke 54 → 58/58, three cases run RED
   first; `test_output_verify.sh` 25 assertions). **Remaining: the Qt half** —
   `runCommand()` still passes the real target to the engine, so the GUI Cancel
   keeps the old behaviour until the same guard is wired there and
   `tests/test_gui_offscreen.cpp` gets a cancel-with-preexisting case (needs
   Qt6). The row is PARTIAL, not DONE.
2. **U-09 / P0-4 + U-95 / P1-45** — re-cut release artifacts from one exact
   tagged SHA; the owner marks the pre-relicence `snapshot-2026-09-07` release
   superseded in its notes (never delete — rollback policy).
3. **W-18 clean-Windows smoke + W-19 desktop probes** — real machine, published
   artifact: `docs/ci/README.md` §2–§3. Nothing in a Linux sandbox can close
   these.
4. **GS-203 / P1-25 GUI half** — wire the shared output verifier into the Qt
   run lifecycle (execution detail: `docs/planning/PLANNING.md` §4).
5. **The Qt/platform rows** — U-12 (P1-24 async state machine), U-58 (P1-38
   batch settings snapshot), U-59's GUI half, U-70/U-72 (P1-42), DS-10, GS-205,
   U-71 (now CI-testable per P2-17's S24 note).
6. **The S24 web-intake batch** — P1-44 first (U-78/U-87), then the P2/P3 rows
   above; all are provable with node alone except where noted.
7. **Owner decisions** — `docs/planning/OWNER_DECISIONS.md`: remaining
   OD-03…OD-07, OD-10, OD-13…OD-16, OD-18 (OD-16 blocks `web/wasm/`;
   OD-18 blocks closing U-76). Version decision (0.2.0 vs 1.0.0) last, per
   `OD-11 = a`.
8. WebP/APNG stay blocked until all of the above ships (D-01..D-04).

## Deferred bucket list — after GIF `1.0.0`

Tracked as **D-01…D-08** in `STATUS.md`. All `OPEN`: known, scoped, and
deliberately not started until GIF 1.0.0 ships.

- Common RGBA animation frame model (timing, disposal, blend, alpha, canvas, loop).
- Animated WebP (libwebp AnimDecoder/AnimEncoder).
- APNG (libpng/zlib with APNG support).
- GIF ⇄ APNG ⇄ WebP convert, explode, merge, reorder, loop controls.
- Frame editor, text/watermark overlays, presets, richer previews.
- Optional: logging framework, i18n, dark mode, system tray.
- **Web client-side wasm** (`web/wasm/`, D-07): scaffolded S19, unbuilt, and
  NOT SHIPPABLE until `OD-16` (in-process licence — `docs/legal/README.md` §3)
  + a real emcc byte proof. The Node server stays the shipped web path.
  History of the option analysis: `web/README.md` §History.
- **Language migration (only if a trigger fires):** Rust + Tauri spike —
  `docs/planning/PLANNING.md` §1 triggers.
- **Stills → animated / video endpoints** (owner request 2026-09-14, parked
  plan scope): gated by U-90/P3-19 — a PROJECT_VISION amendment + decoder +
  licence story must land first.

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

# Web app (self-hosted product surface). All nine suites run in the CI linux
# job and in verify_audit.sh W1-W6; quote the RUNTIME counter of the run you
# just executed, never a number from a doc:
node web/test/command.test.mjs        # JS ⇄ C++ argv parity (needs the built CLI)
node web/test/validate.test.mjs       # JS ⇄ C++ validation parity + U-78 wrong-type (needs the built CLI)
node web/test/numeric-honesty.test.mjs # U-78/U-87 empty-vs-zero + wrong type (P1-44; engine-free)
node web/test/transport.test.mjs      # live-server HTTP net (needs a discoverable engine)
node web/test/body-limit.test.mjs     # U-68 413 mapping (engine-stub, no real engine)
node web/test/static-hygiene.test.mjs # U-67 allow-list + HEAD contract
node web/test/request-guard.test.mjs  # U-54/U-69 run ownership (pure module)
node web/test/device-names.test.mjs   # U-56 shared reserved-name table
node web/test/server-bounds.test.mjs  # U-06 concurrency/rate/timeout bounds
node web/server.mjs 8000              # from the repo root; binds 127.0.0.1
```

## Do not

- Bump to 1.0.0 as a placeholder.
- Add WebP/APNG before GIF UI is stable.
- “Fix” `--loopcount=0`, `-O0`, crop plus-form, or gamma sentinel (verified
  correct — `COMPILED_AUDIT.md` §15.1 VP-1..VP-5).
- Link gifsicle into the GUI binary (keep subprocess for GPL v2-only vs Ms-PL).
- Edit `reference_code/` (read-only; the manifest is the one exception).
- Hand-edit the generated block in `STATUS.md` — run `check_docs.sh --emit`.
- Push, open or merge a PR with a red `check_docs.sh`, or bypass the pre-push
  hook with `--no-verify`.
- Reintroduce the removed `scripts/build_gifsicle.sh` shim.
- Recreate scattered audit/review copies at the repo root — external reviews
  are incorporated into `COMPILED_AUDIT.md` (§20 is the pattern) and the
  originals live in git history (G17/S2 will fail stale copies anyway).
