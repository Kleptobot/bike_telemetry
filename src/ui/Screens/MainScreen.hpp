#pragma once
#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/BatteryWidget.hpp"
#include "UI/Widgets/BigDataWidget.hpp"
#include "UI/Widgets/SmallDataWidget.hpp"
#include "UI/Widgets/IconWidget.hpp"
#include "UI/Widgets/TimeWidget.hpp"
#include "UI/Widgets/DurationWidget.hpp"
#include "UI/GFX.h"

// Icon layout: the icons sit on a drawn D-pad cross (a switch shape) in the
// free band below the data grid, so the mapping to the physical 5-position
// switch is explicit. Cross centre is (190,296); horizontal arm spans
// x=160..220 at y=294..298, vertical arm x=188..192 at y=272..320. The
// vertical arm reserves the currently unused UP/DOWN lanes - a future icon
// drops straight into (182,272) or (182,304).
//
//   tip:       LEFT(x162)       CENTER(x182)                 RIGHT(x202)
//   IDLE:      gear(settings)   play -> power while held     -
//   LOGGING:   loop(new lap)    pause -> power while held    stop
//   PAUSED:    loop(new lap)    play -> power while held     stop
//
// which matches the input mapping in handleInput():
//   Left press       = settings (IDLE) / new lap (active)
//   Select press     = start (IDLE) / pause (LOGGING) / resume (PAUSED)
//   Select held      = sleep (icon flips to power while held, all states)
//   Right press      = stop logging (active)
//
// NOTE on button semantics (see HAL/button.cpp): press fires on RELEASE of
// any press longer than _nShortPressTime (50 ms), so a long center-hold also
// emits a press when released. That is safe here: Sleep fires during the
// hold and the device powers down before the release-press lands.
class MainScreen : public UIScreen {
public:
    MainScreen (DataModel& model) : 
        UIScreen(model),
        batt(202,5),
        gpsIcon(5,5,16,16,epd_bitmap_antenna),

        leftIcon      (162,288,16,16,epd_bitmap_gear),   // Left:  settings (IDLE) / new lap (active)
        centerIcon    (182,288,16,16,epd_bitmap_play),   // Select: start/pause/resume; power while held
        rightIcon     (202,288,16,16,epd_bitmap_stop),   // Right: stop (active)

        // Outward-pointing arrows flanking the tips: they say "this icon is
        // that switch position". The right one only means something while a
        // ride is active (the stop tip is empty in IDLE).
        leftArrow     (146,288,16,16,epd_bitmap_left),
        rightArrow    (218,288,16,16,epd_bitmap_right),

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

        // Bottom bar: one icon per switch position, always in the same slot.
        // Only visibility, the glyph in the center slot (play/pause/stop/power)
        // and the glyph in the left slot (gear/loop) ever change - positions
        // are fixed, so nothing shifts between states.
        const auto& appState = model.app();
        const bool active = appState == AppState::LOGGING || appState == AppState::PAUSED;
        lapTime.setVisible(active);

        // Left: settings in IDLE, new lap while a ride is active.
        leftIcon.setVisible(true);
        leftIcon.setIcon(active ? epd_bitmap_loop : epd_bitmap_gear);

        // Center: start (IDLE) / pause (LOGGING) / resume (PAUSED);
        // power while the user is holding it (sleep gesture).
        centerIcon.setVisible(true);
        if (_centerHeld)
            centerIcon.setIcon(epd_bitmap_power);
        else if (appState == AppState::IDLE)
            centerIcon.setIcon(epd_bitmap_play);
        else if (appState == AppState::LOGGING)
            centerIcon.setIcon(epd_bitmap_pause);
        else
            centerIcon.setIcon(epd_bitmap_play);

        // Right: stop while a ride is active, nothing in IDLE.
        rightIcon.setVisible(active);

        leftArrow.setVisible(true);
        rightArrow.setVisible(active);
    }

    void handleInput(physIO input) override {
        // Capture the center-hold state for the icon feedback in update().
        // handleInput runs every loop, so the flag tracks the gesture live.
        _centerHeld = input.Select.held;

        switch (model.app()) {
            case AppState::LOGGING:
            case AppState::PAUSED:
                if (input.Select.held) {
                    emitAppEvent({AppEventType::Sleep,0});
                } else if (input.Select.press) {
                    // pause while logging, resume while paused
                    if (model.app() == AppState::LOGGING) {
                        emitAppEvent({AppEventType::PauseLogging,0});
                    } else {
                        emitAppEvent({AppEventType::ResumeLogging,0});
                    }
                } else if (input.Right.press) {
                    emitAppEvent({AppEventType::StopLogging,0});
                } else if (input.Left.press) {
                    emitAppEvent({AppEventType::NewLap,0});
                }
                break;

            case AppState::IDLE:
                if (input.Select.held) {
                    emitAppEvent({AppEventType::Sleep,0});
                } else if (input.Select.press) {
                    emitAppEvent({AppEventType::StartLogging,0});
                } else if (input.Left.press) {
                    emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                }
                break;

            default:
                if (input.Select.held) {
                    emitAppEvent({AppEventType::Sleep,0});
                } else if (input.Left.press) {
                    emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
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

        // D-pad cross behind the icons, mirroring the 5-pos switch. Drawn
        // after the icons it would slice through their glyphs, so it goes
        // first; the icon bitmaps have a transparent background, letting the
        // arm show through. The vertical arm spans the full free band and
        // reserves the unused UP/DOWN lanes: a future icon drops into
        // (182,272) or (182,304).
        Disp::fillRect(146, 294, 89,  5, CROSS_COLOR);  // horizontal arm (extended under the arrows)
        Disp::fillRect(188, 272,  5, 49, CROSS_COLOR);  // vertical arm

        leftArrow.render();
        leftIcon.render();
        centerIcon.render();
        rightIcon.render();
        rightArrow.render();
    }
    
private:
    BatteryWidget batt;
    IconWidget gpsIcon;

    // Bottom bar, one widget per fixed switch-position slot (see class comment).
    IconWidget leftIcon;
    IconWidget centerIcon;
    IconWidget rightIcon;
    IconWidget leftArrow;
    IconWidget rightArrow;

    TimeWidget timeWidget;
    DurationWidget lapTime;

    std::vector<BigDataWidget> dataDisplays;

    uint8_t _rows = 2;
    uint8_t _cols = 2;

    // Live mirror of input.Select.held, captured in handleInput (which runs
    // every loop) and consumed in update() for the power-icon feedback.
    bool _centerHeld = false;

    // Dim grey for the switch cross, same value as the grid edge lines.
    static constexpr uint16_t CROSS_COLOR = 0x4208;
};