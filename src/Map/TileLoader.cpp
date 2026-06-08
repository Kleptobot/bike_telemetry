#include "TileLoader.hpp"
#include <cmath>
#include <cstdio>

using namespace TileLoader;

void TileLoader::latLonToTileXY(double lat, double lon, int zoom, uint32_t& outX, uint32_t& outY) {
    // Web Mercator / Slippy tile formula
    double latRad = lat * M_PI / 180.0;
    uint32_t n = 1u << zoom;
    double xtile = (lon + 180.0) / 360.0 * n;
    double ytile = (1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / M_PI) / 2.0 * n;
    outX = (uint32_t)floor(xtile);
    outY = (uint32_t)floor(ytile);
}

void TileLoader::latLonToTilePixel(double lat, double lon, int zoom, int& pixelX, int& pixelY) {
    double latRad = lat * M_PI / 180.0;
    double n = (double)(1u << zoom);
    double xtile = (lon + 180.0) / 360.0 * n;
    double ytile = (1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / M_PI) / 2.0 * n;

    double fracX = xtile - floor(xtile);
    double fracY = ytile - floor(ytile);

    pixelX = (int)floor(fracX * TILE_SIZE);
    pixelY = (int)floor(fracY * TILE_SIZE);
}

static String tilePath(int z, uint32_t x, uint32_t y) {
    // path: /tiles/{z}/{x}/{y}.raw
    char buf[128];
    snprintf(buf, sizeof(buf), "/tiles/%d/%" PRIu32 "/%" PRIu32 ".raw", z, x, y);
    return String(buf);
}

bool TileLoader::tileExists(int z, uint32_t x, uint32_t y) {
    auto storage = HAL::inst().SD();
    if (!storage) return false;
    String p = tilePath(z, x, y);
    return storage->exists(p.c_str());
}

static bool loadRawTileImpl(int z, uint32_t x, uint32_t y,
                             uint16_t* destBuf, int bufStride,
                             int destX, int destY,
                             int srcX, int srcY,
                             int copyW, int copyH) {
    auto storage = HAL::inst().SD();
    if (!storage) return false;
 
    char buf[128];
    snprintf(buf, sizeof(buf), "/tiles/%d/%" PRIu32 "/%" PRIu32 ".raw", z, x, y);
 
    if (!storage->exists(buf)) return false;
 
    File32 f = storage->openFile(buf, O_RDONLY);
    if (!f) {
        Serial.printf("[TileLoader] failed open %s\n", buf);
        return false;
    }
 
    // Clamp src region to tile bounds.
    if (srcX < 0) { copyW += srcX; destX -= srcX; srcX = 0; }
    if (srcY < 0) { copyH += srcY; destY -= srcY; srcY = 0; }
    if (srcX >= TileLoader::TILE_SIZE || srcY >= TileLoader::TILE_SIZE) { f.close(); return false; }
    if (srcX + copyW > TileLoader::TILE_SIZE) copyW = TileLoader::TILE_SIZE - srcX;
    if (srcY + copyH > TileLoader::TILE_SIZE) copyH = TileLoader::TILE_SIZE - srcY;
    if (copyW <= 0 || copyH <= 0) { f.close(); return false; }
 
    static uint16_t rowBuf[TileLoader::TILE_SIZE];
 
    bool success = true;
    for (int row = 0; row < copyH; ++row) {
        uint32_t rowOffset = (uint32_t)(srcY + row) * TileLoader::TILE_SIZE * 2u
                           + (uint32_t)(srcX * 2);
        if (!f.seekSet(rowOffset)) { success = false; break; }
 
        size_t bytesToRead = (size_t)(copyW * 2);
        if (f.read(rowBuf, bytesToRead) != (int)bytesToRead) { success = false; break; }
 
        // Write into dest buffer using caller-supplied stride.
        uint16_t* dst = destBuf + (destY + row) * bufStride + destX;
        memcpy(dst, rowBuf, bytesToRead);
    }
 
    f.close();
    return success;
}

bool TileLoader::loadRawTileToCanvas(int z, uint32_t x, uint32_t y, int destX, int destY, int srcX, int srcY, int copyW, int copyH) {
    // Clamp dest to canvas bounds before delegating.
    if (destX >= SCREEN_WIDTH || destY >= SCREEN_HEIGHT) return false;
    if (destX < 0) { srcX -= destX; copyW += destX; destX = 0; }
    if (destY < 0) { srcY -= destY; copyH += destY; destY = 0; }
    if (copyW <= 0 || copyH <= 0) return false;
    if (destX + copyW > SCREEN_WIDTH)  copyW = SCREEN_WIDTH  - destX;
    if (destY + copyH > SCREEN_HEIGHT) copyH = SCREEN_HEIGHT - destY;
 
    uint16_t* canvasBuf = canvas.getBuffer()
                        + destY * SCREEN_WIDTH + destX;
 
    // Pass canvas buffer with SCREEN_WIDTH stride; destX/Y are already
    // baked into the pointer so pass 0,0 as the dest offset.
    return loadRawTileImpl(z, x, y, canvasBuf - destY * SCREEN_WIDTH - destX,
                           SCREEN_WIDTH, destX, destY, srcX, srcY, copyW, copyH);
}

bool TileLoader::loadRawTileToBuffer(int z, uint32_t x, uint32_t y, uint16_t* destBuf, int bufStride, int destX, int destY, int srcX, int srcY, int copyW, int copyH) {
    return loadRawTileImpl(z, x, y, destBuf, bufStride,
                           destX, destY, srcX, srcY, copyW, copyH);
}
