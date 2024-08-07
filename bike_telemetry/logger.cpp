#include "logger.h"

void logger::addSource(String name, float* data)
{
  if(!_logging)
  {
    //memcpy(sources[_num_sources].name, name, 16);
    sources[_num_sources].name=name;
    sources[_num_sources].data=data;
    _num_sources++;
  }
}

void logger::log_data(DateTime current_time, uint32_t milliseconds)
{
  logFile.open(_filename, FILE_WRITE);
  if(logFile)
  {
    logFile.print(current_time.timestamp(DateTime::TIMESTAMP_FULL));
    logFile.print('.');
    logFile.print(milliseconds);
    for(int i =0; i<_num_sources; i++)
    {
      logFile.print(",");
      logFile.print(*sources[i].data);
    }
    logFile.println(' ');
    logFile.close();
  }else{
    Serial.print(_filename);
    Serial.println(" not found!");
  }
}

void logger::start_logging(DateTime current_time)
{
  _startLog = current_time;
  _PauseTime = 0;
  sprintf(_filename, "%d-%d-%d_%02d-%02d-%02d.csv", current_time.day(),current_time.month(),current_time.year(),current_time.hour(),current_time.minute(),current_time.second());
  
  if(SD.exists(_filename))
  {
    SD.remove(_filename);
  };
  Serial.println(_filename);
  
  logFile.open(_filename, FILE_WRITE);

  logFile.print("Time");
  for(int i =0; i<_num_sources; i++)
  {
    logFile.print(",");
    logFile.print(sources[i].name);
  }
  logFile.println(' ');

  logFile.close();
}

void logger::log(DateTime current_time, uint32_t milliseconds)
{
  _elapsed_time = current_time-_last_log;
  _elapsed_millis = _elapsed_time.totalseconds()*1000 + (milliseconds - _last_millis);
  _totalTime = current_time - _startLog;
  if(_logging){
    if(_elapsed_millis >=500)
    {
      log_data(current_time, milliseconds);
      _last_log = current_time;
      _last_millis = milliseconds;
    }
  }else{
    _PauseTime = _PauseTime + (current_time-_lastPause);
  }
    _lastPause = current_time;
}
void logger::write_tail(){
  logFile.open(_filename, FILE_WRITE);
  if(logFile)
  {
    logFile.println(' ');
    logFile.println(' ');
    logFile.print("Total duration: ");
    logFile.println(elapsedString());
    logFile.close();
  }else{
    Serial.print(_filename);
    Serial.println(" not found!");
  }
}
void logger::write_tail(float f32_avgSpeed, float f32_maxSpeed, float f32_avgCad, float f32_maxCad)
{
  write_tail();
  logFile.open(_filename, FILE_WRITE);
  if(logFile)
  {
    logFile.print("Average speed: ");
    logFile.println(f32_avgSpeed,1);
    logFile.print("Max speed: ");
    logFile.println(f32_maxSpeed,1);
    logFile.print("Average Cadence: ");
    logFile.println(f32_avgCad,1);
    logFile.print("Max Cadence: ");
    logFile.print(f32_maxCad,1);
    logFile.close();
  }else{
    Serial.print(_filename);
    Serial.println(" not found!");
  }
}