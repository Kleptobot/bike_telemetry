#pragma once
#include <Arduino.h>
#include <vector>
#include <memory>

#include "HAL/InputInterface.hpp"
#include "UI/Widgets/Widget.hpp"
#include "UIEventBus.hpp"
#include "DataModel.hpp"

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
        void emitAppEvent(AppEventType type) {
            if (eventBus) eventBus->postAppEvent({type});
        }

        void emitUIEvent(UIEventType type, ScreenID target = ScreenID::None) {
            if (eventBus) eventBus->postUIEvent({type, target});
        }
        DataModel& model;

    private:
        UIEventBus* eventBus = nullptr;
};