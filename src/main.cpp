#include <Arduino.h>
#include "HAL/HAL.hpp"
#include "App.hpp"

bool started = false;

void setup() {
    HAL::init_low();
}

uint32_t last = millis();
void loop() {
    if (!started) {
        Serial.begin(115200);
        HAL::init();
        App::instance().begin(HAL::SD());
        started = true;
        
        Serial.println("App started");
    }
    HAL::update();
    App::instance().update();

    if ( !App::instance().getGpsEnableState() ) {
      NRF_POWER->SYSTEMOFF = 1;
    }
}