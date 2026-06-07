#pragma once
#include <stdint.h>
#include <Arduino.h>
#include "HAL/HAL.hpp"
#include "display/Display.hpp"

namespace TileLoader {
    // Standard slippy tile size (pixels)
    constexpr int TILE_SIZE = 256;

    // Convert lat/lon to slippy tile X/Y at integer zoom.
    // Returns tile X and Y (integer tile coordinates).
    void latLonToTileXY(double lat, double lon, int zoom, uint32_t& outX, uint32_t& outY);

    // Convert lat/lon to pixel X/Y within the tile at given zoom.
    // Returns pixel offsets (0..TILE_SIZE-1) within the tile.
    void latLonToTilePixel(double lat, double lon, int zoom, int& pixelX, int& pixelY);

    // Check whether a raw RGB565 tile file exists on SD (path: /tiles/z/x/y.raw)
    bool tileExists(int z, uint32_t x, uint32_t y);

    // Load a raw RGB565 tile from SD and blit a (possibly cropped) region into the global canvas at destX,destY.
    // The raw file must be stored row-major RGB565 (2 bytes per pixel), TILE_SIZE wide.
    // srcX/srcY define the top-left pixel within the tile to copy from (0..TILE_SIZE-1).
    // copyW/copyH define the size to copy. Returns true on success.
    bool loadRawTileToCanvas(int z, uint32_t x, uint32_t y, int destX, int destY, int srcX = 0, int srcY = 0, int copyW = TILE_SIZE, int copyH = TILE_SIZE);

    bool loadRawTileToBuffer(int z, uint32_t x, uint32_t y, uint16_t* destBuf, int bufStride, int destX, int destY, int srcX, int srcY, int copyW, int copyH);
}
