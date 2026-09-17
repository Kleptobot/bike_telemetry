#ifndef JSONFILE_HPP
#define JSONFILE_HPP

#include <Arduino.h>
#include <ArduinoJson.h>
#include "HAL/StorageInterface.hpp"

// Shared boilerplate for JSON file persistence.
// Handles the open/remove/serialize/close pattern that was repeated
// across every save/load function in App.cpp.

inline bool saveJson(IStorage* storage, const char* path, JsonDocument& doc) {
    if (storage->exists(path))
        storage->remove(path);

    File32 file = storage->openFile(path, FILE_WRITE);
    if (!file) {
        Serial.print("[Settings] failed to open ");
        Serial.print(path);
        Serial.println(" for writing");
        return false;
    }

    serializeJson(doc, file);
    file.close();
    return true;
}

inline bool loadJson(IStorage* storage, const char* path, JsonDocument& doc) {
    if (!storage->exists(path))
        return false;

    File32 file = storage->openFile(path, FILE_READ);
    if (!file) {
        Serial.print("[Settings] failed to open ");
        Serial.print(path);
        Serial.println(" for reading");
        return false;
    }

    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.print("[Settings] deserializeJson() failed for ");
        Serial.print(path);
        Serial.print(": ");
        Serial.println(error.c_str());
        return false;
    }

    return true;
}

#endif /* JSONFILE_HPP */
