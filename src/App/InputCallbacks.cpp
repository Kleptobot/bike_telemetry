#include "InputCallbacks.hpp"

#include "../HAL/HAL.hpp"
#include "../HAL/InputSystem.hpp"

void InputCallbacks::begin(DataModel& model) {
    _model = &model;

    HAL::inst().bluetooth().onDeviceList([this](std::vector<BluetoothDevice> devices) {
        this->updateBluetooth(devices);
    });

    InputSystem::onEnableStateRead([this](bool pinState) {
        this->updateGpsEnable(pinState);
    });
}

void InputCallbacks::updateBluetooth(const std::vector<BluetoothDevice>& devices) {
    if (_model) _model->bluetooth().update(devices);
}

void InputCallbacks::updateGpsEnable(bool state) {
    _gpsEnableState = state;
}
