# Windows test run-sheet — for the owner's own machine (W-18 / W-19 evidence)

**Why you:** the portable promise is the last unverified claim, and it can only be evidenced on a
machine with **no Qt, no MinGW, no dev tools** — yours. This run-sheet is the console version of
`docs/ci/README.md` §2/§3; §2 there is the canonical checklist. Do the steps in order and paste
back the transcript/screenshots; I record them as the W-18/W-19 evidence and tick C4/D3/D4.

It is generated here **and committed into the repo** (`docs/ci/WINDOWS_TEST_RUNSHEET.md`) so a
sandbox reset can never take it from you again — the previous copy lived only in `/home/user` and
was lost to exactly that.

---

## 0. What to fetch

* Release: **`snapshot-2026-10-06`** — <https://github.com/freeforall1932-design/gifscythe/releases/tag/snapshot-2026-10-06>
* Asset: **`Gifscythe-0.1.0-windows-portable.zip`**
* Size **23 777 470 B**, sha256 `add856c850076b2bf3a37ad01b25d4c5d8ab8d0d60890f4f591be60e90d8096e`
* The zip is produced by the tag's own run, so the published bytes are the ones that run asserted.
* **Do not use `snapshot-2026-09-07`** (predates S18 relicence + pinned a wrong SHA).

Check it first, in PowerShell:

```powershell
Get-FileHash .\Gifscythe-0.1.0-windows-portable.zip -Algorithm SHA256
```

Must equal the string above. Unzip to `C:\gifscythe-test\` — **no spaces**. Afterwards do a second
run from a spaced path (e.g. `C:\gifscythe test\`) if you have patience: that re-probes argv quoting (A4).

## 1. Engine identity (C5)

```bat
cd /d C:\gifscythe-test\build-win
gifsicle.exe --version
```
First line must read: `LCDF Gifsicle 1.96 (Windows)`.

## 2. CLI end-to-end (C3)

Copy any test `.gif` next to the exe, then create `test.conf` in `C:\gifscythe-test\build-win\`:

```ini
mode = auto
optimize = 3
input  = C:\gifscythe-test\build-win\in.gif
output = C:\gifscythe-test\build-win\out.gif
```

```bat
gifscythe-cli.exe test.conf
```
* prints the command line it would run, and

```bat
gifscythe-cli.exe --run test.conf
```
* exits **0**, writes `out.gif`, and

```bat
gifsicle.exe --info out.gif
```
* shows the same frame count.

**Honesty probe** (must fail loudly, not silently):
```bat
gifscythe-cli.exe --run --engine C:\nope\gifsicle.exe test.conf
```
→ non-zero exit + `ERROR: engine not found`.

## 3. GUI by double-click (D3/D4)

Open `build-win\gifscythe.exe` by double-clicking (not from a shell):
1. **No missing-DLL dialog.**
2. Status bar shows the engine found **beside the exe**.
3. Add GIF → **Optimize** → completes, output written, preview animates, tabs stay responsive, the
   command pane updates live.

## 4. Desktop probes (W-19)

From `docs/ci/README.md` §3 — the high-value ones:
* cancel a long run mid-flight → UI returns, no zombie `gifsicle.exe` in Task Manager;
* run twice back-to-back → second run does not inherit the first's output;
* pick a nonexistent input file → readable error, no crash;
* space in a path (§0 second pass) → still correct.

## 5. What to send back

For each of §1–§4: the command you ran, the output, and what you saw. Anything that fails, send the
failure **verbatim** — a RED is worth more than a tick, and a failed step leaves C4/D3/D4 open
until re-run green (hard rule: this checklist evidences, it does not waive).

Also useful: `winver` (OS build) and `Get-FileHash` output. File paths with spaces, `"` and `'`
inside arguments, and a non-ASCII filename (`tegést.gif`) are the three quoting cases; if you have
two minutes, those three are the highest-yield extras.
