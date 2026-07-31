// Simulator control surface.
//
// The simulator replaces HAL.cpp, InputSystem.cpp, Sensors.cpp and LC76G.cpp
// at link time. HAL.hpp itself is untouched, so App.cpp, every screen and
// every widget compile and run exactly as they do on device -- which is the
// whole point. If the simulator needed source changes in the UI, it would
// stop being evidence about the UI.
//
// Everything the scenario can drive lives here.
#ifndef SIMHAL_H
#define SIMHAL_H

#include <cstdint>
#include "HAL/InputInterface.hpp"

namespace Sim {

/** Live telemetry the simulated HAL reports back to the application. */
struct State {
    // Bike
    float wheelRPM      = 0.0f;
    bool  wheelRPMLive  = false;
    float cadence       = 0.0f;
    float power         = 0.0f;
    float heartRate     = 0.0f;

    // Environment
    float altitude      = 100.0f;   // metres
    float altVelocity   = 0.0f;     // m/s, drives the grade calculation
    float temperature   = 21.5f;    // celsius
    int16_t battery     = 78;       // percent

    // GNSS
    bool   gpsValid     = false;
    double latitude     = 51.5007;  // somewhere to start
    double longitude    = -0.1246;
    float  gpsSpeedKmh  = 0.0f;

    // Storage
    bool sdMounted      = true;
    bool sdPresent      = true;
};

State& state();

enum class ButtonName { Up, Down, Left, Right, Select, SDDetect };

/** Button edges are computed by the real `button` class; we set raw levels. */
void pressButton(ButtonName);
void releaseButton(ButtonName);
void setButton(ButtonName b, bool pressed);
bool buttonState(ButtonName b);

/** Advances the simulated HAL by one tick. Called from the frontend loop. */
void tick(uint32_t deltaMs);

/** Injects a raw BLE notification into the real csc/cps/hrm parsers. */
void injectCscNotification(const uint8_t* data, uint16_t len);
void injectCpsNotification(const uint8_t* data, uint16_t len);
void injectHrmNotification(const uint8_t* data, uint16_t len);

/** Builds spec-shaped measurement payloads, so the real parsers get real input. */
uint16_t buildCscMeasurement(uint8_t* out, uint32_t wheelRevs, uint16_t wheelEventTime,
                             uint16_t crankRevs, uint16_t crankEventTime);
uint16_t buildCpsMeasurement(uint8_t* out, int16_t watts,
                             uint16_t crankRevs, uint16_t crankEventTime);
uint16_t buildHrmMeasurement(uint8_t* out, uint16_t bpm);

} // namespace Sim

#endif /* SIMHAL_H */
