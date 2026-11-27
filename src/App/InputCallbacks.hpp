#ifndef INPUT_CALLBACKS_H
#define INPUT_CALLBACKS_H

#include "../DataModel/DataModel.hpp"
#include "../HAL/BluetoothInterface.hpp"

#include <vector>

class InputCallbacks {
public:
    void begin(DataModel& model);

    bool gpsEnableState() const { return _gpsEnableState; }

private:
    DataModel* _model = nullptr;
    bool _gpsEnableState = true;

    void updateBluetooth(const std::vector<BluetoothDevice>& devices);
    void updateGpsEnable(bool state);
};

#endif /* INPUT_CALLBACKS_H */
