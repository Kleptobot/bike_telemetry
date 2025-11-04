#include "common/FsApiConstants.h"
#include "TCXLogger.hpp"

void TCXLogger::startLogging(DateTime currentTime) {
  _startTime = currentTime;
  laps.clear();
  laps.push_back({currentTime, 1, 1, 0, 0, 0, 0, 0});
  sprintf(_filename, "%d-%d-%d_%02d-%02d-%02d.tcx", _startTime.day(),_startTime.month(),_startTime.year(),_startTime.hour(),_startTime.minute(),_startTime.second());

  //open the first lap file
  memset(lap_name,0,32);
  sprintf(lap_name, "lap_%d_%d", laps.size()-1, laps.back().parts);
  if (!file.open(lap_name, O_WRITE | O_APPEND | O_CREAT)) {
    Serial.print("Error opening file: ");
    Serial.println(lap_name);
  }
};

void TCXLogger::writeLapHeader(int lapIndex, File32 *file){
  if (!file->isOpen()) {
    Serial.println("Error file not open: ");
    return;
  }
  char time[32];

  Lap lp = laps[lapIndex];
  TimeSpan ts;
  if(lapIndex < laps.size()-1 )
  {
    ts = laps[lapIndex+1].startTime - laps[lapIndex].startTime;
  }else{
    ts = elapsed_Lap();
  }

  sprintf(time, "%d-%02d-%02dT%02d:%02d:%02d", lp.startTime.year(), lp.startTime.month(), lp.startTime.day(), lp.startTime.hour(), lp.startTime.minute(), lp.startTime.second());
  file->print("      <Lap StartTime=\"");file->print(time);file->println("\">");
  // Initialize total time, distance, etc. 
  file->print("        <TotalTimeSeconds>");file->print(ts.totalseconds());file->print("</TotalTimeSeconds>\n");
  file->print("        <DistanceMeters>");file->print(lp.totalDistance);file->print("</DistanceMeters>\n");
  file->print("        <MaximumSpeed>");file->print(lp.maxSpeed);file->print("</MaximumSpeed>\n");
  file->print("        <Calories>");file->print(lp.Calories);file->print("</Calories>\n");
  file->print("        <AverageHeartRateBpm>\n");
  file->print("          <Value>");file->print(lp.avgHRM);file->print("</Value>\n"); // Set to at least 1
  file->print("        </AverageHeartRateBpm>\n");
  file->print("        <MaximumHeartRateBpm>\n");
  file->print("          <Value>");file->print(lp.maxHRM);file->print("</Value>\n"); // Set to at least 1
  file->print("        </MaximumHeartRateBpm>\n");
  file->print("        <Intensity>Active</Intensity>\n"); // Add intensity element
  file->print("        <Cadence>");file->print(lp.avgCadence,2);file->print("</Cadence>\n"); // Placeholder value
  file->print("        <TriggerMethod>Manual</TriggerMethod>\n");
  file->println("        <Track>");
};

void TCXLogger::addTrackpoint(const Trackpoint& tp){

  char time[32];
  sprintf(time, "%d-%02d-%02dT%02d:%02d:%02d", tp.currentTime.year(), tp.currentTime.month(), tp.currentTime.day(), tp.currentTime.hour(), tp.currentTime.minute(), tp.currentTime.second());

  file.println("          <Trackpoint>");
  file.print("            <Time>");file.print(time);file.println("</Time>");
  file.println("            <Position>");
  file.print("              <LatitudeDegrees>");file.print(tp.latitude,6);file.println("</LatitudeDegrees>");
  file.print("              <LongitudeDegrees>");file.print(tp.longitude,6);file.println("</LongitudeDegrees>");
  file.println("            </Position>");
  file.print("            <AltitudeMeters>");file.print(tp.altitude);file.println("</AltitudeMeters>");
  file.print("            <DistanceMeters>");file.print(tp.distance);file.println("</DistanceMeters>");

  if (tp.heartRate > 0) { // Optional heart rate
    file.println("            <HeartRateBpm>");
    file.println("              <Value>" + String(tp.heartRate) + "</Value>");
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
    file.print("                <Speed>");file.print(tp.speed);file.println("</Speed>");
  }

  file.println("              </TPX>");
  file.println("            </Extensions>");
  file.println("          </Trackpoint>");
  file.flush();

  TimeSpan ts = tp.currentTime - _currentTime;

  double beats = (tp.heartRate + _BPM_last)/2;
  _BPM_last = tp.heartRate;
  double cads = (tp.cadence + _CAD_last)/2;
  _CAD_last = tp.cadence;

  totalHeartBeats += beats*(ts.totalseconds()/60);
  total_RPMS += cads*(ts.totalseconds()/60);
  
  ts = tp.currentTime - _startTime;
  laps.back().avgHRM = totalHeartBeats/(ts.totalseconds()/60);
  laps.back().avgCadence = total_RPMS/(ts.totalseconds()/60);

  if(tp.speed>laps.back().maxSpeed)
    laps.back().maxSpeed=tp.speed;
  if(tp.heartRate>laps.back().maxHRM)
    laps.back().maxHRM=tp.heartRate;

  laps.back().totalDistance = tp.distance;

  
  _currentTime = tp.currentTime;
  _elapsed_Lap = (tp.currentTime - _startTime);

  auto& a = _model.app().get();
  float age = TimeSpan(_currentTime - a.birthday).days()/356.25;

  int f = ((age * 0.074) - (a.mass * 0.05741) + (laps.back().avgHRM * 0.4472) - 20.4022) * _elapsed_Lap.totalseconds() / 4.184;
  int m = ((age * 0.2017) - (a.mass * 0.09036) + (laps.back().avgHRM * 0.6309) - 55.0969) * _elapsed_Lap.totalseconds() / 4.184;

  switch(a.caloricProfile) {
    case CaloricProfile::Female: 
      laps.back().Calories = f;
      break;
    case CaloricProfile::Male:
      laps.back().Calories = m;
      break;
    case CaloricProfile::Other:
      laps.back().Calories = (m + f) /2;
      break;
  }
  
  if(laps.back().Calories<0)
    laps.back().Calories=0;

  
  totalPoints ++;
  if(totalPoints >= points_per_chunk)
  {
    totalPoints = 0;
    laps.back().parts++;
    file.close();

    memset(lap_name,0,32);
    sprintf(lap_name, "lap_%d_%d", laps.size()-1, laps.back().parts);
    if (!file.open(lap_name, O_WRITE | O_APPEND | O_CREAT)) {
      Serial.print("Error opening file: ");
      Serial.println(lap_name);
    }
  }
};

void TCXLogger::resetTotals()
{
    totalHeartBeats = 0;
    _BPM_last = 0;
    totalPoints = 0;
    total_RPMS = 0;
    _CAD_last = 0;
};

void TCXLogger::newLap(DateTime currentTime)
{
  totalPoints = 0;
  resetTotals();
  laps.push_back({currentTime, 1, 1, 0, 0, 0, 0, 0});
  file.close();

  memset(lap_name,0,32);
  sprintf(lap_name, "lap_%d_%d", laps.size()-1, laps.back().parts);
  if (!file.open(lap_name, O_WRITE | O_APPEND | O_CREAT)) {
    Serial.print("Error opening file: ");
    Serial.println(lap_name);
  }
};

void TCXLogger::dataTransfer(File32 *from, File32 *to)
{
  bool bReading=true, bWriting=false;

  while(bReading || bWriting)
  {
    if (bReading) {
      memset(buffer, 0, sizeof(buffer)); // Clear buffer
      bytesRead = from->read(buffer, sizeof(buffer)-1); // Leave space for null-terminator
      if (bytesRead > 0) {
        bWriting = true; // Data is available to write
      } else {
        // Finished reading
        bReading = false; // Set reading to false only after confirming no more data
      }
    }

    // Check if we are writing
    if (bWriting) {
      // Open the destination file for appending
      to->write(buffer, bytesRead);
      bWriting = false; // Reset writing flag after writing
    }
  }
  to->flush();
}

bool TCXLogger::finaliseLogging()
{
  File32 final_file;
  file.close();

  int lapSize = laps.size();

  int numparts = 0;
  for(int i=0;i<lapSize;i++)
  {
    numparts += laps[i].parts;
  }

  if (!final_file.open(_filename, O_WRITE | O_APPEND | O_CREAT)) {
    final_file.print("Error opening : ");Serial.println(_filename);
    return false;
  }

  //nested loop to add all laps and their parts into the main file
  char time[32];
  sprintf(time, "%d-%02d-%02dT%02d:%02d:%02d", _startTime.day(),_startTime.month(),_startTime.year(),_startTime.hour(),_startTime.minute(),_startTime.second());

  Serial.print("Writing head to "); Serial.println(_filename);

  // Write the XML header and opening tags
  final_file.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
  final_file.println("<TrainingCenterDatabase xmlns=\"http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2\">");
  final_file.println("  <Activities>");
  final_file.println("    <Activity Sport=\"Biking\">");
  final_file.print("      <Id>");final_file.print(time);final_file.println("</Id>");

  Serial.println("Processing laps...");
  for(int i=0;i<lapSize;i++)
  {
    char lapString[30];
    sprintf(lapString,"processing lap %d of %d", i+1, lapSize);
    Serial.println(lapString);
    writeLapHeader(i, &final_file);
    for(int j=0;j<=laps[i].parts;j++)
    { 
      //transfer lap_%d_%d, i,j to _filename
      memset(lap_name, 0 , 32);
      sprintf(lap_name, "lap_%d_%d", i, j);
      Serial.print("processing file "); Serial.println(lap_name);
      if (!file.open(lap_name, O_READ))
      {
        Serial.println("Error opening : ");Serial.println(lap_name);
        return false;
      }
      memset(buffer, 0, sizeof(buffer)); // Clear buffer
      while ((bytesRead = file.read(buffer, sizeof(buffer)-1)) > 0) {
        int written = final_file.write(buffer, bytesRead);
        memset(buffer, 0, sizeof(buffer)); // Clear buffer
        if (written != bytesRead) {
          Serial.println("Write error!");
          break;
        }
      }
      file.close();
      final_file.flush();
      _storage->remove(lap_name);
    }
    final_file.println("        </Track>");
    final_file.println("      </Lap>"); // Close the Lap element
  }
  final_file.println("    </Activity>");
  final_file.println("  </Activities>");
  final_file.println("</TrainingCenterDatabase>");
  final_file.close();

  return true;
}