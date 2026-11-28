// Standalone smoke test for re-entrant GPS response timeouts. Build and run
// from sim/ after `pio run`:
//   g++ -std=gnu++17 -fpermissive -I stubs -I ..\src -I ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\TinyGPSPlus\src tests\LC76GResponseTimeout_smoke.cpp ..\src\HAL\LC76G.cpp stubs\Arduino.cpp stubs\SdFat.cpp ..\.pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense\TinyGPSPlus\src\TinyGPS++.cpp -o build\LC76GResponseTimeout_smoke.exe
//   build\LC76GResponseTimeout_smoke.exe

#include "HAL/LC76G.hpp"
#include "DebugConfig.hpp"

#include <cstdio>

const bool ENABLE_GPS_DEBUG = false;
TwoWire Wire;

class TestStorage : public IStorage {
public:
    File32 openFile(const char*, oflag_t) override { return File32(); }
    bool exists(const char*) override { return false; }
    bool remove(const char*) override { return false; }
    bool listFiles(const char*, StorageEntry*, uint8_t, uint8_t& outCount) override {
        outCount = 0;
        return false;
    }
};

struct CallbackState {
    LC76G* gps = nullptr;
    int firstTimeouts = 0;
    int followupTimeouts = 0;
    bool invalidFirstResult = false;
    bool invalidFollowupResult = false;
};

void onFollowupTimeout(int numArgs, const void* payload, void* userContext) {
    auto* state = static_cast<CallbackState*>(userContext);
    ++state->followupTimeouts;
    state->invalidFollowupResult = numArgs != 0 || payload != nullptr;
}

void onFirstTimeout(int numArgs, const void* payload, void* userContext) {
    auto* state = static_cast<CallbackState*>(userContext);
    ++state->firstTimeouts;
    state->invalidFirstResult = numArgs != 0 || payload != nullptr;

    state->gps->sendCommand(LC76G::GET_NAVIGATION_MODE, onFollowupTimeout,
                            state, nullptr);
}

int main() {
    simSetMillis(1000);
    TestStorage storage;
    LC76G gps(storage);
    gps.begin(&Wire);

    CallbackState state;
    state.gps = &gps;
    LC76G::Payload1U8 rate = {0};
    gps.sendCommand(LC76G::GET_NMEA_MSG_RATE, onFirstTimeout, &state, &rate);

    simAdvanceMillis(90001);
    gps.update();

    if (state.firstTimeouts != 1 || state.invalidFirstResult) {
        std::printf("FAIL: first response did not time out cleanly\n");
        return 1;
    }

    simAdvanceMillis(90001);
    gps.update();

    if (state.followupTimeouts != 1 || state.invalidFollowupResult) {
        std::printf("FAIL: callback-queued response was lost or corrupted\n");
        return 1;
    }

    std::printf("LC76G RESPONSE TIMEOUT SMOKE PASS\n");
    return 0;
}
