#define DEBUG_INVALIDATE 0
#define DEBUG_GPS 0
#define DEBUG_INPUTS 0
#define DEBUG_BLUETOOTH 0

#include <Arduino.h>
#include "DebugConfig.hpp"
#include "HAL/HAL.hpp"
#include "App.hpp"

const bool ENABLE_INVALIDATE_DEBUG = DEBUG_INVALIDATE;
const bool ENABLE_GPS_DEBUG = DEBUG_GPS;
const bool ENABLE_INPUTS_DEBUG = DEBUG_INPUTS;
const bool ENABLE_BLUETOOTH_DEBUG = DEBUG_BLUETOOTH;

bool started = false;

void setup() {
    //run the bare minimum init, GPIO and RTC
    HAL::inst().init_low();
}

void loop() {
    //check if we need to run the higher level init functions (only once started)
    if (!started) {
        digitalWrite(D6, true); //turn on the auxilary supply
        Serial.begin(115200);
        delay(500);
        
        HAL::inst().init();
        App::instance().begin(HAL::inst().SD());
        started = true;
        Serial.println("App started");
        Serial.printf("Free heap: %d bytes\n", dbgHeapTotal() - dbgHeapUsed());
        Serial.println("SD card det state: " + String(HAL::inst().inputs().SD_Det.state));
    }

    //update the HAL and App
    HAL::inst().update();
    App::instance().update();

    //read the state of the gps enable pin, if its low then sleep
    if ( !App::instance().getGpsEnableState() ) {
        digitalWrite(D6, false); //turn off the auxilary supply
        NRF_POWER->SYSTEMOFF = 1;
    }
}