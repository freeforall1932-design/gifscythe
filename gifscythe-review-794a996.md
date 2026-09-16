# Independent Code Review — freeforall1932-design/gifscythe

**Repository:** https://github.com/freeforall1932-design/gifscythe  
**Reviewed commit:** `794a9964550fde0b7006474170926a6382c835f3` (Merge pull request #28 (2026-09-15 22:33:41 +0000))  
**Review date:** 2026-09-16 · **Reviewer:** Independent review (this report)
**Product:** Gifscythe 0.1.0 — GIF/APNG/WebP animation tool (C++17/Qt6 desktop + CLI + zero-dep Node web, gifsicle 1.96 subprocess engine)

> **Verdict:** the register's compiled audit ends at U-76. This review adds **U-77, U-78, U-79, U-80** — four new findings, three with live executed proof, all with ready-to-apply fixes and acceptance tests. No false positives: every candidate that reproduced as correct behavior is listed under "Cleared candidates" with its evidence.

---

## 1. Reproduction environment

| Key | Value |
|---|---|
| Reviewed commit | 794a996 (main, PR #28 — newest at review time) |
| Host | Linux 6.1.158 x86_64 (sandbox) |
| Node.js | v20.20.2 (web server + suites) |
| g++ | 12.2.0 (Debian 12.2.0-14+deb12u1) |
| bash | 5.2.15(1)-release |
| Product version | 0.1.0 (VERSION.md) |
| Method | full-tree read + static trace + live execution (build, unit/smoke/web suites, docs gates, HTTP probes against the real server) |

One-command baseline (exactly what this review ran before judging anything):

```bash
git clone https://github.com/freeforall1932-design/gifscythe.git && cd gifscythe
git checkout 794a9964550fde0b7006474170926a6382c835f3
cd working_code/gifscythe && ./build.sh            # engine + CLI + unit tests
./build/test_gifsicle_command                       # unit suite
./scripts/smoke_cli.sh                              # integration smoke
./scripts/check_docs.sh --no-gate-run               # docs register gate
./scripts/sweep_stale.sh                            # stale-claim sweep
cd ../.. && for t in command validate transport body-limit static-hygiene; do node web/test/$t.test.mjs; done
```

## 2. Verified baseline (measured, not quoted)

| Command | Result |
|---|---|
| `working_code/gifscythe/build.sh` | PASS — engine + CLI + unit tests built (rc=0) |
| `build/test_gifsicle_command` | PASS — 372 checks, 0 failures |
| `scripts/smoke_cli.sh` | PASS — 45 passed, 0 failed |
| `node web/test/command.test.mjs` | PASS (JS⇄C++ argv parity vs the real CLI) |
| `node web/test/validate.test.mjs` | PASS (validation parity) |
| `node web/test/transport.test.mjs` | PASS (67-case HTTP transport suite) |
| `node web/test/body-limit.test.mjs` | PASS (413 mapping, both endpoints) |
| `node web/test/static-hygiene.test.mjs` | PASS (static allow-list, HEAD contract) |
| `scripts/check_docs.sh --no-gate-run` | PASS — 23 passed, 0 failed, 2 skipped (STATUS.md is current) |
| `scripts/sweep_stale.sh` | PASS — 5 passed, 0 failed (no stale claims on the reviewed commit) |

## 3. Executive summary

The Gifscythe repository is in an unusually disciplined state: a 148-item machine-checked status register (112 DONE / 8 PARTIAL / 28 OPEN / 0 UNTRIAGED), parity-tested C++/JS mirrors, and five docs gates. Every gate and suite passes on the reviewed commit — verified by re-running them, not by trusting the docs.

After reading the full product surface (C++ core, CLI, Qt GUI, Node web, scripts, CI, and the audit docs) and probing the running web server, this review finds FOUR NEW defects the compiled audit has not recorded (its register ends at U-76). All four are in the web/tooling lane; none touches the frozen desktop core. Three are confirmed with live reproduction below; the fourth is a static, fail-closed brittleness.

All four fixes are small, isolated and ready to apply (unified diffs + acceptance tests included in this report). Combined patch size ≈ 40 lines + 6 regression tests, entirely inside web/, so they do not disturb the desktop release lane.

Planning verdict: the repo's own language decision is correct — stay on C++17/Qt6 desktop + zero-dependency Node web through 1.0.0. The realistic faster-to-ship lever is surface reduction and blocker order, not a language change. A concrete order is proposed in the Planning section.

---

# New findings (not in the compiled audit)

## U-77 (proposed) — POST /run with a JSON `null` body → HTTP 500 + raw TypeError leak (also /optimize?settings=null)

**State:** NEW — confirmed with executed proof · **Severity:** Medium · **Surface:** web / server.mjs

### Where

- `web/server.mjs` (452–458) — JSON.parse result is dereferenced as an object without a null guard — `payload.files` is the crash site
- `web/server.mjs` (255, 268) — second vector: `settings = JSON.parse(raw)` then `settings.info` — `settings=null` throws the same way
- `web/server.mjs` (689–693) — last-resort catch returns 500 text/plain with the internal error text — outside the documented JSON error shape

### Summary

RFC 8259 allows any JSON value at the top level. Every scalar sibling of `null` (42, "x", true) already fails closed with a documented 400 — but a literal `null` body reaches `Array.isArray(payload.files)`, throws a TypeError, and only the outermost catch answers: HTTP 500, Content-Type text/plain, body = the internal error message. Same crash on the legacy endpoint via `settings=null`. The server's own contract (header comment: usage errors are 400 JSON) and the web suite's 400-shape assertions make this an unintended path, not a documented one.

### Root cause

Two assignments trust JSON.parse to return an object: `const files = Array.isArray(payload.files)` (null has no properties) and `if (settings.info)`. JSON.parse("null") is the one top-level value that is falsy AND non-object.

### Impact

API consumers get an inconsistent contract (400 for 42/"x"/true, 500 for null) and the server leaks runtime internals in the response body. It is a crash-per-request path handled only by the emergency catch — the exact pattern this codebase calls a silent/dishonest failure class everywhere else. Low security weight on loopback, but it is one line of fuzzing away from being the loudest 500 in the project.

### Reproduce (executed during this review)

**Vector A — /run with body `null`**

```bash
node web/server.mjs 8000 &
curl -i -X POST http://127.0.0.1:8000/run \
  -H 'Content-Type: application/json' --data 'null'
```

- **Observed:** HTTP/1.1 500 Internal Server Error · Content-Type: text/plain · body: "TypeError: Cannot read properties of null (reading 'files')"
- **Expected:** HTTP 400 with the documented JSON shape: {"ok":false,"error":"bad JSON request body"} — same as a malformed body

**Vector B — /optimize?settings=null**

```bash
curl -i -X POST 'http://127.0.0.1:8000/optimize?settings=null' --data-binary @tiny.gif
```

- **Observed:** HTTP/1.1 500 Internal Server Error (throws at `settings.info`)
- **Expected:** HTTP 400 {"ok":false,"error":"bad settings JSON"}

### Proposed fix (ready to apply)

`web/server.mjs (handleRun, after the JSON.parse catch)`:

```diff
@@ handleRun, right after the parse try/catch @@
   try {
     payload = JSON.parse(raw.toString("utf8"));
   } catch {
     sendJson(res, 400, { ok: false, error: "bad JSON request body" });
     return;
   }
+  // U-77: JSON.parse also yields scalars. 42 / "x" / true already fail closed
+  // on the checks below, but null reaches payload.files and throws into the
+  // 500 handler. A usage error must be a 400 in the documented JSON shape.
+  if (payload === null) {
+    sendJson(res, 400, { ok: false, error: "bad JSON request body" });
+    return;
+  }
   const settings = (payload && typeof payload.settings === "object" && payload.settings) || {};
```

`web/server.mjs (handleOptimize, after the settings parse)`:

```diff
@@ handleOptimize, after settings = JSON.parse(raw) @@
+  // U-77: settings=null survives JSON.parse and crashes at settings.info.
+  if (settings === null) {
+    res.writeHead(400, { "Content-Type": "application/json" });
+    res.end(JSON.stringify({ ok: false, error: "bad settings JSON" }));
+    return;
+  }
   if (settings.info) {
```

### Acceptance test (add before closing — the repo's own closing rule)

```js
// web/test/transport.test.mjs — add to the /run payload section
{
  const r = await rawPost(port, "/run", "null");
  assert.equal(r.status, 400);            // not 500
  assert.equal(r.type, "application/json");
  assert.equal(r.json.ok, false);
  assert.match(r.json.error, /bad JSON request body/);
}
// and for /optimize:
const r2 = await postOptimize(port, "null", GIF);
assert.equal(r2.status, 400);
assert.match(r2.json.error, /bad settings JSON/);
```

**Fix size:** ≈ 8 lines + 2 tests · web/server.mjs only · no desktop impact

---

## U-78 (proposed) — validate.mjs has no wrong-type gate: NaN slips every numeric check → settings silently dropped, run succeeds

**State:** NEW — confirmed with executed proof · **Severity:** Medium · **Surface:** web / validate.mjs

### Where

- `web/validate.mjs` (16) — `num()` — Number(v) with no finite check; Number("abc") = NaN
- `web/validate.mjs` (18–45) — every range check compares NaN — NaN < 2 and NaN > 256 are both false, so no issue is added
- `web/command.mjs` (76, 87, 92…) — the same NaN then fails the emit guards (x >= 2 && x <= 256), so the flag is silently omitted
- `working_code/gifscythe/src/core/SettingsIO.h` (141, 156) — C++ parity anchor: need_long() warns "not an integer" and --strict refuses with rc=3

### Summary

The JSON API accepts settings of any JSON type. A wrong-typed numeric field ("abc", {}, [], true→accepted as 1) flows into validate.mjs, where `Number(v)` yields NaN. NaN fails every comparison gate silently (no 422), then fails every emit guard in buildArgs (the flag is silently omitted) — so the engine runs WITHOUT the setting, the response is 200 ok:true, and nothing anywhere says the value was dropped. Verified end-to-end: sending {"color_count":"abc","lossy":"lots","threads":"many","delay_cs":"soon"} returns HTTP 200 with the bare command `gifsicle -j in0.gif -o x_opt.gif` — all four settings gone. The same garbage through the C++ conf parser prints 2× WARNING "not an integer", and `--strict` refuses with exit 3. The web lane — the surface U-30 was created to make as-honest-as-the-desktop — is the only silent one for this class.

### Root cause

validate.mjs mirrors the RANGE gates of core/Validate.h but not the TYPE gate that, on the C++ side, lives one layer earlier in SettingsIO (to_long/to_double warn-and-reject). The JSON body has no parser layer, so the type check must exist inside validate.mjs — it does not.

### Impact

Silent-intent-loss class (same family as U-11's "malformed booleans degrade silently", which the audit closed on the C++ side). Any programmatic client that sends a mistyped or computed value gets a successful run that ignored it. U-30's parity promise ("same settings, same refusal") is broken for the wrong-type class.

### Reproduce (executed during this review)

**End-to-end proof (live server)**

```bash
node web/server.mjs 8000 &
node -e '
const GIF=Buffer.from([0x47,0x49,0x46,0x38,0x39,0x61,1,0,1,0,0x80,0,0,0,0,0,0xff,0xff,0xff,0x21,0xf9,4,1,0,0,0,0,0x2c,0,0,0,0,1,0,1,0,0,2,2,0x44,1,0,0x3b]);
fetch("http://127.0.0.1:8000/run",{method:"POST",headers:{"Content-Type":"application/json"},
 body:JSON.stringify({settings:{color_count:"abc",lossy:"lots",threads:"many",delay_cs:"soon"},
 files:[{name:"x.gif",data:GIF.toString("base64")}]})}).then(r=>r.json().then(j=>console.log(r.status,JSON.stringify(j))));'
```

- **Observed:** 200 {"ok":true, … "commands":["…/gifsicle -j …/in0.gif -o …/x_opt.gif"]} — no -k, no --lossy, no -d, no thread flag; no warning field in the response
- **Expected:** 422 {"ok":false,"issues":[{"field":"colors","value":"abc","reason":"must be a number…"}, …]} — the same refusal the C++ parser gives before any engine starts

**C++ parity anchor (what the desktop does with the same garbage)**

```bash
printf 'colors = abc\nlossy = lots\ninput = logo.gif\noutput = out.gif\n' > bad.conf
build/gifscythe-cli bad.conf            # observe warnings
build/gifscythe-cli bad.conf --strict   # observe refusal
```

- **Observed:** 2× "WARNING: … not an integer"; --strict exits 3 with a named refusal
- **Expected:** the web API should refuse the same way (422) — parity is the stated goal of validate.mjs

### Proposed fix (ready to apply)

`web/validate.mjs`:

```diff
@@ line 16 @@
-  const num = (v, dflt) => (v === undefined || v === null || v === "" ? dflt : Number(v));
+  const num = (v, dflt) => (v === undefined || v === null || v === "" ? dflt : Number(v));
+  // U-78: the JSON API has no parser layer, so the TYPE gate that SettingsIO's
+  // need_long()/to_double() enforce on the C++ side ("not an integer") must
+  // live here. Number("abc") is NaN, and NaN passes every comparison below —
+  // the setting then falls out of buildArgs' emit guards and is silently
+  // dropped while the run reports success.
+  const numChecked = (field, v, dflt) => {
+    const n = num(v, dflt);
+    if (v !== undefined && v !== null && v !== "" && !Number.isFinite(n)) {
+      add(field, String(v), "must be a finite number (the conf parser rejects this as 'not an integer')");
+      return dflt;
+    }
+    return n;
+  };
```

`web/validate.mjs — switch the ten numeric readers`:

```diff
-  const colors = num(s.color_count, -1);
+  const colors = numChecked("colors", s.color_count, -1);
-  const disposal = num(s.disposal, -1);
+  const disposal = numChecked("disposal", s.disposal, -1);
-  const opt = num(s.optimize_level, -1);
+  const opt = numChecked("optimize", s.optimize_level, -1);
-  const lossy = num(s.lossy, -1);
+  const lossy = numChecked("lossy", s.lossy, -1);
-  const delay = num(s.delay_cs, -1);
+  const delay = numChecked("delay", s.delay_cs, -1);
-  const threads = num(s.threads, -1);
+  const threads = numChecked("threads", s.threads, -1);
   … (same for rw, rh, sx, sy)
```

### Acceptance test (add before closing — the repo's own closing rule)

```js
// web/test/transport.test.mjs
{
  const bad = { color_count: "abc", lossy: "lots", delay_cs: "soon" };
  const r = await postRun(port, { settings: bad, files: [gifFile("x.gif")] });
  assert.equal(r.status, 422);
  const fields = r.json.issues.map((i) => i.field);
  assert.deepEqual(fields, ["colors", "lossy", "delay"]);
}
// web/test/validate.test.mjs — unit-level: validate({color_count:"abc", inputs:["a"]})
// must return one issue with field "colors" (NaN is not a valid -k value).
```

**Fix size:** ≈ 12 lines in validate.mjs + 2 suites get cases · web only

---

## U-79 (proposed) — /optimize accepts mode:"explode" → misleading 422 "no output file" while the engine honestly wrote frames

**State:** NEW — confirmed with executed proof · **Severity:** Low · **Surface:** web / server.mjs

### Where

- `web/server.mjs` (255–300) — handleOptimize gates `info` (U-64) but never gates `mode` — explode flows through to a single-file contract it can never satisfy
- `web/server.mjs` (320–340) — the run writes frames as out.gif.NNN; verifyOutput(out.gif) then reports 'the engine exited 0 but produced no output file'

### Summary

U-64 closed this exact dishonesty for info:true (a clear 400 instead of a misleading 422), but the twin misroute was missed: /optimize is the single-GIF endpoint, and explode never produces a single GIF — it writes frames next to the -o prefix. Verified: POST /optimize?settings={"mode":"explode"} with a valid GIF answers 422 exitCode:0 "the engine exited 0 but produced no output file" while the temp dir contains out.gif.000 — the response blames the engine for a routing mistake the server made. (mode:"batch" and mode:"merge" were also probed and behave — 200 with the single output — so the gate needs to refuse explode only.)

### Root cause

handleOptimize spreads the client's settings into the run without constraining `mode`; the endpoint's post-condition (exactly one verified GIF at -o) is only expressible in auto/batch/merge. Explode's frames land under <prefix>.NNN, which the single-file verifier is blind to.

### Impact

Same trust cost as U-64 pre-fix: a correct engine result is reported as an engine failure. API consumers cannot tell routing errors from engine errors — the distinction the whole web remediation (GS-203/DS-13) exists to protect.

### Reproduce (executed during this review)

**Live probe**

```bash
node web/server.mjs 8000 &
curl -s -X POST "http://127.0.0.1:8000/optimize?settings=%7B%22mode%22%3A%22explode%22%7D" \
  --data-binary @tiny.gif | jq .
```

- **Observed:** 422 {"ok":false,"exitCode":0,"stderr":"the engine exited 0 but produced no output file","command":"…/gifsicle -e -j …/in.gif -o …/out.gif"} — while …/out.gif.000 exists in the (now-deleted) temp dir
- **Expected:** 400 {"ok":false,"error":"mode=explode is not supported by /optimize; use POST /run (mode=explode)"} — the framing U-64 uses for info

### Proposed fix (ready to apply)

`web/server.mjs (handleOptimize, next to the U-64 info gate)`:

```diff
@@ after the settings.info gate (line ~268) @@
   if (settings.info) {
     sendJson(res, 400, { ok: false, error: INFO_UNSUPPORTED });
     return;
   }
+  // U-79: /optimize returns ONE gif. Explode writes frames as <prefix>.NNN, so
+  // it can only ever land in the single-file verifier as a misleading "no
+  // output" 422 — the U-64 class. batch/merge of one file are fine (verified
+  // 200); explode is the only contract breaker. /run carries all four modes.
+  const optMode = settings.mode === undefined || settings.mode === null || settings.mode === ""
+    ? "auto" : String(settings.mode);
+  if (optMode === "explode") {
+    sendJson(res, 400, { ok: false,
+      error: "mode=explode is not supported by /optimize (single GIF output); use POST /run with mode=explode" });
+    return;
+  }
```

### Acceptance test (add before closing — the repo's own closing rule)

```js
// web/test/transport.test.mjs
{
  const r = await postOptimize(port, JSON.stringify({ mode: "explode" }), GIF);
  assert.equal(r.status, 400);
  assert.match(r.json.error, /explode.*POST \/run/);
  // and the modes that must keep working:
  for (const m of ["auto", "batch", "merge"]) {
    const ok = await postOptimize(port, JSON.stringify({ mode: m }), GIF);
    assert.equal(ok.status, 200);
  }
}
```

**Fix size:** ≈ 8 lines + 1 test · web/server.mjs only

---

## U-80 (proposed) — web/wasm/glue_harness.mjs hardcodes release/0.1.0/gifsicle — breaks (loudly) on the next VERSION.md bump

**State:** NEW — confirmed by static trace (fail-closed, not silent) · **Severity:** Low · **Surface:** web / wasm tooling

### Where

- `web/wasm/glue_harness.mjs` (17) — const ENGINE = …/release/0.1.0/gifsicle — literal version; the only product-surface code file with one
- `working_code/gifscythe/scripts/verify_audit.sh` (A5 gate) — the no-hardcoded-version grep covers only src/cli and src/qtui — web/ and scripts/ are outside it

### Summary

Every shell entry point parses the version from VERSION.md at runtime; the wasm glue harness is the single code path that pins `0.1.0` literally. When VERSION.md moves to 0.2.0 the harness exits 2 with "engine not found" — fail-closed and honest, but a guaranteed red step in the exact session that bumps the version (and their own U-66 row already tracks a related discovery-pinning divergence).

### Root cause

One literal where every sibling uses the VERSION.md grep.

### Impact

Maintenance landmine, not a shipped defect: wasted triage during the version-bump session; also invisible to the A5 gate as scoped today.

### Reproduce (executed during this review)

**Static trace (no execution needed — it fails closed)**

```bash
grep -n "0\.1\.0" web/wasm/glue_harness.mjs
# → 17:const ENGINE = path.resolve(HERE, "../../working_code/gifscythe/release/0.1.0/gifsicle");
# Simulate the bump: build at release/0.2.0, then node web/wasm/glue_harness.mjs
# → "glue_harness: engine not found at …/release/0.1.0/gifsicle", exit 2
```

- **Observed:** exit 2 naming the stale 0.1.0 path (loud, not silent — confirmed by reading the guard)
- **Expected:** derive the version from VERSION.md like scripts/verify_audit.sh and package_common.sh do

### Proposed fix (ready to apply)

`web/wasm/glue_harness.mjs`:

```diff
@@ line 17 @@
-const ENGINE = path.resolve(HERE, "../../working_code/gifscythe/release/0.1.0/gifsicle");
+// U-80: parse VERSION.md like the shell suites do; never pin the version dir.
+const VERSION_MD = readFileSync(
+  path.resolve(HERE, "../../working_code/gifscythe/VERSION.md"), "utf8");
+const PRODUCT_VERSION =
+  (VERSION_MD.match(/Current version:[^\n]*?(\d+\.\d+\.\d+)/) || [])[1] || "0.1.0";
+const ENGINE = path.resolve(
+  HERE, `../../working_code/gifscythe/release/${PRODUCT_VERSION}/gifsicle`);
```

### Acceptance test (add before closing — the repo's own closing rule)

```js
// One-line addition to verify_audit.sh A5-style grep (extends its scope):
! grep -rEn '"[^"]*release/[0-9]+\.[0-9]+\.' web scripts 2>/dev/null
  && ok "A5b" "no hardcoded release/<version> paths outside fixtures"
  || bad "A5b" "hardcoded versioned engine path found (tests/fixtures exempt)"
```

**Fix size:** ≈ 5 lines + optional gate extent · tooling only

---

# Cleared candidates — suspected, investigated, NOT defects

These were the strongest bug candidates this review generated. Each was executed or traced and found to behave correctly; they are recorded so the next reviewer does not spend the same cycles (the repo calls this the false-positive ledger discipline).

- **[NOT A BUG]** CLI called with a flag as argv[1] (e.g. `gifscythe-cli --strict conf.conf`) — Suspected a silent swallow or a Mangrove of confuse; verified live: exits 2 with usage + "unknown argument". Strict positional parsing by design (audit U-23) — fails closed.
- **[INERT]** gifscythe.pro HEADERS omits OutputVerify.h / ProcessRunner.h (CMake and the web/C++ suites all steady) — INCLUDEPATH+=src covers compilation; the qmake HEADERS list drives IDE display and dependency tracking only, never what compiles. GUI never includes those two headers. Hygiene-only: adding the two names costs nothing.
- **[NOT A BUG]** Explode empty-output prefix might disagree between summary, engine and verifier — Checked against the engine source: GUI always synthesizes -o <dir>/<stem>_frame; CLI keeps basename-in-CWD, matching gifsicle.c input_done() ("Explode into current directory") and support.c explode_filename(); ExplodeVerify matches on <prefix basename>. — consistent. (Per-surface default difference is already tracked as U-76.)
- **[GREEN]** STATUS.md generated counts vs the actual register rows — Re-counted: 112 DONE / 8 PARTIAL / 28 OPEN / 0 UNTRIAGED = 148 — matches the generated header exactly. check_docs.sh --no-gate-run: 23 pass / 0 fail / 2 skip on the reviewed commit.
- **[GREEN]** web/app.js referencing missing DOM ids (load-time crash class) — Cross-checked: 30 distinct $("id") references in app.js; all 30 exist among index.html's 35 ids. No dangling reference.
- **[GREEN]** Web delay label could reintroduce the E7 'ms' invariant break — index.html labels it "Frame delay (1/100 s)" — the no-"ms" rule holds on the web surface too.
- **[NOT A BUG]** /optimize with mode:"batch"/"merge" — suspected same contract break as explode — Probed live: both answer 200 with a valid single GIF — gifsicle -b with an explicit -o writes to -o, and -m of one file behaves like auto. Only explode breaks the single-GIF contract (reported as U-79).
- **[GREEN]** Stale-claim docs on the reviewed commit (their own GS-208/N-01 class) — Ran scripts/sweep_stale.sh: 5 passed, 0 failed. No stale current-state claims at 794a996.

---

# Planning guidance

## Language verdict — stay the course (independent agreement with the repo's own decision)

- Desktop: stay on C++17 / Qt6 through 1.0.0. This review confirms the reasoning is right, not just documented: the defect class found by all seven reviews (the repo's six + this one) is specification/honesty bugs at surface boundaries — wrong status codes, silent drops, unplanned outputs. None is a memory-safety, lifetime or data-race bug. A Rust/C# rewrite would not have prevented a single one of U-01…U-80, and would burn the assets that actually catch them: the 372-check header-only core suite, the JS⇄C++ parity fixtures, the offscreen Qt harness, the Wine E2E, and the Windows packaging.
- Web: stay on zero-dependency Node ESM. All three new web findings are ≤ 15 lines each and add no dependency; a framework or a TS build step solely for this pass buys nothing.
- WASM stays parked until OD-16 (license) and a real emcc byte-proof land; csharp stays parked until after 1.0.0 (owner already decided, S19) — this review endorses both.
- Post-1.0.0 rewrite trigger: spike Rust/Tauri ONLY if a measured requirement appears (single-binary size, embedded web UI). The subprocess engine boundary is the product's architectural asset — keep it language-agnostic.

## The faster-to-ship lever is surface count and order, not language

- ONE shippable artifact until 1.0.0: the Windows portable zip (OD-17) with the web build as internal tool. Every additional surface multiplies the parity cost — this review found the same honesty-bug family independently in CLI, GUI and web (and so did the six audits before it).
- Apply U-77…U-79 immediately: ~35 lines + 6 tests, entirely inside web/, zero impact on the frozen desktop lane, and they complete the U-30/U-64 honesty arc the web surface already committed to.
- Close release blockers in dependency order: GS-203 GUI integration (P1-25) → release re-cut (U-09, needs current-SHA artifacts) → UI-thread waits (U-12 / P1-24, the riskiest GUI change — do it right after the re-cut while CI signal is fresh).
- Run docs/ci/CLEAN_WINDOWS_SMOKE.md once on a real VM: C4/D3/D4 and the B-probes are the ONLY items a Linux sandbox cannot close — schedule hardware time instead of letting them gate silently.
- Then 0.2.0. Version decision (0.2.0 vs 1.0.0) stays the owner's call (OD-11), after the release criteria in PROJECT_VISION.md are met.

## Process — keep the gates, shrink the narration

- The doc machine earns its keep: the register, check_docs.sh, sweep_stale.sh and the parity suites all re-ran green in this review and each has caught real drift (GS-208, N-01, N-02). Keep them.
- But the narration ratio is now the bottleneck: COMPILED_AUDIT.md (2,692 lines) + IMPROVEMENT_LOG.md (2,565 lines) exceed the entire product source (~4,000 LOC core+CLI+GUI+web server). The external reviewer's cost-centre observation (§17.2 #3) is consistent with what this review measured.
- Concrete middle path: (a) route NEW findings to GitHub Issues with the U-id as a label, (b) keep STATUS.md + the gates as the machine-checked register, (c) let COMPILED_AUDIT.md stop growing — append only §5-style rows with proof links, archive narrative sections as dated snapshots. Rigor unchanged, writing-per-session roughly halved.
- Adopt this report's trifecta as the first Issues: U-77, U-78, U-79, U-80 — each ships failing-test-first with executed proof, matching the repo's own closing rule.

## Path after 1.0.0 (in order)

- 2.0.0: WebP + APNG per VERSION.md's published scheme — via the frame-format spike already deferred, not before.
- Revisit web/wasm shipping only after OD-16's license answer AND a real emcc byte-proof; fix U-80 when the version bumps either way.
- SkillOpt integration only after OD-15, in the pinned-submodule shape the planning doc recommends.
- Keep the engine a subprocess forever — it is what makes every surface (CLI, GUI, web, future wasm) honest and replaceable, and it is what GPL v2-only requires anyway.

## Proposed order of work (ready to copy into WORKLIST.md)

| # | Action | Why | Effort |
|---|---|---|---|
| 1 | Apply U-77 / U-78 / U-79 (+ U-80 hygiene) with the regression tests in this report | Completes the web honesty arc (U-30/U-64 class); ~40 LOC, web-only, zero desktop-freeze impact | ½ session |
| 2 | GS-203 GUI integration (P1-25) — match the HTTP-signature refusal in the desktop | Release blocker; same verification the web already enforces | 1 session |
| 3 | Re-cut the release from one reviewed SHA (U-09 / P0-4) | The banked snapshot predates its own notes; blocks an honest pre-release | 1 session + CI |
| 4 | UI-thread waits (U-12 / P1-24) — async run/cancel state machine | Last known UX-blocker class; deliberately scoped but not yet implemented | 1–2 sessions |
| 5 | Clean-Windows smoke on real hardware (docs/ci/CLEAN_WINDOWS_SMOKE.md) | C4/D3/D4 + B-probes cannot close under emulation; unblocks 1.0.0 criteria | ½ day + a Windows VM |
| 6 | 0.2.0 release → owner version decision (OD-11) → 1.0.0 | Criteria in PROJECT_VISION.md; naming rules in VERSION.md | owner + 1 session |

---

# Method notes + anti-false-positive statement

- Scope read: every product file outside reference_code/ (241 files; C++ core headers, CLI, full Qt GUI, Node web server + suites, packaging/CI scripts, hooks, and the audit/planning docs), plus the engine source where behavior had to be confirmed (gifsicle.c input_done, support.c explode_filename, xform.c threading).
- False-positive control: every claim in this report was reproduced against the running build at 794a996 (build.sh rc=0; unit 372/0; smoke 45/0; five web suites green; docs gates green). Candidates that reproduced as correct behavior were moved to the Cleared section instead of being reported as defects.
- Known-open items this review did NOT duplicate: U-69 (web failure leaves stale results — OPEN/P2-16), U-70..U-76, U-12, U-65/U-66, U-09. They remain tracked in the repo's own register; see STATUS.md.
- One deliberate exclusion: reference_code/gifsicle is upstream source material marked do-not-edit; it was read only to confirm engine semantics (explode naming, -b/-o, threading defaults), never audited as product.

*Generated from the review app's data model (the on-screen content and this file share one source, so they cannot drift). Repo taxonomy respected: ids continue the register's U-nn scheme; severities map to its P2/P3 bands.*
