#ifndef TELEMETRYTYPE_H
#define TELEMETRYTYPE_H

#include <Arduino.h>

// The tile vocabulary: which quantity a layout cell displays. This is layout
// CONFIGURATION, not telemetry state -- it survives the removal of the
// Telemetry struct because screens/layouts/widgets still speak it.
//
// Adding a displayable signal:
//   1. append an enum member BEFORE Undefined
//   2. add one row to kTypeTable (name + units label)
// The cycle, string conversion and units all derive from that table.
enum class TelemetryType : uint8_t {
    // Derived (fusion / DataBus)
    Speed,          // selected speed        km/h
    Cadence,        // crank cadence         rpm
    Altitude,       // fused altitude        m
    BaroAlt,        // raw baro altitude     m
    Grade,          // smoothed grade        %
    Vario,          // vertical velocity     m/s
    GearRatio,      // wheel rev / crank rev x
    Energy,         // accumulated work      kJ
    Accel,          // forward acceleration  m/s2
    EstPower,       // physics-estimated power W
    Ascent,         // total ascent          m
    Descent,        // total descent         m
    // Measured
    Power,          // power meter           W
    HeartRate,      // HRM                   bpm
    Temperature,    // baro/RTC fold         C
    Distance,       // per-tick distance     m
    TotalDist,      // accumulated distance  km
    GpsSpeed,       // GPS ground speed      km/h
    GpsAlt,         // GPS altitude          m
    GpsCourse,      // GPS course            deg
    GpsSats,        // satellites used       (count)
    GpsHdop,        // horizontal DOP
    PedalBalance,   // left pedal balance    %
    Torque,         // pedal torque          Nm
    BatteryVolts,   // cell voltage          V
    // Special-cased (non-numeric rendering)
    Location,       // lat/lng text
    Coasting,       // PEDAL / COAST text
    Undefined
};

// One row per displayable signal: the wire name used in layout.txt and the
// unit label drawn in the tile's corner.
struct TelemetryTypeInfo {
    TelemetryType type;
    const char* name;   // layout.txt string / DisplayEditScreen display
    const char* units;  // tile corner label ("-" rendered as blank-ish)
};

// NOTE: keep in the same order as the enum body above (each row's type must
// match its enum ordinal's meaning, though the table is searched by value so
// order is actually free -- just keep rows aligned for readability).
static constexpr TelemetryTypeInfo kTypeTable[] = {
    { TelemetryType::Speed,         "Speed",        "km/h" },
    { TelemetryType::Cadence,       "Cadence",      "rpm"  },
    { TelemetryType::Altitude,      "Altitude",     "m  "  },
    { TelemetryType::BaroAlt,       "BaroAlt",      "m  "  },
    { TelemetryType::Grade,         "Grade",        "%  "  },
    { TelemetryType::Vario,         "Vario",        "m/s"  },
    { TelemetryType::GearRatio,     "GearRatio",    "x  "  },
    { TelemetryType::Energy,        "Energy",       "kJ "  },
    { TelemetryType::Accel,         "Accel",        "m/s2" },
    { TelemetryType::EstPower,      "EstPower",     "W~ "  },
    { TelemetryType::Ascent,        "Ascent",       "up "  },
    { TelemetryType::Descent,       "Descent",      "dn "  },
    { TelemetryType::Power,         "Power",        "W  "  },
    { TelemetryType::HeartRate,     "HeartRate",    "bpm"  },
    { TelemetryType::Temperature,   "Temperature",  "C  "  },  // deg sign cannot be rendered
    { TelemetryType::Distance,      "Distance",     "m  "  },
    { TelemetryType::TotalDist,     "TotalDist",    "km "  },
    { TelemetryType::GpsSpeed,      "GpsSpeed",     "km/h" },
    { TelemetryType::GpsAlt,        "GpsAlt",       "m  "  },
    { TelemetryType::GpsCourse,     "GpsCourse",    "deg"  },
    { TelemetryType::GpsSats,       "GpsSats",      "sat"  },
    { TelemetryType::GpsHdop,       "GpsHdop",      "dop"  },
    { TelemetryType::PedalBalance,  "PedalBalance", "%L "  },
    { TelemetryType::Torque,        "Torque",       "Nm "  },
    { TelemetryType::BatteryVolts,  "BattVolts",    "V  "  },
    { TelemetryType::Location,      "Location",     ""     },
    { TelemetryType::Coasting,      "Coasting",     ""     },
};

static constexpr size_t kTypeTableSize = sizeof(kTypeTable) / sizeof(kTypeTable[0]);

// Index of a type in the table, or kTypeTableSize if not displayable
// (Undefined and any future non-display member).
constexpr size_t typeIndex(TelemetryType t) {
    for (size_t i = 0; i < kTypeTableSize; i++) {
        if (kTypeTable[i].type == t) return i;
    }
    return kTypeTableSize;
}

// Cycle through the DISPLAYABLE signals (the table order, which reads as a
// sensible menu: derived block, measured block, specials last).
inline TelemetryType& operator++(TelemetryType& t) {
    const size_t i = typeIndex(t);
    t = (i >= kTypeTableSize) ? TelemetryType::Speed          // Undefined wraps to start
                              : kTypeTable[(i + 1) % kTypeTableSize].type;
    return t;
};

inline TelemetryType& operator--(TelemetryType& t) {
    const size_t i = typeIndex(t);
    t = (i >= kTypeTableSize) ? kTypeTable[kTypeTableSize - 1].type  // Undefined wraps to end
                              : kTypeTable[(i + kTypeTableSize - 1) % kTypeTableSize].type;
    return t;
};

inline const char* toString(const TelemetryType& t) {
    const size_t i = typeIndex(t);
    return (i < kTypeTableSize) ? kTypeTable[i].name : "-";
}

inline const char* unitsForType(const TelemetryType& t) {
    const size_t i = typeIndex(t);
    return (i < kTypeTableSize) ? kTypeTable[i].units : " - ";
}

inline TelemetryType TelemetryTypefromString(String s) {
    for (size_t i = 0; i < kTypeTableSize; i++) {
        if (s == kTypeTable[i].name) return kTypeTable[i].type;
    }
    return TelemetryType::Undefined;
}

#endif /* TELEMETRYTYPE_H */
