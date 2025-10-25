#pragma once
#include "AppEvents.hpp"
#include "UIEvents.hpp"

class UIEventBus {
public:
    virtual void postAppEvent(const AppEvent& e) = 0;
    virtual void postUIEvent(const UIEvent& e) = 0;
    virtual ~UIEventBus() = default;
};