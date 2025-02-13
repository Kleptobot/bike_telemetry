# OBike
The OBike is an open source and open hardware telemetry logger.
It is based on the Seeed Studio XIAO nRF52840 (Sense) development board.

The OBike PCB incorporates a real time clock, a pressure sensor and a battery connector.
The PCB also includes a 5 position momentary switch, with  hardware debouncing for the switch inputs.
The switch input signals are connected to an IO expander module that is on the I2C bus.
Currently the display is a 128x128 monochrome OLED also connected to I2C.

Data from the BLE peripherals and onboard sensors is logged to a micro SD card. 
The data is saved in the garmin TCX xml format and csv.

Sensors and modules included in the PCB are:
- nRF52840 MCU, includes a 6 DOF IMU
- DS3231M RTC, providing time keeping and temperature
- DPS368XTSA1 pressure sensor, provides temperature compensated pressure readings
- uBlox MAX-M8C-0 GPS module

## Currently supported BLE peripherals are:
- peripherals that implement the Cycling Speed and Cadence (CSC) profile.
- peripherals that implement the heart rate monitor (HRM) profile
- peripherals that implement the Cycling Power Service (CPS) profile

## Future goals
- Better sleep power usage
- Advanced filtering to combine multiple signals into a best estimate. eg. GPS speed, CSC speed and acceleration values blended to create an estimated velocity.

### Extreme Goals
- Add maps support
- Add ANT+ support