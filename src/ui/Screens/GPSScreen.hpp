#pragma once
#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/DateWidget.hpp"
#include "UI/Widgets/TimeWidget.hpp"
#include "HAL/InputInterface.hpp"
#include "UI/Widgets/SelectableTextIcon.hpp"
#include "UI/GFX.h"

class GPSScreen : public UIScreen {
    public:
        GPSScreen (DataModel& model) : 
            UIScreen(model) {}

    void onEnter() override {}

    void render() override {}

    void update(float dt) override {}

    void handleInput(physIO input) override {}

    private:

};