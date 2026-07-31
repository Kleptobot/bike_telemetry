@echo off
REM OBike simulator launcher for Windows (cmd.exe, PowerShell, or double-click).
REM
REM Builds if needed, then runs. Locates the MinGW-w64 toolchain that
REM `winget install BrechtSanders.WinLibs.POSIX.UCRT` installs, and Git for
REM Windows' usr\bin (the Makefile recipes use mkdir -p and rm -rf).
REM
REM   run.bat                          interactive window, or headless if no SDL2
REM   run.bat --headless --out out     frame dump
REM   run.bat --help                   all flags

setlocal enabledelayedexpansion
cd /d "%~dp0"

set "MINGW=%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin"
if not exist "%MINGW%\g++.exe" (
    echo.
    echo   No MinGW-w64 compiler found.
    echo   Install it with:
    echo.
    echo       winget install BrechtSanders.WinLibs.POSIX.UCRT
    echo.
    exit /b 1
)

REM Git for Windows supplies the sh/mkdir/rm the Makefile recipes use.
set "GITUSR=C:\Program Files\Git\usr\bin"
if not exist "%GITUSR%\sh.exe" set "GITUSR=%ProgramFiles%\Git\usr\bin"

set "PATH=%MINGW%;%GITUSR%;%PATH%"

if not exist "..\.pio\libdeps" (
    echo.
    echo   .pio\libdeps is missing. Run `pio run` in the repo root once first --
    echo   the simulator links the same library sources the firmware uses.
    echo.
    exit /b 1
)

dir /b /s "third_party\libSDL2.dll.a" >nul 2>&1
if errorlevel 1 (
    echo   [note] SDL2 not found -- building headless only.
    echo          For the interactive window run: tools\get-sdl2.bat
    echo.
)

REM Seed a fresh card from the sample. Never overwrites an existing sdcard/,
REM so settings changed in the simulator survive.
if not exist "sdcard" (
    if exist "sample-sd" (
        mkdir sdcard
        copy /y "sample-sd\*" sdcard\ >nul
        echo   [sim] seeded sdcard\ from sample-sd\
    )
)

"%MINGW%\mingw32-make.exe" all
if errorlevel 1 (
    echo.
    echo   Build failed.
    exit /b 1
)

build\obike-sim.exe %*
endlocal
