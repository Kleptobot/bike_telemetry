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
    file.println("acc_x,acc_y,acc_z,gyro_x,gyro_y,gyro_z,BattPercentage,speed,cadence,temperature,altitude,heartrate,power,validLocation,longitude,latitude,distance,totalDistance,grade");
    file.flush();
}

void CSVLogger::addTrackpoint(const Telemetry& tp, const timeData& currentTime) {
    if (!file.isOpen()) {
        Serial.println("Error file not open: ");
        return;
    }
    file.print(currentTime.toString()); file.print(",");
    file.print(tp.latitude, 6); file.print(",");
    file.print(tp.longitude, 6); file.print(",");
    file.print(tp.altitude); file.print(",");
    file.print(tp.speed); file.print(",");
    file.print(tp.heartrate); file.print(",");
    file.print(tp.cadence); file.print(",");
    file.println(tp.power);
    file.flush();

    _currentTime = currentTime;

    if (tp.speed > laps.back().maxSpeed)
        laps.back().maxSpeed = tp.speed;
    if (tp.heartrate > laps.back().maxHRM)
        laps.back().maxHRM = tp.heartrate;

    laps.back().totalDistance = tp.distance;
}

void CSVLogger::newLap(const timeData& currentTime) {
    laps.push_back({currentTime, 1, 1, 0, 0, 0, 0});
}

bool CSVLogger::finaliseLogging() {
    if (file.isOpen()) {
        file.flush();
        file.close();
    }
    return true;
}