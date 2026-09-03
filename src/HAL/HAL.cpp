#include "HAL.hpp"
#include "DebugConfig.hpp"
#include <numeric>

namespace {
    // Set by i2cUnwedgeIfStuck() at boot; surfaced in the [i2c] summary.
    bool s_busWasWedged = false;

    /**
     * Release a latched I2C slave before anything claims the bus.
     *
     * The GNSS module keeps backup power across MCU resets, so its I2C
     * engine can come up still holding SDA from a session that ended
     * mid-transaction. The first Wire call then hangs forever inside the
     * core's unbounded wait loops -- which at boot happens before
     * Serial.begin, i.e. a totally silent freeze (no output, no screen).
     *
     * Standard remedy: sense the lines; if SDA is held low, clock SCL up to
     * nine times until the slave lets go, then frame a STOP so every state
     * machine returns to idle. Runs at the very top of init_low(), ahead of
     * the MCP23017 and RTC bring-up. The bus pull-ups live on the always-on
     * rail (inputSystem turns the aux rail off before its own I2C and still
     * works), so the levels read here are the true bus state.
     */
    void i2cUnwedgeIfStuck() {
        const uint8_t sda = PIN_WIRE_SDA;
        const uint8_t scl = PIN_WIRE_SCL;

        // Sense with both lines released; the bus pull-ups define idle-high.
        pinMode(sda, INPUT);
        pinMode(scl, INPUT);
        delayMicroseconds(10);
        if (digitalRead(sda) == HIGH) {
            return;   // healthy idle bus -- the common case
        }
        s_busWasWedged = true;

        // Clock SCL until the stuck slave releases SDA (nine pulses cover a
        // full byte plus its ACK slot). SCL is only ever driven low and then
        // released, so a clock-stretching slave is respected.
        for (int i = 0; i < 9 && digitalRead(sda) == LOW; ++i) {
            pinMode(scl, OUTPUT);
            digitalWrite(scl, LOW);
            delayMicroseconds(5);
            pinMode(scl, INPUT);   // released; the pull-up raises it
            delayMicroseconds(5);
        }

        // Frame a STOP -- SDA low -> high while SCL is high -- so every
        // listener's state machine returns to the idle state.
        pinMode(sda, OUTPUT);
        digitalWrite(sda, LOW);
        delayMicroseconds(5);
        pinMode(sda, INPUT);       // released; the pull-up makes the edge
        delayMicroseconds(10);
        // Leave both pins as inputs; Wire.begin() takes ownership from here.
    }
}

void HAL::init_low() {
    i2cUnwedgeIfStuck();
    inputSystem.init();
    sensorSystem.init_low();
    _resetGPSTime = 0;
    _sleep = false;
}

void HAL::init(timeData* date) {

    HAL::inst().enableAuxRail();
    //turn the gps power supply on
    inputSystem.setOutput(GPIOB3, true);
    inputSystem.setOutput(GPIOB7, false);
    inputSystem.update(false);
    delayMicroseconds(20);
    inputSystem.setOutput(GPIOB7, true);
    inputSystem.update(false);
    _LC76G.i2c_wait();

    Wire.begin();
    Wire.setClock(100000); // 100kHz
    sensorSystem.init();
    bluetoothSystem.init(&storageSystem);
    for (int i=0; i<5; i++) {
        inputSystem.update(false);
        delay(50);
    }
    if (!inputs().SD_Det.state) {   //SD card pin is inverted, low means card is present
        while (!storageSystem.init(date)) {
            Serial.println("SD card detected, initializing...");
            delay(200);
        }
    } else {
        Serial.println("No SD card detected.");
    }
    _LC76G.begin(&Wire);
}

void HAL::update() {
    _tickStartMs = millis();
    _LC76G.update();

    // Bus grants flow through the arbiter (see HAL/I2CArbiter.hpp): foreign
    // devices take the bus only while the GNSS is idle AND getting its turns,
    // and every foreign use resets the GNSS settle window.
    const bool foreignBusy =
        _bus.foreignBusy(_LC76G.isBusy(), _LC76G.msSinceLastCycleStart());

    //Call GPIO inputs
    const uint32_t inT0 = micros();
    const bool inputsUpdated = inputSystem.update(foreignBusy);
    _dbgInBusyUs += micros() - inT0;
    if(inputsUpdated) {
        //rule 2: postpone the GNSS's next cycle start after foreign use
        _LC76G.i2c_wait();
    }
    
    //Call sensors
    const uint32_t sensT0 = micros();
    const bool sensorsUpdated = sensorSystem.update(foreignBusy);
    _dbgSensBusyUs += micros() - sensT0;
    if(sensorsUpdated) {
        //rule 2
        _LC76G.i2c_wait();
    }
    bluetoothSystem.update();

    //if reset time is non zero check if 100ms has passed since the trigger, then reset time to zero and write reset pin high
    if (_resetGPSTime > 0) {
        if (millis() - _resetGPSTime > 100) {
            inputSystem.setOutput(GPIOB5, true);
            _resetGPSTime = 0;
        }
    }
    if (_resetDispTime > 0) {
        if (millis() - _resetDispTime > 100) {
            inputSystem.setOutput(GPIOB7, true);
            _resetDispTime = 0;
        }
    }

    // Assemble the measurement frame for this tick (see HAL/Measurements.hpp)
    refreshFrame();

    debugBusSummary();
}

// Per-second I2C bus summary (ENABLE_I2C_DEBUG). These numbers make the
// LC76G-vs-sensors bus budget visible on hardware:
//   tx/s     -- raw Wire transactions the GNSS driver issued
//   drain/s  -- completed NMEA buffer reads (found data, len > 0)
//   bytes/s  -- NMEA payload drained
//   idle/s   -- length polls that found nothing queued
//   err      -- lifetime state-machine error count (grows on bus trouble)
//   maxgap   -- worst wait between completed drains, ms
//   bp       -- lifetime ticks where the arbiter held foreign devices off
//               for the GNSS (rule 3); zero in normal operation
//   in/sens  -- microseconds of bus time input/sensor passes consumed
void HAL::debugBusSummary() {
    if (!ENABLE_I2C_DEBUG) return;
    const uint32_t now = millis();
    if (now - _dbgSummaryMs < 1000) return;
    _dbgSummaryMs = now;

    const uint32_t tx     = _LC76G.dbgTxCount();
    const uint32_t cycles = _LC76G.dbgDrainCycles();
    const uint32_t bytes  = _LC76G.dbgBytesDrained();
    const uint32_t zero   = _LC76G.dbgZeroLenPolls();

    Serial.printf("[i2c] tx=%lu drain=%lu bytes=%lu idle=%lu err=%u maxgap=%lums bp=%lu wedge=%d in=%luus sens=%luus\r\n",
                  (unsigned long)(tx - _dbgPrevTx),
                  (unsigned long)(cycles - _dbgPrevCycles),
                  (unsigned long)(bytes - _dbgPrevBytes),
                  (unsigned long)(zero - _dbgPrevZero),
                  _LC76G.dbgErrorCount(),
                  (unsigned long)_LC76G.dbgMaxDrainGapMs(),
                  (unsigned long)_bus.dbgBackpressureTicks(),
                  (int)s_busWasWedged,
                  (unsigned long)_dbgInBusyUs,
                  (unsigned long)_dbgSensBusyUs);

    _dbgPrevTx = tx;
    _dbgPrevCycles = cycles;
    _dbgPrevBytes = bytes;
    _dbgPrevZero = zero;
    _dbgInBusyUs = 0;
    _dbgSensBusyUs = 0;
}

void HAL::resetGPS() {
    inputSystem.setOutput(GPIOB5, false);
    _resetGPSTime = millis();
}

void HAL::buzzStart() {
    inputSystem.setOutput(GPIOB2, true);
}

void HAL::buzzStop() {
    inputSystem.setOutput(GPIOB2, false);
}

void HAL::sleep() {
    Serial.println("sleep");
    //disableAuxRail();
    digitalWrite(D6, false); //turn off the auxilary supply
    inputSystem.setOutput(GPIOB6, false);    //turn off the screen backlight
    if (!_sleep) {
        _sleep = true;
        _LC76G.sendCommand(LC76G::PAIR_LOW_POWER_ENTRY_RTC_MODE,&HAL::onSleep,this,nullptr);
    }
}

void HAL::setNMEArates(uint8_t type, uint8_t rate) {
    LC76G::Payload2U8 p = {type, rate};
    _LC76G.sendCommand(LC76G::SET_NMEA_MSG_RATE,&HAL::onPAIRResponse,this,&p);
}

void HAL::onSleep(int numArgs, const void* payload, void* context) {
    auto* self = static_cast<HAL*>(context);
    self->_LC76G.closeDataFile();
    self->inputSystem.setOutput(GPIOB3,false);    //turn off the GPS enable supply
    self->inputSystem.setOutput(GPIOB6,false);    //turn off the screen backlight
}

void HAL::onPAIRResponse(int numArgs, const void* payload, void* context) {
    auto* self = static_cast<HAL*>(context);

    self->handlePAIRResponse(numArgs, payload);
}

void HAL::handlePAIRResponse(int numArgs, const void* payload) {
    // Validate payload before using it
    if (payload == nullptr || numArgs <= 0) {
        if (ENABLE_GPS_DEBUG) {
            Serial.println("[HAL] PAIR response received with no payload");
        }
        return;
    }
    
    uint8_t* byte_array = (uint8_t*)payload;
    if (ENABLE_GPS_DEBUG) {
        for(int i=0; i<numArgs && i<5; i++) {  // Added bounds check (max 5 bytes)
            Serial.print("[PAIR] Arg ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(byte_array[i]);
        }
    }
}
void HAL::disableAuxRail() {
  // Stop hardware SPI first so it releases its pin drive
  SPI.end();

  // Explicitly float the shared bus lines so no output stage
  // can backfeed AUX_3V3 through internal clamp diodes
  pinMode(D0,   INPUT); // TFT_CS
  pinMode(D1,   INPUT); // SD_CS
  pinMode(D2,   INPUT); // TFT_DC
  pinMode(D4,   INPUT); // 
  pinMode(D5,   INPUT); // 
  pinMode(D7,   INPUT); // FLASH_CS
  pinMode(D9,   INPUT); // MISO
  pinMode(D10,  INPUT); // MOSI

  // Now safe to cut the rail
  digitalWrite(D6, LOW);
}

void HAL::enableAuxRail() {
  digitalWrite(D6, HIGH);
  delay(5); // allow AUX_3V3 to stabilize before driving the flash chip

  // Restore CS as output, deasserted (idle high for most SPI flash)
  pinMode(D0,   OUTPUT); // TFT_CS
  pinMode(D1,   OUTPUT); // SD_CS
  pinMode(D2,   OUTPUT); // TFT_DC
  pinMode(D7,   OUTPUT); // FLASH_CS
  pinMode(D9,   OUTPUT); // MISO
  pinMode(D10,  OUTPUT); // MOSI

  // Re-init hardware SPI for use
  SPI.begin();
}