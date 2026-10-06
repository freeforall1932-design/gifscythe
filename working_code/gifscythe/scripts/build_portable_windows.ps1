<#
  build_portable_windows.ps1 - build the Windows portable package on YOUR machine, and zip it.

  What you get:
      <repo>\working_code\gifscythe\release\<version>\Gifscythe\   the portable tree
                                                        (engine + CLI + Qt6 GUI + windeployqt runtime)
      <WorkRoot>\dist\Gifscythe-<version>-windows-portable.zip    that folder, zipped, ready to carry
      <WorkRoot>\dist\SHA256SUMS                                  the hash, also printed at the end

  Run it (no admin rights; everything lands under -WorkRoot):

      # easiest: double-click scripts\build_portable_windows.bat
      # or, from PowerShell in working_code\gifscythe:
      powershell -ExecutionPolicy Bypass -File .\scripts\build_portable_windows.ps1

      # engine + CLI only, no Qt, much faster and smaller:
      powershell -ExecutionPolicy Bypass -File .\scripts\build_portable_windows.ps1 -EngineCliOnly

      # also run the CLI smoke on the staged tree (see the W-18 note it prints):
      powershell -ExecutionPolicy Bypass -File .\scripts\build_portable_windows.ps1 -Smoke

  Requirements (the script fetches everything else):
      Git for Windows - it provides the bash that every build script in this repo is written in
      Python 3 on PATH - aqtinstall (Qt + MinGW), cmake and ninja are pip-installed into -WorkRoot
      ~3 GB free disk and a network; the first run downloads Qt 6.7.3 (~1.5 GB) and later runs reuse it

  PROVENANCE - read before trusting this:
  Every command below is transcribed from the `windows` job of .github/workflows/build.yml, which is
  GREEN on windows-latest (run 37518884447) and is what produced the published
  Gifscythe-0.1.0-windows-portable.zip. The transcription itself - this file - has never been
  EXECUTED: the sandbox that wrote it has no Windows and no PowerShell, so this is a careful copy of
  a proven recipe, not a proven script. If a step fails, the fix belongs in both places at once (here
  and the workflow), and the failure text is worth sending back verbatim.

  The alternative that needs no local toolchain at all: a snapshot tag makes CI do all of this and
  publish the zip as a Release asset - see docs/ci/WINDOWS_TEST_RUNSHEET.md section 0.
#>
#Requires -Version 5.1
[CmdletBinding()]
param(
  [string]$WorkRoot = (Join-Path $env:USERPROFILE 'gifscythe-build'),
  [string]$QtVersion = '6.7.3',
  [switch]$EngineCliOnly,
  [switch]$Smoke
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Say([string]$m) { Write-Host "==> $m" -ForegroundColor Cyan }
function Fail([string]$m) { Write-Host "ERROR: $m" -ForegroundColor Red; exit 1 }

# Bash wants POSIX paths: C:\a\b -> /c/a/b
function To-Posix([string]$p) {
  if ([string]::IsNullOrWhiteSpace($p)) { return $p }
  $full = [System.IO.Path]::GetFullPath($p).Replace('\', '/')
  if ($full -match '^([A-Za-z]):/(.*)$') { return '/' + $Matches[1].ToLower() + '/' + $Matches[2] }
  return $full
}
function Invoke-Bash([string]$bashScript) {
  & $script:BashExe -lc $bashScript
  if ($LASTEXITCODE -ne 0) { Fail "bash step failed (exit $LASTEXITCODE) in: $($bashScript.Trim().Split("`n")[0])" }
}
function Find-Tool([string]$name) {
  $c = Get-Command $name -ErrorAction SilentlyContinue
  if ($c) { return $c.Source }
  return $null
}

# ---------------------------------------------------------------------------
# 0. Preflight
# ---------------------------------------------------------------------------
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path          # <repo>\working_code\gifscythe
if (-not (Test-Path (Join-Path $repo 'VERSION.md'))) { Fail "not the Gifscythe product dir: $repo" }

Say "repo:      $repo"
Say "work root: $WorkRoot"
New-Item -ItemType Directory -Force -Path $WorkRoot | Out-Null
$posixRepo = To-Posix $repo
$posixWorkRoot = To-Posix $WorkRoot

$script:BashExe = $null
$candidates = @()
foreach ($base in @($env:ProgramFiles, ${env:ProgramFiles(x86)})) {
  if ($base) { $candidates += (Join-Path $base 'Git\bin\bash.exe') }
}
$candidates += (Find-Tool 'bash.exe')
foreach ($cand in $candidates) {
  if ($cand -and (Test-Path $cand)) { $script:BashExe = $cand; break }
}
if (-not $script:BashExe) { Fail "Git for Windows (bash.exe) not found - install it from https://git-scm.com/download/win and re-run." }
Say "bash:      $script:BashExe"

$python = Find-Tool 'python.exe'
if (-not $python) { $python = Find-Tool 'python3.exe' }
if (-not $python) { Fail "Python 3 not found on PATH - install it from https://python.org (tick 'Add python.exe to PATH') and re-run." }
Say "python:    $python"

# ---------------------------------------------------------------------------
# 1. Toolchain: Qt6 + MinGW (aqtinstall), cmake + ninja (pip wheels)
# ---------------------------------------------------------------------------
$qtDir = Join-Path $WorkRoot 'qt'
Say "installing aqtinstall + cmake + ninja (pip --user)"
& $python -m pip install --quiet --disable-pip-version-check --user aqtinstall cmake ninja
if ($LASTEXITCODE -ne 0) { Fail "pip install failed" }
$pyScripts = (& $python -c "import sysconfig; print(sysconfig.get_path('scripts', 'nt_user'))").Trim()

$qtBin = Join-Path $qtDir "6.$($QtVersion.Split('.')[1]).$($QtVersion.Split('.')[2])\mingw_64\bin"
if (-not (Test-Path $qtBin)) {
  Say "installing Qt $QtVersion (win64_mingw) - the first run downloads about 1.5 GB"
  & $python -m aqt install-qt windows desktop $QtVersion win64_mingw -O $qtDir
  if ($LASTEXITCODE -ne 0) { Fail "aqt install-qt failed" }
} else {
  Say "Qt $QtVersion already present at $qtBin (reusing)"
}

# MinGW: aqt's tools package, same fallback chain the workflow uses.
$mingwRoot = Join-Path $qtDir 'Tools'
function Find-MingwBin {
  $hit = Get-ChildItem -Path $mingwRoot -Directory -Filter 'mingw*' -ErrorAction SilentlyContinue |
         Where-Object { Test-Path (Join-Path $_.FullName 'bin\g++.exe') } | Select-Object -First 1
  if ($hit) { return (Join-Path $hit.FullName 'bin') }
  return $null
}
$mingwBin = Find-MingwBin
if (-not $mingwBin) {
  foreach ($pkg in @('tools_mingw1310', 'tools_mingw1120', 'tools_mingw90')) {
    Say "installing MinGW ($pkg)"
    & $python -m aqt install-tool windows desktop $pkg -O $qtDir 2>$null
    $mingwBin = Find-MingwBin
    if ($mingwBin) { break }
  }
}
if (-not $mingwBin) { Fail "no MinGW with g++.exe under $mingwRoot - run 'python -m aqt list-tool windows desktop' and edit the package list in this script" }
Say "mingw:     $mingwBin"

# MinGW first: windeployqt, cmake and ninja must all see the same g++.
$env:PATH = "$mingwBin;$qtBin;$pyScripts;$env:PATH"
Say "toolchain identity:"
(& g++ --version | Select-Object -First 1) | ForEach-Object { Write-Host "    $_" }
(& cmake --version | Select-Object -First 1) | ForEach-Object { Write-Host "    $_" }
Write-Host "    ninja $(& ninja --version)"
(& windeployqt --version | Select-Object -First 1) | ForEach-Object { Write-Host "    $_" }

# ---------------------------------------------------------------------------
# 2. Engine (the vendored gifsicle, built as a Windows PE)
# ---------------------------------------------------------------------------
Say "building the engine (scripts/build_engine.sh --windows)"
# The workflow aliases the prefixed compiler name to plain gcc when MinGW ships only the latter.
Invoke-Bash @"
set -euo pipefail
cd '$posixRepo'
if ! command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
  if command -v gcc >/dev/null 2>&1; then
    mkdir -p '$posixWorkRoot/shim'
    printf '#!/bin/bash\nexec gcc "`$@"\n' > '$posixWorkRoot/shim/x86_64-w64-mingw32-gcc'
    chmod +x '$posixWorkRoot/shim/x86_64-w64-mingw32-gcc'
    export PATH='$posixWorkRoot/shim:`$PATH'
  else
    echo 'ERROR: no MinGW gcc on PATH' >&2; exit 1
  fi
fi
./scripts/build_engine.sh --windows
ls -la release/*/gifsicle.exe
"@

# ---------------------------------------------------------------------------
# 3. CLI + unit tests (static, so the zip runs where no MinGW exists)
# ---------------------------------------------------------------------------
Say "building the CLI + unit tests (-static)"
Invoke-Bash @"
set -euo pipefail
cd '$posixRepo'
version=`$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' VERSION.md | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
mkdir -p build
printf '%s\n' \
  '#ifndef GIFSCYTHE_CORE_VERSION_H' \
  '#define GIFSCYTHE_CORE_VERSION_H' \
  "#define GS_VERSION \"`$version\"" \
  "#define GS_VERSION_STR \"`$version\"" \
  '#endif' > src/core/version.h
g++ -std=c++17 -O2 -static -I src -o build/gifscythe-cli.exe src/cli/main.cpp
g++ -std=c++17 -O2 -static -I src -o build/test_gifsicle_command.exe tests/test_gifsicle_command.cpp
./build/test_gifsicle_command.exe
"@

# ---------------------------------------------------------------------------
# 4. GUI (CMake + Ninja + MinGW) and the Qt runtime
# ---------------------------------------------------------------------------
if ($EngineCliOnly) {
  Say "engine + CLI only (-EngineCliOnly): skipping the GUI, Qt and windeployqt steps"
} else {
  $posixQt = To-Posix $qtBin
  Say "building the Qt6 GUI (cmake -G Ninja, MinGW Makefiles as fallback)"
  Invoke-Bash @"
set -euo pipefail
cd '$posixRepo'
qt_prefix=`$(dirname '$posixQt')
if ! cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
      -DCMAKE_PREFIX_PATH="`$qt_prefix" -DBUILD_GUI=ON; then
  cmake -S . -B build-win -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
      -DCMAKE_PREFIX_PATH="`$qt_prefix" -DBUILD_GUI=ON
fi
cmake --build build-win --config Release
GUI_EXE=""
for c in build-win/gifscythe.exe build-win/Release/gifscythe.exe build-win/Debug/gifscythe.exe; do
  if [ -f "`$c" ]; then GUI_EXE="`$c"; break; fi
done
if [ -z "`$GUI_EXE" ]; then
  echo 'ERROR: GUI executable not found after the cmake build' >&2
  find build-win -name 'gifscythe*' || true
  exit 1
fi
windeployqt --release --no-translations "`$GUI_EXE"
version=`$(grep -oE 'Current version:.*[0-9]+\.[0-9]+\.[0-9]+' VERSION.md | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
if [ -f "release/`$version/gifsicle.exe" ]; then cp "release/`$version/gifsicle.exe" "`$(dirname "`$GUI_EXE")/"; fi
if [ -f build/gifscythe-cli.exe ]; then cp build/gifscythe-cli.exe "`$(dirname "`$GUI_EXE")/"; fi
echo "GUI built: `$GUI_EXE"
"@
}

# ---------------------------------------------------------------------------
# 5. Stage the portable folder (the repo's own fail-closed stager)
# ---------------------------------------------------------------------------
$stageFlag = if ($EngineCliOnly) { ' --engine-cli-only' } else { '' }
Say "staging the portable package (scripts/package_portable.sh --windows$stageFlag)"
Invoke-Bash "set -euo pipefail; cd '$posixRepo'; ./scripts/package_portable.sh --windows$stageFlag"

$version = (Select-String -Path (Join-Path $repo 'VERSION.md') -Pattern 'Current version:.*?(\d+\.\d+\.\d+)' |
            Select-Object -First 1).Matches[0].Groups[1].Value
if (-not $version) { Fail "could not read the product version from VERSION.md" }
$pkg = Join-Path $repo "release\$version\Gifscythe"
if (-not (Test-Path $pkg)) { Fail "expected package folder not found: $pkg" }

# ---------------------------------------------------------------------------
# 6. Assert the manifest - the same gate CI runs, so a broken zip cannot ship
# ---------------------------------------------------------------------------
Say "asserting the package manifest (the CI gate, verbatim)"
$required = @('gifsicle.exe', 'gifscythe-cli.exe')
if (-not $EngineCliOnly) { $required += 'gifscythe.exe' }
$required += @('LICENSE', 'COPYING.gifsicle', 'COPYING.ms-pl', 'COPYING.lgplv3', 'COPYING.gplv3')
if (-not $EngineCliOnly) { $required += 'QT_NOTICE.txt' }
$required += @('README.txt')

$missing = @()
foreach ($f in $required) {
  $full = Join-Path $pkg $f
  if (-not (Test-Path $full) -or (Get-Item $full).Length -eq 0) { $missing += $f }
}
if ($missing.Count -gt 0) {
  Write-Host "FAIL: the package is incomplete - missing or empty: $($missing -join ', ')" -ForegroundColor Red
  Get-ChildItem $pkg | Select-Object Name, Length | Format-Table | Out-String | Write-Host
  Fail "manifest assert failed"
}
Say "manifest OK ($($required.Count) required files present and non-empty)"

# ---------------------------------------------------------------------------
# 7. Zip it and hash it - the thing you can carry to another machine
# ---------------------------------------------------------------------------
$dist = Join-Path $WorkRoot 'dist'
New-Item -ItemType Directory -Force -Path $dist | Out-Null
$zipName = "Gifscythe-$version-windows-portable.zip"
$zip = Join-Path $dist $zipName
if (Test-Path $zip) { Remove-Item $zip -Force }
Say "zipping -> $zip"
Add-Type -AssemblyName System.IO.Compression.FileSystem
# includeBaseDirectory = $true: unzips to a self-contained Gifscythe\ folder (what W-18 expects).
[System.IO.Compression.ZipFile]::CreateFromDirectory(
  $pkg, $zip, [System.IO.Compression.CompressionLevel]::Optimal, $true)

$hash = (Get-FileHash -Path $zip -Algorithm SHA256).Hash.ToLower()
$bytes = (Get-Item $zip).Length
"$hash  $zipName" | Set-Content -Path (Join-Path $dist 'SHA256SUMS') -Encoding ascii

Write-Host ''
Say 'DONE'
Write-Host "    folder : $pkg"
Write-Host "    zip    : $zip"
Write-Host "    bytes  : $bytes"
Write-Host "    sha256 : $hash"
Write-Host ''

# ---------------------------------------------------------------------------
# 8. Optional smoke - proves the build runs HERE, not on a clean machine
# ---------------------------------------------------------------------------
if ($Smoke) {
  Say "smoke: engine identity + CLI end-to-end on the staged tree"
  $work = Join-Path $WorkRoot 'smoke'
  if (Test-Path $work) { Remove-Item $work -Recurse -Force }
  New-Item -ItemType Directory -Force -Path $work | Out-Null
  Copy-Item (Join-Path $pkg '*') $work -Recurse -Force

  $engine = Join-Path $work 'gifsicle.exe'
  $cli = Join-Path $work 'gifscythe-cli.exe'
  # The repo holds no images (N-36): materialise the text fixtures first, then use the known dir.
  & $script:BashExe -lc "cd '$posixRepo' && ./scripts/fixtures.sh" | Out-Null
  $logo = Join-Path $repo 'build\fixtures\logo.gif'

  Write-Host '    gifsicle --version:' -ForegroundColor DarkGray
  (& $engine --version | Select-Object -First 1) | ForEach-Object { Write-Host "      $_" }

  if (Test-Path $logo) {
    $in = Join-Path $work 'in.gif'; $out = Join-Path $work 'out.gif'
    Copy-Item $logo $in -Force
    "mode = auto`noptimize = 3`ninput  = $in`noutput = $out`n" |
      Set-Content -Path (Join-Path $work 'test.conf') -Encoding ascii

    Write-Host '    gifscythe-cli.exe test.conf (dry run - prints the command line):' -ForegroundColor DarkGray
    (& $cli (Join-Path $work 'test.conf')) | ForEach-Object { Write-Host "      $_" }
    Write-Host '    gifscythe-cli.exe --run test.conf:' -ForegroundColor DarkGray
    (& $cli --run (Join-Path $work 'test.conf')) | ForEach-Object { Write-Host "      $_" }
    if ($LASTEXITCODE -ne 0) { Fail "the CLI --run smoke exited $LASTEXITCODE" }
    if (-not (Test-Path $out)) { Fail 'the CLI --run smoke wrote no output GIF' }
    Write-Host "      out.gif written: $((Get-Item $out).Length) bytes" -ForegroundColor DarkGray
  } else {
    Write-Host "    (no fixture at $logo - skipping the GIF leg)" -ForegroundColor Yellow
  }
  Write-Host ''
  Write-Host 'SMOKE PASS (on this machine). This is NOT the W-18 evidence:' -ForegroundColor Yellow
  Write-Host 'W-18 needs this zip run on a machine with no Qt/MinGW/dev tools installed.' -ForegroundColor Yellow
}

Write-Host 'Next: copy the zip to a clean Windows box and follow docs\ci\WINDOWS_TEST_RUNSHEET.md (W-18/W-19).'
Write-Host 'Send back the sha256 above plus the transcripts; they become the register evidence.'
