#pragma once
#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/DisplayEditWidget.hpp"
#include "UI/Widgets/SelectableTextIcon.hpp"

class DisplayEditScreen : public UIScreen {
    public:
        DisplayEditScreen (DataModel& model) : 
            UIScreen(model),
            disp1{0,0},
            disp2{0,20},
            disp3{0,40},
            saveWidget{0,60,"Save",epd_bitmap_save} {
                //register the save press event callback to send a change screen and app save event
                saveWidget.setOnPress([this] () {
                    this->model.layout().update( {  this->disp1.type(),
                                                    this->disp2.type(),
                                                    this->disp3.type()});
                    emitAppEvent({AppEventType::SaveLayout,0});
                    emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                });
        }

        void onEnter() override {
            auto& l = model.layout().get();
            disp1.setType(l.disp1);
            disp2.setType(l.disp2);
            disp3.setType(l.disp3);
        }

        void update(float dt) override {
            disp1.update(dt);
            disp2.update(dt);
            disp3.update(dt);

            disp1.setFocused(_index == 0);
            disp2.setFocused(_index == 1);
            disp3.setFocused(_index == 2);
            saveWidget.setFocused(_index == 3);
        }

        void handleInput(physIO input) override {
            if (anySelected) {
                switch(_index) {
                    case 0: disp1.handleInput(input);
                    case 1: disp2.handleInput(input);
                    case 2: disp3.handleInput(input);
                    case 3: saveWidget.handleInput(input);
                }
            } else {
                if (input.Up.press) (_index + 3) % 4;
                else if (input.Down.press) (_index + 1) % 4;
            }
        }

        void render() override {
            disp1.render();
            disp2.render();
            disp3.render();
            saveWidget.render();
        }
    
    private:
        DisplayEditWidget disp1;
        DisplayEditWidget disp2;
        DisplayEditWidget disp3;
        SelectableTextIconWidget saveWidget;

        int _index = 0;

        bool anySelected() {
            return  disp1.isSelected() ||
                    disp2.isSelected() ||
                    disp3.isSelected() ||
                    saveWidget.isSelected();
        }
};