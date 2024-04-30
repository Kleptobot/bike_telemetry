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

void logger::log_data(DateTime current_time)
{
  logFile = SD.open("datalog.csv", FILE_WRITE);
  if(logFile)
  {
    logFile.print(current_time.timestamp(DateTime::TIMESTAMP_FULL));
    for(int i =0; i<_num_sources; i++)
    {
      logFile.print(",");
      logFile.print(*sources[i].data);
    }
    logFile.println(' ');
    logFile.close();
  }
}

void logger::start_logging()
{
  if(SD.exists("datalog.csv"))
  {
    SD.remove("datalog.csv");
  };
  
  logFile = SD.open("datalog.csv", FILE_WRITE);

  logFile.print("Time");
  for(int i =0; i<_num_sources; i++)
  {
    logFile.print(",");
    logFile.print(sources[i].name);
  }
  logFile.println(' ');

  logFile.close();
}

void logger::log(DateTime current_time)
{
  _elapsed_time = (current_time-_last_log);
  if(_logging){
    if(_elapsed_time.totalseconds()>=1)
    {
      log_data(current_time);
      _last_log = current_time;
    }
  }
}