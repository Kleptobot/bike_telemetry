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
        UIScreen(model),
        restGPS{5,0,"Reset GPS",epd_bitmap_loop},
        restoreGPSDefaults{5,20,"Restore Defaults",epd_bitmap_gear},
        RMCrateLabel{5, 40, "RMC rate: "},
        RMCrate{120, 40, "1"},
        VTGrateLabel{5, 60, "VTG rate: "},
        VTGrate{120, 60, "1"},
        saveNVRAM{5, 80,"Save NVRAM",epd_bitmap_save},
        backWidget{5, 120,"Back",epd_bitmap_left} {

            //attach App events
            restGPS.setOnPress([this] () {
                emitAppEvent({AppEventType::ResetGPS});
            });
            restoreGPSDefaults.setOnPress([this] () {
                emitAppEvent({AppEventType::RestoreDefaultsGPS});
            });
            saveNVRAM.setOnPress([this] () {
                emitAppEvent({AppEventType::saveGPSNVRAM});
            });

            //register press event callback to send a change screen event
            backWidget.setOnPress([this] () {
                emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
            });
        }

    void onEnter() override {
    }

    void render() override {
        restGPS.render();
        restoreGPSDefaults.render();
        RMCrateLabel.render();
        RMCrate.render();
        VTGrateLabel.render();
        VTGrate.render();
        saveNVRAM.render();
        backWidget.render();
    }

    void update(float dt) override {
        restGPS.setFocused(_index==0);
        restoreGPSDefaults.setFocused(_index==1);
        RMCrate.setFocused(_index==2);
        VTGrate.setFocused(_index==3);
        saveNVRAM.setFocused(_index==4);
        backWidget.setFocused(_index==5);
    }

    void handleInput(physIO input) override {
        if (!anySelected()) {
            if (input.Up.press) _index = (_index + 5) % 6;
            if (input.Down.press) _index = (_index + 1) % 6;
        }

        

        restGPS.handleInput(input);
        restoreGPSDefaults.handleInput(input);
        RMCrate.handleInput(input);
        VTGrate.handleInput(input);
        saveNVRAM.handleInput(input);
        backWidget.handleInput(input);
    }

    private:
        SelectableTextIconWidget restGPS;
        SelectableTextIconWidget restoreGPSDefaults;
        SelectableTextWidget RMCrateLabel;
        SelectableTextWidget RMCrate;
        SelectableTextWidget VTGrateLabel;
        SelectableTextWidget VTGrate;
        SelectableTextIconWidget saveNVRAM;
        SelectableTextIconWidget backWidget;

        uint _index = 0;

        bool anySelected() {
            return  RMCrate.isSelected() ||
                    VTGrate.isSelected() ||
                    restGPS.isSelected() ||
                    restoreGPSDefaults.isSelected() ||
                    saveNVRAM.isSelected() ||
                    backWidget.isSelected();
        }

};