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

## Quick start

One prerequisite: run `pio run` once in the repo root so `.pio/libdeps` exists
(the simulator links the same library sources the firmware does).

From Git Bash:

```
cd sim
./run.sh --headless --frames 30 --out out      # build + run, writes 30 frames
python tools/ppm2png.py out --scale 2          # frames -> viewable PNGs
```

`run.sh` builds if needed and finds the MinGW-w64 compiler automatically, so
nothing has to be added to PATH. The Makefile does the same, if you prefer:

```
mingw32-make            # SDL window if SDL2 is available
mingw32-make headless   # force the no-SDL build
mingw32-make run
```

## Run

```
./run.sh                                       # interactive window (needs SDL2)
./run.sh --headless --frames 200 --out out     # frame dump, no dependencies
```

Frames are RGB565 PPM. `tools/ppm2png.py` converts them using only the Python
standard library; pass `--gif out/ride.gif` for an animation (that part needs
Pillow).

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
