//
//  (c) Dietmar Krause, DL2SBA 2021
//
#include <arduino.h>
#include <driver/adc.h>
#include <driver/rtc_io.h>

#include "data.h"

//  number of times the device woke up
RTC_DATA_ATTR int bootCount = 0;

//*******************************************************
//  measurement buffers
RTC_DATA_ATTR byte bufferInitialized = false;
RTC_DATA_ATTR t_Measurement measurements[MEASUREMENT_BUFFER_SIZE];
RTC_DATA_ATTR int measurementsIndex;



//*******************************************************
//
// dump a measurement point
//
void dumpMeasurement(t_Measurement * meas ) {
  D_PRINTF("%ld / %d\n", meas->time, measurementsIndex);
  D_PRINTF(" Temp=%4.1f\n", meas->temp);
  D_PRINTF(" Humi=%4.1f\n", meas->humi);
  D_PRINTF(" Vsup=%4.2f\n", meas->vsup);
  D_PRINTF(" Vbat=%4.2f\n", meas->vbat);
}
