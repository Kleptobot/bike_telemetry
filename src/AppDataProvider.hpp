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
    Male = 0,
    Female,
    Other

};

inline CaloricProfile& operator++(CaloricProfile& a) {
    switch(a){
        case CaloricProfile::Female: a = CaloricProfile::Male; break;
        case CaloricProfile::Male: a = CaloricProfile::Other; break;
        case CaloricProfile::Other: a = CaloricProfile::Female; break;
    }
    return a;
};

inline CaloricProfile& operator--(CaloricProfile& a) {
    switch(a){
        case CaloricProfile::Female: a = CaloricProfile::Other; break;
        case CaloricProfile::Male: a = CaloricProfile::Female; break;
        case CaloricProfile::Other: a = CaloricProfile::Male; break;
    }
    return a;
};

inline String toString(CaloricProfile a) {
    String s;
    switch(a){
        case CaloricProfile::Female: s = "F"; break;
        case CaloricProfile::Male: s = "M"; break;
        case CaloricProfile::Other: s = "O"; break;
    }
    return s;
};

inline CaloricProfile fromString(String s) {
    CaloricProfile c;
    if(s=="F") c = CaloricProfile::Female;
    else if(s=="M") c = CaloricProfile::Male;
    else c = CaloricProfile::Other;
    return c;
}

struct AppData {
    AppState state;
    DateTime birthday;
    uint16_t mass;
    CaloricProfile caloricProfile ;
};

class AppDataProvider {
public:
    const AppData& get() const { return _data; }
    uint32_t version() const { return _version; }


    void update(const AppData& newData) {
        _data.birthday = newData.birthday;
        _data.mass = newData.mass;
        _data.caloricProfile = newData.caloricProfile;
        ++_version;
    }

private:
    friend class App;
    void setState(AppState s) { _data.state = s; }
    AppData _data{};
    uint32_t _version = 0;
};