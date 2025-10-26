#include <Arduino.h>
#include "HAL/HAL.hpp"
#include "App.hpp"
#include "UI/Screens/MainScreen.hpp"
#include "UI/Screens/BluetoothScreen.hpp"
#include "UI/Screens/TimeEditScreen.hpp"
#include "UI/Screens/SettingsScreen.hpp"

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

        ui.registerScreen<MainScreen>(ScreenID::MainMenu,App::instance().getModel());
        ui.registerScreen<TimeEditScreen>(ScreenID::TimeMenu,App::instance().getModel());
        ui.registerScreen<SettingsScreen>(ScreenID::SettingsMenu,App::instance().getModel());
        ui.registerScreen<BluetoothScreen>(ScreenID::Bluetooth,App::instance().getModel());

        ui.begin(ScreenID::MainMenu);
    }

    HAL::update();
    physIO inputs = HAL::inputs();

    if (inputs.Up.RE) {
        Serial.println("UP!!!!");
        delay(500);
        HAL::sleep();
    }

    if ( !App::instance().getGpsEnableState() ) {
      NRF_POWER->SYSTEMOFF = 1;
    }
}