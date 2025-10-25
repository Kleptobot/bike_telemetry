#include <Arduino.h>
#include "HAL/HAL.hpp"
#include "App.hpp"

bool started = false;
UIManager ui;

void setup() {
    HAL::init_low();
}

uint32_t last = millis();
void loop() {
    if (!started) {
        Serial.begin(115200);
        HAL::init();
        App::instance().begin(&ui,HAL::SD());
        started = true;
    }

    HAL::update();
    physIO inputs = HAL::inputs();

    if (inputs.Up.RE) {
        Serial.println("UP!!!!");
        delay(500);
        HAL::sleep();
    }
        
    if (millis() - last > 2000) {
        Serial.println(App::instance().getSpeed());
        last = millis();
    }

    if ( !App::instance().getGpsEnableState() ) {
      NRF_POWER->SYSTEMOFF = 1;
    }
}