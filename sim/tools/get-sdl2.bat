@echo off
REM Fetch the SDL2 MinGW SDK for the simulator's windowed frontend.
REM Optional: without it the build falls back to headless and still runs.
setlocal
set "VER=%~1"
if "%VER%"=="" set "VER=2.30.9"
cd /d "%~dp0.."

if exist "third_party\SDL2-%VER%\x86_64-w64-mingw32" (
    echo SDL2 %VER% already present.
    exit /b 0
)

if not exist third_party mkdir third_party
cd third_party

echo Downloading SDL2 %VER% ...
curl -fSL --retry 3 -o sdl2.tar.gz "https://github.com/libsdl-org/SDL/releases/download/release-%VER%/SDL2-devel-%VER%-mingw.tar.gz"
if errorlevel 1 (
    echo Download failed.
    exit /b 1
)

tar xzf sdl2.tar.gz
del /q sdl2.tar.gz

REM The SDK ships 32- and 64-bit trees; only the 64-bit one is used.
if exist "SDL2-%VER%\i686-w64-mingw32" rmdir /s /q "SDL2-%VER%\i686-w64-mingw32"

echo SDL2 %VER% ready. Rebuild with: run.bat
endlocal
