// Diagnostic: drive the REAL FITLogger for a ~10 s ride, then dump and strictly
// validate the bytes it produced. Reports whatever a third-party parser
// (GPXSee, fitfiletools, Garmin Connect) would object to.
//
// Build (from sim/):
//   g++ -Wall -std=gnu++17 -DARDUINO=10800 -Istubs -I../src
//       tests/FitDiag.cpp ../src/Loggers/FITLogger.cpp ../src/Loggers/FitWriter.cpp
//       ../src/HAL/SDCard.cpp stubs/Arduino.cpp stubs/SdFat.cpp -o build/obike-fit-diag

#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

#include "Loggers/FITLogger.hpp"
#include "Loggers/ILogger.hpp"
#include "HAL/SDCard.hpp"

static void makeDir(const char* d) {
#ifdef _WIN32
    ::_mkdir(d);
#else
    ::mkdir(d, 0755);
#endif
}

static uint16_t crcUpdate(uint16_t crc, uint8_t b) {
    static const uint16_t table[16] = {
        0x0000,0xCC01,0xD801,0x1400,0xF001,0x3C00,0x2800,0xE401,
        0xA001,0x6C00,0x7800,0xB401,0x5000,0x9C01,0x8801,0x4400};
    uint16_t tmp = table[crc & 0xF];
    crc = (crc >> 4) & 0x0FFF;
    crc = crc ^ tmp ^ table[b & 0xF];
    tmp = table[crc & 0xF];
    crc = (crc >> 4) & 0x0FFF;
    crc = crc ^ tmp ^ table[(b >> 4) & 0xF];
    return crc;
}

struct Def {
    bool     used;
    uint16_t global;
    uint8_t  nFields;
    uint8_t  fnum[64];
    uint8_t  fsize[64];
};

int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    const char* card = (argc > 1) ? argv[1] : "fit-diag.dir";
    static const long long T0 = 1750000000LL;
    // FITLogger::startLogging names the file from the timestamp it is given,
    // so re-derive that name rather than assuming one.
    static char path[64];
    snprintf(path, sizeof(path), "/activity_%lld.fit", T0);

    makeDir(card);
    simSetSdRoot(card);

    SDCardSystem sd;
    if (!sd.init(nullptr)) { printf("[diag] sd init failed\n"); return 2; }

    // ---- A deliberately realistic ~10 s ride through the real logger ----
    {
        FITLogger logger(&sd);
        timeData t0(T0);
        logger.startLogging(t0);

        for (int i = 0; i <= 10; i += 2) {
            Trackpoint tp;
            tp.latitude  = -37.8 + i * 0.0001;
            tp.longitude = 144.9 + i * 0.0001;
            tp.altitude  = 42.0f;
            tp.speed     = 20.0f + i;          // km/h
            tp.heartrate = 130.0f;
            tp.power     = 200.0f;
            tp.cadence   = 85.0f;
            tp.distance  = i * 11.0f;          // metres cumulative
            tp.calories  = i * 0.5f;
            logger.addTrackpoint(tp, timeData(T0 + i));
        }

        // Reproduce the real stop path: App::update handles PauseLogging (or
        // auto-pause) by calling _logger->pause(now) BEFORE finaliseLogging().
        // ILogger::pause() shifts _startTime and every lap start forward, which
        // is what the file's duration is derived from.
        if (argc > 3 && strcmp(argv[3], "pause") == 0) {
            logger.pause(timeData(T0 + 10));
            printf("[diag] called pause() at T0+10 before finalising\n");
        }

        bool ok = logger.finaliseLogging();
        printf("[diag] finaliseLogging=%d\n", (int)ok);
    }
    // ---- Read the raw bytes back ----
    File32 f = sd.openFile(path, O_RDONLY);
    if (!f) { printf("[diag] reopen failed\n"); return 2; }
    static uint8_t buf[8192];
    int n = f.read(buf, sizeof(buf));
    f.close();
    if (n <= 0) { printf("[diag] read failed\n"); return 2; }
    const uint32_t fileLen = (uint32_t)n;
    printf("[diag] file length = %u bytes\n\n", fileLen);

    for (uint32_t i = 0; i < fileLen; i += 16) {
        printf("  %04u: ", i);
        for (uint32_t j = 0; j < 16; j++) {
            if (i + j < fileLen) printf("%02X ", buf[i + j]); else printf("   ");
        }
        printf(" |");
        for (uint32_t j = 0; j < 16 && i + j < fileLen; j++) {
            uint8_t c = buf[i + j];
            printf("%c", (c >= 32 && c < 127) ? c : '.');
        }
        printf("|\n");
    }
    printf("\n");

if (fileLen < 14) { printf("[diag] FAIL: shorter than a FIT header\n"); return 1; }

    // ---- Header ----
    const uint8_t headerSize = buf[0];
    const uint8_t protoVer   = buf[1];
    uint16_t profileVer; memcpy(&profileVer, &buf[2], 2);
    uint32_t dataSize;   memcpy(&dataSize, &buf[4], 4);
    char magic[5] = { (char)buf[8], (char)buf[9], (char)buf[10], (char)buf[11], 0 };
    uint16_t headerCrc;  memcpy(&headerCrc, &buf[12], 2);

    printf("HEADER\n");
    printf("  header_size      = %u      (must be 12 or 14)\n", headerSize);
    printf("  protocol_version = 0x%02X   -> %u.%u\n", protoVer, protoVer >> 4, protoVer & 0x0F);
    printf("  profile_version  = %u (0x%04X) -> major %u minor %u\n",
           profileVer, profileVer, profileVer >> 8, profileVer & 0xFF);
    printf("  data_size        = %u\n", dataSize);
    printf("  magic            = \"%s\"  (must be \".FIT\")\n", magic);
    printf("  header_crc       = 0x%04X %s\n", headerCrc,
           headerCrc == 0 ? "(0 => not present)" : "");
    printf("  expect file len  = %u + %u + 2 = %u (actual %u) %s\n",
           headerSize, dataSize, headerSize + dataSize + 2, fileLen,
           (headerSize + dataSize + 2 == fileLen) ? "OK" : "*** MISMATCH ***");

    if (strcmp(magic, ".FIT") != 0) printf("[diag] FAIL: bad magic\n");
    if (headerSize != 12 && headerSize != 14) printf("[diag] FAIL: bad header_size\n");
    if (headerSize + dataSize + 2 != fileLen) printf("[diag] FAIL: data_size inconsistent\n");

    // ---- Walk the record stream strictly (desync detector) ----
    Def defs[16];
    memset(defs, 0, sizeof(defs));

    uint32_t off = headerSize;
    const uint32_t dataEnd = headerSize + dataSize;
    uint32_t recCount = 0, defCount = 0;
    bool desync = false;

    printf("\nMESSAGES\n");
    while (off < dataEnd && !desync) {
        const uint8_t hdr = buf[off];
        if (hdr & 0x80) { printf("  [%u] compressed-timestamp header\n", off); desync = true; break; }
        const bool isDef = (hdr & 0x40) != 0;
        const uint8_t lt = hdr & 0x0F;

        if (isDef) {
            if (off + 6 > dataEnd) { printf("  [%u] definition truncated\n", off); desync = true; break; }
            uint16_t global; memcpy(&global, &buf[off + 3], 2);
            const uint8_t nf = buf[off + 5];
            if (off + 6 + (uint32_t)nf * 3 > dataEnd) { printf("  [%u] def fields truncated\n", off); desync = true; break; }
            defs[lt].used = true; defs[lt].global = global; defs[lt].nFields = nf;
            uint32_t payload = 0;
            for (uint32_t i = 0; i < nf; i++) {
                defs[lt].fnum[i]  = buf[off + 6 + i * 3];
                defs[lt].fsize[i] = buf[off + 6 + i * 3 + 1];
                payload += defs[lt].fsize[i];
            }
            printf("  [%u] DEF  local=%u global=%u fields=%u payload=%u B\n", off, lt, global, nf, payload);
            off += 6 + (uint32_t)nf * 3;
            defCount++;
        } else {
            if (!defs[lt].used) { printf("  [%u] DATA local=%u with NO definition\n", off, lt); desync = true; break; }
            uint32_t payload = 0;
            for (uint32_t i = 0; i < defs[lt].nFields; i++) payload += defs[lt].fsize[i];
            if (off + 1 + payload > dataEnd) {
                printf("  [%u] DATA local=%u payload=%u overruns data region\n", off, lt, payload);
                desync = true; break;
            }
            printf("  [%u] DATA local=%u global=%u payload=%u B\n", off, lt, defs[lt].global, payload);
            off += 1 + payload;
            recCount++;
        }
    }
    printf("  => consumed %u of %u data bytes (%s), %u defs, %u data msgs\n",
           off - headerSize, dataSize, (off == dataEnd && !desync) ? "exact" : "*** DESYNC ***",
           defCount, recCount);
    if (desync || off != dataEnd) printf("[diag] FAIL: stream does not tile data_size exactly\n");

// __APPEND_HERE__

    // ---- Trailing file CRC ----
    uint16_t calc = 0;
    for (uint32_t i = 0; i < dataEnd; i++) calc = crcUpdate(calc, buf[i]);
    uint16_t stored; memcpy(&stored, &buf[dataEnd], 2);
    printf("\nFILE CRC\n  computed = 0x%04X\n  stored   = 0x%04X  %s\n",
           calc, stored, calc == stored ? "OK" : "*** MISMATCH ***");
    if (calc != stored) printf("[diag] FAIL: trailing CRC wrong\n");

    // ---- Decode the SESSION/LAP values the summary reads ----
    printf("\nDECODED (summary-relevant)\n");
    off = headerSize;
    while (off < dataEnd) {
        const uint8_t hdr = buf[off];
        if (hdr & 0x80) break;
        const bool isDef = (hdr & 0x40) != 0;
        const uint8_t lt = hdr & 0x0F;
        if (isDef) { off += 6 + (uint32_t)defs[lt].nFields * 3; continue; }
        uint32_t payload = 0;
        for (uint32_t i = 0; i < defs[lt].nFields; i++) payload += defs[lt].fsize[i];
        if (defs[lt].global == 18 || defs[lt].global == 19) {
            printf("  global %u:", defs[lt].global);
            uint32_t p = off + 1;
            for (uint32_t i = 0; i < defs[lt].nFields; i++) {
                uint32_t v = 0;
                memcpy(&v, &buf[p], defs[lt].fsize[i]);
                printf(" f%u=%u", defs[lt].fnum[i], v);
                p += defs[lt].fsize[i];
            }
            printf("\n");
        }
        off += 1 + payload;
    }

    // Pass "keep" as argv[2] to leave the file on disk for external validation.
    if (!(argc > 2 && strcmp(argv[2], "keep") == 0)) {
        remove((std::string() + card + path).c_str());
#ifdef _WIN32
        _rmdir(card);
#else
        rmdir(card);
#endif
    } else {
        printf("[diag] kept %s%s\n", card, path);
    }
    printf("\n[diag] done\n");
    return 0;
}