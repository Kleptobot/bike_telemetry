# OBike — Open Bicycle Telemetry Platform

OBike is an open-source hardware and firmware project for bicycle telemetry and logging. This repository contains everything needed to build, program, and enclose the device: firmware source code, PCB design and schematics, and 3D CAD files for a printable enclosure.

Purpose: capture bike telemetry (speed, cadence, power, heart rate, GPS tracks, barometric altitude and sensor data), present it on a colour display, and log sessions to an SD card in TCX/CSV formats.

Standard bike peripherals that connect via Bluetooth and implement the CSC, HRM and CPS profiles are compatible.

## Contents
 - `src/` — Firmware source (C++). Core subsystems in `src/HAL`, UI in `src/ui`, display driver in `src/display`, logging in `src/TCXLogger.cpp`.
 - `include/` — Public headers and shared definitions.
 - `kicad/` — KiCad schematic and PCB layout files (board, symbols, footprints).
 - `3D_CAD/` — STEP/STL and other CAD files for 3D-printing the enclosure and mounting hardware.
 - `lib/` — Project-specific libraries and footprints used by the PCB and build.
 - `devices.txt`, `platformio.ini` and other project metadata.

## Hardware overview
 - MCU: Seeed Studio XIAO nRF52840 (Sense) (nRF52840 Cortex-M4 BLE-capable MCU).
 - GNSS: Quectel LC76G module (handled by the `LC76G` driver in `src/HAL`).
 - Display: ST7789-based full-colour 240×320 SPI display (driver: `Adafruit_ST7789`, wrappers in `src/display`).
 - Sensors: DS3231M RTC, DPS368 barometric sensor, IMU (on-chip 6DOF), SD card for logging.
 - IO: 5-position momentary switch (hardware debounced, connected via I2C IO expander), Bluetooth peripheral support (CSC/HRM/CPS).

## Software & features
 - Firmware: PlatformIO-based build using the Arduino framework. Main application lives in `src/App.*`.
 - Telemetry aggregation: blends readings from BLE sensors (CSC, CPS, HRM), onboard sensors and GPS to estimate speed, cadence, power, altitude.
 - Logging: TCX (Garmin) and CSV track logging to microSD. Raw NMEA sentences are also optionally logged to `/gpsLog.txt` for debugging.
 - GPS control: routines to configure NMEA message rates and send PAIR/PQTMCFG commands to the LC76G.
 - UI: Simple app UI with screens in `src/ui/Screens` and widgets in `src/ui/Widgets`.
 - Bluetooth: device scanning, pairing and connection management implemented in `src/HAL/Bluetooth` and `src/Bluetooth` subsystems.

## Dependencies
 - PlatformIO project (see `platformio.ini`).
 - Arduino/Adafruit libraries required by the display and graphics: `Adafruit_GFX`, `Adafruit_ST7789`.
 - `TinyGPSPlus` for parsing NMEA sentences.

## Key files to inspect
 - `src/HAL/LC76G.cpp` / `src/HAL/LC76G.hpp` — low-level driver for the Quectel LC76G module, NMEA assembly and PAIR command handling.
 - `src/display/Display.hpp` / `src/display/Display.cpp` — ST7789 display wrapper and canvas handling (240×320 canvas, 16-bit colour buffer).
 - `src/TCXLogger.*` — TCX/CSV formatting and SD card logging.
 - `kicad/` — schematic (`.kicad_sch`) and PCB (`.kicad_pcb`) files.
 - `3D_CAD/` — STEP/STL files for the housing and mounting hardware.

## Troubleshooting & debugging
 - Serial logs: enable GPS debug via build-time `DebugConfig.hpp` flags to print raw NMEA and driver activity.
 - Raw NMEA capture: the LC76G driver writes raw NMEA sentences to `/gpsLog.txt` on the SD card. Check this file for GPS traffic.
 - Common problems seen during development:
	- NMEA assembly overflow: long or malformed NMEA streams previously caused buffer issues; ensure firmware is up-to-date with bounds checks in `LC76G`.
	- PAIR response handling: the LC76G PAIR command flow expects specific responses — check `COMMANDS[]` in `LC76G.hpp` and `HAL::handlePAIRResponse` if you see unexpected behavior.


## Appendix — repo map (high level)
 - `src/` — firmware implementation
 - `kicad/` — PCB and schematic files
 - `3D_CAD/` — mechanical CAD files