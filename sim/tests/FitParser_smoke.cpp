// Host round-trip test: FitWriter -> FitLogParser.
//
// Regression test for the LogSummaryScreen .fit display bug: FitLogParser used
// to parse fixed 96-byte chunks independently and dropped every record that
// straddled a chunk boundary, then resumed mid-record -- desynchronizing the
// stream so LAP/SESSION values (the only ones the summary shows) were lost or
// garbage. This test writes a FIT file with the real FitWriter, mirroring
// FITLogger's message flow (definitions + interleaved RECORD/LAP/SESSION with
// unaligned lengths), then parses it back and asserts every aggregate.
//
// Build and run (from sim/):
//   g++ -Wall -std=gnu++17 -DARDUINO=10800 -Istubs -I../src
//       tests/FitParser_smoke.cpp ../src/Loggers/LogSummary.cpp
//       ../src/Loggers/FitWriter.cpp ../src/HAL/SDCard.cpp
//       stubs/Arduino.cpp stubs/SdFat.cpp -o build/obike-fit-smoke
//   ./build/obike-fit-smoke fit-smoke.dir

#include <cstdio>
#include <string>
#include <cstring>
#include <cmath>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

static void makeDir(const char* d) {
    #ifdef _WIN32
        ::_mkdir(d);
    #else
        ::mkdir(d, 0755);
    #endif
}
#include "Loggers/LogSummary.hpp"
#include "Loggers/FitWriter.hpp"
#include "HAL/SDCard.hpp"

static int failures = 0;
static void check(bool cond, const char* what, float got = 0, float want = 0) {
    if (cond) return;
    ++failures;
    if (want || got) printf("[FIT smoke] FAIL: %s (got %g want %g)\n", what, got, want);
    else printf("[FIT smoke] FAIL: %s\n", what);
}
static bool near(float a, float b, float eps = 0.01f) {
    return fabsf(a - b) <= eps;
}

int main(int argc, char** argv) {
    // Unbuffered: this test is run under a timeout, and a block-buffered stdout
    // loses the diagnostic line printed just before a runaway (which is exactly
    // how the first eof/stall bug hid itself for several runs).
    setvbuf(stdout, nullptr, _IONBF, 0);

    // The sim SD root is a DIRECTORY holding the filesystem's files.
    const char* card = (argc > 1) ? argv[1] : "fit-smoke.dir";
    makeDir(card);
    simSetSdRoot(card);

    SDCardSystem sd;
    if (!sd.init(nullptr)) { printf("[FIT smoke] sd init failed\n"); return 2; }

    // ---- Write a FIT file with the real FitWriter, FITLogger-style ----
    {
        FitWriter w(&sd);
        if (!w.open("/activity_1.fit")) {
            printf("[FIT smoke] FitWriter open failed\n"); return 2;
        }

        // Same definitions as FITLogger::writeDefinitionsOnce.
        FitFieldDef lapF[] = {
            {253, 4, FitBaseType::UINT32}, {2, 4, FitBaseType::UINT32},
            {7, 4, FitBaseType::UINT32},   {9, 4, FitBaseType::UINT32},
            {13, 2, FitBaseType::UINT16},  {15, 1, FitBaseType::UINT8},
            {16, 1, FitBaseType::UINT8},   {17, 1, FitBaseType::UINT8},
            {32, 4, FitBaseType::UINT32},
        };
        w.writeDefinition(2, FitMesg::LAP, lapF, 9);

        FitFieldDef sesF[] = {
            {253, 4, FitBaseType::UINT32}, {2, 4, FitBaseType::UINT32},
            {7, 4, FitBaseType::UINT32},   {9, 4, FitBaseType::UINT32},
            {5, 1, FitBaseType::ENUM},
        };
        w.writeDefinition(3, FitMesg::SESSION, sesF, 5);

        FitFieldDef recF[] = {
            {253, 4, FitBaseType::UINT32}, {0, 4, FitBaseType::SINT32},
            {1, 4, FitBaseType::SINT32},   {2, 2, FitBaseType::UINT16},
            {5, 4, FitBaseType::UINT32},   {6, 2, FitBaseType::UINT16},
            {3, 1, FitBaseType::UINT8},    {4, 1, FitBaseType::UINT8},
            {7, 2, FitBaseType::UINT16},   {15, 2, FitBaseType::UINT16},
        };
        w.writeDefinition(1, FitMesg::RECORD, recF, 10);

        auto writeRec = [&w](uint32_t ts) {
            w.beginDataMessage(1);
            w.writeU32(ts);          // timestamp
            w.writeS32(501234567);   // lat
            w.writeS32(512345678);   // lon
            w.writeU16(2500);        // altitude (encoded)
            w.writeU32(0);           // distance
            w.writeU16(0);           // speed
            w.writeU8(0xFF);         // hr invalid
            w.writeU8(0xFF);         // cadence invalid
            w.writeU16(0xFFFF);      // power invalid
            w.writeU16(0);           // calories
        };

        // Deliberately unaligned interleaving so records straddle any fixed
        // read size: 5 RECORDs, LAP1, 7 RECORDs, LAP2, 3 RECORDs, SESSION.
        for (int i = 0; i < 5; i++) writeRec(1000 + (uint32_t)i * 10);
        w.beginDataMessage(2);
        w.writeU32(60000);  w.writeU32(0);     w.writeU32(60000); // elapsed 60 s
        w.writeU32(12345);  w.writeU16(5000);  // 123.45 m, 5 m/s max
        w.writeU8(120);     w.writeU8(150);    w.writeU8(90);     // hr avg/max, cad
        w.writeU32(2500);                                         // 250 kcal
        for (int i = 0; i < 7; i++) writeRec(60000 + (uint32_t)i * 10);
        w.beginDataMessage(2);
        w.writeU32(150000); w.writeU32(60000); w.writeU32(90000); // elapsed 90 s
        w.writeU32(20000);  w.writeU16(8333);  // 200 m, 8.333 m/s max
        w.writeU8(130);     w.writeU8(160);    w.writeU8(95);
        w.writeU32(1000);                                         // 100 kcal
        for (int i = 0; i < 3; i++) writeRec(150000 + (uint32_t)i * 10);
        w.beginDataMessage(3);
        w.writeU32(150000); w.writeU32(0);     w.writeU32(150000); // 150 s
        w.writeU32(32345);  w.writeU8(2);      // 323.45 m, cycling
        w.close();
        printf("[FIT smoke] file written\n");
    }

    // ---- Parse it back and verify every aggregate ----
    FitLogParser p;
    if (!p.open(&sd, "/activity_1.fit")) {
        printf("[FIT smoke] parser open failed\n"); return 2;
    }
    {
        int steps = 0;
        bool runaway = false;
        while (p.step()) {
            if (++steps > 10000) { runaway = true; break; }
        }
        printf("[FIT smoke] steps=%d done=%d\n", steps, (int)p.done());
        check(!runaway, "step() terminates at EOF (no runaway)");
    }
    check(p.done(), "parser reached EOF");

    const RideStats& s = p.stats();
    check(s.valid, "stats valid");
    check(s.lapCount == 2, "lapCount == 2", (float)s.lapCount, 2.0f);
    check(s.durationSec == 150, "durationSec == 150", (float)s.durationSec, 150.0f);
    check(near(s.distanceM, 323.45f), "distanceM", s.distanceM, 323.45f);
    check(near(s.maxSpeedKmh, 29.999f, 0.01f), "maxSpeedKmh", s.maxSpeedKmh, 29.999f);
    check(near(s.calories, 350.0f), "calories", s.calories, 350.0f);
    check(near(s.maxHr, 160.0f), "maxHr", s.maxHr, 160.0f);
    check(near(s.avgCadence, 95.0f), "avgCadence", s.avgCadence, 95.0f);
    check(near(s.avgSpeedKmh, 323.45f / 150.0f * 3.6f), "avgSpeedKmh", s.avgSpeedKmh, 7.7628f);

    if (failures == 0) printf("[FIT smoke] OK\n");

    // Leave no trace: the simulator maps the card to a directory of host files,
    // so tear both the file and the directory down (::remove does not delete a
    // non-empty directory on Windows). The parser must release the file first --
    // Windows refuses to unlink a handle that is still open.
    p.close();
    ::remove((std::string(card) + "/activity_1.fit").c_str());
#ifdef _WIN32
    _rmdir(card);
#else
    rmdir(card);
#endif
    return failures ? 1 : 0;
}
