# OBike host simulator.
#
#   mingw32-make            build (SDL if available, otherwise headless-only)
#   mingw32-make headless   force the no-SDL build
#   mingw32-make run        build and launch
#   mingw32-make clean
#
# Deliberately links the SAME vendored library sources the firmware uses
# (Adafruit_GFX, ArduinoJson, TinyGPSPlus) out of .pio/libdeps rather than
# vendoring a second copy, so the simulator cannot drift from the device
# build. Run `pio run` once first to populate that directory.

CXX      ?= g++
ROOT     := ..
LIBDEPS  := $(ROOT)/.pio/libdeps/seeed-xiao-afruitnrf52-nrf52840-sense
GFX      := $(LIBDEPS)/Adafruit GFX Library
AJSON    := $(LIBDEPS)/ArduinoJson/src
TINYGPS  := $(LIBDEPS)/TinyGPSPlus/src

BUILD    := build
BIN      := $(BUILD)/obike-sim

INCLUDES := -Istubs -Ihost -I$(ROOT)/src -I"$(GFX)" -I"$(AJSON)" -I"$(TINYGPS)"
CXXFLAGS := -std=gnu++17 -DARDUINO=10800 -g -O1 -Wall -Wextra -Wno-unused-parameter $(INCLUDES)
LDFLAGS  :=

# --- SDL detection --------------------------------------------------------
# sdl2-config is not always present on Windows; fall back to a plain -lSDL2
# probe and drop to headless if that also fails.
SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL_LIBS   := $(shell sdl2-config --libs 2>/dev/null)
ifeq ($(strip $(SDL_LIBS)),)
  HAVE_SDL := $(shell echo "int main(){return 0;}" | $(CXX) -x c++ - -lSDL2main -lSDL2 -o /dev/null 2>/dev/null && echo yes)
  ifeq ($(HAVE_SDL),yes)
    SDL_LIBS := -lSDL2main -lSDL2
  endif
endif
ifneq ($(strip $(SDL_LIBS)),)
  CXXFLAGS += -DSIM_USE_SDL $(SDL_CFLAGS)
  LDFLAGS  += $(SDL_LIBS)
endif

# --- Sources --------------------------------------------------------------
# Firmware sources that compile unmodified on the host. Note what is ABSENT:
#   src/HAL/HAL.cpp     replaced by host/SimHAL.cpp
#   src/HAL/LC76G.cpp   I2C GNSS transport; the sim feeds NMEA directly
# Everything else -- including the CSC/CPS/HRM parsers and both loggers --
# is the real thing.
FW_SRCS := \
  $(ROOT)/src/App.cpp \
  $(ROOT)/src/main.cpp \
  $(ROOT)/src/display/Display.cpp \
  $(ROOT)/src/HAL/AltitudeFusion.cpp \
  $(ROOT)/src/HAL/InputSystem.cpp \
  $(ROOT)/src/HAL/Sensors.cpp \
  $(ROOT)/src/HAL/SDCard.cpp \
  $(ROOT)/src/HAL/button.cpp \
  $(ROOT)/src/HAL/Bluetooth/BluetoothSystem.cpp \
  $(ROOT)/src/HAL/Bluetooth/BT_Device.cpp \
  $(ROOT)/src/HAL/Bluetooth/csc.cpp \
  $(ROOT)/src/HAL/Bluetooth/cps.cpp \
  $(ROOT)/src/HAL/Bluetooth/hrm.cpp \
  $(ROOT)/src/Loggers/FITLogger.cpp \
  $(ROOT)/src/Loggers/FITWriter.cpp \
  $(ROOT)/src/Loggers/TCXLogger.cpp \
  $(ROOT)/src/Map/TileLoader.cpp \
  $(ROOT)/src/ui/UIManager.cpp \
  $(ROOT)/src/ui/Screens/BikeStatsScreen.cpp \
  $(ROOT)/src/ui/Screens/BiometricsScreen.cpp \
  $(ROOT)/src/ui/Screens/DisplayEditScreen.cpp \
  $(ROOT)/src/ui/Screens/GPSScreen.cpp \
  $(ROOT)/src/ui/Screens/TimeEditScreen.cpp \
  $(ROOT)/src/ui/Widgets/DateWidget.cpp \
  $(ROOT)/src/ui/Widgets/DurationWidget.cpp \
  $(ROOT)/src/ui/Widgets/MapWidget.cpp \
  $(ROOT)/src/ui/Widgets/SelectableText.cpp \
  $(ROOT)/src/ui/Widgets/SelectableTextIcon.cpp \
  $(ROOT)/src/ui/Widgets/TimeWidget.cpp

STUB_SRCS := \
  stubs/Arduino.cpp \
  stubs/SdFat.cpp \
  stubs/RTClib.cpp \
  stubs/bluefruit.cpp \
  stubs/Dps3xx.cpp \
  stubs/Adafruit_ST7789.cpp \
  stubs/Adafruit_MCP23X17.cpp

HOST_SRCS := host/SimHAL.cpp host/SimLC76G.cpp host/main.cpp

LIB_SRCS := "$(GFX)/Adafruit_GFX.cpp" "$(TINYGPS)/TinyGPS++.cpp"

# LIB_SRCS is kept OUT of the prerequisite list: those paths contain spaces
# ("Adafruit GFX Library"), which make cannot express as prerequisites. They
# are vendored and effectively immutable, so losing change-tracking on them
# costs nothing; they are still passed to the compiler in the recipe.
SRCS := $(FW_SRCS) $(STUB_SRCS) $(HOST_SRCS)

.PHONY: all headless run clean
all: $(BIN)

$(BIN): $(SRCS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LIB_SRCS) -o $(BIN) $(LDFLAGS)
	@echo "built $(BIN)"

headless:
	@$(MAKE) all SDL_LIBS= SDL_CFLAGS=

run: $(BIN)
	./$(BIN)

clean:
	rm -rf $(BUILD)
