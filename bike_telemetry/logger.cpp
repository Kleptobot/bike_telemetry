#include "logger.h"

void logger::addSource(String name, float* data)
{
  if(!_logging)
  {
    sources[_num_sources].name=name;
    sources[_num_sources].data=data;
    _num_sources++;
  }
}

void logger::log_data(DateTime current_time, uint32_t milliseconds)
{
  dataFile.open(_filename, FILE_WRITE);
  if(dataFile)
  {
    dataFile.print(current_time.timestamp(DateTime::TIMESTAMP_FULL));
    dataFile.print('.');
    dataFile.print(milliseconds);
    for(int i =0; i<_num_sources; i++)
    {
      dataFile.print(",");
      dataFile.print(*sources[i].data);
    }
    dataFile.println(' ');
    dataFile.close();
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
  
  dataFile.open(_filename, FILE_WRITE);

  dataFile.print("Time");
  for(int i =0; i<_num_sources; i++)
  {
    dataFile.print(",");
    dataFile.print(sources[i].name);
  }
  dataFile.println(' ');

  dataFile.close();
}

void logger::log(DateTime current_time, uint32_t milliseconds)
{
  _elapsed_time = current_time-_last_log;
  _elapsed_millis = _elapsed_time.totalseconds()*1000 + (milliseconds - _last_millis);
  _totalTime = current_time - _startLog;
  if(_logging){
    if(_elapsed_millis >=_interval)
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
  dataFile.open(_filename, FILE_WRITE);
  if(dataFile)
  {
    dataFile.println(' ');
    dataFile.println(' ');
    dataFile.print("Total duration: ");
    dataFile.println(elapsedString());
    dataFile.close();
  }else{
    Serial.print(_filename);
    Serial.println(" not found!");
  }
}
void logger::write_tail(float f32_avgSpeed, float f32_maxSpeed, float f32_avgCad, float f32_maxCad)
{
  write_tail();
  dataFile.open(_filename, FILE_WRITE);
  if(dataFile)
  {
    dataFile.print("Average speed: ");
    dataFile.println(f32_avgSpeed,1);
    dataFile.print("Max speed: ");
    dataFile.println(f32_maxSpeed,1);
    dataFile.print("Average Cadence: ");
    dataFile.println(f32_avgCad,1);
    dataFile.print("Max Cadence: ");
    dataFile.print(f32_maxCad,1);
    dataFile.close();
  }else{
    Serial.print(_filename);
    Serial.println(" not found!");
  }
}