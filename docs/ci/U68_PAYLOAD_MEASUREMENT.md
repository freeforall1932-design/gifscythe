# U-68 — the body cap, measured (not argued)

Owner's instruction: *"do not lock the 64 MB / 48 MB-effective figure into docs everywhere yet.
Treat it like U-14 — measure real payload sizes first, then either confirm-or-raise the envelope
and then sync the docs, or record a measured exclusion."* This file is the measurement. **No
repository doc has been changed for U-68 by this work.**

## 1. The arithmetic, pinned to the code

`web/server.mjs`: `MAX_BODY = 64 * 1024 * 1024` = 67 108 864 bytes, overridable with
`GS_MAX_BODY`. It caps the **raw HTTP body**:

* `/optimize` — the body **is** the GIF → 64 MiB of GIF.
* `/run` — the GIF travels base64-encoded inside a JSON envelope → base64 costs exactly **4/3**,
  so the decoded ceiling is 67 108 864 × 3/4 = **50 331 648 bytes = 48.00 MiB**, minus the
  envelope's own bytes.

## 2. Measured on a real file

A 60-frame GIF built with the repo's own engine (`gifsicle` from `release/0.1.0`,
`logo.gif` × 60 → 508 905 bytes):

| quantity | measured |
|---|---|
| GIF bytes | 508 905 |
| base64 chars | 678 540 |
| base64 overhead | **+33.33 %** (exactly 4/3) |
| full `/run` envelope (`JSON.stringify`) | 678 608 bytes |
| envelope overhead beyond the base64 | **68 bytes** |
| effective decoded ceiling at the default cap | **50 331 648 B = 48.00 MiB** |

So the "64 MB" and "48 MB effective" figures are both exact, and the envelope's own weight is
negligible (68 bytes) next to base64's third.

## 3. The cap is the ENVELOPE — proven against a live server

Server started with `GS_MAX_BODY=1048576` (1 MiB) so the boundary is observable without shipping
64 MB; envelope prefix 48 bytes; POSTs to `/run`:

| envelope size | HTTP | reading |
|---|---|---|
| exactly the cap | **400** | passed the size check; 400 is the *content* rejection later on |
| 1 byte over the cap | **413** | the size rule bites exactly where the code says |
| 1 KiB over the cap | **socket error (client)** | `sendTooLarge` writes the 413 and destroys the socket; a client may see ECONNRESET rather than a readable 413 on a large overage |

The third row is a nuance worth knowing before tuning anything: the 413 exists and is exact at
the boundary, but for bodies far over the cap the client may only see a reset. That is the
documented destroy-after-flush behaviour, not a defect — it is simply not a *readable* refusal.

## 4. What this means for the decision (yours)

* Nothing here is broken: the cap is enforced exactly, the 413 path is tested (`body-limit.test.mjs`
  8/8, W4 in CI), and the effective limit is documented in source.
* The only question is the **value**, and it is a product choice:
  * **Keep 64 MiB** → single-file `/run` uploads are capped at ~48 MiB; `/optimize` gets the full
    64 MiB. Typical animated GIFs (a few MB) and even large ones (10–20 MB) fit comfortably.
    Long screen-recording GIFs (100 MB+) do not — they would need `/optimize` or chunking.
    Cost of keeping: zero, plus syncing the user-facing docs to state the **effective** limit.
  * **Raise the envelope** (e.g. 96 MiB → 72 MiB effective) → cost is memory: `readBody`
    accumulates the body in memory, and the cap is the only thing bounding that per request; the
    server is single-user by design (U-06), so the practical exposure is one request's worth.
* **Recommendation:** keep 64 MiB and document the effective 48 MiB (cheap, honest, no new
  failure mode). Raising it is defensible but buys nothing a GIF user has asked for.

Either way the work once you decide is small: one constant or one paragraph, plus the docs sync —
and the numbers above are what both options should quote.
