# Testing the current Gifscythe desktop build

Use this checklist after building from `INSTALL.md`. It is intended for the
current branch:

```text
arena/dde1f972-gifscythe
```

Do not record a test against an older portable release as a test of the current
GUI changes.

## 1. Confirm the build identity

From the repository root:

```bash
git rev-parse --show-toplevel
git rev-parse --short HEAD
```

The commit should be the commit you intended to test. A source ZIP may have a
GitHub-generated directory name and no `.git` directory; in that case, record
the branch or download URL instead.

On Windows PowerShell, locate the application and engine:

```powershell
$Root = 'C:\Users\Administrator\Videos\Gifscythe'

$Gui = Get-ChildItem -LiteralPath $Root -Filter 'gifscythe.exe' -File -Recurse |
    Select-Object -First 1
$Engine = if ($null -ne $Gui) {
    Join-Path $Gui.DirectoryName 'gifsicle.exe'
}

Write-Host "GUI:    $($Gui.FullName)"
Write-Host "Engine: $Engine"

if (-not (Test-Path -LiteralPath $Engine)) {
    throw "gifsicle.exe is not beside the GUI"
}
```

The GUI and engine must come from the same portable build folder.

## 2. Basic desktop smoke test

1. Start `gifscythe.exe` by double-clicking it, not only from a developer shell.
2. Confirm there is no second command-prompt window for gifsicle.
3. Confirm the application opens with the `Input`, `Actions`, `Output`, and
   `Guide` tabs.
4. Confirm the status bar does not report that the engine is missing.
5. Add a small animated GIF by using the add button or dragging it onto the
   Input queue.
6. Confirm the Before preview shows the selected source GIF.
7. Confirm the Preview controls are visible: `Preview changes`, `Play`, `Stop`,
   and `Auto-play previews`.
8. Turn off autoplay, change an Action, click `Preview changes`, and confirm
   that the preview is generated without starting playback automatically.
9. Click `Play` and `Stop` and confirm that both preview panes respond.

## 3. Settings and Guide test

Open the Actions tab and check that unfamiliar controls have descriptions, not
only tooltips. In particular, inspect:

- engine mode and optimization
- lossy compression
- resize method
- color method and dithering
- disposal and frame timing
- loop count
- threads and metadata

Open the Guide tab and confirm that it explains the terms without requiring the
user to already know gifsicle terminology.

## 4. Output and progress test

1. Choose an output folder that is easy to inspect.
2. Run the optimization.
3. While it is running, confirm that:
   - the Run button is disabled or otherwise clearly busy;
   - progress shows activity;
   - the activity log receives entries;
   - the command/status area identifies the current operation.
4. When it finishes, open the Output tab.
5. Confirm that the Files on disk list shows the actual verified output path.
6. Use `Open selected file` and `Open output folder`.
7. Confirm that the After preview is labelled as the file written to disk, not
   only as a temporary settings preview.
8. Change the selected output or input and confirm that stale After content is
   cleared or replaced rather than being presented as current.

## 5. Large-GIF test

Use the large GIF that motivated the UI work, or another GIF larger than 32 MB.
The expected behavior is:

- selecting or changing a setting does not repeatedly force a full temporary
  re-encode;
- the preview area explains that automatic preview is paused for a large GIF;
- clicking `Preview changes` explicitly starts a preview when requested;
- the normal Run operation still processes the GIF;
- progress, status, and activity log remain understandable;
- the verified output appears in the selected destination;
- the After preview loads the verified destination file.

Do not judge a large file as failed merely because automatic settings preview
is paused. That pause is intentional to keep the UI responsive.

## 6. Cancellation and repeat-run test

1. Start a run on a large or multi-file input.
2. Click Cancel while the engine is active.
3. Confirm the UI returns to an idle state and reports cancellation.
4. Confirm there is no lingering gifsicle process.
5. Run the same job again.
6. Confirm the second run does not reuse the first run's stale output, log, or
   progress state.

For Explode mode, confirm that the Output tab identifies the frame folder and
that a cancelled run is not presented as a complete frame set.

## 7. Command-line regression checks

From `working_code/gifscythe`:

```bash
./build.sh
./scripts/test_engine.sh
./scripts/smoke_cli.sh
```

If Qt6 is installed, also run:

```bash
./build.sh --all
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=ON
cmake --build build-cmake
QT_QPA_PLATFORM=offscreen ./build-cmake/test_gui_offscreen
```

## 8. What to report if something fails

Record the following rather than only saying that the app did not work:

- Windows version (`winver`) or Linux distribution
- whether the app came from a source build or portable package
- branch/commit or package build date
- the first visible error message
- the input GIF size and path shape, including whether the path contains spaces
- the exact step above that failed
- a screenshot of the relevant tab or a copied activity log

For a build failure, include the first compiler or PowerShell error. For a
runtime failure, include the activity log and the Output paths shown by the
application.
