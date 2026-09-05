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

- all of `App`, `DataModel`, `Loggers`, `Map`, `Fusion` and `ui`
- `InputSystem` — including the real `button` edge/hold/repeat logic
- `Sensors`, `SDCard`, and the whole `Fusion` layer (altitude fusion, speed
  selection, grade, distance) — the simulated barometer is driven from the
  scenario altitude, so the real estimator computes what the tiles show
- the entire Bluetooth stack, **including the CSC/CPS/HRM measurement parsers**

The vendored libraries (`Adafruit_GFX`, `ArduinoJson`, `TinyGPSPlus`) are linked
straight out of `.pio/libdeps`, not copied, so the simulator cannot drift from
the firmware build.

GPS is driven by generating **real NMEA sentences** (with correct checksums) and
feeding them through the real `TinyGPSPlus`, so fix acquisition, speed and
altitude age all exercise the production code path.

## Quick start

One prerequisite: run `pio run` in the repo root once, so `.pio/libdeps` exists
— the simulator links the same library sources the firmware does.

**Windows** (cmd.exe, PowerShell, or just double-click `run.bat`):

```
cd sim
.
un.bat --headless --frames 30 --out out
python tools\ppm2png.py out --scale 2
```

Note the leading `.\` — PowerShell does not run programs from the current
directory without it. In cmd.exe plain `run.bat` is fine.

**Git Bash / Linux / macOS:**

```
cd sim
./run.sh --headless --frames 30 --out out
python3 tools/ppm2png.py out --scale 2
```

Then open `out/frame_0029.png`.

Both launchers build if needed and locate the compiler themselves, so nothing
has to be added to PATH. If you would rather drive make directly:

```
mingw32-make            # SDL window if SDL2 is available
mingw32-make headless   # force the no-SDL build
mingw32-make run
```

Note the Makefile recipes use `mkdir -p` and `rm -rf`; `run.bat` puts Git for
Windows' `usrin` on PATH so those exist under cmd.exe.

## Run

```
run.bat                                     # interactive window (needs SDL2)
run.bat --headless --frames 200 --out out   # frame dump, no dependencies
```

Frames are RGB565 PPM, which most image viewers will not open.
`tools/ppm2png.py` converts them using only `zlib` and `struct` from the
standard library, so no Pillow install is needed. `--gif out/ride.gif` also
writes an animation — that one path does want Pillow.

## Toolchain

If the launcher reports no compiler:

```
winget install BrechtSanders.WinLibs.POSIX.UCRT
```

## SDL2 (optional, for the interactive window)

```
tools\get-sdl2.bat          # Windows
tools/get-sdl2.sh           # Git Bash / Linux / macOS
```

Downloads the SDL2 MinGW SDK into `third_party/` and drops the 32-bit half,
leaving about 60 MB. **`third_party/` is gitignored** — it is a platform
specific binary SDK, and the repo links third-party sources rather than
vendoring them, the same way `Adafruit_GFX` comes from `.pio/libdeps`.

Without SDL2 the build falls back to headless automatically and still runs and
writes frames; only the window and keyboard input are missing. `SDL2.dll` is
copied next to the binary at build time, so nothing needs to be on PATH.

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

## Sample card contents

`sample-sd/` holds a starting configuration, copied into `sdcard/` on first run
(never overwriting an existing one, so anything you change in the simulator
survives):

| File | What it gives you |
|---|---|
| `layout.txt` | a 2x3 tile grid: full-width Speed, then Cadence/HeartRate and Power/TotalDist |
| `bikeStats.txt` | 2105 mm wheel circumference -- without this the wheel-speed path computes exactly 0 |
| `biometrics.txt` | mass and HR zones, so the heart-rate tile colours correctly |
| `time.txt` | UTC offset 0 |

To start clean, delete `sdcard/` and run again. To keep several
configurations, copy them elsewhere and use `--sd DIR`.

## The SD card is a real directory

`./sdcard/` backs `IStorage`, so `layout.txt`, `biometrics.txt` and
`devices.txt` behave exactly as on the device — and a simulated ride writes a
genuine `.fit` file you can open in Garmin Connect, Strava or the FIT SDK.
That is a considerably stronger check on the logger than any assertion.

## Injecting BLE measurements

`Sim::buildCpsMeasurement()` and friends construct spec-shaped payloads;
`Sim::injectCpsNotification()` hands them to the real parser. Useful for
checking power/cadence decoding without a sensor.

## Simulated sensors

The sim can run the whole sensor story -- pairing, discovery, notification
decoding, baselining and the RPM smoothing filters -- with no radio:

- At init the card is seeded with a `devices.txt` fixture (three sensors:
  wheel, power, heart rate) unless one is already present, so the firmware's
  real `loadDevices()` path creates the parser devices at App BOOT.
- ~1.5 s of simulated time in, each device's real `discover()` runs against
  the stub radio and the stub connection handles are aligned.
- `HAL::update()` then feeds spec-shaped notifications into the parsers at
  the ~1 Hz cadence real sensors use, whenever the scenario says a sensor is
  present (`wheelRPMLive`, `power > 0`, `heartRate > 0`).

`--sensors` starts a headless run with all three sensors live at riding
values (24 km/h, 85 rpm, 180 W, 132 bpm). Interactively, `]` brings the
wheel sensor up as before. Values reaching the UI, the fusion engine and the
loggers have been decoded by the unmodified firmware parsers.

Note the ramp-in: the CSC parsers low-pass their revolution-derived estimates
with a ~1000-update time constant, so wheel speed and cadence climb toward
the scenario values over roughly 20-30 s of simulated time -- the same
behaviour you see on hardware after pairing. Power and heart rate decode
immediately. For a converged headless run use a longer frame count, e.g.
`--headless --sensors --frames 4000`.

## Headless navigation and telemetry

Headless runs can also navigate the UI and report what the app published:

```
./build/obike-sim --headless --sensors --stats --frames 1200 --last 2 \
                  --screen BikeStats --out out
```

- `--sensors` starts the run with all three sensors paired and live
- `--stats` prints the published telemetry every 250 frames -- a numeric
  trace of exactly what the app derived from the injected data
- `--screen NAME` navigates once boot completes (BikeStats, Settings,
  Bluetooth, Biometrics, GPS, Time, Display, UnmountSD, MainMenu)
- `--last N` writes only the final N PPM frames of the run

## Relationship to unit tests

The `stubs/` directory is the same substrate a native test build needs. A test
target reuses `stubs/` and the host `IStorage`, drops `host/main.cpp` and the
UI sources, and adds a test runner. The expensive part — the Arduino and
hardware stubs — is already done.
