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

# 2. start the web server (binds 0.0.0.0)
node web/server.mjs 8000

# 3. open http://localhost:8000 (or use the live preview)
```

`GS_ENGINE=/path/to/gifsicle` overrides the engine location.

## Test (command-layer parity with the desktop app)

```bash
node web/test/command.test.mjs
```

Serializes 12 settings fixtures to conf files, runs the real C++
`gifscythe-cli` in print mode, and asserts the JS builder produces a
byte-identical command line. (2026-09-09: 13/13 PASS.)

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
