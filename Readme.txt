# OBike
The OBkike is an open source and open hardware telemetry logger.
It is based on the Seeed Studio XIAO nRF52840 (Sense) development board.

The OBike PCB incorporates a real time clock, a pressure sensor and a battery connector.
The PCB also includes a 5 position momentary switch, with  hardware debouncing for the switch inputs.
The switch input signals are connected to an IO expander module that is on the I2C bus.
Currently the display is a 128x128 monochrome OLED also connected to I2C.

The XIAO nRF52840 (Sense) includes a temperature sensor and a 6 axis IMU

Data from the BLE peripherals is logged to a micro SD card. Temperature from the RTC, acceleration, angular velocity and battery voltage from the XIAO nRF52840 (Sense) are also logged.


Currently supported peripherals are:
- peripherals that implement the Cycling Speed and Cadence (CSC) profile.


Future goals
- support for heart rate monitor (HRM) peripherals
- support for Cycling Power Service (CPS) peripherals
- add GPS module and log GPS speed and position