# Gifscythe — External Review Intake (new findings not in COMPILED_AUDIT)

**Repository:** https://github.com/freeforall1932-design/gifscythe (`main`)  
**Product version reviewed:** 0.1.0  
**Existing register at review time:** STATUS.md: 89 DONE · 7 PARTIAL · 26 OPEN · 0 UNTRIAGED · 122 total (S20)  
**Generated:** 2026-09-15  
**Findings in this export:** 19 (HIGH 2 · MEDIUM 7 · LOW 10)

> Register these as **UNTRIAGED** rows (repo vocabulary) with ids `NF-nn`; each has a file pointer, a reproduction and a proposed fix. Nothing here was executed — the reviewing sandbox had no toolchain — so every row needs one smoke/harness case before it can move to DONE.

## 0. Method
- Fetched main live via raw.githubusercontent.com and the GitHub trees API; read the master audit (COMPILED_AUDIT §1–§13) and STATUS.md first so every item below was checked against the 122 tracked rows before being listed.
- Read the full text of all core headers, the CLI driver, the web server/UI/builder, MainWindow.cpp (except a truncated middle section), MainWindow.h and SettingsPanel.h.
- Used the gifsicle 1.96 man page as engine truth for frame selections, '-', --crop zero dims, --no-loopcount, --info semantics.
- Every finding carries a confidence mark: 'verified-by-source' (the code path is unambiguous) or 'source-read, needs runtime probe' (behaviour depends on code or engine behaviour not visible here). Nothing was executed — this sandbox has no toolchain — so treat all rows as UNTRIAGED in the repo's vocabulary until a smoke/harness case reproduces them.

**Read:**
- README.md, COMPILED_AUDIT.md (§1–§13), STATUS.md
- working_code/gifscythe/src/core/{GifsicleCommand,GifsicleSettings,SettingsIO,Validate,OutputPlan,OutputVerify,ExplodeVerify,EngineLocator,ProcessRunner}.h
- working_code/gifscythe/src/cli/main.cpp
- working_code/gifscythe/src/qtui/{MainWindow.cpp,MainWindow.h,SettingsPanel.h}
- working_code/gifscythe/scripts/build_engine.sh
- web/{server.mjs,app.js,command.mjs,run-paths.mjs,output-verify.mjs,index.html}
- Engine truth: gifsicle 1.96 man page (lcdf.org)

**Not read (limits of this review):**
- SettingsPanel.cpp, PreviewPanel.cpp, DropListWidget.cpp (only the header of SettingsPanel)
- MainWindow.cpp middle section (~setBusy / planBatch / refreshOutputSummary) — truncated by fetch size
- web/validate.mjs, web/test/*, scripts/*.sh other than build_engine.sh, .github/workflows/build.yml
- reference_code/gifsicle sources (man page used as engine truth instead)

## 1. Summary table

| ID | Sev | Area | Class | Title | Confidence |
|---|---|---|---|---|---|
| NF-01 | HIGH | GUI (Qt) | BROKEN | Batch continuation re-reads LIVE settings for every file after the first | verified-by-source |
| NF-02 | HIGH | GUI (Qt) | BROKEN | Cancel (or engine failure) leaves a truncated file over a PRE-EXISTING output — previous good result destroyed | verified-by-source |
| NF-03 | MEDIUM | CLI | BROKEN | CLI turns gifsicle frame selections (`#0`, `#0-2`, `#name`) into bogus file paths | verified-by-source |
| NF-04 | MEDIUM | CLI | BROKEN | `output = -` (engine's documented stdout name) is treated as a file → false failure rc=1 after a successful run | verified-by-source |
| NF-05 | MEDIUM | Core | MISALIGNED | Validate.h refuses crop width/height 0, which the engine defines as 'extend to the image edge' | verified-by-source |
| NF-06 | MEDIUM | Core | MISSING | `--no-loopcount` (play once) is unrepresentable in every surface | verified-by-source |
| NF-07 | MEDIUM | Web server | MISALIGNED | `/run` and `/optimize` accept `info:true`, then always answer a misleading 422 'engine produced invalid GIF output' with exitCode 0 | verified-by-source |
| NF-08 | MEDIUM | Build/Discovery | BROKEN | CLI invoked through a symlink or bare PATH name loses 'engine beside the executable' discovery | verified-by-source |
| NF-09 | MEDIUM | Build/Discovery | MISALIGNED | Desktop engine discovery is pinned to GS_VERSION while the web picks the newest release dir — a VERSION.md bump silently breaks CLI/GUI | source-read, needs runtime probe |
| NF-10 | LOW | Web server | BROKEN | serveStatic() relies on a raw string-prefix containment check (shielded only by URL dot-segment normalisation); serves server source/tests; HEAD returns a body | verified-by-source |
| NF-11 | LOW | Web server | MISALIGNED | Oversized bodies answer 400 'bad JSON request body' (/run) or 500 (/optimize) instead of 413; real /run GIF cap is ≈48 MB, not 64 MB | verified-by-source |
| NF-12 | LOW | Web UI | MISALIGNED | After a failed run the previous run's After image and download links stay on screen under a 'Failed —' status | verified-by-source |
| NF-13 | LOW | GUI (Qt) | MISALIGNED | Preview engine check bypasses the U-07/N-04 UTF-8 path boundary | verified-by-source |
| NF-14 | LOW | Core | MISALIGNED | Windows exit code is masked with `& 0xff` — NTSTATUS crash codes whose low byte is 0 collapse to 'success' | verified-by-source |
| NF-15 | LOW | GUI (Qt) | MISALIGNED | cancelling_ is cleared after a 3 s bounded wait — a slow kill produces a spurious 'Optimization failed' dialog after 'Cancelled.' | source-read, needs runtime probe |
| NF-16 | LOW | CLI | MISALIGNED | resolve_path() silently falls back to the CWD, contradicting its own 'CWD-independent' contract | verified-by-source |
| NF-17 | LOW | CLI | MISSING | Batch + `output` + N>1 inputs passes the planner in 'merge shape' — engine semantics of `-b a b -o out` are undocumented and never probed | source-read, needs runtime probe |
| NF-18 | LOW | Core | MISSING | `-E` (explode by name) is exposed but `--name` is not; several whole-GIF engine options are absent | verified-by-source |
| NF-19 | LOW | CLI | MISALIGNED | Explode default prefix differs per surface (GUI/web: `<dir>/<stem>_frame`; CLI: engine default `<basename>` in the CWD) | verified-by-source |

## 2. Findings

### NF-01 · [HIGH] Batch continuation re-reads LIVE settings for every file after the first

| Field | Value |
|---|---|
| Area | GUI (Qt) |
| Class | BROKEN |
| Confidence | verified-by-source |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |
| Nearest existing row | U-45 (output group lock) and U-01 (target plan) — neither covers settings snapshotting. |

**Files**
- `working_code/gifscythe/src/qtui/MainWindow.cpp — onProcessFinished() batch branch`
- `MainWindow.h — no settings snapshot member`

**Finding.** runCommand() plans TARGETS once (batchTargets_, U-01 fix) but the SETTINGS are not snapshotted. Every subsequent batch item calls currentSettings() again, so the Actions tab is a time-of-use input for files 2..N. U-45 locked the *output group* (Browse buttons); nothing in MainWindow.h suggests the SettingsPanel is frozen or that a per-run settings copy exists. Result: one batch can silently apply different optimisation/colour/resize settings to different files while the UI reports one 'Optimization complete — N file(s)'.

**Evidence**
```
// onProcessFinished(), batch continuation:
++batchIndex_;
if (batchIndex_ < batchQueue_.size()) {
  QString in = batchQueue_.at(batchIndex_);
  pendingOutput_ = batchTargets_.value(batchIndex_);  // from the plan  <-- targets are planned
  auto settings = currentSettings();                    // <-- settings are NOT: read again, live
  gs::Settings one = settings; one.mode = gs::Mode::Auto; ...
  process_->start();

// MainWindow.h run-state members: enginePath_, inputs_, process_, busy_, cancelling_,
// pendingOutput_, batchTargets_, batchIndex_, batchQueue_, batchMode_, explodeSnapshot_
// -> there is no 'batchSettings_' / plan-of-record for settings.
```

**Reproduction**
1. Queue three large GIFs (several MB each), Mode = Batch, Optimize = 1.
2. Click Optimize GIF. While file 1 is encoding, open the Actions tab and set Optimize = 3 and Colors = 16 (verify whether the controls are enabled — setBusy() was not visible in the fetched excerpt; if they are enabled the bug is user-reachable, if disabled it is still a design hole).
3. Compare the three <name>_opt.gif sizes / `gifsicle --info` colour tables: file 1 uses the old settings, files 2–3 the new ones. The command pane shows only the last argv.

**Proposed fix.** In runCommand(): `batchSettings_ = currentSettings();` once, next to batchTargets_, and use it in the continuation branch. Additionally disable settingsPanel_ (setEnabled(!busy)) in setBusy() so the UI matches the plan-of-record. Add harness case: change a control mid-batch, assert argv of run #2 equals run #1 (T18-style).

---
### NF-02 · [HIGH] Cancel (or engine failure) leaves a truncated file over a PRE-EXISTING output — previous good result destroyed

| Field | Value |
|---|---|
| Area | GUI (Qt) |
| Class | BROKEN |
| Confidence | verified-by-source |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |
| Nearest existing row | U-01/U-45 (planning), GS-203 (post-conditions) — none address partial-output cleanup on cancel. |

**Files**
- `src/qtui/MainWindow.cpp — cancelRun(), onProcessFinished() failure path`
- `src/cli/main.cpp — after run_argv() rc != 0`
- `src/core/OutputPlan.h header comment (temp+rename explicitly declined)`

**Finding.** gifsicle writes straight to `-o <target>`. Cancel kills the engine mid-write; the failure branch shows a dialog but never removes or restores the partial target. OutputPlan.h reports pre-existing targets as 'normal, re-running an optimize is expected' and declines temp+rename because it would 'only protect a previous output from a crashed engine'. Cancel is a first-class button, not a crash: re-optimising an existing <name>_opt.gif and pressing Cancel replaces the user's last good result with a 0-byte/truncated file, and nothing says so. Same class as U-01 (silent data loss) but through a different door.

**Evidence**
```
void MainWindow::cancelRun() {
  cancelling_ = true;
  if (process_ && process_->state() != QProcess::NotRunning) { process_->kill(); process_->waitForFinished(3000); }
  cancelling_ = false; batchQueue_.clear(); batchIndex_ = -1; invalidatePreview(); setBusy(false);
  updateStatus(QStringLiteral("Cancelled."));   // pendingOutput_ is never inspected or cleaned
}
// OutputPlan.h: "Not implemented here, and why: temp-file + rename. It would only protect a PREVIOUS
// output from a crashed engine ..."
```

**Reproduction**
1. Optimize a large GIF to out.gif once (success). Note its size.
2. Run again to the same out.gif and Cancel. Note: gifsicle writes the output only after processing, so the destructive window is the WRITE phase — use a large output on a slow disk, or make it deterministic with a stub engine that opens -o, truncates, sleeps 5 s, then writes (the repo already uses stub engines for U-17).
3. `ls -l out.gif` → 0 bytes or truncated; `gifsicle --info out.gif` → error. Status bar says only 'Cancelled.' and no dialog mentions the file.
4. CLI equivalent: `gifscythe-cli conf --run` then kill the engine during the write → same partial file left; rc reflects the signal (U-32) but the file is not cleaned or reported.

**Proposed fix.** Minimum: after cancel or rc≠0, if the pre-run snapshot (already taken for GS-203 in CLI; take one in GUI) said the target did NOT exist → delete the partial file; if it DID exist → tell the user explicitly it is now damaged. Proper: run the engine to `<target>.gs-partial`, verify, then rename over the target (atomic on same FS). The live pane can show the real argv including the temp name plus a one-line note — honesty is preserved. Add harness/smoke cases: cancel-with-preexisting-target keeps old bytes.

---
### NF-03 · [MEDIUM] CLI turns gifsicle frame selections (`#0`, `#0-2`, `#name`) into bogus file paths

| Field | Value |
|---|---|
| Area | CLI |
| Class | BROKEN |
| Confidence | verified-by-source |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |
| Nearest existing row | None. GS-206 is about numeric domains, not input syntax. |

**Files**
- `src/cli/main.cpp — resolve_path() applied to every s.inputs entry`
- `src/core/GifsicleSettings.h — `inputs; // GIF files or frames "#0", etc.``

**Finding.** GifsicleSettings documents frame selections as legal `input` values, and the engine man page defines them ('frame selections start with #'). main.cpp resolves every input relative to the conf directory, so `#0` becomes `/abs/dir/#0` and the engine fails with 'No such file'. The header advertises a feature the CLI structurally breaks; OutputPlan/validate also count selections as inputs.

**Evidence**
```
// main.cpp
for (auto& in : s.inputs) in = resolve_path(in, base_dir);   // no '#' / '-' exemption
// resolve_path(): candidate = base_dir / path; ... return path_u8string(candidate);
// GifsicleSettings.h
std::vector<std::string> inputs;   // GIF files or frames "#0", etc.
```

**Reproduction**
1. conf: `input = anim.gif` / `input = #0` / `output = first.gif`
2. `gifscythe-cli conf` (print mode) → argv shows `/…/#0` instead of `#0`.
3. `--run` → engine error, rc=1.

**Proposed fix.** In main.cpp skip resolution when the value starts with `#` (frame selection) or equals `-` (stdin). Exclude selections from plan_outputs() input keys and from the 'multi-input explode' count. Or delete the claim from GifsicleSettings.h and reject `#…` in SettingsIO with a warning. Add a smoke case.

---
### NF-04 · [MEDIUM] `output = -` (engine's documented stdout name) is treated as a file → false failure rc=1 after a successful run

| Field | Value |
|---|---|
| Area | CLI |
| Class | BROKEN |
| Confidence | verified-by-source |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |

**Files**
- `src/cli/main.cpp — verify_file / plan_outputs branch`
- `src/core/OutputPlan.h path_key()`

**Finding.** The man page: '-o file … The special filename - means the standard output'. The CLI plans `-` as a CWD file (`path_key("-")`), snapshots it, streams the GIF to stdout correctly, then verify_output("-") reports 'engine produced no output file' and the exit code becomes 1. Scripts piping output get a valid GIF and a failure code. Input `-` (stdin) is likewise resolved to `/dir/-`.

**Evidence**
```
const bool verify_file = !s.output.empty() && s.mode != gs::Mode::Explode && !s.info;  // "-" is non-empty
...
if (rc == 0 && verify_file) { const auto error = gs::verify_output(s.output, output_before); if (!error.empty()) { ... rc = 1; } }
```

**Reproduction**
1. conf: `input = a.gif` / `output = -`
2. `gifscythe-cli conf --run > out.gif; echo $?` → out.gif is a valid GIF, exit code 1, stderr 'output verification failed: -: engine produced no output file'.

**Proposed fix.** Normalise `output == "-"` to the streaming contract (same path as empty output) before planning/verification; treat input `-` as stdin (skip resolve). GUI/web should refuse `-` explicitly.

---
### NF-05 · [MEDIUM] Validate.h refuses crop width/height 0, which the engine defines as 'extend to the image edge'

| Field | Value |
|---|---|
| Area | Core |
| Class | MISALIGNED |
| Confidence | verified-by-source |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |

**Files**
- `src/core/Validate.h — crop rule`
- `src/core/GifsicleSettings.h — crop_w/crop_h are unsigned`
- `web/validate.mjs (mirror, parity-pinned)`

**Finding.** Man page for `--crop x1,y1+widthxheight`: 'width and height can be zero or negative. A zero dimension means the cropping area goes to the edge of the image; a negative dimension brings it back from the edge'. The validator rejects 0x? / ?x0 outright (GUI refuses to run, CLI warns, web 422), and the unsigned fields make the documented negative form unrepresentable. This is a false refusal of engine-valid input, the mirror image of the U-22 class.

**Evidence**
```
if (s.crop && (s.crop_w == 0 || s.crop_h == 0)) {
  add("crop", "0x0", "crop width/height must be > 0");
}
```

**Reproduction**
1. conf: `crop = true`, `crop_x = 2`, `crop_y = 2`, `crop_w = 0`, `crop_h = 0` (= 'shave 2px top/left, keep to the edge').
2. `gifscythe-cli conf --strict` → rc=3 refusal; run `gifsicle --crop 2,2+0x0 in.gif -o out.gif` directly → rc=0, valid output.

**Proposed fix.** Allow 0 (and consider int + negative) for crop_w/crop_h; keep refusing only the truly invalid (x/y beyond image is an engine-side check). Update the JS mirror and the parity fixture in the same commit (gate G-parity).

---
### NF-06 · [MEDIUM] `--no-loopcount` (play once) is unrepresentable in every surface

| Field | Value |
|---|---|
| Area | Core |
| Class | MISSING |
| Confidence | verified-by-source |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |

**Files**
- `src/core/GifsicleSettings.h — `int loopcount = -1; // -1 unchanged, 0 forever, >0 count``
- `src/core/GifsicleCommand.h`
- `web/command.mjs`
- `SettingsPanel.h loopCombo_`

**Finding.** The engine has three states: unchanged (--same), forever (--loopcount / =0), N, and OFF (`--no-loopcount`, 'show every frame once'). The model only has unchanged / forever / N. A user who wants a non-looping GIF cannot get one from GUI, CLI conf or web. The UI copy 'Loop' with keep/forever/N hides that the fourth engine state exists.

**Evidence**
```
if (s.loopcount == 0) add(args_, "--loopcount=0"); else if (s.loopcount > 0) add(args_, "--loopcount=" + i2s(s.loopcount));
// nothing ever emits --no-loopcount
```

**Reproduction**
1. Try to produce a GIF that plays once from any surface; inspect argv: no `--no-loopcount` is possible.

**Proposed fix.** Add an explicit tri-state+off: e.g. `loopcount = -2` → `--no-loopcount` (documented in SettingsIO as `loopcount = off`), a 'Play once' item in loopCombo_, and a `once` option in the web select. Mirror in command.mjs + parity fixture.

---
### NF-07 · [MEDIUM] `/run` and `/optimize` accept `info:true`, then always answer a misleading 422 'engine produced invalid GIF output' with exitCode 0

| Field | Value |
|---|---|
| Area | Web server |
| Class | MISALIGNED |
| Confidence | verified-by-source |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |

**Files**
- `web/server.mjs — handleOptimize()/runOne() → verifyOutput()`
- `src/cli/main.cpp — `verify_file = … && !s.info` (CLI exempts info; web does not)`

**Finding.** `--info -o file` makes gifsicle write TEXT into the output (man page: 'suppresses normal output'). The CLI explicitly exempts info from output verification. The web transport does not: validate.mjs lets `info` through in auto mode, the engine exits 0, verifyOutput sees no GIF87a/89a signature and the API blames the engine ('exited 0 but produced invalid GIF output'). Not exposed in app.js controls, but the JSON API is a supported surface (S14: web is a product alternative).

**Evidence**
```
// server.mjs runOne():
const verified = await verifyOutput(outPath, before);
if (verified.error) { sendJson(res, 422, { ok:false, exitCode:0, stderr: verified.error ... }); }
// main.cpp:
const bool verify_file = !s.output.empty() && s.mode != gs::Mode::Explode && !s.info;
```

**Reproduction**
1. `curl -s -X POST localhost:8000/run -H 'content-type: application/json' -d '{"settings":{"mode":"auto","info":true},"files":[{"name":"a.gif","data":"<b64>"}]}'`
2. → 422 { exitCode: 0, stderr: 'the engine exited 0 but produced invalid GIF output …' } although the engine did exactly what was asked.

**Proposed fix.** Reject `info` (and `-II`-style requests) at web validation with a 400 that names the reason ('info output is text; not supported by this transport'), or return the info text as text/plain. Add a transport test.

---
### NF-08 · [MEDIUM] CLI invoked through a symlink or bare PATH name loses 'engine beside the executable' discovery

| Field | Value |
|---|---|
| Area | Build/Discovery |
| Class | BROKEN |
| Confidence | verified-by-source |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |
| Nearest existing row | U-05 fixed PATH search; this is the step-2 'beside the executable' path. |

**Files**
- `src/cli/main.cpp — exe_path_of(argv0)`
- `src/core/EngineLocator.h — resolve_engine(exe_path)`

**Finding.** exe_path_of() trusts argv[0]: a symlink is not resolved (exe_dir becomes the symlink's directory) and a bare name that does not exist in the CWD makes resolve_engine() fall back to exe_dir = CWD. The packaged layout ('engine next to the binary', README/portable zip) therefore fails whenever the CLI is installed via `ln -s` into /usr/local/bin, or launched by a wrapper with argv[0] rewritten. The GUI is unaffected (QCoreApplication::applicationFilePath()).

**Evidence**
```
fs::path exe_path_of(const char* argv0) {
  fs::path p = gs::u8path_compat(argv0);
  if (p.is_absolute()) return p;                       // symlink kept as-is
  if (fs::exists(p, ec)) return fs::absolute(p, ec);   // only if it exists relative to CWD
  return p;                                            // bare name -> EngineLocator uses CWD
}
```

**Reproduction**
1. Unpack the portable build to /opt/gs (gifscythe-cli + gifsicle side by side).
2. `ln -s /opt/gs/gifscythe-cli /usr/local/bin/gifscythe-cli; cd /tmp; gifscythe-cli conf --run`
3. → 'ERROR: engine not found … Searched … the folders next to this executable' — although it IS next to the real executable. (PATH fallback rescues only if /opt/gs itself is on PATH.)

**Proposed fix.** Resolve the real executable: `/proc/self/exe` (Linux), `_NSGetExecutablePath` (macOS), `GetModuleFileNameW` (Windows, already in WinUnicode territory), then fs::canonical(). Smoke case: symlinked CLI finds the sibling engine.

---
### NF-09 · [MEDIUM] Desktop engine discovery is pinned to GS_VERSION while the web picks the newest release dir — a VERSION.md bump silently breaks CLI/GUI

| Field | Value |
|---|---|
| Area | Build/Discovery |
| Class | MISALIGNED |
| Confidence | source-read, needs runtime probe |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |

**Files**
- `src/core/EngineLocator.h — every candidate uses `release/<GS_VERSION>/``
- `scripts/build_engine.sh — writes release/<VERSION.md>/`
- `web/server.mjs findEngine() — newest numeric dir`

**Finding.** W-29 (bump to 0.2.0 / 1.0.0) is on the board. After editing VERSION.md the CLI/GUI look only in release/0.2.0/, which does not exist until the engine is rebuilt, while the web server happily keeps using release/0.1.0/. Two discovery policies for the same binary; the desktop one fails closed on a pure version bump with the message 'build it with ./scripts/build_engine.sh' even though a valid engine sits one folder over.

**Evidence**
```
candidates.push_back(exe_dir / ".." / "release" / GS_VERSION / base);   // EngineLocator.h
mkdir -p "$OUT/$VERSION"   # build_engine.sh — VERSION grep'd from VERSION.md
versions.sort(...newest first...)  // server.mjs — any version
```

**Reproduction**
1. With release/0.1.0/gifsicle present, change VERSION.md to 0.2.0, rebuild only the CLI (`./build.sh` — check whether it re-runs build_engine.sh; if not, this reproduces directly).
2. `./build/gifscythe-cli examples/animation.conf --run` → engine not found; `node web/server.mjs` → Engine [release]: …/release/0.1.0/gifsicle.

**Proposed fix.** Pick one policy and share it: version-independent `release/engine/` (or `release/current` symlink written by build_engine.sh), plus a 'newest release/*' fallback in EngineLocator that logs which dir was chosen. Add to RELEASE_PROCEDURE §2 as a step on version bump.

---
### NF-10 · [LOW] serveStatic() relies on a raw string-prefix containment check (shielded only by URL dot-segment normalisation); serves server source/tests; HEAD returns a body

| Field | Value |
|---|---|
| Area | Web server |
| Class | BROKEN |
| Confidence | verified-by-source |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |
| Nearest existing row | GS-202 (upload containment) — static path was out of its scope. |

**Files**
- `web/server.mjs — serveStatic(), createServer handler`
- `web/run-paths.mjs — assertContainedPath() (the correct helper already exists)`

**Finding.** `file.startsWith(ROOT)` with ROOT=/repo/web would admit /repo/web-anything/…. Self-check during this review: the request handler builds `new URL(req.url, 'http://localhost')` first, and the WHATWG parser collapses `..` and `%2e%2e` segments, so the weak check is NOT reachable over HTTP today — this is hygiene, not an exploit. It is still the exact pattern GS-202 replaced for uploads with a path.relative-based check, and a future refactor that passes a raw path would re-open it. Concrete today: every file under web/ (server.mjs, run-paths.mjs, test fixtures, README) is served to any client, and HEAD requests get a full body.

**Evidence**
```
const url = new URL(req.url || "/", "http://localhost");   // dot-segments already collapsed here
...
const file = normalize(join(ROOT, rel));
if (!file.startsWith(ROOT)) { 403 }   // '/repo/web' is a prefix of '/repo/web-x' — latent
```

**Reproduction**
1. `curl -I localhost:8000/server.mjs` → 200 with the server source (and a body on HEAD).
2. Latent path: unit-call serveStatic(res, '/../web-x/s.txt') directly (bypassing new URL()) with a sibling web-x/ present → 200.

**Proposed fix.** Reuse assertContainedPath(ROOT, file) from run-paths.mjs; allow-list served files (index.html, app.js, style.css, command.mjs, validate.mjs); return headers only for HEAD.

---
### NF-11 · [LOW] Oversized bodies answer 400 'bad JSON request body' (/run) or 500 (/optimize) instead of 413; real /run GIF cap is ≈48 MB, not 64 MB

| Field | Value |
|---|---|
| Area | Web server |
| Class | MISALIGNED |
| Confidence | verified-by-source |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |

**Files**
- `web/server.mjs — readBody(), handleRun() catch, handleOptimize() catch`

**Finding.** readBody rejects with a generic Error; handleRun's try/catch converts it into 'bad JSON request body', hiding the real cause. MAX_BODY applies to the JSON envelope, so base64 (+33%) plus JSON overhead lowers the effective upload to ~48 MB while README/server comment say 64 MB.

**Evidence**
```
try { const raw = await readBody(req, MAX_BODY); payload = JSON.parse(...) } catch { sendJson(res, 400, { error: "bad JSON request body" }); }
```

**Reproduction**
1. POST /run with a 50 MB GIF → 400 'bad JSON request body'.

**Proposed fix.** Throw a typed error (status 413) from readBody and map it; document the effective limit or raise MAX_BODY for /run to 64 MB × 4/3 + slack.

---
### NF-12 · [LOW] After a failed run the previous run's After image and download links stay on screen under a 'Failed —' status

| Field | Value |
|---|---|
| Area | Web UI |
| Class | MISALIGNED |
| Confidence | verified-by-source |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |

**Files**
- `web/app.js — run click handler, `!resp.ok || !body.ok` branch`

**Finding.** revokeResults()/hiding #outputs happen only on the success path and on queue change. Changing a setting and re-running to a 422 leaves the OLD results visible next to the new failure message — a stale-visual-result issue of the U-47 class, on the web.

**Evidence**
```
if (!resp.ok || !body || !body.ok) { $("status").textContent = 'Failed — …'; run.disabled = false; return; }  // outputs/after untouched
```

**Reproduction**
1. Run once successfully; set Colors on with value 1 (validate 422); Run → status Failed, After pane and links still show the previous result.

**Proposed fix.** Call revokeResults(); hide #outputs; clear #after before every run (or on failure).

---
### NF-13 · [LOW] Preview engine check bypasses the U-07/N-04 UTF-8 path boundary

| Field | Value |
|---|---|
| Area | GUI (Qt) |
| Class | MISALIGNED |
| Confidence | verified-by-source |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |
| Nearest existing row | N-04 closed 'every core boundary'; this is a qtui boundary. |

**Files**
- `src/qtui/MainWindow.cpp — startPreview()`

**Finding.** ensureEngine() wraps the path in gs::u8path_compat(); startPreview() passes `enginePath_.toStdString()` straight into path_is_executable(const fs::path&), i.e. the narrow fs::path conversion that N-04 documented as byte-mangling on MinGW. On Windows with a non-ASCII engine path the main run works but the preview says 'engine not found — preview unavailable'.

**Evidence**
```
// ensureEngine():   gs::path_is_executable(gs::u8path_compat(enginePath_.toStdString()))
// startPreview():   if (!gs::path_is_executable(enginePath_.toStdString())) { clearAfter("engine not found — preview unavailable"); return; }
```

**Reproduction**
1. Windows, portable folder under C:\Users\José\…: run succeeds, preview pane stays 'engine not found'. (Wine reproduction possible with the S11 harness.)

**Proposed fix.** One-liner: wrap in gs::u8path_compat(). Grep for other `.toStdString()` → fs::path boundaries (verify_audit gate candidate).

---
### NF-14 · [LOW] Windows exit code is masked with `& 0xff` — NTSTATUS crash codes whose low byte is 0 collapse to 'success'

| Field | Value |
|---|---|
| Area | Core |
| Class | MISALIGNED |
| Confidence | verified-by-source |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |

**Files**
- `src/core/ProcessRunner.h — run_argv() Windows branch`

**Finding.** Windows exit codes are 32-bit; a crashed child returns an NTSTATUS such as 0xC0000005. Masking to the low byte keeps most non-zero, but any status ending in 0x00 becomes 0 and the 'honest exit code' contract is broken exactly in the crash case. The information (which fault) is also lost for the user-facing message.

**Evidence**
```
return static_cast<int>(code) & 0xff;
```

**Reproduction**
1. Needs a child exiting with e.g. 0x100 or an NTSTATUS ending in 00; a test stub `exit(256)` under Wine shows rc=0 from run_argv.

**Proposed fix.** `if (code == 0) return 0; const int low = code & 0xff; return low ? low : 1;` and log the raw hex on stderr when code > 255.

---
### NF-15 · [LOW] cancelling_ is cleared after a 3 s bounded wait — a slow kill produces a spurious 'Optimization failed' dialog after 'Cancelled.'

| Field | Value |
|---|---|
| Area | GUI (Qt) |
| Class | MISALIGNED |
| Confidence | source-read, needs runtime probe |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |
| Nearest existing row | U-12 / P1-24 (UI-thread waits). |

**Files**
- `src/qtui/MainWindow.cpp — cancelRun(), onProcessFinished()`

**Finding.** cancelRun() resets cancelling_ = false right after waitForFinished(3000). If the engine takes longer to die (large file flush on Windows, AV scanning), finished() arrives later with cancelling_ == false and the failure branch shows 'The GIF engine returned an error (exit …)'. Related to the U-12 blocking waits but a distinct state bug.

**Evidence**
```
process_->kill(); process_->waitForFinished(3000);
cancelling_ = false;   // cleared regardless of whether finished() was delivered
```

**Reproduction**
1. Hard to force on Linux; on Windows with a huge output on a slow disk: Cancel → 'Cancelled.' then a failure dialog.

**Proposed fix.** Clear cancelling_ inside onProcessFinished()/onProcessError() (the point where the kill is actually observed), not in cancelRun(). Fold into P1-24.

---
### NF-16 · [LOW] resolve_path() silently falls back to the CWD, contradicting its own 'CWD-independent' contract

| Field | Value |
|---|---|
| Area | CLI |
| Class | MISALIGNED |
| Confidence | verified-by-source |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |

**Files**
- `src/cli/main.cpp — resolve_path()`

**Finding.** A relative input that is missing next to the conf but exists in the current directory is picked up from the CWD. The same conf then produces different runs from different directories — the exact property the 'absolutize the settings path so everything is CWD-independent' comment claims to rule out.

**Evidence**
```
if (fs::exists(candidate, ec)) return …candidate…;   // conf-relative
if (fs::exists(path, ec))      return …path…;        // CWD-relative fallback
```

**Reproduction**
1. conf in /a with `input = x.gif` (absent in /a); `cd /b` (which has x.gif); run → /b/x.gif is optimised without a note.

**Proposed fix.** Drop the CWD fallback (fail with 'input not found next to the conf'), or print a NOTE naming the resolution.

---
### NF-17 · [LOW] Batch + `output` + N>1 inputs passes the planner in 'merge shape' — engine semantics of `-b a b -o out` are undocumented and never probed

| Field | Value |
|---|---|
| Area | CLI |
| Class | MISSING |
| Confidence | source-read, needs runtime probe |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |
| Nearest existing row | GS-201 (Batch with NO output). |

**Files**
- `src/cli/main.cpp — plan_outputs(s.inputs, {s.output}) for Mode::Batch`
- `src/core/OutputPlan.h — one_target shape`

**Finding.** GS-201 made the CLI refuse Batch WITHOUT output. Batch WITH one output and several inputs is accepted because plan_outputs treats (N inputs → 1 output) as the legal merge shape. The gifsicle man page defines -b as 'modify each input in place' and says nothing about -o in batch mode, so the CLI allows a command whose outcome nobody has pinned (does the first file go to out and the rest in place? are all rewritten in place?). Either outcome contradicts the one target the user named.

**Evidence**
```
if (!s.output.empty() && s.mode != gs::Mode::Explode) {
  const gs::OutputPlan plan = gs::plan_outputs(s.inputs, {s.output});   // N->1 accepted for Batch too
```

**Reproduction**
1. conf: `mode = batch`, `input = a.gif`, `input = b.gif`, `output = out.gif`; run with copies; inspect which files changed. Pin the observed behaviour in a smoke case.

**Proposed fix.** Refuse Batch with >1 input and a single `output` in the CLI (the GUI never emits -b at all: it runs per-file Auto). Long term, make the CLI batch identical to the GUI/web batch (per-file Auto runs with a template) and retire -b entirely.

---
### NF-18 · [LOW] `-E` (explode by name) is exposed but `--name` is not; several whole-GIF engine options are absent

| Field | Value |
|---|---|
| Area | Core |
| Class | MISSING |
| Confidence | verified-by-source |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |

**Files**
- `src/core/GifsicleSettings.h`
- `SettingsPanel.h explodeByNameCheck_`

**Finding.** Without `--name` the only way -E differs from -e is an input that already carries name extensions, so the checkbox is mostly inert. Not bugs, but gaps relative to the '~30 engine-truth controls' claim: `--name`, `--no-loopcount` (NF-06), `-II` (info to stderr), `--resize-colors`, `--resize-geometry`, `--resize-fit-width/-height`, `--logical-screen`, `--conserve-memory`, `--no-warnings`, `--use-colormap`, `--change-color`, `--app-extension`. Worth listing in the UI/README as 'not exposed' so the 'engine truth' claim stays honest.

**Evidence**
```
bool explode_by_name = false;      // -E   (no 'names' field anywhere in Settings)
```

**Reproduction**
1. Enable 'Explode by name' on a GIF without named frames → output identical to -e.

**Proposed fix.** Either add a per-frame `--name` list next to comments, or grey out -E with a tooltip 'only affects inputs with named frames'. Add a 'Not exposed' list to README.

---
### NF-19 · [LOW] Explode default prefix differs per surface (GUI/web: `<dir>/<stem>_frame`; CLI: engine default `<basename>` in the CWD)

| Field | Value |
|---|---|
| Area | CLI |
| Class | MISALIGNED |
| Confidence | verified-by-source |
| Register state (repo vocabulary) | UNTRIAGED — not in COMPILED_AUDIT §5/§13 or STATUS.md |

**Files**
- `src/qtui/MainWindow.cpp runCommand() explode branch`
- `web/server.mjs targets`
- `src/core/ExplodeVerify.h explode_prefix_for()`

**Finding.** The GUI persists sessions with `output` cleared. A conf exported from the GUI state and run by the CLI in Explode mode scatters `<basename>.NNN` into whatever directory the shell is in, while the GUI would have written `<dir>/<stem>_frame.NNN` next to the input. Documented in the CLI, but the three surfaces should agree.

**Evidence**
```
// GUI: settings.output = fi.absolutePath() + "/" + fi.completeBaseName() + "_frame"
// CLI: explode_prefix_for(): if output empty -> first input's basename (CWD)
```

**Reproduction**
1. GUI: explode a.gif → /path/a_frame.000; CLI with same conf minus output, from /tmp → /tmp/a.gif.000.

**Proposed fix.** CLI: default explode prefix to `<input dir>/<stem>_frame` like the GUI/web, and say so in print mode.

---

## 3. Planning recommendations

### 1 · Language / stack — keep C++17 core, stop adding surfaces

**Verdict.** Do NOT migrate before 1.0.0. The core (src/core/*.h) is small, header-only, Qt-free, and has 296 unit checks plus JS parity fixtures. Rewriting it buys nothing; the risk is in the surfaces, not the language.

- Currently five surfaces exist or are scaffolded: Qt6 GUI, CLI, Node web server, web/wasm (no binary yet), csharp/spike (parked), plus a Rust+Tauri spike trigger (D-08). For a 0.1.0 product that is scope creep — every extra surface re-opens the same honesty bugs (this review found the same class in GUI, CLI and web independently).
- Recommendation: freeze web at 'internal tool' status until 1.0.0 ships on desktop, delete csharp/spike from main (keep in a branch), and mark web/wasm as post-1.0. One shippable artefact = Windows portable zip (OD-17 already says Windows-only).
- If a rewrite is ever justified (post-1.0, WebP/APNG frame model), the fastest-to-ship option that keeps the subprocess model and packaging story is Rust + egui/Tauri or Go + Wails for the shell, with gifsicle still a subprocess. Python/Electron would be faster to write but contradict the offline/portable/no-runtime goal in OFFLINE_BUILD_REVIEW.
- For the frame model (2.x) prefer linking libwebp/libpng directly from C++ over introducing a second language; the Ms-PL/GPL boundary already forces subprocess isolation for gifsicle only.

### 2 · Fix order for the new findings (evidence first, then code)

**Verdict.** Two HIGHs are data-integrity bugs of the U-01 class and should be closed before any release re-cut (U-09).

- P0: NF-01 settings snapshot for batch (30 lines + one harness case). NF-02 partial-output handling on cancel/failure (snapshot exists already for GS-203; deleting a partial when the target did not pre-exist is ~20 lines; temp+rename is the proper fix and can share the live-pane note).
- P1: NF-03/NF-04 CLI input/output syntax (frame selections, '-') — small, smoke-testable; NF-05 crop-zero false refusal (needs JS mirror + fixture); NF-07 web info 422; NF-08 real exe path; NF-09 unify engine discovery before the version bump (W-29).
- P2: NF-06 --no-loopcount, NF-10/11/12 web hygiene, NF-13 preview path boundary, NF-14 exit-code mask, NF-15 cancelling_ lifetime, NF-16/17/19 CLI consistency, NF-18 doc honesty.
- Every fix ships with the same discipline the repo already uses: failing test first, executed proof in the row, no narrative-only closes.

### 3 · Process — the documentation machine is now a cost centre

**Verdict.** COMPILED_AUDIT.md (132 KB) + IMPROVEMENT_LOG.md (146 KB) + SESSION_HANDOFF.md (55 KB) + WORKLIST (41 KB) + 18 doc gates. Main went red twice from doc drift alone (GS-208, N-01). That is more text than the product source.

- Keep exactly two living documents: STATUS.md (generated register) and a CHANGELOG.md. Archive COMPILED_AUDIT/IMPROVEMENT_LOG/SESSION_HANDOFF under docs/archive as dated snapshots and stop editing them; new findings go straight into STATUS.md rows with a link to a PR.
- Keep gates that protect code (parity fixtures, smoke, harness, packaging negatives). Drop gates that only police prose counts (S2/S4/G10 base-commit line). A CI that fails because a SHA in a paragraph is stale is not protecting users.
- Adopt one issue tracker (GitHub Issues) with labels = the four states; the register can be regenerated from issues instead of from a 130 KB markdown file.

### 4 · Path to 0.2.0 → 1.0.0 (concrete, in order)

**Verdict.** Ship a Windows 0.2.0 within one or two sessions; use it to collect the real-desktop evidence the sandbox cannot produce.

- 0.2.0: close NF-01/NF-02, GS-203 GUI integration (P1-25, same code path as NF-02), GS-205 input admission, re-cut artefacts (U-09), run CLEAN_WINDOWS_SMOKE.md once on a real VM (W-18/W-19). Tag, publish, done.
- 0.3.0: numeric domains tri-state (DS-06/07/09 + GS-206 + NF-06) in one PR; engine discovery unification (NF-08/09); CLI batch = per-file Auto like GUI (retire -b, closes NF-17).
- 1.0.0 criteria stay as PROJECT_VISION says (UI/UX done). Decide OD-11 then. Only after 1.0.0: frame model → WebP/APNG (D-01..D-04), wasm (D-07).
- Time box: if 1.0.0 is not reachable in ~5 more sessions at the current pace, the cause will be the surface count and doc overhead, not the language.

## 4. Ready-to-paste STATUS.md rows

| ID | Item | State | Session | Proof / Blocker | Next action |
|----|-------|-------|---------|-----------------|-------------|
| NF-01 | Batch continuation re-reads LIVE settings for every file after the first | UNTRIAGED | - | source-read only (verified-by-source); see intake NF-01 | write the failing smoke/harness case named in the intake, then scope |
| NF-02 | Cancel (or engine failure) leaves a truncated file over a PRE-EXISTING output — previous good result destroyed | UNTRIAGED | - | source-read only (verified-by-source); see intake NF-02 | write the failing smoke/harness case named in the intake, then scope |
| NF-03 | CLI turns gifsicle frame selections (`#0`, `#0-2`, `#name`) into bogus file paths | UNTRIAGED | - | source-read only (verified-by-source); see intake NF-03 | write the failing smoke/harness case named in the intake, then scope |
| NF-04 | `output = -` (engine's documented stdout name) is treated as a file → false failure rc=1 after a successful run | UNTRIAGED | - | source-read only (verified-by-source); see intake NF-04 | write the failing smoke/harness case named in the intake, then scope |
| NF-05 | Validate.h refuses crop width/height 0, which the engine defines as 'extend to the image edge' | UNTRIAGED | - | source-read only (verified-by-source); see intake NF-05 | write the failing smoke/harness case named in the intake, then scope |
| NF-06 | `--no-loopcount` (play once) is unrepresentable in every surface | UNTRIAGED | - | source-read only (verified-by-source); see intake NF-06 | write the failing smoke/harness case named in the intake, then scope |
| NF-07 | `/run` and `/optimize` accept `info:true`, then always answer a misleading 422 'engine produced invalid GIF output' with exitCode 0 | UNTRIAGED | - | source-read only (verified-by-source); see intake NF-07 | write the failing smoke/harness case named in the intake, then scope |
| NF-08 | CLI invoked through a symlink or bare PATH name loses 'engine beside the executable' discovery | UNTRIAGED | - | source-read only (verified-by-source); see intake NF-08 | write the failing smoke/harness case named in the intake, then scope |
| NF-09 | Desktop engine discovery is pinned to GS_VERSION while the web picks the newest release dir — a VERSION.md bump silently breaks CLI/GUI | UNTRIAGED | - | source-read only (source-read, needs runtime probe); see intake NF-09 | write the failing smoke/harness case named in the intake, then scope |
| NF-10 | serveStatic() relies on a raw string-prefix containment check (shielded only by URL dot-segment normalisation); serves server source/tests; HEAD returns a body | UNTRIAGED | - | source-read only (verified-by-source); see intake NF-10 | write the failing smoke/harness case named in the intake, then scope |
| NF-11 | Oversized bodies answer 400 'bad JSON request body' (/run) or 500 (/optimize) instead of 413; real /run GIF cap is ≈48 MB, not 64 MB | UNTRIAGED | - | source-read only (verified-by-source); see intake NF-11 | write the failing smoke/harness case named in the intake, then scope |
| NF-12 | After a failed run the previous run's After image and download links stay on screen under a 'Failed —' status | UNTRIAGED | - | source-read only (verified-by-source); see intake NF-12 | write the failing smoke/harness case named in the intake, then scope |
| NF-13 | Preview engine check bypasses the U-07/N-04 UTF-8 path boundary | UNTRIAGED | - | source-read only (verified-by-source); see intake NF-13 | write the failing smoke/harness case named in the intake, then scope |
| NF-14 | Windows exit code is masked with `& 0xff` — NTSTATUS crash codes whose low byte is 0 collapse to 'success' | UNTRIAGED | - | source-read only (verified-by-source); see intake NF-14 | write the failing smoke/harness case named in the intake, then scope |
| NF-15 | cancelling_ is cleared after a 3 s bounded wait — a slow kill produces a spurious 'Optimization failed' dialog after 'Cancelled.' | UNTRIAGED | - | source-read only (source-read, needs runtime probe); see intake NF-15 | write the failing smoke/harness case named in the intake, then scope |
| NF-16 | resolve_path() silently falls back to the CWD, contradicting its own 'CWD-independent' contract | UNTRIAGED | - | source-read only (verified-by-source); see intake NF-16 | write the failing smoke/harness case named in the intake, then scope |
| NF-17 | Batch + `output` + N>1 inputs passes the planner in 'merge shape' — engine semantics of `-b a b -o out` are undocumented and never probed | UNTRIAGED | - | source-read only (source-read, needs runtime probe); see intake NF-17 | write the failing smoke/harness case named in the intake, then scope |
| NF-18 | `-E` (explode by name) is exposed but `--name` is not; several whole-GIF engine options are absent | UNTRIAGED | - | source-read only (verified-by-source); see intake NF-18 | write the failing smoke/harness case named in the intake, then scope |
| NF-19 | Explode default prefix differs per surface (GUI/web: `<dir>/<stem>_frame`; CLI: engine default `<basename>` in the CWD) | UNTRIAGED | - | source-read only (verified-by-source); see intake NF-19 | write the failing smoke/harness case named in the intake, then scope |

## 5. Suggested regression cases (one per finding)

- **NF-01** — Compare the three <name>_opt.gif sizes / `gifsicle --info` colour tables: file 1 uses the old settings, files 2–3 the new ones. The command pane shows only the last argv.
- **NF-02** — CLI equivalent: `gifscythe-cli conf --run` then kill the engine during the write → same partial file left; rc reflects the signal (U-32) but the file is not cleaned or reported.
- **NF-03** — `--run` → engine error, rc=1.
- **NF-04** — `gifscythe-cli conf --run > out.gif; echo $?` → out.gif is a valid GIF, exit code 1, stderr 'output verification failed: -: engine produced no output file'.
- **NF-05** — `gifscythe-cli conf --strict` → rc=3 refusal; run `gifsicle --crop 2,2+0x0 in.gif -o out.gif` directly → rc=0, valid output.
- **NF-06** — Try to produce a GIF that plays once from any surface; inspect argv: no `--no-loopcount` is possible.
- **NF-07** — → 422 { exitCode: 0, stderr: 'the engine exited 0 but produced invalid GIF output …' } although the engine did exactly what was asked.
- **NF-08** — → 'ERROR: engine not found … Searched … the folders next to this executable' — although it IS next to the real executable. (PATH fallback rescues only if /opt/gs itself is on PATH.)
- **NF-09** — `./build/gifscythe-cli examples/animation.conf --run` → engine not found; `node web/server.mjs` → Engine [release]: …/release/0.1.0/gifsicle.
- **NF-10** — Latent path: unit-call serveStatic(res, '/../web-x/s.txt') directly (bypassing new URL()) with a sibling web-x/ present → 200.
- **NF-11** — POST /run with a 50 MB GIF → 400 'bad JSON request body'.
- **NF-12** — Run once successfully; set Colors on with value 1 (validate 422); Run → status Failed, After pane and links still show the previous result.
- **NF-13** — Windows, portable folder under C:\Users\José\…: run succeeds, preview pane stays 'engine not found'. (Wine reproduction possible with the S11 harness.)
- **NF-14** — Needs a child exiting with e.g. 0x100 or an NTSTATUS ending in 00; a test stub `exit(256)` under Wine shows rc=0 from run_argv.
- **NF-15** — Hard to force on Linux; on Windows with a huge output on a slow disk: Cancel → 'Cancelled.' then a failure dialog.
- **NF-16** — conf in /a with `input = x.gif` (absent in /a); `cd /b` (which has x.gif); run → /b/x.gif is optimised without a note.
- **NF-17** — conf: `mode = batch`, `input = a.gif`, `input = b.gif`, `output = out.gif`; run with copies; inspect which files changed. Pin the observed behaviour in a smoke case.
- **NF-18** — Enable 'Explode by name' on a GIF without named frames → output identical to -e.
- **NF-19** — GUI: explode a.gif → /path/a_frame.000; CLI with same conf minus output, from /tmp → /tmp/a.gif.000.

*End of intake.*