#pragma once
#ifndef logger_H
#define logger_H

#include <Arduino.h>
#include <SPI.h>
#include "SdFat.h"
#include "sdios.h"
#include <Wire.h>
#include <RTClib.h>

#define MAX_SOURCES 10
#define SD_CS       D3      // SD CS

// Struct containing peripheral info
typedef struct
{
  String name;

  float* data;

} data_source_t;

class logger {

  private:
    File32 logFile;
    data_source_t sources[MAX_SOURCES];
    uint16_t _num_sources;
    TimeSpan _interval, _elapsed_time, _totalTime, _PauseTime;
    DateTime _last_log, _startLog, _lastPause;
    uint32_t _last_millis, _elapsed_millis;
    bool _logging;
    void log_data(DateTime current_time, uint32_t milliseconds);
    char _filename[32];

  public:
    SdFat32 SD;

    logger()
    {
      logger(500);
    };

    logger(uint16_t interval)
    {
      set_log_interval(interval);

      // initialise SD card
      //if (!SD.begin(SD_CS)) {
      //  //SD card did not init
      //  _SD_OK = 0;
      //} else {
      //  _SD_OK = 1;
      //}
//
    };

    void addSource(String name, float* data);

    void set_log_interval(uint16_t interval)
    {
      //_interval = interval;
      ;
    };

    void start_logging(DateTime current_time);
    void play_logging(){_logging=1;};
    void pause_logging(){_logging=0;};
    void write_tail();
    void write_tail(float f32_avgSpeed, float f32_maxSpeed, float f32_avgCad, float f32_maxCad);
    void playPause_logging(){_logging=!_logging;};

    void log(DateTime current_time, uint32_t milliseconds);
    bool logging(){return _logging;};
    TimeSpan elapsed(){return _totalTime-_PauseTime;};
    String elapsedString()
    {
      TimeSpan ts = elapsed();
      return String(ts.hours())+":"+String(ts.minutes())+":"+String(ts.seconds());
    }
    char* filename(){return _filename;};

};


#endif /* logger_H */