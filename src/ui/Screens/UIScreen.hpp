#pragma once
#include <Arduino.h>
#include <vector>
#include <memory>

#include "HAL/InputInterface.hpp"
#include "UI/Widgets/Widget.hpp"
#include "UIEventBus.hpp"
#include "DataModel/DataModel.hpp"

// ---------------------------------------------------------------------------
// Shared input vocabulary (all screens):
//   Up/Down press   = move focus / (when a field is selected) adjust value,
//                     with hold-repeat for numeric fields
//   Select press    = select/confirm the focused item; Select again = done
//   Left/Right      = move focus horizontally (e.g. Back <-> Save buttons)
//   Select HELD     = SLEEP -- MainScreen ONLY. Other screens must ignore a
//                     held center; this is a deliberate main-screen gesture.
// Available-input hints: use InputHintsWidget (UI/Widgets/InputHints.hpp),
// call setHint() in onEnter(); keep labels consistent with the table above.
// ---------------------------------------------------------------------------
class UIScreen {
    public:
        explicit UIScreen(DataModel& model) : model(model) {}
        virtual ~UIScreen() = default;

        virtual void onEnter() {}
        virtual void onExit() {}
        virtual void update(float dt) = 0;
        virtual void render() = 0;
        virtual void handleInput(const physIO input) = 0;

        void setEventBus(UIEventBus* bus) { eventBus = bus; }

    protected:
        void emitAppEvent(AppEvent event) {
            if (eventBus) eventBus->postAppEvent(event);
        }

        void emitUIEvent(UIEventType type, ScreenID target = ScreenID::None) {
            if (eventBus) eventBus->postUIEvent({type, target});
        }
        DataModel& model;

    private:
        UIEventBus* eventBus = nullptr;
};