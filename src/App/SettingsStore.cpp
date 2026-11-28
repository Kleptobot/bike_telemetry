#include "SettingsStore.hpp"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <cstdio>

namespace {
constexpr size_t SETTINGS_PATH_CAPACITY = 64;

bool makeSettingsSiblingPath(const char* path, const char* suffix,
                             char* out, size_t outSize) {
    const int length = snprintf(out, outSize, "%s%s", path, suffix);
    if (length < 0 || static_cast<size_t>(length) >= outSize) {
        Serial.printf("[SettingsStore] settings path is too long: %s\n", path);
        return false;
    }
    return true;
}

bool recoverSettingsFile(IStorage& storage, const char* path,
                         bool preparingForWrite) {
    char tempPath[SETTINGS_PATH_CAPACITY];
    char backupPath[SETTINGS_PATH_CAPACITY];
    if (!makeSettingsSiblingPath(path, ".tmp", tempPath, sizeof(tempPath)) ||
        !makeSettingsSiblingPath(path, ".bak", backupPath, sizeof(backupPath))) {
        return false;
    }

    if (!storage.exists(path) && storage.exists(backupPath) &&
        !storage.rename(backupPath, path)) {
        Serial.printf("[SettingsStore] failed to restore backup for %s\n", path);
        return false;
    }

    if (storage.exists(path) && storage.exists(backupPath) &&
        !storage.remove(backupPath)) {
        Serial.printf("[SettingsStore] failed to remove stale backup for %s\n", path);
        if (preparingForWrite) return false;
    }

    if (preparingForWrite && storage.exists(tempPath) &&
        !storage.remove(tempPath)) {
        Serial.printf("[SettingsStore] failed to remove incomplete temporary file for %s\n",
                      path);
        return false;
    }
    return true;
}

void removeTemporaryFile(IStorage& storage, const char* tempPath) {
    if (storage.exists(tempPath) && !storage.remove(tempPath)) {
        Serial.printf("[SettingsStore] failed to remove temporary file %s\n", tempPath);
    }
}

bool writeJsonWithBackup(IStorage& storage, const char* path,
                         JsonDocument& document) {
    char tempPath[SETTINGS_PATH_CAPACITY];
    char backupPath[SETTINGS_PATH_CAPACITY];
    if (!makeSettingsSiblingPath(path, ".tmp", tempPath, sizeof(tempPath)) ||
        !makeSettingsSiblingPath(path, ".bak", backupPath, sizeof(backupPath)) ||
        !recoverSettingsFile(storage, path, true)) {
        return false;
    }

    File32 dataFile = storage.openFile(tempPath, FILE_WRITE);
    if (!dataFile) {
        Serial.printf("[SettingsStore] failed to open %s for writing\n", tempPath);
        removeTemporaryFile(storage, tempPath);
        return false;
    }

    const size_t expected = measureJson(document);
    const size_t written = serializeJson(document, dataFile);
    const bool synced = dataFile.sync();
    const bool writeError = dataFile.getWriteError();
    dataFile.close();
    if (written != expected || !synced || writeError) {
        Serial.printf("[SettingsStore] failed to write complete settings file %s\n", path);
        removeTemporaryFile(storage, tempPath);
        return false;
    }

    // The serialized byte count and sync result verify this generated JSON
    // without allocating a second document, which is costly for layouts.
    const bool hadOriginal = storage.exists(path);
    if (hadOriginal && !storage.rename(path, backupPath)) {
        Serial.printf("[SettingsStore] failed to preserve existing settings file %s\n", path);
        removeTemporaryFile(storage, tempPath);
        return false;
    }

    if (!storage.rename(tempPath, path)) {
        Serial.printf("[SettingsStore] failed to install new settings file %s\n", path);
        if (hadOriginal && !storage.rename(backupPath, path)) {
            Serial.printf("[SettingsStore] failed to restore existing settings file %s\n",
                          path);
        }
        removeTemporaryFile(storage, tempPath);
        return false;
    }

    if (hadOriginal && storage.exists(backupPath) && !storage.remove(backupPath)) {
        Serial.printf("[SettingsStore] saved %s but could not remove its backup\n", path);
    }
    return true;
}
}

void SettingsStore::begin(IStorage* storage, DataModel* model) {
    _storage = storage;
    _model = model;
}

void SettingsStore::saveBiometrics() {
    if (!_storage || !_model) return;

    JsonDocument doc;
    const BioData& a = _model->bio().get();

    doc["birthday"] = a.birthday.unixtime();
    doc["mass"] = a.mass;
    doc["caloricProfile"] = toString(a.caloricProfile);
    doc["ftp"] = a.ftpWatts;
    doc["zone1Start"] = a.zone1Start;
    doc["zone2Start"] = a.zone2Start;
    doc["zone3Start"] = a.zone3Start;
    doc["zone4Start"] = a.zone4Start;
    doc["zone5Start"] = a.zone5Start;

    writeJsonWithBackup(*_storage, "/biometrics.txt", doc);
}

void SettingsStore::loadBiometrics() {
    if (!_storage || !_model) return;
    if (!recoverSettingsFile(*_storage, "/biometrics.txt", false)) return;

    if (_storage->exists("/biometrics.txt")) {
        Serial.println("Found biometrics.txt");
        File32 dataFile = _storage->openFile("/biometrics.txt", FILE_READ);
        if (!dataFile) return;

        JsonDocument jsonBuffer;
        DeserializationError error = deserializeJson(jsonBuffer, dataFile);
        if (error) {
            Serial.print("[SettingsStore] deserializeJson() failed for /biometrics.txt: ");
            Serial.println(error.c_str());
            dataFile.close();
            return;
        }

        BioData a;
        long long unix = jsonBuffer["birthday"];
        a.birthday = timeData(unix, 0);
        a.mass = jsonBuffer["mass"];
        a.caloricProfile = fromString(jsonBuffer["caloricProfile"]);
        a.ftpWatts = jsonBuffer["ftp"] | 200;
        a.zone1Start = jsonBuffer["zone1Start"];
        a.zone2Start = jsonBuffer["zone2Start"];
        a.zone3Start = jsonBuffer["zone3Start"];
        a.zone4Start = jsonBuffer["zone4Start"];
        a.zone5Start = jsonBuffer["zone5Start"];

        _model->bio().update(a);
        dataFile.close();
    }
}

void SettingsStore::saveBikeStats() {
    if (!_storage || !_model) return;

    JsonDocument doc;
    const BikeData& a = _model->bike().get();

    doc["mass"] = a.mass;
    doc["wheelCircumference"] = a.wheelCircumference;
    doc["logger"] = loggerToString(a.logger);
    doc["autoPause"] = a.autoPause;
    doc["idleSleepMinutes"] = a.idleSleepMinutes;
    doc["autoLapMode"] = autoLapModeToString(a.autoLapMode);
    doc["autoLapValue"] = a.autoLapValue;

    writeJsonWithBackup(*_storage, "/bikeStats.txt", doc);
}

void SettingsStore::loadBikeStats() {
    if (!_storage || !_model) return;
    if (!recoverSettingsFile(*_storage, "/bikeStats.txt", false)) return;

    if (_storage->exists("/bikeStats.txt")) {
        Serial.println("Found bikeStats.txt");
        File32 dataFile = _storage->openFile("/bikeStats.txt", FILE_READ);
        if (!dataFile) return;

        JsonDocument jsonBuffer;
        DeserializationError error = deserializeJson(jsonBuffer, dataFile);
        if (error) {
            Serial.print("[SettingsStore] deserializeJson() failed for /bikeStats.txt: ");
            Serial.println(error.c_str());
            dataFile.close();
            return;
        }

        BikeData a;
        a.mass = jsonBuffer["mass"];
        a.wheelCircumference = jsonBuffer["wheelCircumference"];
        a.logger = loggerFromString(jsonBuffer["logger"]);
        a.autoPause = jsonBuffer["autoPause"] | true;
        a.idleSleepMinutes = jsonBuffer["idleSleepMinutes"] | 5;
        a.autoLapMode = autoLapModeFromString(jsonBuffer["autoLapMode"] | String("Off"));
        a.autoLapValue = jsonBuffer["autoLapValue"] | 10;

        _model->bike().update(a);
        dataFile.close();
    }
}

void SettingsStore::saveLayout() {
    if (!_storage || !_model) return;

    JsonDocument doc;
    const LayoutData& l = _model->layout().get();

    JsonArray d = doc["displays"].to<JsonArray>();
    doc["rows"] = l.rows;
    doc["cols"] = l.cols;

    for (size_t j = 0; j < l.displays.size(); j++) {
        d[j]["x0"] = l.displays[j].x0;
        d[j]["y0"] = l.displays[j].y0;
        d[j]["x1"] = l.displays[j].x1;
        d[j]["y1"] = l.displays[j].y1;
        d[j]["type"] = topicDisplayName(l.displays[j].type);
    }

    writeJsonWithBackup(*_storage, "/layout.txt", doc);
}

void SettingsStore::loadLayout() {
    if (!_storage || !_model) return;
    if (!recoverSettingsFile(*_storage, "/layout.txt", false)) return;

    if (_storage->exists("/layout.txt")) {
        Serial.println("Found layout.txt");
        File32 dataFile = _storage->openFile("/layout.txt", FILE_READ);
        if (!dataFile) return;

        JsonDocument jsonBuffer;
        DeserializationError error = deserializeJson(jsonBuffer, dataFile);
        if (error) {
            Serial.print("[SettingsStore] deserializeJson() failed for /layout.txt: ");
            Serial.println(error.c_str());
            dataFile.close();
            return;
        }

        std::vector<DisplayItem> displays;
        JsonArray d = jsonBuffer["displays"];
        for (const auto& member : d) {
            DisplayItem disp;
            disp.type = topicFromDisplayName(String(member["type"] | ""));
            disp.x0 = member["x0"];
            disp.y0 = member["y0"];
            disp.x1 = member["x1"];
            disp.y1 = member["y1"];
            displays.push_back(disp);
        }

        uint8_t rows = constrain((int)(jsonBuffer["rows"] | 0), LAYOUT_MIN_ROWS, LAYOUT_MAX_ROWS);
        uint8_t cols = constrain((int)(jsonBuffer["cols"] | 0), LAYOUT_MIN_COLS, LAYOUT_MAX_COLS);

        _model->layout().update({displays, rows, cols});
        dataFile.close();
    }
}

void SettingsStore::saveTime() {
    if (!_storage || !_model) return;

    JsonDocument doc;
    doc["UTCOffset"] = _model->time().get().offset();

    writeJsonWithBackup(*_storage, "/time.txt", doc);
}

void SettingsStore::loadTime() {
    if (!_storage || !_model) return;
    if (!recoverSettingsFile(*_storage, "/time.txt", false)) return;

    if (_storage->exists("/time.txt")) {
        Serial.println("Found time.txt");
        File32 dataFile = _storage->openFile("/time.txt", FILE_READ);
        if (!dataFile) return;

        JsonDocument jsonBuffer;
        DeserializationError error = deserializeJson(jsonBuffer, dataFile);
        if (error) {
            Serial.print("[SettingsStore] deserializeJson() failed for /time.txt: ");
            Serial.println(error.c_str());
            dataFile.close();
            return;
        }

        int utcOffset = jsonBuffer["UTCOffset"];
        _model->time().setUTCOffset(utcOffset);
        dataFile.close();
    }
}

bool SettingsStore::saveGpsSettings(const GpsSettings& settings) {
    if (!_storage) {
        Serial.println("[SettingsStore] cannot save GPS settings: storage is unavailable");
        return false;
    }

    JsonDocument doc;
    JsonArray rates = doc["nmeaRates"].to<JsonArray>();
    for (uint8_t i = 0; i < GPS_NMEA_RATE_COUNT; ++i) {
        rates.add(settings.nmeaRates[i]);
    }

    return writeJsonWithBackup(*_storage, "/gpsConfig.txt", doc);
}

void SettingsStore::loadGpsSettings() {
    if (!_storage || !_model ||
        !recoverSettingsFile(*_storage, "/gpsConfig.txt", false) ||
        !_storage->exists("/gpsConfig.txt")) return;

    Serial.println("Found gpsConfig.txt");
    File32 dataFile = _storage->openFile("/gpsConfig.txt", FILE_READ);
    if (!dataFile) {
        Serial.println("[SettingsStore] failed to open /gpsConfig.txt for reading");
        return;
    }

    JsonDocument jsonBuffer;
    DeserializationError error = deserializeJson(jsonBuffer, dataFile);
    if (error) {
        Serial.print("[SettingsStore] deserializeJson() failed for /gpsConfig.txt: ");
        Serial.println(error.c_str());
        dataFile.close();
        return;
    }

    JsonArrayConst rates = jsonBuffer["nmeaRates"].as<JsonArrayConst>();
    if (rates.isNull() || rates.size() != GPS_NMEA_RATE_COUNT) {
        Serial.println("[SettingsStore] invalid NMEA rate list in /gpsConfig.txt");
        dataFile.close();
        return;
    }

    GpsSettings settings = _model->gpsSettings().get();
    for (uint8_t i = 0; i < GPS_NMEA_RATE_COUNT; ++i) {
        if (!rates[i].is<uint8_t>()) {
            Serial.println("[SettingsStore] invalid NMEA rate in /gpsConfig.txt");
            dataFile.close();
            return;
        }
        settings.nmeaRates[i] = rates[i].as<uint8_t>();
    }
    _model->gpsSettings().update(settings);
    dataFile.close();
}
