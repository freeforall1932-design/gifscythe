# Testing the current Gifscythe desktop build

Use this checklist after building from `INSTALL.md`. PR #18 is merged into
`main`; record the exact commit or follow-up branch under test. Do not treat an
older portable release as proof of current GUI changes.

**Safety first:** the S48 audit found unresolved collision and verification
issues. Test with disposable copies and use a separate empty output folder,
especially for Explode. See the current `WORKLIST.md` before testing an existing
file, a time-dependent output name, or Source-folder persistence.

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
3. Confirm the tabs are `Input`, `Actions`, `Output`, `Status`, `Settings`,
   and `About`; the guide is inside About. The footer has `Convert` and a
   cancel control that says `Stop` during a run.
4. Confirm the status bar does not report that the engine is missing.
5. Add a small animated GIF using `Add files…` or drag-and-drop. Check the
   `Filter` placeholder, its two queue-only removal options, and the single
   thumbnail-size menu. Do not run removal tests on valuable originals.
6. Open Actions and confirm the selected source appears in the one large
   aspect-fitted viewport. Switch its `Before` and `After` tabs; the full image
   should remain visible without stretching or cropping.
7. The filename template and token picker belong on **Output**; there is no
   visible Start index control. Check the `▼`/`▲` collision button and
   left-aligned options.
8. Turn off `Auto-play previews`, change an Action, then click **Apply setting
   preview**. Confirm a preview is generated without starting playback
   automatically; changing settings alone must not start an encoder.
9. Click `Play` and `Stop` and check playback. These controls must not start
   another encode. Explode and non-GIF settings previews are unavailable and
   should say so rather than display a simulated result.

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

Open **About** and confirm that its guide explains the terms without requiring
the user to already know gifsicle terminology.

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

- selecting or changing a setting does not start a temporary re-encode;
- clicking **Apply setting preview** explicitly starts one preview when
  requested (this manual-only rule applies to GIFs of every size);
- the normal Run operation still processes the GIF;
- progress, status, and activity log remain understandable;
- the verified output appears in the selected destination;
- the After preview loads the verified destination file.

Do not judge the lack of an automatic settings preview as a failure: preview
encoding is deliberately on demand for all GIF sizes.

## 6. Cancellation and repeat-run test

1. Start a run on a large or multi-file input.
2. Click Cancel while the engine is active.
3. Confirm the UI returns to an idle state and reports cancellation.
4. Confirm there is no lingering gifsicle process.
5. Run the same job again.
6. Confirm the second run does not reuse the first run's stale output, log, or
   progress state.

For Explode mode, use disposable copies and a separate empty output folder;
confirm that Output identifies the frame folder and a cancelled run is not
presented as a complete frame set. Existing frame/source protection remains an
open S48 finding, not a safety claim.

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
