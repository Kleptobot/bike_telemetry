#include "CSVLogger.hpp"

void CSVLogger::startLogging(const timeData& currentTime) {
    _startTime = currentTime;
    _currentTime = currentTime;
    laps.clear();
    laps.push_back({currentTime, 0, 0, 0, 0, 0, 0});

    if (file.isOpen()) {
        file.close();
    }

    snprintf(_filename, sizeof(_filename), "/%04d%02d%02d_%02d%02d%02d.csv",
             _startTime.year(), _startTime.month(), _startTime.day(),
             _startTime.hour(), _startTime.minute(), _startTime.second());

    if (!file.open(_filename, O_WRITE | O_CREAT | O_TRUNC)) {
        Serial.print("Error opening CSV file: ");
        Serial.println(_filename);
        return;
    }

    writeHeader();
    file.flush();
}

void CSVLogger::writeHeader() {
    if (!file.isOpen()) {
        Serial.println("Error file not open: ");
        return;
    }
    file.println("Timestamp,Latitude,Longitude,Altitude,Speed,HeartRate,Cadence,Power");
    file.flush();
}

void CSVLogger::addTrackpoint(const Trackpoint& tp) {
    if (!file.isOpen()) {
        Serial.println("Error file not open: ");
        return;
    }
    file.print(tp.currentTime.toString()); file.print(",");
    file.print(tp.latitude, 6); file.print(",");
    file.print(tp.longitude, 6); file.print(",");
    file.print(tp.altitude); file.print(",");
    file.print(tp.speed); file.print(",");
    file.print(tp.heartRate); file.print(",");
    file.print(tp.cadence); file.print(",");
    file.println(tp.power);
    file.flush();

    _currentTime = tp.currentTime;

    Lap& lap = laps.back();
    lap.parts++;
    if (tp.speed > lap.maxSpeed) lap.maxSpeed = static_cast<float>(tp.speed);   // km/h, converted on write
    if (tp.heartRate > 0) {
        lap.totalHRM += static_cast<float>(tp.heartRate);
        if (tp.heartRate > lap.maxHRM) lap.maxHRM = static_cast<float>(tp.heartRate);
    }
    if (tp.cadence > 0) {
        lap.totalCadence += static_cast<float>(tp.cadence);
    }

    laps.back().totalHRM += tp.heartRate;
    laps.back().totalCadence += tp.cadence;

    if (tp.speed > laps.back().maxSpeed)
        laps.back().maxSpeed = tp.speed;
    if (tp.heartRate > laps.back().maxHRM)
        laps.back().maxHRM = tp.heartRate;

    laps.back().totalDistance = tp.distance;
}

void CSVLogger::newLap(timeData currentTime) {
    laps.push_back({currentTime, 1, 1, 0, 0, 0, 0});
}

bool CSVLogger::finaliseLogging() {
    if (file.isOpen()) {
        file.flush();
        file.close();
    }
    return true;
}