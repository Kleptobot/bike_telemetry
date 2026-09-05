#ifndef HAL_H
#define HAL_H

#include <TinyGPSPlus.h>

#include "InputSystem.hpp"
#include "Sensors.hpp"
#include "Bluetooth/BluetoothSystem.hpp"
#include "LC76G.hpp"
#include "SDCard.hpp"
#include "Measurements.hpp"
#include "I2CArbiter.hpp"

class HAL {
    public:
    static HAL& inst() {
        static HAL instance;
        return instance;
    }

    void init_low();
    void init(timeData* date);
    void update();
    physIO inputs() { return inputSystem.state(); };
    IStorage* SD() { return &storageSystem; }
    BluetoothSystem& bluetooth() { return bluetoothSystem; }
    void reInitStorage(timeData* date) {
        while (!storageSystem.init(date)) {
            Serial.println("SD card detected, initializing...");
            delay(200);
        }
    }

    void sleep();
    void setNMEArates(uint8_t type, uint8_t rate);
    void gpsRestoreDefaults() { _LC76G.sendCommand(LC76G::RESTORE_DEFAULT_SETTING,nullptr,this,nullptr); }
    void gpsSaveNVRAM() { _LC76G.sendCommand(LC76G::SAVE_TO_NVRAM,nullptr,this,nullptr); }
    void gpsHotStart() { _LC76G.sendCommand(LC76G::GNSS_SUBSYS_HOT_START,nullptr,this,nullptr); }
    void gpsWarmStart() { _LC76G.sendCommand(LC76G::GNSS_SUBSYS_WARM_START,nullptr,this,nullptr); }
    void gpsColdStart() { _LC76G.sendCommand(LC76G::GNSS_SUBSYS_COLD_START,nullptr,this,nullptr); }
    void setRMCRate() { 
        LC76G::Payload1Ch1U8 p = {"RMC",1};
        _LC76G.sendCommand(LC76G::SET_NMEA_RATE, &HAL::onPAIRResponse, this, &p);
    }
    void resetGPS();

    bool SDMounted() const { return storageSystem.isMounted(); };
    void unMountSD() {storageSystem.unMount(); };

    // Complete acquisition snapshot, rebuilt once per tick (see
    // HAL/Measurements.hpp). Read-side contract for the App layer: raw facts
    // with timestamps, sequence numbers and observed sample intervals;
    // interpretation lives above this line, never inside the frame.
    const MeasurementFrame& measurements() const { return _frame; }

    void setTime(struct tm date) { sensorSystem.setTime(date); }
    void screenOn() { inputSystem.setOutput(GPIOB6,true); }
    void screenOff() { inputSystem.setOutput(GPIOB6,false); }

    void buzzStart();
    void buzzStop();

    void disableAuxRail();
    void enableAuxRail();

    private:
    HAL() : _LC76G(storageSystem) {}

    //internal HAL systems
    InputSystem inputSystem;
    SensorSystem sensorSystem;
    BluetoothSystem bluetoothSystem;
    SDCardSystem storageSystem;
    LC76G _LC76G;

    // private memeber variables
    uint8_t _rxBuffer[1024];
    uint32_t _resetGPSTime, _resetDispTime;
    bool _sleep;

    // Measurement-frame state (see HAL/Measurements.hpp). refreshFrame() is
    // defined inline below the class so the simulator -- which replaces only
    // HAL.cpp at link time -- assembles the frame from the same code path.
    MeasurementFrame _frame;
    GpsTrackers _gpsTrk;
    uint32_t _tickStartMs = 0;
    uint16_t _frameSeq = 0;

    // I2C bus debug state (ENABLE_I2C_DEBUG, summary printed from HAL.cpp).
    // HAL.cpp is replaced at link time by the simulator, so this is
    // firmware-only; the accumulators are just data to the sim.
    uint32_t _dbgSummaryMs = 0;
    uint32_t _dbgInBusyUs = 0, _dbgSensBusyUs = 0;
    uint32_t _dbgPrevTx = 0, _dbgPrevCycles = 0, _dbgPrevBytes = 0, _dbgPrevZero = 0;

    // Bus policy (see HAL/I2CArbiter.hpp). Orchestration lives in HAL.cpp,
    // which the simulator replaces at link time.
    I2CArbiter _bus;

    //private methods
    static void onSleep(int numArgs, const void* payload, void* context);
    static void onPAIRResponse(int numArgs, const void* payload, void* context);
    void handlePAIRResponse(int numArgs, const void* payload);
    void refreshFrame();
    void debugBusSummary();

    //
};

// =============================================================================
// Frame assembly (see HAL/Measurements.hpp).
//
// Inline here -- not in HAL.cpp -- so the simulator, which replaces only
// HAL.cpp at link time, builds the frame from this exact implementation
// instead of a forked copy. It reads the same acquisition state the legacy
// getters fold, stamps it, and changes no behaviour: it runs after everything
// else in HAL::update(), and nothing consumes the frame yet.
// =============================================================================
inline void HAL::refreshFrame() {
    MeasurementFrame f;
    f.tickStartMs = _tickStartMs;
    f.frameSeq = ++_frameSeq;

    // Motion / environment -- stamped by SensorSystem at acquisition time.
    const dps_data dps = sensorSystem.dps();
    fillSample(f.imu, sensorSystem.imu(), true,
               sensorSystem.imuTsMs(), sensorSystem.imuSeq(), sensorSystem.imuDtMs());
    fillSample(f.baroPressurePa, dps.f32_DSP_Pa, dps.dpsValid,
               sensorSystem.dpsTsMs(), sensorSystem.dpsSeq(), sensorSystem.dpsDtMs());
    fillSample(f.baroTempC, dps.f32_DSP_Temp, dps.dpsValid,
               sensorSystem.dpsTsMs(), sensorSystem.dpsSeq(), sensorSystem.dpsDtMs());
    fillSample(f.rtcTempC, dps.f32_RTC_Temp, sensorSystem.rtcSeq() != 0,
               sensorSystem.rtcTsMs(), sensorSystem.rtcSeq(), sensorSystem.rtcDtMs());
    fillSample(f.rtcNow, sensorSystem.now(), true,
               sensorSystem.rtcTsMs(), sensorSystem.rtcSeq(), sensorSystem.rtcDtMs());
    fillSample(f.charging, sensorSystem.charging(), sensorSystem.battSeq() != 0,
               sensorSystem.battTsMs(), sensorSystem.battSeq(), sensorSystem.battDtMs());

    // GNSS -- from the real TinyGPSPlus state, identical on device and sim.
    fillGps(f, _LC76G.gps(), _tickStartMs, _gpsTrk);

    // Drivetrain: exact event counters from the primary device...
    const csc* wheelSrc = csc::latestWheelSource();
    if (wheelSrc) {
        fillEventCount(f.wheelRevs, wheelSrc->wheelRevTotal(), wheelSrc->lastWheelEvt1024(),
                       wheelSrc->b_speed_present, wheelSrc->wheelEvtMillis());
    }
    const csc* crankSrc = csc::latestCrankSource();
    if (crankSrc) {
        fillEventCount(f.crankRevs, crankSrc->crankRevTotal(), crankSrc->lastCrankEvt1024(),
                       crankSrc->b_cadence_present, crankSrc->crankEvtMillis());
    }
    // ...and the legacy filtered aggregates exactly as the getters fold them.
    f.wheelRpmFiltered   = csc::getSpeed();
    f.cadenceRpmFiltered = csc::getCadence();

    // Power / bio -- torque, balance and force are decoded by the parser but
    // were previously dropped at the boundary.
    f.powerWatts      = cps::getPower();
    f.torqueNm        = cps::getTorque();
    f.pedalBalancePct = cps::getPedalBalance();
    f.forceMagN       = cps::getForceMagnitude();
    f.heartRateBpm    = hrm::getHRM();

    // Electrical
    f.batteryPct = sensorSystem.batt();
    fillSample(f.vbatVolts, sensorSystem.vbatVolts(), sensorSystem.battSeq() != 0,
               sensorSystem.battTsMs(), sensorSystem.battSeq(), sensorSystem.battDtMs());

    // Storage state (SD_DET is inverted: low = card present)
    f.sdPresent = !inputSystem.state().SD_Det.state;
    f.sdMounted = storageSystem.isMounted();

    _frame = f;
}

#endif /* HAL_H */