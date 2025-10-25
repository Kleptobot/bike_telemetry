#pragma once
#include <Arduino.h>
#include <memory>
#include <unordered_map>

#include "UI/Screens/UIScreen.hpp"
#include "HAL/InputInterface.hpp"



class UIManager {
public:
    void registerScreen(ScreenID id, std::unique_ptr<UIScreen> screen) {
        screens[id] = std::move(screen);
    }

    void showScreen(ScreenID id) {
        if (activeScreen)
            activeScreen->onExit();

        activeScreen = screens[id].get();
        activeScreen->onEnter();
    }

    void update(float dt) {
        if (activeScreen)
            activeScreen->update(dt);
    }

    void render() {
        if (activeScreen)
            activeScreen->render();
    }

    void handleInput(const physIO input){
        if (activeScreen)
            activeScreen->handleInput(input);
    }

    void handleUIEvent(const UIEvent& e);

    UIScreen* getActiveScreen() const { return activeScreen; }

private:
    std::unordered_map<ScreenID, std::unique_ptr<UIScreen>> screens;
    UIScreen* activeScreen = nullptr;
};