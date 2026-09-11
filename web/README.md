# Gifscythe — web build (proof-of-concept)

A browser UI over the same gifsicle engine command layer as the desktop app.
This is **Option 3** of `docs/web/WEB_FEASIBILITY.md`: a web UI + a small
server that runs the bundled engine as a subprocess. The long-term portable
target (client-side `gifsicle.wasm`) is documented there but not built yet.

## Architecture

```
browser (index.html + app.js)          Node (server.mjs)          gifsicle engine
  upload GIF ── POST /optimize ────>  build argv (command.mjs) ──> spawn, argv array
  live pane  <── command.mjs ──────┐  (never a shell)
  before/after + download  <───────┴── optimized GIF + X-Gifscythe-* headers
```

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
`GS_WEB_HOST=0.0.0.0` to expose it deliberately — this is a demo, not a
deployable service.

## Test (parity with the desktop app)

```bash
node web/test/command.test.mjs     # command builder  — 14/14 PASS
node web/test/validate.test.mjs    # validation rules — 19/19 PASS
node web/test/transport.test.mjs   # live-server transport net — 17/17 PASS
```

Both run the **real** C++ `gifscythe-cli` in print mode and compare against the
JS side, so the two clients cannot drift silently:

* `command.test.mjs` serializes 14 settings fixtures to conf files and asserts
  the JS builder emits a byte-identical command line.
* `validate.test.mjs` (audit U-30) asserts `validate.mjs` returns exactly the
  same `(field, value, reason)` triples as `src/core/Validate.h` — same set,
  same order, same wording. Note that `delay < 0` and `lossy < 0` never survive
  a conf round-trip (both writers treat `-1` as "unset" and skip them), so those
  two fixtures feed the C++ side raw conf text instead.

* `transport.test.mjs` (audit U-49/U-50, register P2-8/P2-9/P2-10) starts the
  **real** server on an ephemeral port and pushes tricky values through the
  actual `POST /optimize?settings=` path — literal `%`, `%20`, `%22`, `%2540`,
  plus signs, CJK, emoji, embedded newlines, a value that looks like a flag —
  asserting the status code, that the command the server ran still contains the
  exact value, and (via the returned GIF bytes) that the value actually reached
  the engine. Mutation-tested: re-adding the double decode fails 4 cases,
  dropping `encodeURIComponent` fails 6, removing `validate()` fails 3.

All three are run by the CI linux job and by `scripts/verify_audit.sh`
(gates W1/W2/W3).

## API

`POST /optimize?settings=<urlencoded JSON>` with the raw GIF bytes as the body.

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
- Single-user demo server; no auth/quotas. Do not expose publicly as-is.
- The browser UI exposes a focused subset of controls, but `command.mjs`
  already supports the full desktop settings surface.
