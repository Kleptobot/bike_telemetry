#include "common/FsApiConstants.h"
#include "TCXLogger.hpp"

bool TCXLogger::startLogging(const timeData& currentTime) {
    initTimer(currentTime);
    laps.clear();
    _started = false;
    laps.push_back({currentTime, 0, 0, 0, 0, 0, 0});
    resetTotals();
    snprintf(_filename, sizeof(_filename), "/%04d%02d%02d_%02d%02d%02d.tcx",
             _startTime.year(), _startTime.month(), _startTime.day(),
             _startTime.hour(), _startTime.minute(), _startTime.second());
    Serial.print("[TCXLogger] ride file will be ");
    Serial.println(_filename);

    if (file.isOpen()) {
        if (!file.close()) return false;
    }

    //open the first lap file
    memset(lap_name,0,32);
    snprintf(lap_name, sizeof(lap_name), "/lap_%u_%u", (unsigned)(laps.size()-1), (unsigned)laps.back().parts);
    if (!file.open(lap_name, O_WRITE | O_CREAT | O_TRUNC)) {
        Serial.print("Error opening file: ");
        Serial.println(lap_name);
        return false;
    }
    _started = true;
    return !file.getWriteError();
};

bool TCXLogger::writeLapHeader(uint16_t lapIndex, File32 *file) {
    if (!file || !file->isOpen() || lapIndex >= laps.size()) {
        Serial.println("Error file not open: ");
        return false;
    }
    char time[32];

    Lap lp = laps[lapIndex];
    timeDuration ts;
    if(lapIndex < laps.size()-1 ) {
        ts = laps[lapIndex+1].startTime - laps[lapIndex].startTime;
    } else {
        ts = elapsed_Lap();
    }

        // Live calories from FitnessFusion (cumulative; subtract lap-start to
    // get this lap's delta). Replaces the retrospective Key-et-al formula.
    float lapCalories = lp.totalCalories;
    if (lapIndex > 0) lapCalories -= laps[lapIndex - 1].totalCalories;
    if (lapCalories < 0) lapCalories = 0;

    float avgHRM = 0;
    if (ts._totalSeconds > 0) {
        avgHRM = float(lp.totalHRM) / float(ts._totalSeconds);
    }
    float avgCAD = 0;
    if (ts._totalSeconds > 0) {
        avgCAD = lp.totalCadence / ts._totalSeconds;
    }

    sprintf(time, "%d-%02d-%02dT%02d:%02d:%02d", lp.startTime.year(), lp.startTime.month(), lp.startTime.day(), lp.startTime.hour(), lp.startTime.minute(), lp.startTime.second());
    file->print("      <Lap StartTime=\"");file->print(time);file->println("\">");
    // Initialize total time, distance, etc. 
    file->print("        <TotalTimeSeconds>");file->print(ts._totalSeconds);file->print("</TotalTimeSeconds>\n");
    file->print("        <DistanceMeters>");file->print(lp.totalDistance);file->print("</DistanceMeters>\n");
    file->print("        <MaximumSpeed>");file->print(kmhToMs(lp.maxSpeed),2)   /* m/s */;file->print("</MaximumSpeed>\n");
    file->print("        <Calories>");file->print((int32_t)lapCalories);file->print("</Calories>\n");
    file->print("        <AverageHeartRateBpm>\n");
    file->print("          <Value>");file->print(avgHRM,2);file->print("</Value>\n"); // Set to at least 1
    file->print("        </AverageHeartRateBpm>\n");
    file->print("        <MaximumHeartRateBpm>\n");
    file->print("          <Value>");file->print(lp.maxHRM,2);file->print("</Value>\n"); // Set to at least 1
    file->print("        </MaximumHeartRateBpm>\n");
    file->print("        <Intensity>Active</Intensity>\n"); // Add intensity element
    file->print("        <Cadence>");file->print(avgCAD,2);file->print("</Cadence>\n"); // Placeholder value
        file->print("        <TriggerMethod>Manual</TriggerMethod>\n");
    // Fitness extensions: NP, IF, TSS per lap (Garmin ActivityExtension v2)
    file->println("        <Extensions>\n");
    file->println("          <LX xmlns=\"http://www.garmin.com/xmlschemas/ActivityExtension/v2\">\n");
    file->print("            <AvgNormalizedPower>");file->print(lp.lastNormalizedPower);file->print("</AvgNormalizedPower>\n");
    file->print("            <IntensityFactor>");file->print(lp.lastIntensityFactor,3);file->print("</IntensityFactor>\n");
    file->print("            <TCXTrainingMetrics_TSS>");file->print(lp.totalTss);file->print("</TCXTrainingMetrics_TSS>\n");
    file->println("          </LX>\n");
    file->println("        </Extensions>\n");
    file->println("        <Track>");
    return !file->getWriteError();
};

bool TCXLogger::addTrackpoint(const Trackpoint& tp, const timeData& currentTime) {
    if (!_started || !file.isOpen() || file.getWriteError() || laps.empty()) return false;

    char time[32];
    sprintf(time, "%d-%02d-%02dT%02d:%02d:%02d", currentTime.year(), currentTime.month(), currentTime.day(), currentTime.hour(), currentTime.minute(), currentTime.second());

    file.println("          <Trackpoint>");
    file.print("            <Time>");file.print(time);file.println("</Time>");
    file.println("            <Position>");
    file.print("              <LatitudeDegrees>");file.print(tp.latitude,6);file.println("</LatitudeDegrees>");
    file.print("              <LongitudeDegrees>");file.print(tp.longitude,6);file.println("</LongitudeDegrees>");
    file.println("            </Position>");
    file.print("            <AltitudeMeters>");file.print(tp.altitude);file.println("</AltitudeMeters>");
    file.print("            <DistanceMeters>");file.print(tp.distance);file.println("</DistanceMeters>");
    file.print("            <Calories>");file.print(tp.calories);file.println("</Calories>");

    if (tp.heartrate > 0) { // Optional heart rate
        file.println("            <HeartRateBpm>");
        file.println("              <Value>" + String(tp.heartrate) + "</Value>");
        file.println("            </HeartRateBpm>");
    }

    // Start Extensions section
    file.println("            <Extensions>");
    file.println("              <TPX xmlns=\"http://www.garmin.com/xmlschemas/TrackPointExtension/v1\">");

    if (tp.power > 0) { // Optional power
        file.print("                <Power>");file.print(tp.power);file.println("</Power>");
    }
    if (tp.cadence > 0) { // Optional cadence
        file.print("                <Cadence>");file.print(tp.cadence);file.println("</Cadence>");
    }
    if (tp.speed > 0) { // Optional speed
        // TPX Speed is metres per second; Trackpoint::speed is km/h.
        file.print("                <Speed>");file.print(kmhToMs(tp.speed));file.println("</Speed>");
    }

    file.println("              </TPX>");
    file.println("            </Extensions>");
    file.println("          </Trackpoint>");
    file.flush();
    if (file.getWriteError()) return false;

    _currentTime = currentTime;

    totalPoints ++;
    if(totalPoints >= points_per_chunk) {
        totalPoints = 0;
        laps.back().parts++;
        if (!file.close()) return false;

        memset(lap_name,0,32);
        snprintf(lap_name, sizeof(lap_name), "/lap_%u_%u", (unsigned)(laps.size()-1), (unsigned)laps.back().parts);
        if (!file.open(lap_name, O_WRITE | O_APPEND | O_CREAT)) {
            Serial.print("Error opening file: ");
            Serial.println(lap_name);
            return false;
        }
    }
    
    laps.back().totalHRM += tp.heartrate;
    laps.back().totalCadence += tp.cadence;

    if(tp.speed>laps.back().maxSpeed)
        laps.back().maxSpeed=tp.speed;
    if(tp.heartrate>laps.back().maxHRM)
        laps.back().maxHRM=tp.heartrate;

    laps.back().totalDistance = tp.distance;
    // Fitness accumulators: store cumulative-from-FitnessFusion values
    laps.back().totalCalories = tp.calories;
    laps.back().totalTss = tp.tss;
    laps.back().lastNormalizedPower = tp.normalizedPower;
    laps.back().lastIntensityFactor = tp.intensityFactor;
    return !file.getWriteError();
};

void TCXLogger::resetTotals() { totalPoints = 0; };

bool TCXLogger::newLap(const timeData& currentTime) {
    if (!_started || !file.isOpen() || file.getWriteError()) return false;
    if (!file.close()) return false;

    totalPoints = 0;
    resetTotals();
    laps.push_back({currentTime, 1, 1, 0, 0, 0, 0});
    laps.back().activeAtStart = activeAt(currentTime);

    memset(lap_name,0,32);
    snprintf(lap_name, sizeof(lap_name), "/lap_%u_%u", (unsigned)(laps.size()-1), (unsigned)laps.back().parts);
    if (!file.open(lap_name, O_WRITE | O_APPEND | O_CREAT)) {
        Serial.print("Error opening file: ");
        Serial.println(lap_name);
        return false;
    }
    _currentTime = currentTime;
    return !file.getWriteError();
};

bool TCXLogger::dataTransfer(File32 *from, File32 *to) {
    if (!from || !to || !from->isOpen() || !to->isOpen()) return false;
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        const int readCount = from->read(buffer, sizeof(buffer) - 1);
        if (readCount < 0) return false;
        if (readCount == 0) break;
        if (to->write(buffer, static_cast<size_t>(readCount)) !=
                static_cast<size_t>(readCount) ||
            to->getWriteError()) {
            return false;
        }
    }
    to->flush();
    return !to->getWriteError();
}

bool TCXLogger::finaliseLogging() {
    if (!_started) return false;

    Serial.println("TCX::finaliseLogging entry");
    Serial.flush();

    File32 final_file;
    bool success = true;
    if (file.isOpen()) {
        Serial.println("TCX::closing active lap file");
        Serial.flush();
        file.flush();
        const bool writesSucceeded = !file.getWriteError();
        const bool closeSucceeded = file.close();
        if (!writesSucceeded || !closeSucceeded) success = false;
    }

    int lapSize = laps.size();
    Serial.print("TCX::lap count=");
    Serial.println(lapSize);
    Serial.flush();

    Serial.print("TCX::opening final file ");
    Serial.println(_filename);
    Serial.flush();
    if (!success) {
        _started = false;
        return false;
    }
    if (!final_file.open(_filename, O_WRITE | O_CREAT | O_TRUNC)) {
        Serial.print("TCX::open final file failed: ");
        Serial.println(_filename);
        Serial.flush();
        _started = false;
        return false;
    }

    Serial.println("TCX::opened final file");
    Serial.flush();

    //nested loop to add all laps and their parts into the main file

    Serial.print("Writing head to "); Serial.println(_filename);

    // Write the XML header and opening tags
    final_file.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    final_file.println("<TrainingCenterDatabase xmlns=\"http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2\">");
    final_file.println("  <Activities>");
    final_file.println("    <Activity Sport=\"Biking\">");
    final_file.print("      <Id>");final_file.print(_startTime.toString());final_file.println("</Id>");
    if (final_file.getWriteError()) success = false;

    Serial.println("Processing laps...");
    for(int i=0;i<lapSize && success;i++) {
        char lapString[40];
        sprintf(lapString,"processing lap %d of %d", i+1, lapSize);
        Serial.println(lapString);
        success = writeLapHeader(i, &final_file);
        for(int j=0;j<=laps[i].parts && success;j++) {
            //transfer lap_%d_%d, i,j to _filename
            memset(lap_name, 0 , 32);
            snprintf(lap_name, sizeof(lap_name), "/lap_%d_%d", i, j);
            Serial.print("processing file "); Serial.println(lap_name);
            File32 lap_file;
            if (!lap_file.open(lap_name, O_READ)) {
                Serial.print("Error opening lap file: "); Serial.println(lap_name);
                success = false;
                break;
            }
            Serial.print("Copying lap chunk: "); Serial.println(lap_name);
            uint32_t remaining = lap_file.fileSize();
            while (remaining > 0) {
                const size_t chunkSize = remaining < sizeof(buffer) - 1
                    ? remaining : sizeof(buffer) - 1;
                const int readCount = lap_file.read(buffer, chunkSize);
                if (readCount != static_cast<int>(chunkSize)) {
                    success = false;
                    break;
                }
                const size_t written = final_file.write(buffer, chunkSize);
                memset(buffer, 0, sizeof(buffer)); // Clear buffer
                if (written != chunkSize || final_file.getWriteError()) {
                    Serial.print("Write error for "); Serial.println(lap_name);
                    success = false;
                    break;
                }
                remaining -= static_cast<uint32_t>(readCount);
            }
            if (!lap_file.close()) success = false;
            final_file.flush();
            if (final_file.getWriteError()) success = false;
            if (success && !_storage->remove(lap_name)) {
                Serial.print("Failed to remove lap file: "); Serial.println(lap_name);
                success = false;
            }
        }
        if (success) {
            final_file.println("        </Track>");
            final_file.println("      </Lap>"); // Close the Lap element
            if (final_file.getWriteError()) success = false;
        }
    }
    if (success) {
        final_file.println("    </Activity>");
        final_file.println("  </Activities>");
        final_file.println("</TrainingCenterDatabase>");
        final_file.flush();
        if (final_file.getWriteError()) success = false;
    }
    if (!final_file.close()) success = false;

    _started = false;
    return success;
};