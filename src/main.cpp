#define DEBUG_INVALIDATE 0
#define DEBUG_GPS 1
#define DEBUG_INPUTS 0
#define DEBUG_BLUETOOTH 0
#define DEBUG_I2C 0

#include <Arduino.h>
#include "DebugConfig.hpp"
#include "HAL/HAL.hpp"
#include "HAL/FaultReport.hpp"
#include "App.hpp"

const bool ENABLE_INVALIDATE_DEBUG = DEBUG_INVALIDATE;
const bool ENABLE_GPS_DEBUG = DEBUG_GPS;
const bool ENABLE_INPUTS_DEBUG = DEBUG_INPUTS;
const bool ENABLE_BLUETOOTH_DEBUG = DEBUG_BLUETOOTH;
const bool ENABLE_I2C_DEBUG = DEBUG_I2C;

bool started = false;

void setup() {
    //run the bare minimum init, GPIO and RTC
    HAL::inst().init_low();

    // Install our fault-recording vector table before anything can fault:
    // captures PC/LR/CFSR into .noinit and forces a clean reset instead of
    // the core's silent HardFault spin/reset; printed at next boot by
    // faultReportPrintIfPending(). See HAL/FaultReport.*.
    faultReportInit();
}

uint32_t gpsLastEnableTime = millis();
void loop() {
    if (!started) {
        Serial.begin(115200);
        delay(500);

        HAL::inst().init(&App::instance().getModel().time().get());
        App::instance().begin(HAL::inst().SD());
        started = true;
        Serial.println("App started");
        // If the previous run died with a CPU fault, this prints the captured
        // register record (once a terminal is attached) -- the field-crash
        // diagnosis path; see HAL/FaultReport.*.
        faultReportPrintIfPending();
        Serial.printf("Free heap: %d bytes\n", dbgHeapTotal() - dbgHeapUsed());
        Serial.println("SD card det state: " + String(HAL::inst().inputs().SD_Det.state));
    }

    //update the HAL and App
    HAL::inst().update();
    App::instance().update();

    // Free-heap low-watermark: prints only when the watermark drops into a new
    // 1 KB bucket, so a slow drain (or the pre-fix LogSummary churn) shows up
    // in the log as a staircase without flooding it.
    {
        static uint32_t heapLowBucket = 0xFFFFFFFFu;
        const uint32_t freeNow = dbgHeapTotal() - dbgHeapUsed();
        const uint32_t bucket = freeNow & ~1023u;
        if (bucket < heapLowBucket) {
            heapLowBucket = bucket;
            Serial.printf("[heap] free=%u (new low watermark)\n", (unsigned)freeNow);
        }
        // Cheap poll; returns immediately unless a fault record is pending.
        faultReportPrintIfPending();
    }


    // read the state of the gps enable pin, if its low then sleep
    if ( !App::instance().getGpsEnableState() ) {
        if ((millis() - gpsLastEnableTime) > 500) { //small debounce
            NRF_POWER->SYSTEMOFF = 1;
        }
    } else {
        gpsLastEnableTime = millis();
    }
}