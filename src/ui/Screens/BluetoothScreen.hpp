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
    }

    void refreshDevices() {
        _devices = model.bluetooth().get();
        _version = model.bluetooth().version();
        
        deviceWidgets.clear();
        totalHeight = 0;
        for(uint i = 0; i<_devices.size(); i++) {
            deviceWidgets.push_back({10, 10+i*32, _devices[i]});
            totalHeight += (deviceWidgets.back().height() + 5);
        }
    }

    void onEnter() override {
        refreshDevices();
        emitAppEvent({AppEventType::DiscoverBluetooth});
        _selectedIndex = 0;
        _scrollOffset = 0;
    }

    void onExit() override {
        emitAppEvent({AppEventType::ScanBluetooth});
    }

    void update(float dt) override {
        //update the device list
        if (_version != model.bluetooth().version()) {
            refreshDevices();
        }

        for (uint32_t i =0 ; i<deviceWidgets.size(); i++) {
            //set the visiblity of the device widgets,
            deviceWidgets[i].setVisible((i>=_scrollOffset) && ((i-_scrollOffset) < visibleDevices));
            deviceWidgets[i].setFocused(i==_selectedIndex);
        }
        
        backWidget.setFocused(_selectedIndex == _devices.size());
    }

    void handleInput(physIO input) override {
        if (input.Select.press) {
            if (_selectedIndex < _devices.size()) {
                if (!_devices[_selectedIndex].saved) {
                    emitAppEvent({AppEventType::ConnectBluetooth,this->_devices[_selectedIndex]});
                } else {
                    emitAppEvent({AppEventType::DisconnectBluetooth,this->_devices[_selectedIndex]});
                }
                return;
            } else {
                backWidget.handleInput(input);
            }
        }
        else if (input.Up.press) moveSelection(-1);
        else if (input.Down.press) moveSelection(+1);
    }

    void render() override {
        for (uint32_t i=0; i<deviceWidgets.size() ; i++) {
            if (deviceWidgets[i].isVisible()) {
                deviceWidgets[i].render(10, 10 + (i-_scrollOffset)*32);
            }
        }

        //draw a scroll bar
        if (_devices.size() > visibleDevices) {
            float ratio = (float)visibleDevices / _devices.size();
            int barHeight = int(ratio * totalHeight);
            int barY = int((_scrollOffset / float(_devices.size())) * totalHeight);
            Disp::drawRect(SCREEN_WIDTH - 4, barY, 2, barHeight, ST77XX_WHITE);
        }

        //render the back button after all the BT widgets
        backWidget.render(5, 10 + min(visibleDevices, deviceWidgets.size())*32);
    }

private:
    const uint32_t visibleDevices = 4;
    uint32_t _selectedIndex = 0;
    uint32_t _scrollOffset = 0;
    int totalHeight=0;
    std::vector<BluetoothDeviceWidget> deviceWidgets;
    SelectableTextIconWidget backWidget;
    std::vector<BluetoothDevice> _devices;
    uint32_t _version;


    void moveSelection(int delta) {
        if (_devices.empty()) return;

        uint32_t length = _devices.size() + 1;

        //scroll index must b 0 to _devices.size() to allow one more for the back button
        _selectedIndex = (_selectedIndex + delta + length) % length;

        if (_selectedIndex<_scrollOffset+visibleDevices && _scrollOffset<0) {
            _scrollOffset += delta;
            _scrollOffset = min(_scrollOffset,_devices.size()-visibleDevices);
        }
    }
};