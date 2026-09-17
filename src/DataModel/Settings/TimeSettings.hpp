#ifndef TIMESETTINGS_HPP
#define TIMESETTINGS_HPP

#include "ISettings.hpp"
#include "JsonFile.hpp"
#include "../TimeDataProvider.hpp"

class TimeSettings : public ISettings {
public:
    TimeSettings(TimeDataProvider& provider) : _time(provider) {}

    void save(IStorage* storage) override {
        JsonDocument doc;
        int utcOffset = _time.get().offset();
        doc["UTCOffset"] = utcOffset;

        saveJson(storage, "/time.txt", doc);
    }

    void load(IStorage* storage) override {
        JsonDocument doc;
        if (!loadJson(storage, "/time.txt", doc))
            return;

        Serial.println("Found time.txt");

        int UTCOffset = doc["UTCOffset"];
        _time.setUTCOffset(UTCOffset);
    }

private:
    TimeDataProvider& _time;
};

#endif /* TIMESETTINGS_HPP */
