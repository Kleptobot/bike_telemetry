#ifndef BIKESETTINGS_HPP
#define BIKESETTINGS_HPP

#include "ISettings.hpp"
#include "JsonFile.hpp"
#include "../BikeDataProvider.hpp"

class BikeSettings : public ISettings {
public:
    BikeSettings(BikeDataProvider& provider) : _bike(provider) {}

    void save(IStorage* storage) override {
        JsonDocument doc;
        auto& a = _bike.get();

        doc["mass"] = a.mass;
        doc["wheelCircumference"] = a.wheelCircumference;
        doc["logger"] = loggerToString(a.logger);
        doc["autoPause"] = a.autoPause;
        doc["idleSleepMinutes"] = a.idleSleepMinutes;
        doc["autoLapMode"] = autoLapModeToString(a.autoLapMode);
        doc["autoLapValue"] = a.autoLapValue;

        saveJson(storage, "/bikeStats.txt", doc);
    }

    void load(IStorage* storage) override {
        JsonDocument doc;
        if (!loadJson(storage, "/bikeStats.txt", doc))
            return;

        Serial.println("Found bikeStats.txt");

        BikeData a;
        a.mass = doc["mass"] | 10;
        a.wheelCircumference = doc["wheelCircumference"] | 2105;
        a.logger = loggerFromString(doc["logger"] | "FIT");
        a.autoPause = doc["autoPause"] | true;
        a.idleSleepMinutes = doc["idleSleepMinutes"] | 5;
        a.autoLapMode = autoLapModeFromString(doc["autoLapMode"] | "Off");
        a.autoLapValue = doc["autoLapValue"] | 10;

        _bike.update(a);
    }

private:
    BikeDataProvider& _bike;
};

#endif /* BIKESETTINGS_HPP */
