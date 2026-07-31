#!/usr/bin/env bash
# Convenience wrapper: build if needed, then run.
#   ./run.sh                    interactive (or headless if SDL2 is absent)
#   ./run.sh --headless --out out
set -e
cd "$(dirname "$0")"
WINLIBS="$LOCALAPPDATA/Microsoft/WinGet/Packages/BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe/mingw64/bin"
[ -d "$WINLIBS" ] && export PATH="$WINLIBS:$PATH"
# Seed a fresh card from the sample. Never overwrites an existing sdcard/,
# so settings you change in the simulator survive.
if [ ! -d sdcard ] && [ -d sample-sd ]; then
    mkdir -p sdcard && cp sample-sd/* sdcard/
    echo "[sim] seeded sdcard/ from sample-sd/"
fi

MAKE=$(command -v mingw32-make || command -v make)
"$MAKE" all
exec ./build/obike-sim "$@"
