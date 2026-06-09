#pragma once
#include "UI/Screens/UIScreen.hpp"
#include "HAL/InputInterface.hpp"
#include "UI/GFX.h"

class UnmountSDScreen : public UIScreen {
    public:
    UnmountSDScreen(DataModel& model) : 
    UIScreen(model),
    _waitingText{5,5, "Please wait..."},
    _completeText{5,5, "Safe to remove SD card"} {};

    void onEnter() override {
        //emit an event to the App to tell it to unmount the SD card
        emitAppEvent({AppEventType::UnmountSD,0});
    }

    void onExit() override {}

    void update(float dt) override {
        auto m = model.SD().get();

        //poll the data model to see if the SD card is unmounted
        _waitingText.setVisible(m.mounted);
        _completeText.setVisible(!m.mounted);

        //poll the data model to check if the SD card has been removed
        if (!m.present) {
            //SD card has been physically removed leave the page
            emitUIEvent(UIEventType::ChangeScreen, ScreenID::MainMenu);
        }

    }

    void render() override {
        _waitingText.render();
        _completeText.render();
    }

    void handleInput(const physIO input) override {}

    private:
    SelectableTextWidget _waitingText;
    SelectableTextWidget _completeText;
};