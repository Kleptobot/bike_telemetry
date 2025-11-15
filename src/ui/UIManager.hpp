#pragma once
#include <Arduino.h>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include "UIEventBus.hpp"

#include "UI/Screens/UIScreen.hpp"
#include "HAL/InputInterface.hpp"
#include "display/Display.hpp"



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
        Disp::markDirty(0,0,240,320);
    }

    void showScreen(ScreenID id) {
        if (activeScreen)
            activeScreen->onExit();

        activeScreen = screens[id].get();
        Disp::clear();
        Disp::markDirty(0,0,240,320);
        activeScreen->onEnter();
    }

    void update(float dt) {
        if (activeScreen)
            activeScreen->update(dt);
    }

    void render() {
        canvas.fillScreen(ST77XX_BLACK); // clear once per frame
        if (activeScreen)
            activeScreen->render();

        // push to hardware in one shot
        Disp::flush();
        Disp::resetDirty();
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