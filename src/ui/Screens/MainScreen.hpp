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
            batt(202,5),
            gpsIcon(5,5,16,16,epd_bitmap_antenna),
            
            bigData(50,40),
            auxData1(84,90),
            auxData2(84,130),

            settingsIcon    (88,300,16,16,epd_bitmap_gear),
            playIcon        (112,300,16,16,epd_bitmap_play),
            stopIcon        (136,300,16,16,epd_bitmap_stop),
            powerIcon       (136,300,16,16,epd_bitmap_power),

            timeWidget(0,122) {}

        void onEnter() override {
            auto& l = model.layout().get();

            bigData.setType(l.disp1);
            auxData1.setType(l.disp2);
            auxData2.setType(l.disp3);

            bigData.setSize(6);
            auxData1.setSize(4);
            auxData2.setSize(4);
        }

        void update(float dt) override {
            //display if gps has a vlaid location
            gpsIcon.setVisible(model.telemetry().locationValid());

            //update numeric displays
            const auto& t = model.telemetry().get();

            bigData.update(t);
            auxData1.update(t);
            auxData2.update(t);

            //display the current time
            //timeWidget.setDate(model.time().get());

            //show hide icons based on app state
            const auto& appState = model.app().get().state;
            timeWidget.setVisible(appState == AppState::LOGGING);
            
            stopIcon.setVisible(appState == AppState::LOGGING);
            settingsIcon.setVisible(appState != AppState::LOGGING);
            powerIcon.setVisible(appState != AppState::LOGGING);

            if (appState != AppState::LOGGING){
                playIcon.setIcon(epd_bitmap_play);
            }else{
                playIcon.setIcon(epd_bitmap_loop);
            }
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
            powerIcon.render();
        }
    
    private:
        BatteryWidget batt;
        BigDataWidget bigData;
        BigDataWidget auxData1;
        BigDataWidget auxData2;
        TimeWidget timeWidget;
        IconWidget gpsIcon;
        IconWidget settingsIcon;
        IconWidget playIcon;
        IconWidget stopIcon;
        IconWidget powerIcon;
};