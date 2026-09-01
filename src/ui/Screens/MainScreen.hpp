#pragma once
#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/BatteryWidget.hpp"
#include "UI/Widgets/BigDataWidget.hpp"
#include "UI/Widgets/SmallDataWidget.hpp"
#include "UI/Widgets/IconWidget.hpp"
#include "UI/Widgets/TimeWidget.hpp"
#include "UI/Widgets/DurationWidget.hpp"
#include "UI/GFX.h"

class MainScreen : public UIScreen {
public:
    MainScreen (DataModel& model) : 
        UIScreen(model),
        batt(202,5),
        gpsIcon(5,5,16,16,epd_bitmap_antenna),

        settingsIcon    (88,300,16,16,epd_bitmap_gear),
        playIcon        (112,300,16,16,epd_bitmap_play),
        stopIcon        (136,300,16,16,epd_bitmap_stop),
        powerIcon       (136,300,16,16,epd_bitmap_power),

        timeWidget(30,5,model.time().get()),
        lapTime(5,280, model.logger().get().lapElapsed) {}

    void onEnter() override {
        auto& l = model.layout().get();

        // clear() destroys any previous widgets; their ~BigDataWidget
        // unsubscribes them from the bus, so no stale ctx pointers remain.
        dataDisplays.clear();
        // Reserve up front: push_back must not reallocate AFTER subscriptions
        // are taken, or the moved widgets would invalidate the ctx pointers.
        dataDisplays.reserve(l.displays.size());
        _rows = l.rows;
        _cols = l.cols;

        int x = 0, y = 32;
        int _colPitch = (240)/_cols;
        int _rowPitch = (240)/_rows;

        for (size_t i = 0; i < l.displays.size(); i++) {
            const auto& d = l.displays[i];

            bool drawRight = false, drawBottom = false;
            for (size_t j = 0; j < l.displays.size(); j++) {
                if (i == j) continue;
                const auto& o = l.displays[j];

                // does another widget touch my right edge?
                if (o.x0 == d.x1 && (o.y0 < d.y1 && o.y1 > d.y0)) drawRight = true;

                // does another widget touch my bottom edge?
                if (o.y0 == d.y1 && (o.x0 < d.x1 && o.x1 > d.x0)) drawBottom = true;
            }

            int x0 = d.x0 * _colPitch;
            int y0 = d.y0 * _rowPitch;
            int w  = (d.x1 - d.x0) * _colPitch;
            int h  = (d.y1 - d.y0) * _rowPitch;

            dataDisplays.push_back({x + x0, y + y0, w, h, d.type});
            dataDisplays.back().setEdges(drawRight, drawBottom);
        }

        // Subscribe only once every widget is at its final address.
        // Subscribing inside the loop above risked a vector reallocation
        // moving the widgets out from under the registered ctx pointers
        // (observed as an access violation on the first publish).
        for (auto& disp : dataDisplays) {
            disp.subscribe(model.bus());
        }
    }

    void update(float dt) override {
        // Update numeric displays.
        // Widgets subscribed to DataBus receive updates via callbacks.
        // Their update(float dt) processes the dirty flag and updates display strings.
        // Non-subscribed widgets (Location, Distance, TotalDist) poll the bus.
        const DataBus& bus = model.bus();

        // Always call update(float dt) on subscribed widgets so they process dirty flag
        for (auto& disp : dataDisplays) {
            if (disp.isSubscribed()) {
                disp.update(dt);
            } else {
                disp.poll(bus);
            }
        }

        //display if gps has a valid location
        gpsIcon.setVisible(bus.get<bool>(Topic::GpsValid));

        //display the current time
        timeWidget.update(dt);
        batt.setBat(model.bus().get<int16_t>(Topic::Battery));
        
        //get the current lap time
        lapTime.update(dt);

        //show hide icons based on app state
        const auto& appState = model.app();
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
    }

    void handleInput(physIO input) override {
        switch (model.app()) {
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
    }
    
private:
    BatteryWidget batt;
    IconWidget gpsIcon;
    IconWidget settingsIcon;
    IconWidget playIcon;
    IconWidget stopIcon;
    IconWidget powerIcon;
    TimeWidget timeWidget;
    DurationWidget lapTime;

    std::vector<BigDataWidget> dataDisplays;

    uint8_t _rows = 2;
    uint8_t _cols = 2;

    AppState appState_prev = AppState::IDLE;
};