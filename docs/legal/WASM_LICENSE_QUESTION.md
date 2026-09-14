# WASM in-process licence question — OPEN, blocks shippable (`OD-16`)

**Status (S19, 2026-09-14):** OPEN. The `web/wasm/` track is experimental
and **not shippable** until the owner answers `OD-16`
(`docs/planning/OWNER_DECISIONS.md`). Nothing in this folder is legal
advice.

## The setup

Every shipped Gifscythe build keeps the GPLv2-only gifsicle engine as a
**subprocess** — never linked, only spawned with an argv array. That
boundary is what lets a non-GPL UI (now Ms-PL) distribute alongside a
GPLv2 engine: see `WHY_MSPL.md` and `COPYING_RULES.md`.

The `web/wasm/` track compiles the engine to WebAssembly and runs it
**in-process** (one `.wasm` module driven from the page's own script).
The subprocess boundary does not exist there.

## The question

The first-party UI is Ms-PL, which the FSF lists as GPL-incompatible
(link in `WHY_MSPL.md`). The engine is GPLv2-only. Under what terms, if
any, may a bundle that runs GPLv2 engine code in-process with Ms-PL UI
code ship — and what notices, source offers, or additional grants does
that take? That needs an explicit owner answer, on counsel's terms, via
`OD-16`. The MVP's small scope does not shrink the question: one
in-process call is still in-process.

## What is already handled (necessary, not sufficient)

Whatever `OD-16` decides, the wasm build ships gifsicle's own licence
text and source offer: `web/wasm/` carries `THIRD_PARTY_NOTICES.md` and
its build stages `COPYING.gifsicle` into the output directory. That
covers the engine-redistribution half every build owes. It does not
answer the in-process half above.

## Rule

`web/wasm/README.md` carries NOT SHIPPABLE until `OD-16` is answered.
When the answer lands, it is recorded here (this file is updated, not
deleted) and the `OD-16` row points at it — the same pattern as the
`OD-09` answer and `WHY_MSPL.md`.
