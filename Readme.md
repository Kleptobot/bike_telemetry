# OBike — Open Bicycle Telemetry Platform

OBike is an open-source hardware and firmware project for building a DIY bike computer that feels closer to a purpose-built cycling device than a generic microcontroller project. It combines a colour display, GPS, Bluetooth sensors, barometric altitude, and SD-card logging into a single platform you can build, modify, and improve.

If you like the idea of a compact, open bike computer that can log rides, display live metrics, and be extended with new sensors or UI ideas, this repository is a good place to start.

<img width="1424" height="1196" alt="OBike v3 assembly" src="https://github.com/user-attachments/assets/fc615ce1-27c5-40f6-beea-987b0b69bac0" />

## What this project does

OBike is intended to capture and present ride data such as:

- speed and distance
- cadence and power
- heart rate
- GPS position and track history
- barometric altitude and sensor-derived data

It presents that information on a colour screen and can log sessions to microSD in FIT, TCX, or CSV formats.

It also supports common Bluetooth cycling peripherals that implement the CSC, HRM, and CPS profiles.

## What is in this repository

- `src/` — firmware source in C++
  - `src/App.*` — main application state and control flow
  - `src/HAL/` — hardware abstraction for GPS, SD, Bluetooth, buttons, and sensors
  - `src/ui/` — UI screens and widgets
  - `src/Loggers/` — FIT, TCX, and CSV logging implementations
  - `src/display/` — display driver and rendering wrappers
- `kicad/` — KiCad schematic and PCB layout files
- `3D_CAD/` — enclosure and mounting CAD files for 3D printing
- `sim/` — desktop simulator for trying the firmware logic without hardware
- `platformio.ini` — PlatformIO build configuration

## Hardware overview

The current hardware target is a compact embedded bike-computer build based on:

- MCU: Seeed Studio XIAO nRF52840 (Sense)
- GNSS: Quectel LC76G module
- Display: ST7789-based full-colour 240×320 SPI panel
- Sensors: RTC, barometric pressure sensor, IMU, and SD card storage
- IO: debounced physical controls and Bluetooth cycling-device support

## Quick start

### Build the firmware

1. Install PlatformIO.
2. From the repository root, run:

   ```bash
   platformio run
   ```

3. To upload to the target board:

   ```bash
   platformio run --target upload
   ```

### Try it without hardware

The repository includes a desktop simulator under `sim/` that can exercise much of the application logic and UI without needing the embedded board.

## Software features

- Firmware built with PlatformIO and the Arduino framework
- Telemetry aggregation from BLE sensors, GPS, and onboard sensors
- Logging to microSD in FIT, TCX, and CSV formats
- GPS control and NMEA handling for the LC76G module
- A simple UI for ride data and device interaction
- Bluetooth device discovery, pairing, and connection management

## Dependencies

The firmware depends on a number of Arduino and embedded libraries, including:

- `Adafruit_GFX`
- `Adafruit_ST7789`
- `RTClib`
- `MCP23017` support for the I/O expander
- `LSM6DS3`
- `XENSIV` pressure-sensor support
- `ArduinoJson`
- `TinyGPSPlus`

## Key files to inspect

- `src/HAL/LC76G.cpp` and `src/HAL/LC76G.hpp` — GNSS driver, NMEA handling, and command flow
- `src/display/Display.hpp` and `src/display/Display.cpp` — display wrapper and rendering layer
- `src/Loggers/TCXLogger.cpp` and `src/Loggers/CSVLogger.cpp` — ride logging implementations
- `src/App.cpp` — main application loop and state handling
- `kicad/` — schematic and PCB design files
- `3D_CAD/` — printable enclosure and mounting hardware

## Troubleshooting and debugging

- Serial logs are useful during development and can help trace GPS and device activity.
- Raw NMEA data can be captured to the SD card for debugging GPS behavior.
- The LC76G path has previously been sensitive to malformed or long NMEA streams, so keeping firmware updates current is important if you hit GPS issues.

## Notes for first-time visitors

This project is best viewed as a hands-on, open-source bike-computer platform rather than a polished consumer product. It is a strong fit if you want to learn embedded firmware, PCB design, logging formats, BLE peripherals, and how a compact cycling computer can be assembled from open parts.

If you are curious about building your own ride-data device, this repository is a practical starting point for both the hardware and software sides of the project.
