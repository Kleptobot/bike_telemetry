# OBike simulator

Runs the OBike firmware on a desktop. The UI you see is drawn by the real
`Adafruit_GFX` through the real widgets — not a mock-up — so it can be used
both for UI development and for exercising application logic.

## What is real and what is simulated

Only **two** firmware translation units are replaced:

| Replaced | Why |
|---|---|
| `src/HAL/HAL.cpp` → `host/SimHAL.cpp` | supplies simulated sensor values |
| `src/HAL/LC76G.cpp` → `host/SimLC76G.cpp` | I2C GNSS transport has no meaning on a host |

Everything else is compiled from the real source and runs unmodified:

- all of `App`, `DataModel`, `Loggers`, `Map` and `ui`
- `InputSystem` — including the real `button` edge/hold/repeat logic
- `Sensors`, `SDCard`, `AltitudeFusion`
- the entire Bluetooth stack, **including the CSC/CPS/HRM measurement parsers**

The vendored libraries (`Adafruit_GFX`, `ArduinoJson`, `TinyGPSPlus`) are linked
straight out of `.pio/libdeps`, not copied, so the simulator cannot drift from
the firmware build.

GPS is driven by generating **real NMEA sentences** (with correct checksums) and
feeding them through the real `TinyGPSPlus`, so fix acquisition, speed and
altitude age all exercise the production code path.

## Build

Needs a host g++ and one `pio run` beforehand to populate `.pio/libdeps`.

```
cd sim
mingw32-make            # SDL window if SDL2 is available
mingw32-make headless   # force the no-SDL build
```

SDL2 is optional. Without it the simulator still runs and writes frames.

## Run

```
./build/obike-sim                              # interactive window
./build/obike-sim --headless --frames 200 --out out
```

| Flag | Meaning |
|---|---|
| `--headless` | no window |
| `--frames N` | headless frame count |
| `--step MS` | simulated milliseconds per frame |
| `--out DIR` | write RGB565 frames as PPM |
| `--scale N` | SDL window scale |
| `--sd DIR` | SD card root (default `./sdcard`) |

Keys: arrows = D-pad, Enter/Space = select, `S` toggles the SD card, `G` toggles
the GPS fix, `[` / `]` change speed, Esc quits.

## Simulated time

`millis()` is a counter the frontend advances, not the wall clock. A headless
run is therefore fully deterministic and can be stepped in a debugger, and a
scenario can run faster than real time. `delay()` advances the counter rather
than sleeping.

## The SD card is a real directory

`./sdcard/` backs `IStorage`, so `layout.txt`, `biometrics.txt` and
`devices.txt` behave exactly as on the device — and a simulated ride writes a
genuine `.fit` file you can open in Garmin Connect, Strava or the FIT SDK.
That is a considerably stronger check on the logger than any assertion.

## Injecting BLE measurements

`Sim::buildCpsMeasurement()` and friends construct spec-shaped payloads;
`Sim::injectCpsNotification()` hands them to the real parser. Useful for
checking power/cadence decoding without a sensor.

## Relationship to unit tests

The `stubs/` directory is the same substrate a native test build needs. A test
target reuses `stubs/` and the host `IStorage`, drops `host/main.cpp` and the
UI sources, and adds a test runner. The expensive part — the Arduino and
hardware stubs — is already done.
