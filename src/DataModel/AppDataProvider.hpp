#ifndef APPDATAPROVIDER_H
#define APPDATAPROVIDER_H

#include <Arduino.h>
#include <RTClib.h>
#include "TimeDataProvider.hpp"

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
    timeData birthday = DateTime(1993,5,21);
    uint16_t mass = 75;
    CaloricProfile caloricProfile = CaloricProfile::Other;
    uint8_t zone1Start = 55; // bpm
    uint8_t zone2Start = 117; // bpm
    uint8_t zone3Start = 138; // bpm
    uint8_t zone4Start = 151; // bpm
    uint8_t zone5Start = 162; // bpm

    AppData(){}
    AppData(
        AppState s, 
        timeData b, 
        uint16_t m, 
        CaloricProfile c, 
        uint8_t z1, 
        uint8_t z2, 
        uint8_t z3, 
        uint8_t z4, 
        uint8_t z5) 
        : state(s)
        , birthday(b)
        , mass(m)
        , caloricProfile(c)
        , zone1Start(z1)
        , zone2Start(z2)
        , zone3Start(z3)
        , zone4Start(z4)
        , zone5Start(z5) {}

    AppData& operator=(const AppData& other) {
        state = other.state;
        birthday = other.birthday;
        mass = other.mass;
        caloricProfile = other.caloricProfile;
        zone1Start = other.zone1Start;
        zone2Start = other.zone2Start;
        zone3Start = other.zone3Start;
        zone4Start = other.zone4Start;
        zone5Start = other.zone5Start;
        return *this;
    }
};

class AppDataProvider {
public:
    const AppData& get() const { return _data; }
    uint32_t version() const { return _version; }


    void update(const AppData& newData) {
        _data.birthday = newData.birthday;
        _data.mass = newData.mass;
        _data.caloricProfile = newData.caloricProfile;
        _data.zone1Start = newData.zone1Start;
        _data.zone2Start = newData.zone2Start;
        _data.zone3Start = newData.zone3Start;
        _data.zone4Start = newData.zone4Start;
        _data.zone5Start = newData.zone5Start;
        ++_version;
    }

private:
    friend class App;
    void setState(AppState s) { _data.state = s; }
    AppData _data{};
    uint32_t _version = 0;
};

#endif