#pragma once
#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/BatteryWidget.hpp"
#include "UI/Widgets/BigDataWidget.hpp"
#include "UI/Widgets/SmallDataWidget.hpp"
#include "UI/Widgets/IconWidget.hpp"
#include "UI/Widgets/TimeWidget.hpp"
#include "UI/GFX.h"
// #include "UI/Widgets/MapWidget.hpp"

class MainScreen : public UIScreen {
    public:
        MainScreen (DataModel& model) : 
            UIScreen(model),
            batt(202,5),
            // map(30,30,80,80, model),
            gpsIcon(5,5,16,16,epd_bitmap_antenna),

            settingsIcon    (88,300,16,16,epd_bitmap_gear),
            playIcon        (112,300,16,16,epd_bitmap_play),
            stopIcon        (136,300,16,16,epd_bitmap_stop),
            powerIcon       (136,300,16,16,epd_bitmap_power),

            timeWidget(30,5,&_date),
            lapTime(5,280,&_lap) {}

        void onEnter() override {
            auto& l = model.layout().get();
            
            int size = constrain( 256/(8*l.displays.size()),1,8);

            dataDisplays.clear();
            for ( uint i = 0; i<l.displays.size(); i++) {
                dataDisplays.push_back({5,30+int(i)*8*size,size,l.displays[i]});
            }
        }

        void update(float dt) override {
            //update numeric displays
            const auto& t = model.telemetry().get();
            //display if gps has a vlaid location
            gpsIcon.setVisible(t.validLocation);

            if (version != model.telemetry().version()) {
                for (auto& disp:dataDisplays) {
                    disp.update(t);
                }
                version = model.telemetry().version();
            }

            //display the current time
            _date = model.time().get();
            timeWidget.update(dt);
            batt.setBat(t.BattPercentage);
            
            //get the current lap time
            auto& ts = model.logger().get().lapElapsed;
            _lap = DateTime(0,0,0,ts.hours(),ts.minutes(),ts.seconds());
            lapTime.update(dt);

            //show hide icons based on app state
            const auto& appState = model.app().get().state;
            lapTime.setVisible(appState == AppState::LOGGING);
            
            stopIcon.setVisible(appState == AppState::LOGGING);
            settingsIcon.setVisible(appState != AppState::LOGGING);
            powerIcon.setVisible(appState != AppState::LOGGING);

            if (appState != AppState::LOGGING && appState_prev == AppState::LOGGING) {
                playIcon.setIcon(epd_bitmap_play);
            }else if (appState == AppState::LOGGING && appState_prev != AppState::LOGGING) {
                playIcon.setIcon(epd_bitmap_loop);
            }
            appState_prev = appState;

            //update map widget
            // map.update(dt);

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

                case AppState::IDLE:
                    if (input.Select.press) {
                        emitAppEvent({AppEventType::StartLogging,0});
                    } else if (input.Left.press) {
                        emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                    } else if (input.Right.held) {
                        emitAppEvent({AppEventType::Sleep,0});
                    }
                    break;

                default:
                    if (input.Left.press) {
                        emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                    } else if (input.Right.held) {
                        emitAppEvent({AppEventType::Sleep,0});
                    }
            }
        }

        void render() override {
            batt.render();

            for (auto& disp : dataDisplays) {
                disp.render();
            }

            gpsIcon.render();
            timeWidget.render();
            lapTime.render();

            settingsIcon.render();
            playIcon.render();
            stopIcon.render();
            powerIcon.render();

            // render map
            // map.render();
        }
    
    private:
        BatteryWidget batt;
        IconWidget gpsIcon;
        IconWidget settingsIcon;
        IconWidget playIcon;
        IconWidget stopIcon;
        IconWidget powerIcon;
        TimeWidget timeWidget;
        TimeWidget lapTime;

        std::vector<BigDataWidget> dataDisplays;

        // MapWidget map;

        DateTime _date;
        DateTime _lap;
        uint32_t version = 0;
        AppState appState_prev = AppState::IDLE;
};