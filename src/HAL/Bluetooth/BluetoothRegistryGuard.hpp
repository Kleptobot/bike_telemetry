#ifndef BLUETOOTHREGISTRYGUARD_H
#define BLUETOOTHREGISTRYGUARD_H

#if defined(__arm__) || defined(__thumb__)
extern "C" {
#include <FreeRTOS.h>
#include <task.h>
}
#endif

// BLE callbacks and the main loop run as separate FreeRTOS tasks on device.
// Suspending scheduling protects short registry operations without masking
// interrupts or holding a lock across Bluefruit/GATT calls.
class BluetoothRegistryGuard {
public:
    BluetoothRegistryGuard() {
#if defined(__arm__) || defined(__thumb__)
        vTaskSuspendAll();
#endif
    }

    ~BluetoothRegistryGuard() {
#if defined(__arm__) || defined(__thumb__)
        (void)xTaskResumeAll();
#endif
    }

    BluetoothRegistryGuard(const BluetoothRegistryGuard&) = delete;
    BluetoothRegistryGuard& operator=(const BluetoothRegistryGuard&) = delete;
};

#endif /* BLUETOOTHREGISTRYGUARD_H */
