# Gifscythe — web app

A browser UI over the same gifsicle engine command layer as the desktop app.
**Since S14 (2026-09-12) this is a supported product surface**: a self-hosted
alternative to the `.exe`/portable build — same engine, same command semantics,
loopback by default (`GS_WEB_HOST` for LAN). Plan template, split rules and the
phases that make it product-grade: `WEB_PLAN_TEMPLATE.md` (state line `SKELETON` until the owner's
draft is refitted; mirrored in `SESSION_HANDOFF.md`, gate **G16**).
This is **Option 3** of `docs/web/WEB_FEASIBILITY.md`; the optional client-side
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
The demo derives output names itself; the desktop's Save-as / batch folder /
name-template controls stay desktop-only.

- `command.mjs` is a line-by-line JS mirror of `src/core/GifsicleCommand.h`.
  It is the **single** builder used by the browser live-pane and the server,
  and it is cross-tested against the real C++ `gifscythe-cli`.
- The server is **zero-dependency** (Node built-ins only).

## Run

```bash
# 1. build the engine + CLI (once)
cd working_code/gifscythe && ./build.sh && cd ../..

# 2. start the web server (binds 127.0.0.1 — loopback only)
node web/server.mjs 8000

# 3. open http://localhost:8000 (or use the live preview)
```

`GS_ENGINE=/path/to/gifsicle` overrides the engine location.

**Binding (audit U-06).** The server listens on **`127.0.0.1`** by default and
says so on startup. It used to bind `0.0.0.0` unconditionally, which exposed an
unauthenticated engine-runner to the whole network. Set
`GS_WEB_HOST=0.0.0.0` to expose it deliberately on a LAN — single-user, no
auth, so do not expose it to the public internet.

## Test (parity with the desktop app)

```bash
node web/test/command.test.mjs     # command builder  — 17 PASS
node web/test/validate.test.mjs    # validation rules — 23 PASS
node web/test/transport.test.mjs   # live-server transport net — 30 PASS
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

All three are run by the CI linux job and by `scripts/verify_audit.sh`
(gates W1/W2/W3).

## API

### `POST /run` — all modes (the UI uses this)

JSON body `{ settings, files: [{ name, data(base64) }] }`.

- `200 application/json` → `{ ok:true, mode, outputs:[{ name, bytes,
  data(base64) }], commands:[...], inBytes, outBytes }` — one output per
  result file, one quoted command line per engine run.
- `400` → usage errors: bad JSON, unknown mode, wrong file count for the
  mode, empty file data.
- `422` → settings validation issues (`issues[]`, same layer as `/optimize`),
  batch target collisions / target-equals-source refusals, engine failure
  (`exitCode`, `stderr`, `command`), rc=0-with-no-output, and explode
  rc=0-with-zero-frames (names the prefix searched).
- `503` → engine missing.

### `POST /optimize?settings=<urlencoded JSON>` — single-file legacy path

Raw GIF bytes as the body (single-file Auto only; kept for compatibility and
pinned by the transport suite's U-49/U-50 cases).

- `200 image/gif` → optimized GIF; metadata headers:
  `X-Gifscythe-Command`, `X-Gifscythe-In-Bytes`, `X-Gifscythe-Out-Bytes`.
- `422 application/json` → `{ ok:false, exitCode, stderr, command }`.
- `400/503` → malformed settings / engine missing.

The `settings` object mirrors `gs::Settings` (see `command.mjs`): `mode`,
`optimize_level`, `lossy`, `color_count`, `dither`, `dither_method`,
`resize_kind`, `resize_w/h`, `scale_x/y`, `resize_method`, `rotation`,
`flip_horizontal/vertical`, `interlace`, `has_position`, `position_x/y`,
`crop*`, `background`, `transparent`, `remove_comments/names/extensions`,
`comments`, `delay_cs` (1/100 s), `disposal`, `loopcount`, `unoptimize`,
`threads`, `gamma_str`, `gamma`, `color_method`, `careful`, `explode_by_name`.

## Limitations (by design, for now)

- Server-side processing → not a portable/offline web build (that needs the
  wasm engine; see the feasibility doc).
- Single-user server; no auth/quotas. Self-hosted by design: keep it on loopback (or a trusted LAN) and do not expose it publicly as-is.
- The browser UI exposes a focused subset of controls (all four modes, but no
  Save-as / batch folder / name template / rotation / crop widgets);
  `command.mjs` already supports the full desktop settings surface.
- Output names are derived (`<stem>_opt.gif`, `merged.gif`,
  `<stem>_frame.NNN`), mirroring the desktop defaults but not configurable.
