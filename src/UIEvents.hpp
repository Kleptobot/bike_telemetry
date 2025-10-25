#pragma once
#include "UI/Screens/ScreenID.hpp"

enum class UIEventType {
    None,
    ChangeScreen,
    Popup,
    Toast,
};

struct UIEvent {
    UIEventType type;
    ScreenID target;
};