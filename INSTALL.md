# Installing and building Gifscythe

This is the installation guide for the **current source tree**. It builds the
Qt6 GUI from source; it does not point at an older prebuilt release.

The current Arena development branch is:

```text
arena/dde1f972-gifscythe
```

Gifscythe has two different things that are often called a "ZIP":

- A **source ZIP** from GitHub's Code menu contains C++ source and build
  scripts. It must be built before the GUI can run.
- A **portable package ZIP** contains `gifscythe.exe`, the gifsicle engine,
  and the Qt runtime. It can be extracted and run without installing Qt.

## Windows: build the current Qt GUI

This is the recommended path for testing the current desktop changes. The
provided PowerShell script installs the build dependencies into a work folder
and then builds a portable Windows package.

### Requirements

- Windows 10 or 11, 64-bit
- Git for Windows, including `bash.exe`
- Python 3 on `PATH`
- Internet access for the first build
- About 3 GB of free disk space for Qt, MinGW, and build files

The script downloads Qt6, MinGW, CMake, and Ninja. You do **not** need to
install Qt manually when using this script.

### 1. Get the current source

For this development branch, download the source archive from:

<https://github.com/freeforall1932-design/gifscythe/archive/refs/heads/arena/dde1f972-gifscythe.zip>

Extract it. The examples below assume the extracted folder is:

```text
C:\Users\Administrator\Videos\Gifscythe
```

The folder may contain another top-level directory if Windows extracted the
archive using GitHub's default name. The commands below search recursively and
handle that layout.

### 2. Check the prerequisites

Open PowerShell and paste:

```powershell
$Root = 'C:\Users\Administrator\Videos\Gifscythe'

if (-not (Test-Path -LiteralPath $Root)) {
    throw "Folder not found: $Root"
}

git --version
python --version
```

If either command is not found, install Git for Windows and Python 3, then
close and reopen PowerShell. On a machine with `winget`, the following can be
used:

```powershell
winget install --id Git.Git -e --source winget
winget install --id Python.Python.3.12 -e --source winget
```

### 3. Find and run the builder

Paste this in the same PowerShell window:

```powershell
$Builder = Get-ChildItem `
    -LiteralPath $Root `
    -Filter 'build_portable_windows.ps1' `
    -File `
    -Recurse `
    -ErrorAction SilentlyContinue |
    Select-Object -First 1

if ($null -eq $Builder) {
    throw "build_portable_windows.ps1 was not found. Check that the current source ZIP was extracted."
}

Write-Host "Using build script:" -ForegroundColor Green
Write-Host $Builder.FullName

& powershell.exe `
    -NoProfile `
    -ExecutionPolicy Bypass `
    -File $Builder.FullName

if ($LASTEXITCODE -ne 0) {
    throw "Gifscythe build failed with exit code $LASTEXITCODE"
}
```

To run the CLI smoke test as part of the build, add `-Smoke` to the final
command:

```powershell
& powershell.exe `
    -NoProfile `
    -ExecutionPolicy Bypass `
    -File $Builder.FullName `
    -Smoke
```

Do not continue past a failed build. The first error in the PowerShell output
is the useful one; later packaging errors are often consequences of it.

### 4. Start the newly built GUI

After a successful build, paste:

```powershell
$ProductFolder = (Get-Item (Join-Path $Builder.DirectoryName '..')).FullName
$ReleaseFolder = Join-Path $ProductFolder 'release'

$Gui = Get-ChildItem `
    -LiteralPath $ReleaseFolder `
    -Filter 'gifscythe.exe' `
    -File `
    -Recurse `
    -ErrorAction SilentlyContinue |
    Select-Object -First 1

if ($null -eq $Gui) {
    throw "The build completed but gifscythe.exe was not found under release."
}

Unblock-File -Path $Gui.FullName -ErrorAction SilentlyContinue
Write-Host "Starting: $($Gui.FullName)" -ForegroundColor Green

Start-Process `
    -FilePath $Gui.FullName `
    -WorkingDirectory $Gui.DirectoryName
```

The build also creates a portable ZIP under:

```text
C:\Users\Administrator\gifscythe-build\dist
```

That ZIP can be copied to another Windows machine. The destination machine
does not need Qt, MinGW, CMake, or Python.

## Windows: running an already-built portable package

If the folder already contains `gifscythe.exe`, no compilation is needed. This
is the fully offline path:

```powershell
$Root = 'C:\Users\Administrator\Videos\Gifscythe'

$Gui = Get-ChildItem `
    -LiteralPath $Root `
    -Filter 'gifscythe.exe' `
    -File `
    -Recurse `
    -ErrorAction SilentlyContinue |
    Select-Object -First 1

if ($null -eq $Gui) {
    throw "No compiled gifscythe.exe was found. This is probably a source ZIP."
}

Unblock-File -Path $Gui.FullName -ErrorAction SilentlyContinue
Start-Process `
    -FilePath $Gui.FullName `
    -WorkingDirectory $Gui.DirectoryName
```

The GUI should have `gifsicle.exe` beside it. If it does not, the package is
incomplete and the GUI may report that the engine cannot be found.

## Linux: build the Qt GUI

On Ubuntu or Debian, install the compiler, CMake, Qt6, and the GIF image plugin:

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-base-dev-tools qt6-image-formats-plugins
```

Then build from the product directory:

```bash
cd working_code/gifscythe
./build.sh --all
./build/gifscythe
```

The Qt-free engine and CLI build can be run separately with:

```bash
./build.sh
```

## Build and run the offscreen GUI test harness

With Qt6 and CMake installed:

```bash
cd working_code/gifscythe
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=ON
cmake --build build-cmake
QT_QPA_PLATFORM=offscreen ./build-cmake/test_gui_offscreen
```

The harness does not open a visible window. It verifies the UI contracts,
preview lifecycle, cancellation behavior, output handling, and the Guide and
playback controls.

## Offline limitations

A source ZIP alone is not enough to compile offline. The first source build
needs Qt6, MinGW, CMake, Ninja, Git, and Python either installed already or
available in local caches. The practical offline workflow is:

1. Build the current source on an internet-connected Windows machine.
2. Keep the generated portable ZIP from `gifscythe-build\dist`.
3. Copy and extract that ZIP on the offline machine.
4. Run `gifscythe.exe` from the extracted folder.

Do not use an older release package to test current source changes; build the
branch named at the top of this document or use a portable package produced by
that branch's successful CI run.

## Common failures

| Symptom | Likely cause | Action |
|---|---|---|
| `gifscythe.exe` is missing | A source ZIP was downloaded | Run the Windows builder above |
| `build_portable_windows.ps1` is missing | The archive is incomplete or the wrong folder was selected | Search from the extracted folder with `Get-ChildItem -Recurse` |
| `git` is not recognized | Git for Windows is not installed or PowerShell was not restarted | Install Git for Windows and reopen PowerShell |
| `python` is not recognized | Python is not installed or not on `PATH` | Install Python 3 with the PATH option enabled |
| Qt DLL missing when launching | The app was copied without the portable runtime | Use the complete generated portable folder/ZIP |
| Engine not found | `gifsicle.exe` is not beside the GUI | Rebuild or restore the complete portable package |
| Build works but preview cannot decode GIFs on Linux | Qt image-format plugin is missing | Install `qt6-image-formats-plugins` |
