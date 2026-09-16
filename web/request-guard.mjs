// request-guard.mjs — who owns the result currently on screen.
//
// Audit U-54 (E:NA-02) is the web twin of the desktop's U-46: the page kept a
// `requestGen` counter that only advanced when the FILE QUEUE changed, so
// changing a control while a run was in flight left the old completion looking
// current — the UI showed new settings above an After image produced by the old
// ones. The counter being in `app.js` was also the reason neither half of that
// bug (stale result accepted / stale result displayed) could be tested at all:
// it is browser-only DOM code.
//
// So the ownership rule lives here, as a pure module, and `app.js` wires it to
// the controls. Same shape as command.mjs / validate.mjs / run-paths.mjs: the
// decision is testable in node, and the transport + hygiene suites keep the
// browser side honest about loading it.
//
// Contract:
//   begin()        -> a token for one run; carries the settings SNAPSHOT taken
//                     at launch, so a later control change cannot be confused
//                     with the settings the run actually used
//   isCurrent(tok) -> true only while that token is both the newest generation
//                     AND the active run (a finished run is never "current":
//                     its late `.finally()` must not re-enable the button of a
//                     newer run)
//   invalidate()   -> every in-flight result is now stale; aborts the fetch
//   finish(tok)    -> the run ended; releases the active slot
//
// `now` is injectable only so tests can assert the "aborted" path without
// waiting on real I/O.

export function createRequestGuard() {
  let generation = 0;
  let active = null;              // { token, controller }

  return {
    get generation() { return generation; },
    // Is a run in flight right now? app.js uses this to say "in-flight result
    // discarded" only when something actually was in flight.
    get busy() { return active !== null; },

    begin(settings) {
      const controller = typeof AbortController === "function"
        ? new AbortController() : null;
      const token = {
        gen: generation,
        // Shallow copy on purpose: app.js builds a fresh flat object per call
        // (settings() above), and every field is a scalar or a fresh array, so a
        // spread is a real snapshot. Aliasing the caller's object would let a
        // later control change rewrite history — the very defect U-54 is about.
        settings: { ...(settings || {}) },
        signal: controller ? controller.signal : undefined,
      };
      active = { token, controller };
      return token;
    },

    isCurrent(token) {
      return !!token && active !== null && active.token === token
        && token.gen === generation;
    },

    finish(token) {
      if (active && active.token === token) active = null;
    },

    invalidate() {
      generation += 1;
      const dying = active;
      active = null;              // a stale run is never current again
      if (dying && dying.controller) {
        try { dying.controller.abort(); } catch { /* already aborted */ }
      }
      return generation;
    },
  };
}
