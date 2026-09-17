# Gifscythe — web app

A browser UI over the same gifsicle engine command layer as the desktop app.
**Since S14 (2026-09-12) this is a supported product surface**: a self-hosted
alternative to the `.exe`/portable build — same engine, same command semantics,
loopback by default (`GS_WEB_HOST` for LAN). Plan template, split rules and the
phases that make it product-grade: `WEB_PLAN_TEMPLATE.md` (state line `SKELETON` until the owner's
draft is refitted; mirrored in `SESSION_HANDOFF.md`, gate **G16**).
This was **Option 3** of the 2026-09-09 web feasibility review (folded into §History below in S24); the optional client-side
target (`gifsicle.wasm`) is described there and is not built.

## Architecture

```
browser (index.html + app.js)          Node (server.mjs)          gifsicle engine
  queue GIFs ── POST /run ────────>   build argv (command.mjs) ──> spawn, argv array
  (mode: auto/batch/merge/explode)    per-mode runs + output       (never a shell)
  live pane  <── command.mjs ──────┐  verification (U-24/U-17)
  results list + downloads <───────┴── JSON: outputs[] (base64) + commands[]
```

**All four desktop modes** (audit U-41 / fix-order P2-11) go through
`POST /run`: Auto (one file), Batch (a per-file Auto run each, targets
`<stem>_opt.gif`, planned up front with collision refusal — the desktop
`OutputPlan` semantics), Merge (one `-m` run over the whole queue) and Explode
(`-e`/`-E` against a `<stem>_frame` prefix, with the desktop's frame
verification: rc=0 and zero new GIF frames is a **422**, never a success).
The server derives output names itself; the desktop's Save-as / batch folder /
name-template controls stay desktop-only.

- `command.mjs` is a line-by-line JS mirror of `src/core/GifsicleCommand.h`.
  It is the **single** builder used by the browser live-pane and the server,
  and it is cross-tested against the real C++ `gifscythe-cli`.
- The server is **zero-dependency** (Node built-ins only).

## Bounds, defaults and what is NOT modelled (S23)

| Env var | Default | Why it exists |
|---|---|---|
| `GS_WEB_HOST` | `127.0.0.1` | opt in to exposing the server on the network (U-06, S8) |
| `GS_MAX_BODY` | `67108864` | the HTTP **envelope** cap. `/run` carries the GIF base64-encoded, so the effective decoded-GIF cap is ≈48 MB at the default — the 64 MB figure is the envelope, not the file (U-68 / NF-11, documented rather than silently widened) |
| `GS_MAX_CONCURRENT` | `2` | engine runs allowed at once, `/optimize` and `/run` sharing one semaphore (U-06 / P1-5) |
| `GS_MAX_QUEUED` | `8` | how many wait past that; the rest get `429` with an explanatory body instead of queueing unbounded |
| `GS_RATE_LIMIT_PER_MIN` | `300` | per-client-address POST window; static is exempt because a page load is several GETs by design. `0` disables it |
| `GS_ENGINE_TIMEOUT_MS` | `120000` | the engine-run bound (was a hard 120 s); a timed-out run answers `422` with `exitCode: 124`. `0` disables |
| `GS_ENGINE` | unset | exact engine path; an invalid non-empty value refuses fallback with `503` (GS-207) |

Engine discovery mirrors the desktop: `release/current` first (one pin moves both
surfaces — U-66), then the newest numeric `release/<version>/`.

Three settings the desktop has and this UI does not: raw conf editing, the
per-frame `--name` writer, and `info` (this API returns GIFs/JSON only, so
`info: true` is a clear `400`, not a misleading `422`). The **Loop** control now
offers *Play once*, which is `--no-loopcount` — the absence of the loop
extension, which no count value can express.

Run-ownership rule (U-46 / U-54 / U-69) lives in `web/request-guard.mjs`: a
settings change or queue change invalidates the in-flight run, aborts its fetch and
clears what was on screen, so a result can never be displayed under settings that
did not produce it. Its semantics are unit-tested in
`web/test/request-guard.test.mjs` because nothing here can drive a DOM.

## Run

```bash
# 1. build the engine + CLI (once)
cd working_code/gifscythe && ./build.sh && cd ../..

# 2. start the web server (binds 127.0.0.1 — loopback only)
node web/server.mjs 8000

# 3. open http://localhost:8000 (or use the live preview)
```

`GS_ENGINE=/path/to/gifsicle` is an **exact path override** (relative paths resolve
from the server's working directory, not PATH). Since GS-207 (S17), an invalid
non-empty value never falls back: the server logs `Engine [GS_ENGINE]: ERROR: ...`
and both API endpoints return **503** with the offending value and a diagnostic.
The static UI remains available. A directory or, on POSIX, a non-executable file
is invalid; engine format/architecture failures remain subprocess errors, with no
retry using another engine. Empty/unset values preserve release-tree discovery.
Startup logs name the chosen source (`GS_ENGINE`, `release`, or `none`).

**Binding (audit U-06).** The server listens on **`127.0.0.1`** by default and
says so on startup. It used to bind `0.0.0.0` unconditionally, which exposed an
unauthenticated engine-runner to the whole network. Set
`GS_WEB_HOST=0.0.0.0` to expose it deliberately on a LAN — single-user, no
auth, so do not expose it to the public internet.

## Test (parity with the desktop app)

Nine suites live in `web/test/`; the CI linux job and `verify_audit.sh` W1–W6 run
them all. **No PASS count is quoted here on purpose** — every number this file
used to carry went stale the first time a fixture was added (they read 17/23/63
from S17 while the suites were already at 79+). Quote the counter the run you
just executed printed, never a number from a doc.

```bash
# need the built CLI (./build.sh) — they run the REAL gifscythe-cli:
node web/test/command.test.mjs        # JS ⇄ C++ argv parity
node web/test/validate.test.mjs       # JS ⇄ C++ validation parity + the U-78 wrong-type class
# needs a discoverable engine — drives the real server over HTTP:
node web/test/transport.test.mjs      # live-server transport net
# engine-free:
node web/test/numeric-honesty.test.mjs # U-78/U-87 empty-vs-zero + wrong-type (P1-44)
node web/test/body-limit.test.mjs      # U-68 413 mapping (engine stub)
node web/test/static-hygiene.test.mjs  # U-67 allow-list + HEAD contract
node web/test/request-guard.test.mjs   # U-54/U-69 run ownership (pure module)
node web/test/device-names.test.mjs    # U-56 shared reserved-name table
node web/test/server-bounds.test.mjs   # U-06 concurrency/rate/timeout bounds
```

Both run the **real** C++ `gifscythe-cli` in print mode and compare against the
JS side, so the two clients cannot drift silently:

* `command.test.mjs` serializes 16 settings fixtures (incl. the `-b` and `-e`
  shapes added for U-41) to conf files and asserts the JS builder emits a
  byte-identical command line, plus a direct argv sanity check.
* `validate.test.mjs` (audit U-30) asserts `validate.mjs` returns exactly the
  same `(field, value, reason)` triples as `src/core/Validate.h` — same set,
  same order, same wording. Note that `delay < 0` and `lossy < 0` never survive
  a conf round-trip (both writers treat `-1` as "unset" and skip them), so those
  two fixtures feed the C++ side raw conf text instead.

* `transport.test.mjs` (audit U-49/U-50/U-41, register P2-8/P2-9/P2-10/P2-11)
  starts the **real** server on an ephemeral port and pushes tricky values
  through the actual `POST /optimize?settings=` path — literal `%`, `%20`,
  `%22`, `%2540`, plus signs, CJK, emoji, embedded newlines, a value that
  looks like a flag — asserting the status code, that the command the server
  ran still contains the exact value, and (via the returned GIF bytes) that
  the value actually reached the engine. Mutation-tested: re-adding the double
  decode fails 4 cases, dropping `encodeURIComponent` fails 6, removing
  `validate()` fails 3. The U-41 block then drives `POST /run` for all four
  modes against the real engine: auto/merge/batch/explode happy paths, batch
  collision + target-equals-source refusals, explode `-E`, and the usage 400s.
  S17 adds GS-202: 100 unsafe-name requests across all four modes, a preload that
  logs actual engine spawns (refusals must launch none), outside-request sentinel
  files, case/NFC collisions, legitimate Unicode/space/percent-name successes,
  and direct POSIX/Windows-drive/UNC containment probes. All artifacts, including
  intentional pre-fix escapes, stay in a disposable test root. The original server
  fails seven security groups; disabling the final containment guard fails its probe.
  DS-13 adds 11 output-fixture cases: a test-only preload redirects marked calls
  to a real Node child that writes text/PNG/malformed signatures, valid GIF87a/89a,
  missing/empty output, or exits nonzero. Production has no test hook; existing
  cases still run the real gifsicle engine. The pre-fix server fails all six
  invalid-signature cases; that DS-13 checkpoint passed 53 check groups.
  GS-207 adds 10 groups on Linux: missing/directory/non-executable/bare/whitespace
  overrides, valid absolute/relative paths with spaces, unset/empty discovery and
  an override removed after startup. Both endpoints and startup source logs are
  checked, with the spawn log proving invalid overrides launch nothing. The POSIX
  execute-bit group is omitted on Windows (62 groups there; not measured locally).

All five are run by the CI linux job and by `scripts/verify_audit.sh`
(gates W1/W2/W3/W4/W5): command parity, validation parity, live transport,
oversized-body 413, and static allow-list / HEAD-contract hygiene.

## API

### `POST /run` — all modes (the UI uses this)

JSON body `{ settings, files: [{ name, data(base64) }] }`.

**Upload name contract (GS-202, S17).** `name` must be one portable filename, not
an absolute/relative path. Both slash styles, drive/ADS colons, control characters,
unpaired Unicode surrogates, Windows-special characters, dot components, trailing
dots/spaces and reserved Windows devices are rejected (400), not silently sanitized.
Spaces inside names, Unicode, emoji and literal percent sequences remain valid; JSON
names are not URL-decoded. Batch target/target and target/source collisions use
NFC-normalized, case-insensitive keys on every host (422), preserving original spelling.

Every resolved output target, including the explode prefix, is checked for containment
in the private request directory before the first engine run. Client `settings.inputs`
and `settings.output` cannot override that plan. This is lexical path containment with
a trusted engine, **not** isolation from a malicious engine or hostile local symlink
writer; the existing single-user/no-public-exposure limitations still apply.

- `200 application/json` → `{ ok:true, mode, outputs:[{ name, bytes,
  data(base64) }], commands:[...], inBytes, outBytes }` — one output per
  result file, one quoted command line per engine run.
- `400` → usage errors: bad JSON, unknown mode, wrong file count for the
  mode, empty file data, unsafe upload names (`invalid upload name` and `file`),
  and `info:true` (the web API is GIF-output only; use the CLI for `--info`).
- `422` → settings validation issues (`issues[]`, same layer as `/optimize`),
  batch target collisions / target-equals-source refusals, engine failure
  (`exitCode`, `stderr`, `command`), rc=0-with-no-output, and explode
  rc=0-with-zero-frames (names the prefix searched).
- `503` → engine missing or invalid non-empty `GS_ENGINE` (no fallback).

### `POST /optimize?settings=<urlencoded JSON>` — single-file legacy path

Raw GIF bytes as the body (single-file Auto only; kept for compatibility and
pinned by the transport suite's U-49/U-50 cases).

- `200 image/gif` → optimized GIF; metadata headers:
  `X-Gifscythe-Command`, `X-Gifscythe-In-Bytes`, `X-Gifscythe-Out-Bytes`.
- `422 application/json` → `{ ok:false, exitCode, stderr, command }`.
  Since DS-13 (S17), non-empty output without an exact GIF87a/GIF89a signature
  returns 422 with `exitCode: 0` and an `invalid GIF output` diagnostic instead
  of `200 image/gif`. The signature is checked on the exact response buffer;
  this is not full decoding or a guarantee that all GIF frames are intact.
  Missing/empty output and engine failure retain their existing diagnostics.
- `400/503` → malformed settings, `info:true` (unsupported on this GIF-only
  endpoint), or engine missing.

The `settings` object mirrors `gs::Settings` (see `command.mjs`): `mode`,
`optimize_level`, `lossy`, `color_count`, `dither`, `dither_method`,
`resize_kind`, `resize_w/h`, `scale_x/y`, `resize_method`, `rotation`,
`flip_horizontal/vertical`, `interlace`, `has_position`, `position_x/y`,
`crop*`, `background`, `transparent`, `remove_comments/names/extensions`,
`comments`, `delay_cs` (1/100 s), `disposal`, `loopcount`, `unoptimize`,
`threads`, `gamma_str`, `gamma`, `color_method`, `careful`, `explode_by_name`.

## Limitations (by design, for now)

- Server-side processing → not a portable/offline web build (that needs the
  wasm engine; see §History below and `wasm/README.md`).
- Single-user server; no auth/quotas. Self-hosted by design: keep it on loopback (or a trusted LAN) and do not expose it publicly as-is.
- The browser UI exposes a focused subset of controls (all four modes, but no
  Save-as / batch folder / name template / rotation / crop widgets);
  `command.mjs` already supports the full desktop settings surface.
- Output names are derived (`<stem>_opt.gif`, `merged.gif`,
  `<stem>_frame.NNN`), mirroring the desktop defaults but not configurable.

## History — the 2026-09-09 web feasibility review (folded in S24; superseded conclusion marked)

The original review (docs/web/WEB_FEASIBILITY.md, deleted in the S24
consolidation — full text in git history at `3c67e14`) weighed four ways to
"run Gifscythe on the web":

| # | Option | Verdict then | Fate |
|---|---|---|---|
| 1 | Qt for WebAssembly (existing Widgets UI in-browser) | ❌ rejected — `QProcess`/subprocess spawning does not exist under wasm; the engine layer would need an in-process rewrite, changing the GPLv2 boundary; loses native dialogs/DnD | Never built |
| 2 | Tauri (web UI + Rust shell) | ⚠️ viable but "not web" — a desktop app with a web-tech UI; WebView2 runtime vs the portable promise | Stays a D-08 trigger-based option (`docs/planning/PLANNING.md` §1) |
| 3 | Web UI + server-side engine | ✅ works today — zero new toolchains, same command layer as desktop; "needs a server, so not portable/offline" | **This directory — and since S14 (2026-09-12, owner) a SUPPORTED PRODUCT SURFACE**, self-hosted, loopback by default; the review's "demo only / not the product path" framing is superseded (`WEB_PLAN_TEMPLATE.md` §1) |
| 4 | Client-side WASM engine (compile gifsicle → wasm) | ✅ best end-state — fully portable/offline in the browser; gifsicle is single-process C, ports cleanly; in-process use ends the subprocess separation, so distribution ships the GPLv2 text/source offer | Scaffolded in `wasm/` (S19); NOT SHIPPABLE until `OD-16` (in-process licence) + a real emcc byte proof |

The review's sandbox note (2026-09-09: emscripten toolchain hosts unreachable,
no Qt6 packages) still describes why the wasm binary remains unbuilt in agent
sandboxes; `wasm/README.md` carries the current state. Its parity claims
(command builder byte-identical to the C++ CLI; validation triples identical to
`Validate.h`; argv-array spawn, never a shell; honest 422s) are now enforced by
the nine suites in `test/` and CI rather than being promises.
