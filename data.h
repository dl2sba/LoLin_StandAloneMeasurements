//
//  (c) Dietmar Krause, DL2SBA 2021
//
#ifndef _DATA_H
#define _DATA_H

#define DEBUGGING

#ifdef DEBUGGING
#define D_BEGIN(...) Serial.begin(__VA_ARGS__)
#define D_PRINTLN(...) Serial.println(__VA_ARGS__)
#define D_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define D_BEGIN(...)
#define D_PRINTLN(...)
#define D_PRINTF(...)
#endif


//*******************************************************
// Pin which is connected to the LED
#define LED_PIN 22

//  seconds between measurement
#define SLEEP_IN_SECONDS 60    

//  factor from seconds to microseconds
#define SLEEP_FACTOR     1000000

extern RTC_DATA_ATTR int bootCount;

typedef struct {
  time_t time;
  float vbat;
  float vsup;
  float temp;
  float humi;
} t_Measurement;

#define MEASUREMENT_BUFFER_SIZE  20

extern RTC_DATA_ATTR byte bufferInitialized;
extern RTC_DATA_ATTR t_Measurement measurements[];
extern RTC_DATA_ATTR int measurementsIndex;

extern void dumpMeasurement(t_Measurement * meas );

#define CSV_DATA_FILENAME "/data.csv"
#define CSV_DECIMAL_SEPERATOR ','
#define CSV_VALUE_SEPERATOR   ';'

#endif
