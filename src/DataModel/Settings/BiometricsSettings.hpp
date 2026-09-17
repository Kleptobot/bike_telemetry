#ifndef BIOMETRICSSETTINGS_HPP
#define BIOMETRICSSETTINGS_HPP

#include "ISettings.hpp"
#include "JsonFile.hpp"
#include "../BioDataProvider.hpp"

class BiometricsSettings : public ISettings {
public:
    BiometricsSettings(BioDataProvider& provider) : _bio(provider) {}

    void save(IStorage* storage) override {
        JsonDocument doc;
        auto& a = _bio.get();

        doc["birthday"] = a.birthday.unixtime();
        doc["mass"] = a.mass;
        doc["caloricProfile"] = toString(a.caloricProfile);
        doc["ftp"] = a.ftpWatts;
        doc["zone1Start"] = a.zone1Start;
        doc["zone2Start"] = a.zone2Start;
        doc["zone3Start"] = a.zone3Start;
        doc["zone4Start"] = a.zone4Start;
        doc["zone5Start"] = a.zone5Start;

        saveJson(storage, "/biometrics.txt", doc);
    }

    void load(IStorage* storage) override {
        JsonDocument doc;
        if (!loadJson(storage, "/biometrics.txt", doc))
            return;

        Serial.println("Found biometrics.txt");

        BioData a;
        long long unix = doc["birthday"];
        timeData bd(unix, 0);
        a.birthday = bd;
        a.mass = doc["mass"];
        a.caloricProfile = fromString(doc["caloricProfile"]);
        a.ftpWatts = doc["ftp"] | 200;   // fallback keeps old biometrics.txt files working
        a.zone1Start = doc["zone1Start"];
        a.zone2Start = doc["zone2Start"];
        a.zone3Start = doc["zone3Start"];
        a.zone4Start = doc["zone4Start"];
        a.zone5Start = doc["zone5Start"];

        _bio.update(a);
    }

private:
    BioDataProvider& _bio;
};

#endif /* BIOMETRICSSETTINGS_HPP */
