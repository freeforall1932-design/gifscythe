# Windows test run-sheet — for the owner's own machine (W-18 / W-19 evidence)

**Why you:** the portable promise is the last unverified claim, and it can only be evidenced on a
machine with **no Qt, no MinGW, no dev tools** — yours. This run-sheet is the console version of
`docs/ci/README.md` §2/§3; §2 there is the canonical checklist. Do the steps in order and paste
back the transcript/screenshots; I record them as the W-18/W-19 evidence and tick C4/D3/D4.

It is generated here **and committed into the repo** (`docs/ci/WINDOWS_TEST_RUNSHEET.md`) so a
sandbox reset can never take it from you again — the previous copy lived only in `/home/user` and
was lost to exactly that.

---

## 0. Three ways to get the zip — pick one

There is nothing to build by hand unless you want to; all three produce the same portable tree
(engine + CLI + Qt6 GUI + windeployqt runtime + licences).

| route | what to do | why / why not |
|---|---|---|
| **A. Download the published one** | <https://github.com/freeforall1932-design/gifscythe/releases> → the newest `snapshot-*` → asset `Gifscythe-0.1.0-windows-portable.zip` | Fastest, login-free. It is built and asserted by CI. Use the newest snapshot: a newer tag means a newer commit, and the notes pin the exact sha it was built from. |
| **B. Make CI build it (no local toolchain)** | push a `snapshot-YYYY-MM-DD` tag → the `release-snapshot` job zips the **same run's** Windows artifact and publishes it with its sha256 | The only route that needs nothing installed locally. Costs one full CI run (~15 min). Ask and it is one command. |
| **C. Build it on your own machine** | `cd working_code\gifscythe` then double-click `scripts\build_portable_windows.bat` (or `powershell -ExecutionPolicy Bypass -File .\scripts\build_portable_windows.ps1`) | Needs Git for Windows + Python 3; it pip-installs Qt/MinGW/cmake/ninja (~1.5 GB first run) and zips the result to `%USERPROFILE%\gifscythe-build\dist\`. Add `-EngineCliOnly` for a quick no-Qt build, `-Smoke` to run the §1–§2 checks automatically. |

Route **C** is a transcription of the `windows` CI job, which is green on `windows-latest`; the
`.ps1`/`.bat` pair itself has never been executed (no Windows in the sandbox that wrote it), so if
a step fails, send the text back verbatim — the fix belongs in the script and the workflow at once.
Route **B** is what the maintainer should use when the diff matters: the published bytes are then
asserted by the same run that built them.

Whatever you use, **verify the hash first** (§0 below) and prefer the *newest* snapshot: an older
tag is an older commit, and its notes are honest about which one.

---

## 0b. What to fetch



* Release: **`snapshot-2026-10-07`** — <https://github.com/freeforall1932-design/gifscythe/releases/tag/snapshot-2026-10-07>
  (cut 2026-10-07 from the post-PR-#12 main tip `7f29347`, evidence run `37527057333`)
* Asset: **`Gifscythe-0.1.0-windows-portable.zip`**
* Size **23 777 755 B**, sha256 `158d551bfdbd214e1d7e4c91cdf0dcd8d682fdad2becac92316248289286a5fc`
* Previous snapshot: `snapshot-2026-10-06` (commit `6aaabcf`), 23 777 470 B, sha256
  `add856c850076b2bf3a37ad01b25d4c5d8ab8d0d60890f4f591be60e90d8096e` — honest, but it predates PR #12.
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
