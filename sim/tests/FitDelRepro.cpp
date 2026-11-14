// Repro: deleting a log while the summary parser still holds it open.
// Build+run (from sim/):
//   g++ -Wall -std=gnu++17 -DARDUINO=10800 -Istubs -I../src
//       tests/FitDelRepro.cpp ../src/Loggers/FITLogger.cpp
//       ../src/Loggers/FitWriter.cpp ../src/Loggers/LogSummary.cpp
//       ../src/HAL/SDCard.cpp stubs/Arduino.cpp stubs/SdFat.cpp
//       -o build/obike-fitdel
//   ./build/obike-fitdel fitdel.dir
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include "Loggers/FITLogger.hpp"
#include "Loggers/LogSummary.hpp"
#include "HAL/SDCard.hpp"

static void makeDir(const char* d) {
#ifdef _WIN32
    ::_mkdir(d);
#else
    ::mkdir(d, 0755);
#endif
}

int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    const char* card = (argc > 1) ? argv[1] : "fitdel.dir";
    makeDir(card);
    simSetSdRoot(card);
    SDCardSystem sd;
    if (!sd.init(nullptr)) { printf("sd init failed\n"); return 2; }

    const long long T0 = 1750000000LL;
    const char* path = "/activity_1750000000.fit";
    {
        FITLogger logger(&sd);
        logger.startLogging(timeData(T0));
        for (int i = 0; i <= 60; i++) {   // enough records to need several steps()
            Trackpoint tp;
            tp.latitude = -37.8; tp.longitude = 144.9; tp.altitude = 42.0f;
            tp.speed = 20.0f; tp.heartrate = 130.0f; tp.power = 200.0f;
            tp.cadence = 85.0f; tp.distance = i * 5.5f; tp.calories = i * 0.5f;
            logger.addTrackpoint(tp, timeData(T0 + i));
        }
        logger.finaliseLogging();
    }
    printf("[repro] file exists: %d\n", (int)sd.exists(path));

    // Exactly what LogSummaryScreen does: parser open (file now held open),
    // a few frames of 16 steps, then the Del widget's remove().
    {
        FitLogParser p;
        if (!p.open(&sd, path)) { printf("parser open failed\n"); return 2; }
        for (int f = 0; f < 2; f++) {         // simulate 2 update() frames
            for (int i = 0; i < 16 && p.step(); i++) {}
        }
        printf("[repro] mid-parse (parser holds file open), calling sd.remove...\n");
        bool r1 = sd.remove(path);
        printf("[repro] remove while open: %d  still exists: %d\n",
               (int)r1, (int)sd.exists(path));
    }

    // And the after-parse case (parse done, parser still holds the open file
    // until onExit()).
    {
        FitLogParser p2;
        if (p2.open(&sd, path)) {
            int guard = 0; while (p2.step() && ++guard < 100000) {}
            printf("[repro] parse done, calling sd.remove...\n");
            bool r2 = sd.remove(path);
            printf("[repro] remove after parse: %d  still exists: %d\n",
                   (int)r2, (int)sd.exists(path));
            // p2's handle stays open until the scope ends -- which itself keeps
            // the file undeletable, as the p3 case below proves.
        }
    }

    // The fix: close the parser FIRST, then remove. Must succeed.
    FitLogParser p3;
    if (p3.open(&sd, path)) {
        int guard = 0; while (p3.step() && ++guard < 100000) {}
        p3.close();
        bool r3 = sd.remove(path);
        printf("[repro] close-then-remove: %d  still exists: %d\n",
               (int)r3, (int)sd.exists(path));
        if (!r3 || sd.exists(path)) { printf("[repro] FAIL: close-then-remove\n"); return 1; }
        printf("[repro] OK\n");
    } else {
        printf("[repro] FAIL: reopen for p3 failed\n");
        return 1;
    }
    return 0;
}
