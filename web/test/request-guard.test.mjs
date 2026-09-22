// request-guard.mjs unit test — the ownership rule behind audit U-54 (P1-34)
// and U-69 (P2-16). The DOM wiring itself is not executable in this sandbox
// (no browser), which is exactly why the rule was lifted out of app.js into a
// pure module: these are the semantics the page relies on, measured here.
//
// Run: node web/test/request-guard.test.mjs

import { createRequestGuard } from "../request-guard.mjs";

let failures = 0;
function check(name, ok, detail) {
  if (ok) console.log(`PASS ${name}`);
  else { failures += 1; console.log(`FAIL ${name}\n       ${detail}`); }
}
const guard = () => createRequestGuard();

// 1. A token is current until the page invalidates, and invalidation ABORTS the
//    in-flight request instead of merely ignoring its answer.
{
  const g = guard();
  const tok = g.begin({ mode: "auto", optimize_level: 2 });
  check("U-54 a fresh token is current", g.isCurrent(tok), "not current immediately");
  g.invalidate();
  check("U-54 after invalidate the token is stale", !g.isCurrent(tok), "still current");
  check("U-54 invalidate aborts the fetch", tok.signal && tok.signal.aborted === true,
    `signal ${tok.signal && tok.signal.aborted}`);
  check("U-54 generation advances", g.generation === 1, `generation ${g.generation}`);
}

// 2. Settings are SNAPSHOT at launch. The defect was reading `currentSettings()`
//    live, so a result produced with the old values was judged against the new
//    ones; the token has to keep what the run actually used.
{
  const g = guard();
  const settings = { loopcount: -1 };
  const tok = g.begin(settings);
  settings.loopcount = 5;                     // the control moved after launch
  check("U-54 the token keeps the launch-time snapshot",
    tok.settings !== settings && tok.settings.loopcount === -1,
    `snapshot ${JSON.stringify(tok.settings)}`);
}

// 3. Only the NEWEST run is current; a finished run is never current, so its
//    late `finally` cannot re-enable the button of a run that replaced it.
{
  const g = guard();
  const a = g.begin({ n: 1 });
  const b = g.begin({ n: 2 });
  check("U-46 the older token lost", !g.isCurrent(a), "a still current");
  check("U-46 the newer token owns the screen", g.isCurrent(b), "b not current");
  g.finish(b);
  check("U-54 a finished run is not current", !g.isCurrent(b), "current after finish");
  check("U-54 the page is idle once the active run finishes", !g.busy, "still busy");
}

// 4. Aborting an already-finished run, or invalidating twice, must not throw —
//    the UI calls invalidate from three different handlers.
{
  const g = guard();
  const tok = g.begin({});
  g.finish(tok);
  let threw = null;
  try { g.invalidate(); g.invalidate(); } catch (e) { threw = e; }
  check("U-54 invalidate is idempotent and safe after finish", threw === null, String(threw));
  check("U-54 invalidate with nothing in flight still advances the generation",
    g.generation === 2, `generation ${g.generation}`);
}

// 5. A stale run's finally must not clear the newer run's active slot: this is
//    the exact ordering that made the old counter re-enable Run mid-flight.
{
  const g = guard();
  const stale = g.begin({ n: 1 });
  g.invalidate();
  const fresh = g.begin({ n: 2 });
  g.finish(stale);
  check("U-54 a stale finish cannot release the newer run's slot",
    g.busy === true && g.isCurrent(fresh), `busy=${g.busy} current=${g.isCurrent(fresh)}`);
  g.finish(fresh);
  check("U-54 the newer run's own finish does release it", g.busy === false, "still busy");
}

if (failures === 0) {
  console.log("request-guard: all cases passed");
  process.exit(0);
}
console.log(`request-guard: ${failures} FAILED`);
process.exit(1);
