#define DEBUG_INVALIDATE 0
#define DEBUG_GPS 0
#define DEBUG_INPUTS 0
#define DEBUG_BLUETOOTH 0
#define DEBUG_I2C 0

#include <Arduino.h>
#include "DebugConfig.hpp"
#include "HAL/HAL.hpp"
#include "App.hpp"

#if defined(NRF52_SERIES) || defined(NRF52)
#include <nrf.h>

namespace {
// The nRF52 Wire core busy-waits for TWIM events without a timeout. If an I2C
// call wedges the main loop, this watchdog resets the MCU so boot-time bus
// recovery can run instead of leaving the device frozen indefinitely.
constexpr uint32_t MAIN_LOOP_WATCHDOG_TIMEOUT_SECONDS = 30;
bool s_watchdogFeedEnabled = false;

void startMainLoopWatchdog() {
    if (NRF_WDT->RUNSTATUS == WDT_RUNSTATUS_RUNSTATUS_Running) {
        s_watchdogFeedEnabled = (NRF_WDT->RREN & WDT_RREN_RR0_Msk) != 0;
        return;
    }

    NRF_WDT->CRV = MAIN_LOOP_WATCHDOG_TIMEOUT_SECONDS * 32768UL;
    NRF_WDT->RREN = WDT_RREN_RR0_Msk;
    NRF_WDT->CONFIG =
        (WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos) |
        (WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos);
    NRF_WDT->TASKS_START = 1;
    s_watchdogFeedEnabled = true;
}

void feedMainLoopWatchdog() {
    if (s_watchdogFeedEnabled) {
        NRF_WDT->RR[0] = WDT_RR_RR_Reload;
    }
}
}
#endif

const bool ENABLE_INVALIDATE_DEBUG = DEBUG_INVALIDATE;
const bool ENABLE_GPS_DEBUG = DEBUG_GPS;
const bool ENABLE_INPUTS_DEBUG = DEBUG_INPUTS;
const bool ENABLE_BLUETOOTH_DEBUG = DEBUG_BLUETOOTH;
const bool ENABLE_I2C_DEBUG = DEBUG_I2C;

bool started = false;

void setup() {
#if defined(NRF52_SERIES) || defined(NRF52)
    startMainLoopWatchdog();
#endif
    //run the bare minimum init, GPIO and RTC
    HAL::inst().init_low();
#if defined(NRF52_SERIES) || defined(NRF52)
    feedMainLoopWatchdog();
#endif
}

uint32_t gpsLastEnableTime = millis();
void loop() {
    __NOP();
    if (!started) {
        Serial.begin(115200);
        delay(500);
        
        HAL::inst().init(&App::instance().getModel().time().get());
#if defined(NRF52_SERIES) || defined(NRF52)
        feedMainLoopWatchdog();
#endif
        App::instance().begin(HAL::inst().SD());
#if defined(NRF52_SERIES) || defined(NRF52)
        feedMainLoopWatchdog();
#endif
        started = true;
        Serial.println("App started");
        Serial.printf("Free heap: %d bytes\n", dbgHeapTotal() - dbgHeapUsed());
        Serial.println("SD card det state: " + String(HAL::inst().inputs().SD_Det.state));
        
    }

    //update the HAL and App
    HAL::inst().update();
    App::instance().update();

    // read the state of the gps enable pin, if its low then sleep
    if ( !App::instance().getGpsEnableState() ) {
        if ((millis() - gpsLastEnableTime) > 500) { //small debounce
            NRF_POWER->SYSTEMOFF = 1;
        }
    } else {
        gpsLastEnableTime = millis();
    }
#if defined(NRF52_SERIES) || defined(NRF52)
    feedMainLoopWatchdog();
#endif
}