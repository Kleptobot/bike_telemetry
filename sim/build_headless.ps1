# Headless build replicating sim/Makefile's `headless` target with PowerShell.
# The Makefile's Unix-probing recipes (command -v g++, /dev/null redirection,
# rm) fail under Windows cmd/PowerShell shells, so this drives g++ directly.
$ErrorActionPreference = "Continue"

$root    = ".."
$libroot = Join-Path $root ".pio\libdeps\seeed-xiao-afruitnrf52-nrf52840-sense"
$gfx     = Join-Path $libroot "Adafruit GFX Library"
$ajson   = Join-Path $libroot "ArduinoJson\src"
$tinygps = Join-Path $libroot "TinyGPSPlus\src"

$cxx = "C:\Users\willn\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\g++.exe"

$flags = @(
    "-std=gnu++17",
    "-DARDUINO=10800",
    "-g",
    "-O1",
    "-Wall",
    "-Wextra",
    "-Wno-unused-parameter",
    "-Istubs",
    "-Ihost",
    "-I$root\src",
    "-I$gfx",
    "-I$ajson",
    "-I$tinygps"
)

$fw = @(
    "$root\src\App.cpp",
    "$root\src\main.cpp",
    "$root\src\display\Display.cpp",
    "$root\src\Fusion\AltitudeFusion.cpp",
    "$root\src\Fusion\FitnessFusion.cpp",
    "$root\src\Fusion\Fusion.cpp",
    "$root\src\Fusion\FtpEstimator.cpp",
    "$root\src\HAL\InputSystem.cpp",
    "$root\src\HAL\Sensors.cpp",
    "$root\src\HAL\SDCard.cpp",
    "$root\src\HAL\button.cpp",
    "$root\src\HAL\Bluetooth\BluetoothSystem.cpp",
    "$root\src\HAL\Bluetooth\BT_Device.cpp",
    "$root\src\HAL\Bluetooth\csc.cpp",
    "$root\src\HAL\Bluetooth\cps.cpp",
    "$root\src\HAL\Bluetooth\hrm.cpp",
    "$root\src\Loggers\FITLogger.cpp",
    "$root\src\Loggers\FITWriter.cpp",
    "$root\src\Loggers\TCXLogger.cpp",
    "$root\src\Loggers\CSVLogger.cpp",
    "$root\src\Map\TileLoader.cpp",
    "$root\src\ui\UIManager.cpp",
    "$root\src\ui\Screens\BikeStatsScreen.cpp",
    "$root\src\ui\Screens\BiometricsScreen.cpp",
    "$root\src\ui\Screens\DisplayEditScreen.cpp",
    "$root\src\ui\Screens\GPSScreen.cpp",
    "$root\src\ui\Screens\TimeEditScreen.cpp",
    "$root\src\ui\Widgets\DateWidget.cpp",
    "$root\src\ui\Widgets\DurationWidget.cpp",
    "$root\src\ui\Widgets\MapWidget.cpp",
    "$root\src\ui\Widgets\SelectableText.cpp",
    "$root\src\ui\Widgets\SelectableTextIcon.cpp",
    "$root\src\ui\Widgets\TimeWidget.cpp"
)

$stubs = @(
    "stubs\Arduino.cpp",
    "stubs\SdFat.cpp",
    "stubs\RTClib.cpp",
    "stubs\bluefruit.cpp",
    "stubs\Dps3xx.cpp",
    "stubs\Adafruit_ST7789.cpp",
    "stubs\Adafruit_MCP23X17.cpp"
)

$hostsrc = @("host\SimHAL.cpp", "host\SimLC76G.cpp", "host\main.cpp")

$lib = @(
    (Join-Path $gfx "Adafruit_GFX.cpp"),
    (Join-Path $tinygps "TinyGPS++.cpp")
)

if (-not (Test-Path build)) { New-Item -ItemType Directory build | Out-Null }

Write-Host "Compiling headless sim build..."
& $cxx $flags $fw $stubs $hostsrc $lib "-o" "build\obike-sim"
Write-Host "Exit: $LASTEXITCODE"