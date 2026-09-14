# Copying ScreenToGif fork files — compliance checklist (S18)

Licences match (both Ms-PL), so copying is allowed. "Allowed" still has
rules — Ms-PL §3. Follow them per file, every time.

## Before you copy

1. Confirm the source file is Ms-PL (licence header or upstream `LICENSE.txt`;
   the whole upstream tree is Ms-PL — if that ever changes, stop and re-read
   `WHY_MSPL.md`).
2. Copy from a **pinned** upstream commit. Record it.
3. Copy **into** the matching product tree (the C# shell tree once it exists;
   never into the engine or `reference_code/`).

## When you copy

4. Retain **all** copyright, patent, trademark and attribution notices (§3C)
   — in the file header and anywhere they appear. Never strip or "clean up"
   a header.
5. Keep `COPYING.ms-pl` shipped (packagers + CI already require it; do not
   remove that requirement).
6. Name the file + source commit in `IMPROVEMENT_LOG.md` (the one-way-door
   audit trail — see `WHY_MSPL.md`).

## Never

7. Never copy a file whose licence you haven't checked.
8. Never bring GPL-licensed material in through the side door — this includes
   **icons and assets** (MIT/Apache/system sources only, per vision) and
   snippets from GPL examples.
9. Never "rewrite from memory" to dodge attribution: a rewrite guided file
   by file is still a derivative — copy it properly with notices instead.
10. Never touch the engine boundary: no engine code into the UI, no UI code
    into the engine, whatever the licence.

## If in doubt

Stop, ask the owner, and log the question. An unlogged copy is a future
audit finding with your name on it.
