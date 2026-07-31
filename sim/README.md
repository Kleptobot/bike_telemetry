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
