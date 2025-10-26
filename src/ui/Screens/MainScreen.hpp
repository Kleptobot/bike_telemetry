#pragma once
#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/BatteryWidget.hpp"
#include "UI/Widgets/BigDataWidget.hpp"
#include "UI/Widgets/SmallDataWidget.hpp"
#include "UI/Widgets/IconWidget.hpp"
#include "UI/Widgets/TimeWidget.hpp"
#include "UI/GFX.h"

class MainScreen : public UIScreen {
    public:
        MainScreen (DataModel& model) : 
            UIScreen(model),
            batt(95,0),
            bigData(0,16),
            auxData1(47,48),
            auxData2(47,64),
            timeWidget(0,112),
            gpsIcon(0,0,16,16,epd_bitmap_antenna),
            settingsIcon(32,80,16,16,epd_bitmap_gear),
            playIcon(56,80,16,16,epd_bitmap_play),
            stopIcon(80,80,16,16,epd_bitmap_stop),
            loopIcon(56,80,16,16,epd_bitmap_loop),
            powerIcon(80,80,16,16,epd_bitmap_power) {}

        void update(float dt) override {
            //display if gps has a vlaid location
            gpsIcon.setVisible(model.telemetry().locationValid());

            //update numeric displays
            const auto& t = model.telemetry().get();
            bigData.setData(t.speed);
            auxData1.setData(t.cadence);
            auxData2.setData(t.heartrate);

            //display the current time
            timeWidget.setDate(model.time().get());

            //show hide icons based on app state
            const auto& appState = model.app().get().state;
            timeWidget.setVisible(appState == AppState::LOGGING);
            loopIcon.setVisible(appState == AppState::LOGGING);
            stopIcon.setVisible(appState == AppState::LOGGING);
            playIcon.setVisible(appState != AppState::LOGGING);
            settingsIcon.setVisible(appState != AppState::LOGGING);
        }

        void handleInput(physIO input) override {
            switch (model.app().get().state) {
                case AppState::LOGGING:
                    if (input.Select.press) {
                        emitAppEvent({AppEventType::StopLogging,0});
                    } else if (input.Select.held) {
                        emitAppEvent({AppEventType::PauseLogging,0});
                    }
                    break;

                default:
                    if (input.Select.press) {
                        emitAppEvent({AppEventType::StartLogging,0});
                    } else if (input.Left.press) {
                        emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                    } else if (input.Right.held) {
                        emitAppEvent({AppEventType::Sleep,0});
                    }
                    break;
            }
        }

        void render() override {
            batt.render();
            bigData.render();
            auxData1.render();
            auxData2.render();
            gpsIcon.render();
            timeWidget.render();

            settingsIcon.render();
            playIcon.render();
            stopIcon.render();
            loopIcon.render();
            powerIcon.render();
        }
    
    private:
        BatteryWidget batt;
        BigDataWidget bigData;
        SmallDataWidget auxData1;
        SmallDataWidget auxData2;
        TimeWidget timeWidget;
        IconWidget gpsIcon;
        IconWidget settingsIcon;
        IconWidget playIcon;
        IconWidget stopIcon;
        IconWidget loopIcon;
        IconWidget powerIcon;
};