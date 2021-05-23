//
//  (c) Dietmar Krause, DL2SBA 2021
//
#ifndef _MEASURE_H
#define _MEASURE_H

#define SUPP_PIN            34
#define SUPP_OVERSAMPLE     10

#define BATT_PIN            35
#define BATT_OVERSAMPLE     10

#define LOW_LIMIT           10

extern float readVbatt( void );
extern float readVsupp( void );


//*******************************************************
//  DHT sensor connections
//
// Pin which is connected to the DHT sensor.
#define DHTPIN            17
#define DHTTYPE           DHT22     // DHT 22 (AM2302)
extern DHT_Unified dht;

#endif
