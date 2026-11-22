# AGENTS.md — repository guide for AI agents (and humans)

This file is the orientation map for this repo. Read it before making changes.
It indexes where things live and what the conventions are; it deliberately does
**not** duplicate the detailed docs — when a topic has its own README section,
this file links to it.

Project: **OBike** — an open DIY bike computer. Firmware in C++ (PlatformIO +
Arduino framework) for a Seeed XIAO nRF52840 Sense, plus KiCad hardware and
3D-printable enclosure files. See `Readme.md` for the visitor-facing overview.

---

## 1. Command cheat-sheet

| Task | Command |
|---|---|
| Build firmware | `platformio run` (repo root) |
| Flash firmware | `platformio run --target upload` |
| Static analysis | `pio check` (cppcheck; flags in `platformio.ini`) |
| Build/run simulator | `cd sim` then `.\run.bat` (Windows) or `./run.sh` (Git Bash/Linux/macOS) |
| Headless sim run | `.\run.bat --headless --frames 30 --out out` then `python tools\ppm2png.py out --scale 2` |

**Hard prerequisite for the simulator:** run `pio run` once in the repo root
first. The sim compiles against the vendored library sources in
`.pio/libdeps/seeed-xiao-afruitnrf52-nrf52840-sense` — if that directory is
missing, the sim build fails or links stale copies. Never vendor a second copy
of a library; the sim deliberately shares the firmware's. `LibYxml` is one of
these dependencies and supplies the streaming XML parser used for TCX summaries.

There is **no unit-test framework**. Tests are standalone smoke-test programs
under `sim/tests/` — each carries its own `g++` build command in a header
comment. Current tests cover `FitnessFusion`, `FtpEstimator`, and the FIT/TCX/CSV
log parsers. Build artifacts go in `sim/build/` (gitignored).

Full simulator reference (flags, keys, scenarios, injected BLE sensors):
**`sim/README.md`**. It is accurate and detailed; read it instead of guessing.

---

## 2. Repository map

| Path | What it is |
|---|---|
| `src/` | All firmware source. See §4. |
| `sim/` | Desktop simulator: builds most of the real firmware for the host. See §5 and `sim/README.md`. |
| `platformio.ini` | Firmware build config: board, C++17, build flags, library deps. |

| `tiles/` | Map-tile pipeline. `tiles/convert_to_rgb565.py` (tracked) converts PNG tiles to raw RGB565; the tile images themselves are gitignored. |
| `tiles_raw/` | Raw RGB565 slippy-map tiles (`z/x/y`, 256×256, 2 B/px) copied to the SD card as `/tiles/z/x/y.raw`. Gitignored. |
| `kicad/` | Schematic + PCB. `kicad/obike_schem.pdf` is the readable schematic; `OBike-backups/` are auto-backups. |
| `3D_CAD/` | Enclosure STLs/DXF and renders. |
| `lib/` | Empty PlatformIO placeholder (its README only). |
| `.pio/` | Gitignored PlatformIO build state + downloaded libs. The sim depends on `.pio/libdeps` existing. |
| `power use.ods` | Power-consumption spreadsheet (reference data, not code). |
| `.vscode/` | Recommended extension is `platformio.platformio-ide`. |

Gitignored working dirs you may see locally: `/tiles`, `/tiles_raw`,
`/sim/build`, `/sim/out`, `/sim/sdcard`, `/sim/third_party`, `.pio`.

---

## 3. Architecture in one page

```
main.cpp loop()
  ├─ HAL::inst().update()        acquire everything → MeasurementFrame
  └─ App::instance().update()    one app tick:
       ├─ updateTelemetry()      MeasurementFrame → FusionEngine → DataBus topics
       ├─ drain AppEvent queue   (start/stop logging, save settings, GPS cmds…)
       ├─ ui.update()/render()/handleInput()   (render throttled to ~10 Hz)
       ├─ auto-pause, idle-sleep, buzzer,
       │  ride timers, battery/SD alarms, auto-lap
       └─ AppState switch: BOOT → IDLE ⇄ LOGGING ⇄ PAUSED (CONFIG exists too)
```

Key contracts — both are enforced by house rules stated in their headers:

- **`src/HAL/Measurements.hpp` — `MeasurementFrame`.** One factual snapshot of
  everything *acquired* each HAL tick, with per-channel timestamps, sequence
  numbers, and observed sample intervals. Only raw facts go in; derived values
  (fused altitude, speed selection, grade, distance) must never live in HAL.
- **`src/Fusion/Fusion.hpp` — `DerivedChannels`.** The App-side interpretation
  stage. Consumes the frame, produces derived channels, which
  `App::updateTelemetry()` publishes onto the **DataBus**.

**`src/DataModel/DataBus.hpp`** is the pub/sub hub for derived signals
(fixed-size slots, no allocation after setup, synchronous dispatch, polling
via `get<T>()`). The `Topic` enum there is the canonical list of every derived
signal in the system — consult it before adding a new metric.

**`src/DataModel/DataModel.hpp`** aggregates the persistent providers
(`time()`, `bio()`, `bike()`, `layout()`, `logger()`, `SD()`, `bluetooth()`)
plus `bus()` and `gpsTrack()`. Settings JSON files (`layout.txt`, `bikeStats.txt`,
`biometrics.txt`, `time.txt`) are loaded/saved by `App.cpp` through `IStorage` +
ArduinoJson — see §6.

**Events** (`src/AppEvents.hpp`, `src/UIEvents.hpp`, `src/UIEventBus.hpp`):
screens never call App directly — they post `AppEvent`s (commands: SaveTime,
StartLogging, PauseLogging, NewLap, UnmountSD, GPS commands…) or `UIEvent`s
(navigation/popups). `App` implements `UIEventBus` and drains its queue each
tick in `App::handleAppEvent()`.


---

## 4. `src/` — where things are

| Path | Role |
|---|---|
| `main.cpp` | Arduino `setup()`/`loop()`. Thin: init HAL, begin App, update both, deep-sleep when GPS-enable pin says so. Debug toggles at the top (`DebugConfig.hpp`). |
| `App.cpp` / `App.hpp` | **The application.** Singleton (`App::instance()`). Screen registration (`App::begin`), state machine, event handling, auto-pause (3/5 km/h hysteresis), idle auto-sleep, ride timers, auto-lap, buzzer patterns, battery/SD alarms, FTP estimator hookup. Start reading here after this file. |
| `HAL/HAL.*` | Singleton (`HAL::inst()`). Owns all subsystems, builds the `MeasurementFrame` (`refreshFrame()` is inline in `HAL.hpp` so the sim shares it). |
| `HAL/LC76G.*` | Quectel LC76G GNSS driver: NMEA handling, command/PAIR protocol, NMEA-rate config. Historically sensitive to malformed/long NMEA streams. |
| `HAL/Sensors.*` | Onboard sensors (baro Dps3xx, IMU LSM6DS3, RTC, battery/charger). |
| `HAL/InputSystem.*`, `HAL/button.*` | Physical buttons via MCP23017 expander: edge/hold/repeat logic, outputs (screen backlight, aux rail). |
| `HAL/SDCard.*`, `HAL/StorageInterface.hpp` | SD mount/unmount + the **`IStorage`** interface the sim replaces. Settings/log file access flows through this. |

| `HAL/Bluetooth/` | BLE central: `BluetoothSystem` (scan/pair/mode), `BT_Device`, and measurement parsers `csc.*` (speed+cadence), `cps.*` (power), `hrm.*` (heart rate). `SpscRing.hpp` is the lock-free ring used at the BLE boundary. |
| `HAL/I2CArbiter.hpp` | I2C bus-sharing policy between subsystems. |
| `HAL/Measurements.hpp` | `MeasurementFrame` contract — see §3. |
| `Fusion/` | `Fusion.*` (engine: altitude fusion, speed-source selection, grade, distance), `AltitudeFusion.*` (baro+GPS bias filter), `FitnessFusion.*` (calories, zones, NP/TSS), `FtpEstimator.*` (best 20-min avg × 0.95). |
| `Loggers/` | `ILogger.hpp` interface; `FITLogger`/`FITWriter` (binary FIT), `TCXLogger` (XML), `CSVLogger`, and format-specific `FitLogParser`, `TcxLogParser`, and `CsvLogParser` modules for saved-log summaries. These parsers stream input and populate ride metrics through the small shared `LogParser` interface. New CSV logs end with a versioned summary row for duration, distance, and lap count; older CSV logs are still read for metrics present in their trackpoint rows. |
| `DataModel/` | Providers + `DataBus` + `GpsTrack` (breadcrumb store for the map) — see §3. |
| `Map/TileLoader.*` | Slippy-tile math + loading `/tiles/z/x/y.raw` RGB565 tiles from SD onto the canvas. |
| `display/Display.*` | ST7789 wrapper: init, dirty-rect flush to hardware. |
| `ui/UIManager.*` | Screen registry (`registerScreen<>()`, `showScreen()`), render/update/input dispatch, UI-event handling. |
| `ui/Screens/` | One class per screen; IDs in `ScreenID.hpp` (None, MainMenu, Bluetooth, SettingsMenu, Biometrics, GPSSettings, TimeMenu, DisplayEdit, UnmountSD, BikeStats). Header-only screens exist (e.g. `MainScreen.hpp`, `BluetoothScreen.hpp`). |
| `ui/Widgets/` | Reusable widgets: `BigDataWidget`/`SmallDataWidget` (tile metrics), `MapWidget`, `SelectableText*`/`SelectableTextIcon`, `ListView`, `TimeWidget`/`DateWidget`/`DurationWidget`, `BatteryWidget`, `BluttoothDeviceWidget` (sic), `CursorWidget`, `DisplayEditWidget`, `GridOverlayWidget`, `IconWidget`, `InputHints`, `Widget.hpp`, etc. |
| `ui/GFX.h` | Adafruit_GFX glue + shared canvas helpers. |
| `art/` | Bitmap/PNG assets embedded in firmware. |
| `MAC.hpp` | Misc small utilities. |


---

## 5. `sim/` — the desktop simulator

Only **two** firmware translation units are replaced at link time:
`src/HAL/HAL.cpp` → `host/SimHAL.cpp`, and `src/HAL/LC76G.cpp` →
`host/SimLC76G.cpp`. Everything else — App, DataModel, Fusion, Loggers, UI,
BLE parsers — compiles from the real sources. See `sim/README.md` for the full
story. Orientation:

| Path | Role |
|---|---|
| `host/main.cpp` | Host frontend: SDL window (optional) or headless frame dump; CLI flags (`--headless`, `--frames`, `--sensors`, `--stats`, `--screen`, `--sd`, `--last`, …). |
| `host/SimHAL.*`, `host/SimLC76G.*` | Simulated sensors and NMEA generation (real NMEA sentences through the real TinyGPSPlus). |
| `stubs/` | Arduino/hardware stubs (`Arduino.h`, `SdFat`, `bluefruit`, `Wire`/`SPI`, sensor stubs…). This is the substrate any new host-side test should reuse. |
| `tests/` | Smoke tests: `FitnessFusion_smoke`, `FtpEstimator_smoke`, `FitLogParser_smoke`, `TcxLogParser_smoke`, and `CsvLogParser_smoke`. Build command is in each file's header comment; run from `sim/`. The FIT test generates a fixture with `FitWriter`; the TCX test reads a fixture from `sdcard/`; the CSV test exercises generated and legacy formats. |
| `tools/` | `ppm2png.py` (frame conversion, stdlib-only), `get-sdl2.bat`/`.sh` (optional SDL2 SDK for the interactive window). |
| `Makefile` | The sim build. **Must be kept in sync when adding a source file — see §7.** `run.bat`/`run.sh` wrap it; `build_headless.ps1` is a direct-g++ fallback for shells where the Makefile's Unix probing fails. |
| `sample-sd/` | Seed config copied into gitignored `sdcard/` on first run: `layout.txt`, `bikeStats.txt` (wheel circumference — without it wheel speed is exactly 0), `biometrics.txt`, `time.txt`. `devices.txt` (paired-sensor fixture) is seeded at runtime. |
| `sdcard/` | The live simulated SD card (gitignored). Settings edits made in the sim persist here; a simulated ride writes a genuine `.fit` file. |

---

## 6. SD-card file formats (settings)

Settings are JSON-ish text files at the SD root, loaded and saved directly by
`App.cpp` through `IStorage` + ArduinoJson (there is no `Settings/` directory,
no `ISettings`, and no `JsonFile.hpp`):

| File | Loader / owner | Contents |
|---|---|---|
| `layout.txt` | `App::loadLayout()`/`saveLayout()` (`LayoutDataProvider`) | Tile grid layout for the stats screen. |
| `bikeStats.txt` | `App::loadBikeStats()`/`saveBikeStats()` (`BikeDataProvider`) | Wheel circumference (mm), bike mass, logger format selection, idle-sleep minutes, auto-pause, auto-lap. |
| `biometrics.txt` | `App::loadBiometrics()`/`saveBiometrics()` (`BioDataProvider`) | Rider mass, birthday, HR zones, FTP, caloric profile. |
| `time.txt` | `App::loadTime()`/`saveTime()` (`TimeDataProvider`) | UTC offset and time prefs. |
| `devices.txt` | `BluetoothSystem::loadDevices()`/`saveDevices()` | Paired BLE sensors (not a Settings class). |

Ride logs are written by the active logger (`FIT`/`TCX`/`CSV`, chosen in bike
settings) under the SD root.


---

## 7. Conventions & change-checklists

**House style**
- Verbose explanatory comments are the norm, especially at HAL/App boundaries
  and in tricky parsers. New code should match: say *why*, not just *what*.
- C++17 (`-std=gnu++17`), `-Wall -Wextra` (unused-parameter suppressed).
  `pio check` runs cppcheck with warning/style/performance enabled.
- Singletons: `App::instance()`, `HAL::inst()`. Screens receive `DataModel&`.
- No dynamic allocation after setup in hot paths — the DataBus, screens, and
  logger buffers are all fixed-size by design (232 KB RAM target).

**Checklists (things that silently break otherwise)**
- **Adding a `.cpp` to `src/` that the sim needs → add it to BOTH source lists:**
  `sim/Makefile` (`FW_SRCS`) and `sim/build_headless.ps1` (its `$fw` array).
  A missing file breaks one build path only, which is how drift happens.
- **Changing simulator library dependencies → keep both build paths aligned:**
  add required include paths and source files from `.pio/libdeps` to both
  `sim/Makefile` and `sim/build_headless.ps1`. The TCX parser specifically
  requires `LibYxml` (`yxml.h` and `yxml.c`).
- **Adding a screen →** add the enum to `ui/Screens/ScreenID.hpp` *and* register
  it in `App::begin()` (`ui.registerScreen<T>(…)`). `showScreen()` on an
  unregistered ID logs and keeps the current screen — IDs are not contiguous.
- **Adding a derived metric →** add a `Topic` to `DataBus.hpp`, publish from
  `App::updateTelemetry()` (or the App tick), consume via `bus.get<T>()` or
  `subscribe()`. Don't invent a side channel.
- **New acquired fact →** add a field to `MeasurementFrame`
  (`HAL/Measurements.hpp`) and fill it in `HAL::refreshFrame()` — never derive
  inside HAL, never bypass the frame with a bespoke getter.
- **`SERIAL_BUFFER_SIZE=256` in `platformio.ini` is load-bearing** — the nRF52
  Wire DMA path overflows the 64-byte default when the LC76G driver reads
  128-byte NMEA chunks. Don't lower it without re-reading that comment.
- **New BLE sensor profile →** follow the `csc`/`cps`/`hrm` parser pattern
  (`src/HAL/Bluetooth/`), publish results through `HAL::refreshFrame()`, and
  add an injector in `host/SimHAL.cpp` so it's testable without hardware.

**Validation loop for firmware-visible changes**
1. `platformio run` must compile.
2. Build and run the sim:
   `cd sim && .\run.bat --headless --frames 300 --sensors --stats --out out`.
   Visual check via `tools\ppm2png.py`, numeric check via `--stats`.
3. For smoke-test work, build the relevant `sim/tests/*_smoke.cpp` binary with
   the `g++` command in its header comment and run it from `sim/`.

