# S48 post-merge audit — PR #18 code + docs

**Date:** 2026-10-31 · **Base:** `main` at `53965e0` (PR #18 merge) · **Branch:** `arena/c8a68228-gifscythe`
**Scope:** code merged in PR #18 (S46–S47 Output controls, input filtering, on-demand preview) + all current-state `.md` files.
**Method:** static review of `MainWindow.cpp/h`, `PreviewPanel`, `Collision.h`, `OutputPlan.h`, `ExplodeVerify.h`, `NameTemplate.h`, `MediaFormat.h` plus targeted runtime probes. Qt 6.8.3 harness was building; probes cover integrity paths that need no GUI.

## 1. Summary

PR #18 delivered the owner-requested UI (collision square, keep-structure, size guard, template, filter, preview). CI was green (run 41108310504 success) and the docs gate passed after re-anchoring. No new `UNTRIAGED` rows existed.

This audit found **12 code issues** (2 high data-loss, 5 medium logic, 5 low/documentation) and **~10 stale doc claims** that were true at S46/S47 but false after the merge. None were in the register yet. They are recorded below as checkable tasks, not as assumed fixes.

## 2. Code findings — missing / misaligned / broken logic

### High — data loss or silent overwrite

**S48-01 — Explode can overwrite its own source file (HIGH)**
`explodeTargetPrefix()` builds `dir + "/" + "<stem>_frame"` and ExplodeVerify writes `<prefix>.000…` under that directory. If the input directory contains a file named exactly `<stem>.000` (e.g., `x.000` admitted via odd extension, or user-created), the first exploded frame replaces it via `promote_explode_frames` which does `fs::rename` without checking destination existence.
*Repro:* `x.000` (bytes GIF) as input, prefix `x` → after Explode, `x.000` content changed to first frame (sha before `547d09a…` → after `8db2de0…`). No dialog, status “Explode complete”.
*Fix needed:* snapshot existing `<prefix>.*` before run; refuse or rename-dest if any destination exists, or at least warn with full paths; do not promote over a file that was not created by this run.

**S48-02 — Explode mixed-validity promotion (HIGH)**
`verify_explode_frames` correctly splits `frames` (valid GIF header) vs `suspicious` (changed but not GIF). It returns `ok = !frames.empty()`, so a staged set with 1 valid + 1 garbage file is considered OK. `promote_explode_frames` then moves **every** `explode_frames_under(partial)` entry, including the garbage, overwriting a previously valid destination (e.g., `frame.001`) with non-GIF bytes while reporting success.
*Repro:* staged `frame.gs-partial.000` (valid GIF) + `.001` (text `garbage-not-a-gif`), destination `frame.001` previously valid GIF → promoted, destination now text, GUI still “Explode complete”.
*Fix:* promote only `vr.frames`, never `suspicious`; report `suspicious` as warning and leave those destinations untouched; add unit test with mixed set.

**S48-03 — Batch time-dependent naming can bypass plan protection (MEDIUM)**
`planBatch()` renders names with `time(nullptr)` (call 1), refuses if any output equals an input. `runCommand()` then recomputes `plannedBatchTargets()` again (call 2) for `batchTargets_`, and each batch file's `render_name_template` is re-executed later per file (call 3+). If `time` rolls over a second boundary between call 1 and 2, the approved plan no longer matches the executed targets. With template `{Current Date [S]}` this can approve `late.gif → dest_at_00.gif` then execute `late.gif → dest_at_01.gif` that collides with source or another target, without re-planning.
*Repro (LD_PRELOAD):* intercepted `time()` to return `:00` for first 2 calls, `:01` thereafter → source file overwritten after “Cancelled before run — no files written” check passed.
*Fix:* render once, freeze `batchTargets_` from that single render, reuse it; never call `time()` twice for same batch. Add test that `planBatch()` and `runCommand()` share the same timestamp.

### Medium — logic errors that break contracts

**S48-04 — Single-output Rename exhaustion still overwrites (MEDIUM)**
`MainWindow::runCommand` single-output path for `Rename` checks `decide_collision` with `reserved = inputs`. If all `kMaxRenameAttempts` (10k) names are taken, `decide` returns `Skip` with `final_path == original`. The caller then does `settings.output = dec.final_path` **unconditionally**, so a “skip” still overwrites the original target.
*Repro:* `out/chosen.gif` + `chosen_1.gif`…`chosen_10000.gif` all occupied, single file `in.gif` with template `chosen`, policy Rename → `chosen.gif` overwritten with GIF bytes (sentinel `KEEP-ORIGINAL` gone).
*Fix:* if `dec.action == Skip`, treat as skipped (set skipped state, do not run); same handling as batch path already does.

**S48-05 — Late-appearing collision (TOCTOU) (MEDIUM)**
Both batch and single paths check `existsOnDisk` before starting, then later `claim_partial_output_path` and `promote_partial`. If another process creates the target after the check but before `rename`, `Skip` is not honored and `Overwrite/Rename` paths still replace it. No lock or re-check at promotion time.
*Known hole partially documented as “claim→open TOCTOU” in ledger #17, but not in user-facing docs.* Mitigation: re-stat destination just before `promote_partial`; if policy is Skip and file now exists, discard partial and mark skipped.
*Repro:* start batch with `late.gif` not existing, create `late_result.gif` after engine starts → skipped file still overwritten.

**S48-06 — Destination persistence Source→Folder swap (MEDIUM)**
`serializeGuiState` writes `destination = folder` only when Folder is selected. `applyGuiState` reconstructs `useFolder = (destTok == "folder") || (destTok.empty() && !batch.trimmed().isEmpty())`. If user last saved with `Source folder` checked but `batch_dir` non-empty (e.g., previously Folder path still in line edit), reload incorrectly re-selects Folder. Also, clearing the folder path does not clear `batch_dir` on Source save if non-empty check fails.
*Repro:* set Folder `/tmp/out`, switch to Source (batch_dir still `/tmp/out`), save → reload → Folder checked again.
*Fix:* always persist `destination` explicitly (`source` vs `folder`), or clear `batch_dir` when Source is active.

**S48-07 — FFmpeg Auto with N>1 inputs mislabeled (MEDIUM)**
`currentSettings()` for `Batch` returns no `output`, but for `Auto`/`Merge` it returns `singleTargetOutput()` derived from first input only. If `Auto` has 2 inputs queued and format is non-GIF, `runCommand` checks `inputs.isEmpty()` then allows Auto with 2 inputs, builds `GifsicleCommand` with both inputs (merge shape) or `buildFfmpegRequest` with first input only. Status row then says “merged from 2 files” while FFmpeg only encoded first file.
*Repro:* 2 GIFs queued, Auto + APNG → status “merged from 2 files”, FFmpeg log contains only first input.
*Fix:* refuse non-GIF Auto with >1 input (same as Batch/Merge guard), or force Batch mode for multi-file non-GIF.

**S48-08 — Destination dimensions for Fit/Touch (MEDIUM)**
`templateContextFor` sets `ctx.dst_width/dst_height` to `s.resize_w/h` verbatim for `Fit/Touch/Exact`, but `Fit` (resize-fit) scales preserving aspect (e.g., 60×132 inside 100×100 → 45×100). `{DstWidth}/{DstHeight}` in template preview and note then show 100×100, not actual 45×100, and batch output is 45×100.
*Repro:* Input 60×132, resize-fit 100×100, template `{DstWidth}_{DstHeight}` → note `100_100.gif`, file `100_100.gif` size 45×100.
*Fix:* compute Fit/Touch result via same math as gifsicle (aspect-preserving scale), or until then show “≤100×100” or original dimensions.

### Low — UX / docs mismatches

**S48-09 — FPS shared across formats (LOW)**
`FfmpegFormatPrefs.fps` is global, but `STATUS.md N-65` and `WORKLIST OD-26` claim “per-format FFmpeg quality/fps”. Dialog saves `ffmpeg_fps` single key; WebP fps=15 then H.264 shows 15, not its own value.
*Fix:* either change spec to “global fps” and update docs, or store `fps_apng/webp/h264…` per format like quality.

**S48-10 — Preview savings mismatch on skipped selection (LOW)**
`showWrittenOutputPreview` picks `batchTargets_[selectedRow]` for After, but `previewPanel_->setAfter` compares `before = selectedInput size` vs `after = shown output size`. If selected row was skipped (e.g., `a_result.gif` exists, Skip), current selection is row 0 (skipped `a.gif`) but shown output is `b_result.gif` (first written). Savings “a.gif → b_result.gif -12%” compares unrelated files.
*Repro:* batch a.gif (skipped) + b.gif (written), select a.gif → Before a.gif, After b_result.gif, savings computed.
*Fix:* when selected row is skipped/discarded, show “no preview for skipped file” or compare selected file’s own output (which does not exist) → clear savings.

**S48-11 — Media probe too permissive (LOW)**
`probe_output_header` for APNG/WebP/MP4 checks substring presence in first 64KiB. A 12-byte file `\x89PNG…acTL` (no chunks) passes as APNG; empty WebM header trick similar.
*Repro:* 12-byte file with PNG sig + “acTL” accepted → promoted over user file, replacing it with 12 bytes.
*Fix:* tighten checks (e.g., APNG requires PNG sig + IHDR + acTL chunk header structure; WebP requires RIFF+WEBP+ANIM chunk; video requires ftyp + mdat) or keep as “best-effort” and document limit.

**S48-12 — `relative_under_root("/")` edge (LOW)**
`relative_under_root("/a/b/in.gif","/")` returns empty because `"/"` trim → `""` then `in.size() <= rt.size()` fails. Owner adding “/” as source folder then expecting Keep-structure to work gets flat writes without warning.
*Fix:* handle root specially, or document that “/” is not supported as source folder.

## 3. Stale documentation (swept this session)

- Root `README.md` still claimed “GIF is the only format implemented” (true at S34, false since S45 FFmpeg). Updated to current desktop vs CLI/web split plus safety note (see sweep).
- `INSTALL.md` named `arena/dde1f972-gifscythe` and archive URL; updated to `main` + guidance for follow-up branches.
- `TESTING.md` described `Input/File name pattern/Start` + `Guide` tab + `Preview changes`; updated to actual 6 tabs, Output template, `Apply setting preview`.
- `PROJECT_VISION.md` still said APNG/WebP 0% and “decode-only” without noting encoder discrepancy; updated to delivered state with open owner question.
- `working_code/gifscythe/README.md` S9 status snapshot and deferred wording; marked historical and corrected.
- `docs/ci/README.md` “no image files” absolute; softened to test fixtures vs design refs.
- `docs/planning/XN_CONVERT_REFERENCE.md` claimed “only image material” and “The gap” as current; marked historical with S48 note.
- `docs/planning/XNCONVERT_SCREENTOGIF_SYNTHESIS.md` overstated metadata and absolute staging guarantee; corrected.
- `WORKLIST.md` S47 unchecked “Confirm new GitHub Actions checks” now stale after run 41108310504 success; checked off in this sweep.
- `SESSION_HANDOFF.md`/`COMPILED_AUDIT.md` Base `499cb28` and Docs-synced OPEN now stale after `53965e0` merge; re-anchored this session.

## 4. What was not changed here

No product code fixes applied — this is audit-only. All probes are isolated (temp dirs, fake engines, LD_PRELOAD) and leave no fixtures. New tasks below are the handoff for a fix PR.

## 5. Next steps (ordered)

1. **S48-01/02/04** — collision/promotion fixes (Explode + single Rename) — P0 data-loss class, test with real FS fixtures.
2. **S48-03** — freeze batch targets/timestamp — one-line fix, add `time()`-mock test.
3. **S48-06/05** — persistence + TOCTOU re-check — small patches, add preset round-trip test.
4. **S48-07/09/08/10** — mode/fps/dimensions/preview — docs first (owner decision on fps per-format), then code.
5. **S48-11/12** — tighten probes / root path — low, can batch with other media work.

Owner to confirm: should fps be per-format (like quality) or global? Should Explode refuse when any `<prefix>.*` exists?

---
*Generated by S48 audit, 2026-10-31. Evidence logs in `/home/user/build/*audit*` and `gifscythe-s48-stale-hits.txt`.*
