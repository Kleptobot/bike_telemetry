#!/usr/bin/env bash
# Fetch the SDL2 MinGW SDK the simulator's windowed frontend links against.
# Optional: without it the build falls back to headless and still runs.
set -e
VER="${1:-2.30.9}"
cd "$(dirname "$0")/.."
mkdir -p third_party && cd third_party

if [ -d "SDL2-$VER/x86_64-w64-mingw32" ]; then
    echo "SDL2 $VER already present."
    exit 0
fi

URL="https://github.com/libsdl-org/SDL/releases/download/release-$VER/SDL2-devel-$VER-mingw.tar.gz"
echo "Downloading SDL2 $VER ..."
curl -fSL --retry 3 -o sdl2.tar.gz "$URL"
tar xzf sdl2.tar.gz
rm -f sdl2.tar.gz

# The SDK ships 32- and 64-bit trees; only the 64-bit one is used.
rm -rf "SDL2-$VER/i686-w64-mingw32"

echo "SDL2 $VER ready. Rebuild with: mingw32-make"
