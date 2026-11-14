// Regression test: ride duration must survive a pause at the end of the ride.
//
// The bug this locks down: ILogger::pause() used to throw the completed segment
// away (_startTime = _startTime + d) instead of banking it, and the .fit/.tcx
// writers derived every duration from _currentTime - _startTime. Auto-pause
// fires as soon as the bike stops, so STOPPING A RIDE THAT WAS STANDING STILL
// paused the logger immediately before finaliseLogging() -- and the 10 s test
// ride was written with total_elapsed_time = 0 and a start_time equal to the
// stop time. This test drives the real FITLogger down exactly that path.
//
// Build and run (from sim/):
//   g++ -Wall -std=gnu++17 -DARDUINO=10800 -Istubs -I../src
//       tests/FitLoggerPause_smoke.cpp ../src/Loggers/FITLogger.cpp
//       ../src/Loggers/FitWriter.cpp ../src/Loggers/LogSummary.cpp
//       ../src/HAL/SDCard.cpp stubs/Arduino.cpp stubs/SdFat.cpp
//       -o build/obike-fit-pause-smoke
//   ./build/obike-fit-pause-smoke fit-pause.dir

#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

#include "Loggers/FITLogger.hpp"
#include "Loggers/ILogger.hpp"
#include "Loggers/LogSummary.hpp"
#include "HAL/SDCard.hpp"

static void makeDir(const char* d) {
#ifdef _WIN32
    ::_mkdir(d);
#else
    ::mkdir(d, 0755);
#endif
}

static int failures = 0;
static void check(bool cond, const char* what, long got = 0, long want = 0) {
    if (cond) return;
    ++failures;
    printf("[pause smoke] FAIL: %s (got %ld want %ld)\n", what, got, want);
}

int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    const char* card = (argc > 1) ? argv[1] : "fit-pause.dir";
    makeDir(card);
    simSetSdRoot(card);

    SDCardSystem sd;
    if (!sd.init(nullptr)) { printf("[pause smoke] sd init failed\n"); return 2; }

    const long long T0 = 1750000000LL;
    char path[64];
    snprintf(path, sizeof(path), "/activity_%lld.fit", T0);

    {
        FITLogger logger(&sd);
        logger.startLogging(timeData(T0));

        for (int i = 0; i <= 10; i++) {
            Trackpoint tp;
            tp.latitude = -37.8; tp.longitude = 144.9; tp.altitude = 42.0f;
            tp.speed = 20.0f; tp.heartrate = 130.0f; tp.power = 200.0f;
            tp.cadence = 85.0f; tp.distance = i * 5.5f; tp.calories = i * 0.5f;
            logger.addTrackpoint(tp, timeData(T0 + i));
        }

        // The reported scenario: the ride is paused and THEN finalised.
        // (App::update -> PauseLogging -> _logger->pause(), then finalise.)
        logger.pause(timeData(T0 + 10));
        check(logger.elapsed_Total().totalSeconds() == 10,
              "elapsed_Total after pause == 10", logger.elapsed_Total().totalSeconds(), 10);
        check(logger.elapsed_Lap().totalSeconds() == 10,
              "elapsed_Lap after pause == 10", logger.elapsed_Lap().totalSeconds(), 10);

        logger.finaliseLogging();
    }

    // The file must say 10 s, and must keep the real ride start.
    FitLogParser p;
    if (!p.open(&sd, path)) { printf("[pause smoke] parser open failed\n"); return 2; }
    { int guard = 0; while (p.step() && ++guard < 10000) {} }
    check(p.done(), "parser reached EOF");
    check(p.stats().valid, "stats valid");
    check(p.stats().durationSec == 10, "file duration == 10 s",
          (long)p.stats().durationSec, 10);
    check(p.stats().lapCount == 1, "one lap", (long)p.stats().lapCount, 1);
    check(fabsf(p.stats().distanceM - 55.0f) < 0.01f, "distance == 55 m",
          (long)p.stats().distanceM, 55);

    p.close();
    ::remove((std::string() + card + path).c_str());
#ifdef _WIN32
    _rmdir(card);
#else
    rmdir(card);
#endif

    if (failures == 0) printf("[pause smoke] OK\n");
    return failures ? 1 : 0;
}
