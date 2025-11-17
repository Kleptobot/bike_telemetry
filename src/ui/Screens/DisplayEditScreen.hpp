#pragma once
#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/DisplayEditWidget.hpp"
#include "UI/Widgets/SelectableTextIcon.hpp"

class DisplayEditScreen : public UIScreen {
    public:
        DisplayEditScreen (DataModel& model) : 
            UIScreen(model),
            numDisplaysLabel{5,5,"Num displays: "},
            numDisplays{160,5,""},
            saveWidget{5,80,"Save",epd_bitmap_save} {
                //register the save press event callback to send a change screen and app save event
                saveWidget.setOnPress([this] () {
                    std::vector<TelemetryType> types;
                    for ( const auto& disp : dispEdits) {
                        types.push_back(disp.type());
                    }
                    this->model.layout().update( {types} );
                    emitAppEvent({AppEventType::SaveLayout,0});
                    emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
                });
        }

        void onEnter() override {
            auto& l = model.layout().get();
            dispEdits.clear();
            for ( int i = 0; i<l.displays.size(); i++) {
                dispEdits.push_back({5,21+i*16,l.displays[i]});
            }
            numDisplays.setText(String(dispEdits.size()));
        }

        void update(float dt) override {
            for ( int i = 0; i<dispEdits.size(); i++) {
                dispEdits[i].update(dt);
                dispEdits[i].setFocused( _index == (i+1) );
            }
            numDisplays.setFocused(_index == 0);
            numDisplays.setText(String(dispEdits.size()));
            saveWidget.setFocused(_index == dispEdits.size()+1);
        }

        void handleInput(physIO input) override {
            if (!anySelected()) {
                if (input.Up.press) _index = (_index + dispEdits.size() + 1) % (dispEdits.size()+2);
                else if (input.Down.press) _index = (_index + 1) % (dispEdits.size()+2);
            }
            
            if (_index == 0) {
                if (input.Select.press) numDisplays.setSelected(!numDisplays.isSelected());
                if (numDisplays.isSelected()) {
                    if (input.Up.press){
                        if (dispEdits.size()>7) return;
                        int d = dispEdits.size();
                        dispEdits.push_back({5,21+d*16});
                        dispEdits.back().invalidate();
                    }
                    if (input.Down.press) {
                        if (dispEdits.size()<1) return;
                        dispEdits.pop_back();
                    }
                }
            }
            else if (_index == (dispEdits.size()+1)) saveWidget.handleInput(input);
            else {
                if (dispEdits.size()>0) {
                dispEdits[_index-1].handleInput(input);
                }
            }
        }

        void render() override {
            numDisplaysLabel.render();
            numDisplays.render();
            for ( auto& disp : dispEdits) {
                disp.render();
            }
            saveWidget.render(5,21+dispEdits.size()*16);
        }
    
    private:
        
        SelectableTextWidget numDisplaysLabel;
        SelectableTextWidget numDisplays;
        std::vector<DisplayEditWidget> dispEdits;
        SelectableTextIconWidget saveWidget;

        int _index = 0;
        bool anySelected() {
            bool dispSelected = false;
            for (auto& disp : dispEdits) {
                dispSelected |= disp.isSelected();
            }
            return  dispSelected ||
                    numDisplays.isSelected() ||
                    saveWidget.isSelected();
        }
};