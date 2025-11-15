#pragma once
#include "UI/Screens/UIScreen.hpp"
#include "UI/Widgets/ListView.hpp"
#include "UI/Widgets/BluttoothDeviceWidget.hpp"
#include "UI/Widgets/SelectableTextIcon.hpp"
#include "HAL/InputInterface.hpp"
#include "UI/GFX.h"
#include <memory>

class BluetoothScreen : public UIScreen {
public:
    BluetoothScreen(DataModel& model) : 
    UIScreen(model),
    backWidget{0,0,"Back",epd_bitmap_left} {

        //register press event callback to send a change screen event
        backWidget.setOnPress([this] () {
            emitUIEvent(UIEventType::ChangeScreen, ScreenID::SettingsMenu);
        });

        //populate the wdiget list the number of visible devices
        for (uint16_t i=0; i<visibleDevices ; i++) {
            deviceWidgets.push_back({0,i*32});
            deviceWidgets[i].setOnPress([this] () {
                if (!_devices[_selectedIndex].saved)
                    emitAppEvent({AppEventType::ConnectBluetooth,this->_devices[_selectedIndex]});
                else
                    emitAppEvent({AppEventType::DisconnectBluetooth,this->_devices[_selectedIndex]});
                });
        }
        totalHeight = deviceWidgets.size()*32;
    }

    void update(float dt) override {
        //update the device list
        _devices = model.bluetooth().get();

        for (uint32_t i =0 ; i<_devices.size(); i++) {
            //guard against overrun
            if (i<deviceWidgets.size()) {
                //set the visiblity of the device widgets,
                deviceWidgets[i].setVisible(i<deviceWidgets.size());

                //update the widget with the scroll offset
                deviceWidgets[i].device(&_devices[i+_scrollOffset]);
            }
        }

    }

    void handleInput(physIO input) override {
        if (input.Select.press) {
            for (uint32_t i =0 ; i<deviceWidgets.size(); i++) {
                deviceWidgets[i].handleInput(input);
            }
            backWidget.handleInput(input);
            return;
        }
        else if (input.Up.press) moveSelection(-1);
        else if (input.Down.press) moveSelection(+1);

        //check if a widget is focused
        for (uint32_t i =0 ; i<deviceWidgets.size(); i++) {
            deviceWidgets[i].setFocused(i == (_selectedIndex-_scrollOffset));
        }
        backWidget.setFocused(_selectedIndex == _devices.size());
    }

    void render() override {
        for (uint32_t i=0; i<visibleDevices ; i++) {
            deviceWidgets[i].render();
        }

        //draw a scroll bar
        if (_devices.size() > visibleDevices) {
            float ratio = (float)visibleDevices / _devices.size();
            int barHeight = int(ratio * totalHeight);
            int barY = int((_scrollOffset / float(_devices.size())) * totalHeight);
            Disp::drawRect(SCREEN_WIDTH - 4, barY, 2, barHeight, ST77XX_WHITE);
        }

        //render the back button after all the BT widgets
        backWidget.render(0,min(visibleDevices,_devices.size())*32);
    }

private:
    const uint32_t visibleDevices = 4;
    uint32_t _selectedIndex = 0;
    uint32_t _scrollOffset = 0;
    int totalHeight=0;
    std::vector<BluetoothDeviceWidget> deviceWidgets;
    SelectableTextIconWidget backWidget;
    std::vector<BluetoothDevice> _devices;

    void moveSelection(int delta) {
        if (_devices.empty()) return;

        //scroll index must b 0 to _devices.size() to allow one more for the back button
        if (_selectedIndex<_devices.size() && _selectedIndex>0) {
            _selectedIndex += delta;
        }

        if (_selectedIndex<_scrollOffset+visibleDevices && _scrollOffset<0) {
            _scrollOffset += delta;
            _scrollOffset = min(_scrollOffset,_devices.size()-visibleDevices);
        }
    }
};