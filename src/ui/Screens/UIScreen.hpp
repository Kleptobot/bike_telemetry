#pragma once
#include <Arduino.h>
#include <vector>
#include <memory>

#include "HAL/InputInterface.hpp"
#include "UI/Widgets/Widget.hpp"
#include "display/Display.hpp"
#include "UIEventBus.hpp"
#include "DataModel.hpp"

class UIScreen {
    public:
        explicit UIScreen(DataModel& model) : model(model) {}
        virtual ~UIScreen() = default;

        virtual void onEnter() {}
        virtual void onExit() {Disp::clear();}
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