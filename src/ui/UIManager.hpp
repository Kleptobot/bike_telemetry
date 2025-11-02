#pragma once
#include <Arduino.h>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include "UIEventBus.hpp"

#include "UI/Screens/UIScreen.hpp"
#include "HAL/InputInterface.hpp"



class UIManager {
public:
    UIManager(UIEventBus& bus) : eventBus(bus) {}

    template<typename T, typename... Args>
    void registerScreen(ScreenID id, Args&&... args) {
        static_assert(std::is_base_of<UIScreen, T>::value, "T must derive from UIScreen");
        screens[id] = std::make_unique<T>(std::forward<Args>(args)...);
        screens[id]->setEventBus(&eventBus);
    }

    void begin(ScreenID startScreen) {
        auto it = screens.find(startScreen);
        if (it != screens.end()) {
            activeScreen = it->second.get();
            activeScreen->onEnter();
        } else {
            // Fallback or debug message
            activeScreen = nullptr;
        }
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
    UIEventBus& eventBus;
};