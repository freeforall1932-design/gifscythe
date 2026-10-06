@echo off
REM build_portable_windows.bat - double-clickable wrapper around build_portable_windows.ps1
REM
REM Builds the Windows portable Gifscythe and zips it. Everything lands under
REM %USERPROFILE%\gifscythe-build unless you pass a different -WorkRoot.
REM
REM Pass-through: any arguments you give this .bat go to the .ps1, so
REM     build_portable_windows.bat -EngineCliOnly
REM     build_portable_windows.bat -Smoke
REM     build_portable_windows.bat -WorkRoot D:\gs
REM all work. Add -NoPause for unattended runs.
setlocal
set "PS1=%~dp0build_portable_windows.ps1"
if not exist "%PS1%" (
  echo ERROR: build_portable_windows.ps1 is not next to this .bat - keep the two files together.
  pause
  exit /b 1
)

REM -ExecutionPolicy Bypass: this .bat only runs the script beside it (no network fetch of code).
powershell -NoProfile -ExecutionPolicy Bypass -File "%PS1%" %*
set "RC=%ERRORLEVEL%"

echo.
if "%RC%"=="0" (
  echo Build finished. The zip is under %USERPROFILE%\gifscythe-build\dist - see the sha256 above.
) else (
  echo Build FAILED with exit code %RC%. Scroll up for the first error: that line is the real one.
)
echo.
echo Close this window when done.
pause
exit /b %RC%
