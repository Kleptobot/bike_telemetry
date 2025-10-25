#pragma once
#include <Arduino.h>
#include <vector>
#include <memory>

#include "HAL/InputInterface.hpp"
#include "UI/Widgets/Widget.hpp"
#include "UIEventBus.hpp"

class UIScreen {
    public:
        virtual ~UIScreen() = default;

        virtual void onEnter() {}
        virtual void onExit() {}
        virtual void update(float dt) = 0;
        virtual void render() = 0;
        virtual void handleInput(const physIO input) = 0;

        void setEventBus(UIEventBus* bus) { eventBus = bus; }

    protected:
        void emitAppEvent(AppEventType type, void* data) {
            if (eventBus) eventBus->postAppEvent({type, data});
        }

        void emitUIEvent(UIEventType type, ScreenID target = ScreenID::None) {
            if (eventBus) eventBus->postUIEvent({type, target});
        }

    private:
        UIEventBus* eventBus = nullptr;
};