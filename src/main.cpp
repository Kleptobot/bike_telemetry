#define DEBUG_INVALIDATE 0
#define DEBUG_GPS 0
#include <Arduino.h>
#include "DebugConfig.hpp"
#include "HAL/HAL.hpp"
#include "App.hpp"

const bool ENABLE_INVALIDATE_DEBUG = DEBUG_INVALIDATE;
const bool ENABLE_GPS_DEBUG = DEBUG_GPS;

bool started = false;

void setup() {
    //run the bare minimum init, GPIO and RTC
    HAL::inst().init_low();
}

uint32_t last = millis();
void loop() {
    //check if we need to run the higher level init functions (only once started)
    if (!started) {
        digitalWrite(D6, true); //turn on the auxilary supply
        delay(500);
        
        Serial.begin(115200);
        HAL::inst().init();
        App::instance().begin(HAL::inst().SD());
        started = true;
        Serial.println("App started");
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