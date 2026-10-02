// rate_limit.mjs — the per-client request window (U-06 / P1-5), extracted from
// server.mjs in S32 so finding N-25 can be proven in-process.
//
// N-25 (found by the S31 line-by-line sweep of web/server.mjs): the window only
// pruned an entry when the SAME address was seen again — `hits` was filtered
// per address and re-set, but nothing was ever deleted and the Map was never
// swept. On the default loopback bind the key is effectively constant, so it
// stayed invisible; on the documented GS_WEB_HOST=0.0.0.0 opt-in every one-off
// source address left a permanent entry and the Map grew without bound.
//
// The fix: prune the WHOLE map on every call. An entry whose timestamps have
// all slid out of the window is deleted, so the map holds at most one entry per
// address actually seen inside the last `windowMs` — bounded by live traffic,
// not by uptime. Per-address semantics are unchanged: a blocked request does
// not extend its own window, and `perMin = 0` disables the limit entirely.
//
// Why a pure module: the HTTP suites cannot vary `remoteAddress` (every request
// arrives from loopback) and `server.mjs` cannot be imported without starting
// the listener and engine discovery. `server-bounds.test.mjs` drives THIS
// module directly — many distinct addresses against an injected clock — and
// keeps the HTTP cases as the wiring proof.

export function createRateLimiter({ perMin = 300, windowMs = 60_000, now = Date.now } = {}) {
  const window = new Map(); // address -> [timestamps]

  function prune(current) {
    for (const [key, stamps] of window) {
      const live = stamps.filter((t) => current - t < windowMs);
      if (live.length === 0) window.delete(key);
      else if (live.length !== stamps.length) window.set(key, live);
    }
  }

  function rateLimited(addr) {
    if (!perMin) return false;
    const current = now();
    prune(current);
    const hits = window.get(addr) || [];
    if (hits.length >= perMin) return true;
    hits.push(current);
    window.set(addr, hits);
    return false;
  }

  return {
    rateLimited,
    // Test seams (same role as server.mjs's resetRateLimit): a shared limiter
    // must be resettable between cases, and a boundedness claim must be
    // observable to be a test rather than a promise.
    reset() { window.clear(); },
    size() { return window.size; },
  };
}
