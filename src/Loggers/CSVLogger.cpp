#include "CSVLogger.hpp"
#include "HAL/SensorData.hpp"

namespace {

void writeCsvValue(File32& file, const DataBus& bus, const TopicMetadata& info) {
    switch (info.valueKind) {
        case TopicValueKind::Float:
            file.print(bus.get<float>(info.topic));
            break;
        case TopicValueKind::Bool:
            file.print(bus.get<bool>(info.topic) ? 1 : 0);
            break;
        case TopicValueKind::UInt8:
            file.print(static_cast<unsigned int>(bus.get<uint8_t>(info.topic)));
            break;
        case TopicValueKind::UInt32:
            file.print(static_cast<unsigned long>(bus.get<uint32_t>(info.topic)));
            break;
        case TopicValueKind::Int16:
            file.print(static_cast<int>(bus.get<int16_t>(info.topic)));
            break;
        case TopicValueKind::Location: {
            const location_data& location = bus.get<location_data>(info.topic);
            file.print(location.latitude, 6);
            file.print(",");
            file.print(location.longitude, 6);
            break;
        }
    }
}

void writeDeferredCsvValue(File32& file, const DataBus& bus,
                           const TopicMetadata& info) {
    if (info.valueKind == TopicValueKind::Location) {
        file.print(bus.get<location_data>(info.topic).valid ? 1 : 0);
    }
}

} // namespace

bool CSVLogger::startLogging(const timeData& currentTime) {
    _startTime = currentTime;
    _pauseAnchor = currentTime;
    _currentTime = currentTime;
    _lastDistanceM = 0;
    laps.clear();
    laps.push_back({currentTime, 0, 0, 0, 0, 0, 0});

    if (file.isOpen()) {
        if (!file.close()) return false;
    }

    snprintf(_filename, sizeof(_filename), "/%04d%02d%02d_%02d%02d%02d.csv",
             _startTime.year(), _startTime.month(), _startTime.day(),
             _startTime.hour(), _startTime.minute(), _startTime.second());

    if (!file.open(_filename, O_WRITE | O_CREAT | O_TRUNC)) {
        Serial.print("Error opening CSV file: ");
        Serial.println(_filename);
        return false;
    }

    const bool headerWritten = writeHeader();
    file.flush();
    return headerWritten && !file.getWriteError();
}

bool CSVLogger::writeHeader() {
    if (!file.isOpen()) {
        Serial.println("Error file not open: ");
        return false;
    }
    file.print("time");
    for (const TopicMetadata& info : kTopicMetadata) {
        file.print(",");
        file.print(info.csvName);
        if (info.csvName2) {
            file.print(",");
            file.print(info.csvName2);
        }
    }
    for (const TopicMetadata& info : kTopicMetadata) {
        if (info.csvDeferredName) {
            file.print(",");
            file.print(info.csvDeferredName);
        }
    }
    file.println();
    file.flush();
    return !file.getWriteError();
}

bool CSVLogger::addTrackpoint(const Trackpoint& tp, const timeData& currentTime) {
    if (!file.isOpen()) {
        Serial.println("Error file not open: ");
        return false;
    }
    file.print(currentTime.toString());
    for (const TopicMetadata& info : kTopicMetadata) {
        file.print(",");
        writeCsvValue(file, _bus, info);
    }
    for (const TopicMetadata& info : kTopicMetadata) {
        if (info.csvDeferredName) {
            file.print(",");
            writeDeferredCsvValue(file, _bus, info);
        }
    }
    file.println();
    file.flush();
    if (file.getWriteError()) return false;

    _currentTime = currentTime;

    if (tp.speed > laps.back().maxSpeed)
        laps.back().maxSpeed = tp.speed;
    if (tp.heartrate > laps.back().maxHRM)
        laps.back().maxHRM = tp.heartrate;

    laps.back().totalDistance = tp.distance;
    _lastDistanceM = tp.distance;
    return true;
}

bool CSVLogger::newLap(const timeData& currentTime) {
    if (!file.isOpen() || file.getWriteError()) return false;
    laps.push_back({currentTime, 1, 1, 0, 0, 0, 0});
    return true;
}

bool CSVLogger::finaliseLogging() {
    if (!file.isOpen()) return false;
    const long elapsedSeconds = elapsed_Total().totalSeconds();
    file.print("#summary,1,");
    file.print(elapsedSeconds > 0 ? static_cast<uint32_t>(elapsedSeconds) : 0);
    file.print(",");
    file.print(_lastDistanceM, 2);
    file.print(",");
    file.println(static_cast<unsigned long>(laps.size()));
    file.flush();
    const bool writesSucceeded = !file.getWriteError();
    const bool closeSucceeded = file.close();
    return writesSucceeded && closeSucceeded;
}