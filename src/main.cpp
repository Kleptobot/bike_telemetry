#include <Arduino.h>
#include "HAL/HAL.hpp"
#include "App.hpp"
// #include "display/Display.hpp"

bool started = false;

void setup() {
    //run the bare minimum init, GPIO and RTC
    HAL::init_low();
}

uint32_t last = millis();
void loop() {
    //check if we need to run the higher level init functions (only once started)
    if (!started) {
        Serial.begin(115200);
        HAL::init();
        App::instance().begin(HAL::SD());
        started = true;
        
        delay(500);
        Serial.println("App started");
    }

    //update the HAL and App
    HAL::update();
    App::instance().update();

    //Serial.println(millis()-last);
    //last = millis();

    //read the state of the gps enable pin, if its low then sleep
    if ( !App::instance().getGpsEnableState() ) {
      NRF_POWER->SYSTEMOFF = 1;
    }
}