#pragma once
#include <Arduino.h>

enum class AppState {
    BOOT,
    IDLE,
    CONFIG,
    LOGGING,
    PAUSED
};

enum class CaloricProfile {
    Male,
    Female,
    Other
};

struct AppData {
    AppState state;
    DateTime birthday;
    float mass;
    CaloricProfile caloricProfile ;

};

class AppDataProvider {
public:
    const AppData& get() const { return _data; }
    uint32_t version() const { return _version; }

    void update(const AppData& newData) {
        _data = newData;
        ++_version;
    }

private:
    AppData _data{};
    uint32_t _version = 0;
};