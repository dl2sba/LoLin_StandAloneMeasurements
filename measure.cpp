//
//  (c) Dietmar Krause, DL2SBA 2021
//
#include <arduino.h>
#include <driver/adc.h>
#include <DHT.h>
#include <DHT_U.h>

#include "measure.h"

DHT_Unified dht(DHTPIN, DHTTYPE);



//*******************************************************
//
// VBATT measurement
//
float readVbatt( void ) {
  static float adc_factor = (3.35 / 4096);
  static float adc_divider = ((100.0 + 10.0) / 10.0);
  static float adc_offset = 1.6;

  adc_power_acquire();
  analogReadResolution(12);

  delay(100);

  // do oversampling
  long sum = 0;
  for ( int i = 0; i < BATT_OVERSAMPLE; ++i) {
    sum += analogRead(BATT_PIN);
  }

  adc_power_release();

  // correct sum
  sum /= BATT_OVERSAMPLE;

  if ( sum < LOW_LIMIT ) {
    return 0.0;
  } else {
    // convert according factors
    return sum * adc_factor * adc_divider + adc_offset;
  }
}


//*******************************************************
//
// VSUPP measurement
//
float readVsupp( void ) {
  static float adc_factor = (3.49 / 4096);
  static float adc_divider = ((68.0 + 47.0) / 47.0);
  static float adc_offset = 0.3;

  adc_power_acquire();
  analogReadResolution(12);

  delay(100);

  // do oversampling
  long sum = 0;
  for ( int i = 0; i < SUPP_OVERSAMPLE; ++i) {
    sum += analogRead(SUPP_PIN);
  }

  adc_power_release();

  // correct sum
  sum /= SUPP_OVERSAMPLE;

  if ( sum < LOW_LIMIT ) {
    return 0.0;
  } else {
    // convert according factors
    return sum * adc_factor * adc_divider + adc_offset;
  }
}
