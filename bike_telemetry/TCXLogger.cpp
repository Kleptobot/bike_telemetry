#include "TCXLogger.h"

void TCXLogger::startLogging(DateTime currentTime) {
  _startTime = currentTime;
  if (!file.isOpen()) {
    if (!file.open("data.tmp", FILE_WRITE)) {
      Serial.println("Error opening file: data.tmp");
      return;
    }
  }
}

void TCXLogger::writeHeader(){
  char time[32];

  //write the header to the final file
  sprintf(_filename, "%d-%d-%d_%02d-%02d-%02d.tcx", _startTime.day(),_startTime.month(),_startTime.year(),_startTime.hour(),_startTime.minute(),_startTime.second());
  if (!file.isOpen()) {
    if (!file.open(_filename, FILE_WRITE)) {
      Serial.print("Error opening file: ");
      Serial.println(_filename);
      return;
    }
  }
  sprintf(time, "%d-%02d-%02dT%02d:%02d:%02d", _startTime.year(), _startTime.month(), _startTime.day(), _startTime.hour(), _startTime.minute(), _startTime.second());

  // Write the XML header and opening tags
  file.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
  file.println("<TrainingCenterDatabase xmlns=\"http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2\">");
  file.println("  <Activities>");
  file.println("    <Activity Sport=\"Biking\">");
  file.print("      <Id>");
  file.print(time);
  file.println("</Id>");
  file.print("      <Lap StartTime=\"");
  file.print(time);
  file.println("\">");
  // Initialize total time, distance, etc. 
  file.print("        <TotalTimeSeconds>");
  file.print(0);
  file.print("</TotalTimeSeconds>\n");
  file.print("        <DistanceMeters>");
  file.print(totalDistance);
  file.print("</DistanceMeters>\n");
  file.print("        <MaximumSpeed>");
  file.print(maxSpeed);
  file.print("</MaximumSpeed>\n");
  file.print("        <Calories>");
  file.print(Calories);
  file.print("</Calories>\n");
  file.print("        <AverageHeartRateBpm>\n");
  file.print("          <Value>");
  file.print(avgHRM);
  file.print("</Value>\n"); // Set to at least 1
  file.print("        </AverageHeartRateBpm>\n");
  file.print("        <MaximumHeartRateBpm>\n");
  file.print("          <Value>");
  file.print(maxHRM);
  file.print("</Value>\n"); // Set to at least 1
  file.print("        </MaximumHeartRateBpm>\n");
  file.print("        <Intensity>Active</Intensity>\n"); // Add intensity element
  file.print("        <Cadence>");
  file.print(0);
  file.print("</Cadence>\n"); // Placeholder value
  file.print("        <TriggerMethod>Manual</TriggerMethod>\n");
  file.println("        <Track>");

  //close the file (we'll open it again later)
  file.close();
};

void TCXLogger::addTrackpoint(const Trackpoint& tp){
  if (!file.isOpen()) {
    Serial.println("Error adding trackpoint");
    return;
  }
  char time[32];
  sprintf(time, "%d-%02d-%02dT%02d:%02d:%02d", tp.currentTime.year(), tp.currentTime.month(), tp.currentTime.day(), tp.currentTime.hour(), tp.currentTime.minute(), tp.currentTime.second());

  file.println("          <Trackpoint>");
  file.print("            <Time>");
  file.print(time);
  file.println("</Time>");
  file.println("            <Position>");
  file.print("              <LatitudeDegrees>");
  file.print(tp.latitude);
  file.println("</LatitudeDegrees>");
  file.print("              <LongitudeDegrees>");
  file.print(tp.longitude);
  file.println("</LongitudeDegrees>");
  file.println("            </Position>");
  file.print("            <AltitudeMeters>");
  file.print(tp.altitude);
  file.println("</AltitudeMeters>");
  file.print("            <DistanceMeters>");
  file.print(tp.distance);
  file.println("</DistanceMeters>");

  if (tp.heartRate > 0) { // Optional heart rate
    file.println("            <HeartRateBpm>");
    file.println("              <Value>" + String(tp.heartRate) + "</Value>");
    file.println("            </HeartRateBpm>");
  }

  // Start Extensions section
  file.println("            <Extensions>");
  file.println("              <TPX xmlns=\"http://www.garmin.com/xmlschemas/TrackPointExtension/v1\">");

  if (tp.power > 0) { // Optional power
    file.print("                <Power>");
    file.print(tp.power);
    file.println("</Power>");
  }
  if (tp.cadence > 0) { // Optional cadence
    file.print("                <Cadence>");
    file.print(tp.cadence);
    file.println("</Cadence>");
  }
  if (tp.speed > 0) { // Optional speed
    file.print("                <Speed>");
    file.print(tp.speed);
    file.println("</Speed>");
  }

  file.println("              </TPX>");
  file.println("            </Extensions>");
  file.println("          </Trackpoint>");

  updateTotals(tp);
}

void TCXLogger::writeFooter() {
  // Write closing tags for Track
  file.println("        </Track>");
  file.println("      </Lap>"); // Close the Lap element
  file.println("    </Activity>");
  file.println("  </Activities>");
  file.println("</TrainingCenterDatabase>");
}

void TCXLogger::updateTotals(const Trackpoint& tp) {
  totalPoints ++;
  if (tp.heartRate > 0)
    totalHeartRate += tp.heartRate;
  if(tp.speed>maxSpeed)
    maxSpeed=tp.speed;
  if(tp.heartRate>maxHRM)
    maxHRM=tp.heartRate;

  totalDistance = tp.distance;
  
  avgHRM = totalHeartRate/totalPoints;
  if(avgHRM<1)
    avgHRM=1;
  
  TimeSpan totalTime = (tp.currentTime - _startTime);

  float mass = 75;
  float age = 34;

  Calories = ((age * 0.2017) - (mass * 0.09036) + (avgHRM * 0.6309) - 55.0969) * totalTime.totalseconds() / 4.184;
  if(Calories<0)
    Calories=0;
}

bool TCXLogger::finaliseLogging() {
  if(!bFinalise_Started){
    //close file tmp after writing the last point
    file.close();
    
    // Write the header to another file
    writeHeader();

    // Open the source file for reading
    if (!data_tmp.open("data.tmp", O_READ)) {
      Serial.println("Error opening source file.");
      return false;
    }

    // Open the destination file for appending
    if (!file.open(_filename, O_WRITE | O_APPEND | O_CREAT)) {
      Serial.println("Error opening : ");
      Serial.println(_filename);
      data_tmp.close();
      return false;
    }
    bFinalise_Started = true;
    bReading = true;
  }


  if (bReading) {
    bytesRead = data_tmp.read(buffer, sizeof(buffer));
    if (bytesRead > 0) {
      bWriting = true; // Data is available to write
    } else {
      // Finished reading
      bReading = false; // Set reading to false only after confirming no more data
    }
  }

  // Check if we are writing
  if (bWriting) {
    file.write(buffer, bytesRead);
    bWriting = false; // Reset writing flag after writing
  }

  if (!bReading && !bWriting) {
    data_tmp.close();
    writeFooter();
    file.close();

    bFinalise_Started = false;

    //reset totals
    totalHeartRate = 0;
    totalDistance = 0;
    totalPoints = 0;
    maxSpeed=0;
    maxHRM=1;
    avgHRM=1;
    Calories=0;

    Serial.println("File append complete.");
    return true;
  }
  return false;
}