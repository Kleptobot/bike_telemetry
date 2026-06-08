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

        GGArateLabel{5, 50, "GGA rate: "},
        GGArate{GGArateLabel.width() + 10, 50, String(rates[0])},
        GLLrateLabel{5, 70, "GLL rate: "},
        GLLrate{GLLrateLabel.width() + 10, 70, String(rates[1])},
        GSArateLabel{5, 90, "GSA rate: "},
        GSArate{GSArateLabel.width() + 10, 90, String(rates[2])},
        GSVrateLabel{5, 110, "GSV rate: "},
        GSVrate{GSVrateLabel.width() + 10, 110, String(rates[3])},
        RMCrateLabel{5, 130, "RMC rate: "},
        RMCrate{RMCrateLabel.width() + 10, 130, String(rates[4])},
        VTGrateLabel{5, 150, "VTG rate: "},
        VTGrate{VTGrateLabel.width() + 10, 150, String(rates[5])},
        ZDArateLabel{5, 170, "ZDA rate: "},
        ZDArate{ZDArateLabel.width() + 10, 170, String(rates[6])},
        GRSrateLabel{5, 190, "GRS rate: "},
        GRSrate{GRSrateLabel.width() + 10, 190, String(rates[7])},
        GSTrateLabel{5, 210, "GST rate: "},
        GSTrate{GSTrateLabel.width() + 10, 210, String(rates[8])},
        GNSrateLabel{5, 230, "GNS rate: "},
        GNSrate{GNSrateLabel.width() + 10, 230, String(rates[9])},

        saveNVRAM{5, 260,"Save NVRAM",epd_bitmap_save},
        backWidget{15, 280,"Back",epd_bitmap_left},
        saveWidget{90, 280,"Save",epd_bitmap_save} {

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

        for (int i = 0; i < 10; ++i) {
            rateLabelWidgetRefs[i].get().render();
            ratesWidgetRefs[i].get().render();
        }


        saveNVRAM.render();
        backWidget.render();
        saveWidget.render();
    }

    void update(float dt) override;

    void handleInput(physIO input) override;

    private:
        enum class EditField { 
            resetGPS = 0,
            restoreDefaults,
            GGArate,
            GLLrate,
            GSArate,
            GSVrate,
            RMCrate,
            VTGrate,
            ZDArate,
            GRSrate,
            GSTrate,
            GNSrate,
            saveNVRAM,
            back,
            save };

        EditField focusField = EditField::resetGPS;
        SelectableTextIconWidget restGPS;
        SelectableTextIconWidget restoreGPSDefaults;

        SelectableTextWidget GGArateLabel;
        SelectableTextWidget GGArate;
        SelectableTextWidget GLLrateLabel;
        SelectableTextWidget GLLrate;
        SelectableTextWidget GSArateLabel;
        SelectableTextWidget GSArate;
        SelectableTextWidget GSVrateLabel;
        SelectableTextWidget GSVrate;
        SelectableTextWidget RMCrateLabel;
        SelectableTextWidget RMCrate;
        SelectableTextWidget VTGrateLabel;
        SelectableTextWidget VTGrate;
        SelectableTextWidget ZDArateLabel;
        SelectableTextWidget ZDArate;
        SelectableTextWidget GRSrateLabel;
        SelectableTextWidget GRSrate;
        SelectableTextWidget GSTrateLabel;
        SelectableTextWidget GSTrate;
        SelectableTextWidget GNSrateLabel;
        SelectableTextWidget GNSrate;

        SelectableTextIconWidget saveNVRAM;
        SelectableTextIconWidget backWidget;
        SelectableTextIconWidget saveWidget;

         std::reference_wrapper<SelectableTextWidget> rateLabelWidgetRefs[10] = {
            GGArateLabel,
            GLLrateLabel,
            GSArateLabel,
            GSVrateLabel,
            RMCrateLabel,
            VTGrateLabel,
            ZDArateLabel,
            GRSrateLabel,
            GSTrateLabel,
            GNSrateLabel};

        std::reference_wrapper<SelectableTextWidget> ratesWidgetRefs[10] = {
            GGArate,
            GLLrate,
            GSArate,
            GSVrate,
            RMCrate,
            VTGrate,
            ZDArate,
            GRSrate,
            GSTrate,
            GNSrate};
        
        int rates[10] = {1,0,0,0,1,0,0,0,0,0};
        bool prevSelectedRates[10] = {false,false,false,false,false,false,false,false,false,false};

        bool anySelected() {
            return  restGPS.isSelected() ||
                    restoreGPSDefaults.isSelected() ||
                    ratesWidgetRefs[0].get().isSelected() ||
                    ratesWidgetRefs[1].get().isSelected() ||
                    ratesWidgetRefs[2].get().isSelected() ||
                    ratesWidgetRefs[3].get().isSelected() ||
                    ratesWidgetRefs[4].get().isSelected() ||
                    ratesWidgetRefs[5].get().isSelected() ||
                    ratesWidgetRefs[6].get().isSelected() ||
                    ratesWidgetRefs[7].get().isSelected() ||
                    ratesWidgetRefs[8].get().isSelected() ||
                    ratesWidgetRefs[9].get().isSelected() ||
                    saveNVRAM.isSelected() ||
                    backWidget.isSelected() ||
                    saveWidget.isSelected();
        }

        void moveFocusUp() {
            switch (focusField) {
                case EditField::resetGPS: focusField = EditField::back;break;
                case EditField::restoreDefaults: focusField = EditField::resetGPS; break;
                case EditField::GGArate: focusField = EditField::restoreDefaults; break;
                case EditField::GLLrate: focusField = EditField::GGArate; break;
                case EditField::GSArate: focusField = EditField::GLLrate; break;
                case EditField::GSVrate: focusField = EditField::GSArate; break;
                case EditField::RMCrate: focusField = EditField::GSVrate; break;
                case EditField::VTGrate: focusField = EditField::RMCrate; break;
                case EditField::ZDArate: focusField = EditField::VTGrate; break;
                case EditField::GRSrate: focusField = EditField::ZDArate; break;
                case EditField::GSTrate: focusField = EditField::GRSrate; break;
                case EditField::GNSrate: focusField = EditField::GSTrate; break;
                case EditField::saveNVRAM: focusField = EditField::GNSrate; break;
                case EditField::back: focusField = EditField::saveNVRAM; break;
                default: break;
            }
        }

        void moveFocusDown() {
            switch (focusField) {
                case EditField::resetGPS: focusField = EditField::restoreDefaults;break;
                case EditField::restoreDefaults: focusField = EditField::GGArate; break;
                case EditField::GGArate: focusField = EditField::GLLrate; break;
                case EditField::GLLrate: focusField = EditField::GSArate; break;
                case EditField::GSArate: focusField = EditField::GSVrate; break;
                case EditField::GSVrate: focusField = EditField::RMCrate; break;
                case EditField::RMCrate: focusField = EditField::VTGrate; break;
                case EditField::VTGrate: focusField = EditField::ZDArate; break;
                case EditField::ZDArate: focusField = EditField::GRSrate; break;
                case EditField::GRSrate: focusField = EditField::GSTrate; break;
                case EditField::GSTrate: focusField = EditField::GNSrate; break;
                case EditField::GNSrate: focusField = EditField::saveNVRAM; break;
                case EditField::saveNVRAM: focusField = EditField::back; break;
                case EditField::back: focusField = EditField::resetGPS; break;
                default: break;
            }
        }

        void moveFocusLeft() {
            switch (focusField) {
                case EditField::back: focusField = EditField::save; break;
                case EditField::save: focusField = EditField::back; break;
                default: break;
            }
        }

        void moveFocusRight() {
            switch (focusField) {
                case EditField::back: focusField = EditField::save; break;
                case EditField::save: focusField = EditField::back; break;
                default: break;
            }
        }
};