//*******************************************************
//
//  Compile with LOLIN D32
//
//  Set wakeup source to button
//  
//  include library https://github.com/adafruit/DHT-sensor-library
//
//  (c) Dietmar Krause, DL2SBA 2021
//
#include <DHT.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <driver/rtc_io.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <SPIFFS.h>

#include "data.h"
#include "measure.h"
#include "AccessPointHandler.h"



//*******************************************************
//
// temperature and humidity measurement
//
void handleMeasurements(void ) {
  dht.begin();
  sensors_event_t event;
  dht.temperature().getEvent(&event);

  if (isnan(event.temperature)) {
    D_PRINTLN("Error reading temperature!");
  } else {
    float temp = event.temperature;

    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      D_PRINTF("Error reading humidity!\n");
    } else {
      float humi = event.relative_humidity;

      // first call ever?
      if ( !bufferInitialized ) {
        // yes init the buffers
        measurementsIndex = 0;
        bufferInitialized = true;
        D_PRINTLN("Buffers initialized");
      }


      measurements[measurementsIndex].vsup = readVsupp();
      measurements[measurementsIndex].vbat = readVbatt();
      measurements[measurementsIndex].temp = temp;
      measurements[measurementsIndex].humi = humi;
      measurements[measurementsIndex].time = time(NULL);

      dumpMeasurement(& measurements[measurementsIndex]);

      ++measurementsIndex;
    }
  }
}

//*******************************************************
// Function that prints the reason by which ESP32 has been awaken from sleep
//
// constants in https://github.com/espressif/esp-idf/blob/v3.2/components/esp32/include/esp_sleep.h#L57-L67
//
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason)   {
    case ESP_SLEEP_WAKEUP_UNDEFINED: D_PRINTLN("Wakeup caused by reset"); break;
    case ESP_SLEEP_WAKEUP_ALL:       D_PRINTLN("Not a wakeup cause, used to disable all wakeup sources with esp_sleep_disable_wakeup_source"); break;
    case ESP_SLEEP_WAKEUP_EXT0:      D_PRINTLN("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1:      D_PRINTLN("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER:     D_PRINTLN("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:  D_PRINTLN("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP:       D_PRINTLN("Wakeup caused by ULP program"); break;
    case ESP_SLEEP_WAKEUP_GPIO:      D_PRINTLN("Wakeup caused by GPIO (light sleep only)"); break;
    case ESP_SLEEP_WAKEUP_UART:      D_PRINTLN("Wakeup caused by UART (light sleep only)"); break;
    default:                         D_PRINTLN("Unknown wakeup cause"); break;
  }
}

/***************************************
**
**
**/
void goIntoDeepSleep() {
  D_PRINTF("Going into deepsleep mode for %ds...\n", SLEEP_IN_SECONDS);
  D_PRINTLN("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n\n");

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  esp_wifi_stop();
  esp_wifi_deinit();

  btStop();
  esp_bt_controller_disable();

  //Configure GPIO33 as ext0 wake up source for HIGH logic level
  rtc_gpio_pullup_en(GPIO_NUM_13);
  rtc_gpio_pulldown_dis(GPIO_NUM_13);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);

  // Configure the timer to wake us up!
  esp_sleep_enable_timer_wakeup(SLEEP_IN_SECONDS * SLEEP_FACTOR);

  // Go to sleep! Zzzz
  esp_deep_sleep_start();
}


/***************************************
**
**  format a number with the given decimal separator
*  param  the number to format
*  numDigits  the number of total digits
*  fracDigits the number of digits after the decimal seperator
*  decSep the decimal seperator to use
*  
*  return the reference to the internal static buffer
**/
const char * formatFloat(float param, int numDigits, int fracDigits, char decSep) {
  static char formatBuffer[20];
  sprintf(formatBuffer, "%*.*f", numDigits, fracDigits, param);
  if ( decSep != '.' ) {
    char * cp = formatBuffer;
    while ( *cp) {
      if (*cp == '.') {
        *cp = decSep;
      }
      ++cp;
    }
  }
  return formatBuffer;
}

/***************************************
*
* write the data buffers into the data file
* 
* If the file doesn't exist create it and place the CSV header at the beginning.
* If the file exists simply append to the end of the file
*
**/
void appendBufferToFile( void ) {
  File f;
  // check if file exists
  if (SPIFFS.exists (CSV_DATA_FILENAME) )  {
    //  yes
    //  open for append
    f = SPIFFS.open(CSV_DATA_FILENAME, FILE_APPEND);
  } else {
    //  no
    //  create it
    f = SPIFFS.open(CSV_DATA_FILENAME, FILE_WRITE);
    //  write CSV header
    f.printf("timestamp%cVbattery%cVsupply%ctemperature%chumidity\n", CSV_VALUE_SEPERATOR, CSV_VALUE_SEPERATOR, CSV_VALUE_SEPERATOR, CSV_VALUE_SEPERATOR);
    D_PRINTF("new CSV data file [%s] created\n", CSV_DATA_FILENAME);
  }
  for ( int i = 0; i < measurementsIndex; ++i ) {
    f.printf("%ld%c", measurements[i].time, CSV_VALUE_SEPERATOR);
    f.printf("%s%c",  formatFloat(measurements[i].vbat, 4, 2, CSV_DECIMAL_SEPERATOR), CSV_VALUE_SEPERATOR);
    f.printf("%s%c",  formatFloat(measurements[i].vsup, 4, 2, CSV_DECIMAL_SEPERATOR), CSV_VALUE_SEPERATOR);
    f.printf("%s%c",  formatFloat(measurements[i].temp, 4, 2, CSV_DECIMAL_SEPERATOR), CSV_VALUE_SEPERATOR);
    f.printf("%s\n",  formatFloat(measurements[i].humi, 4, 2, CSV_DECIMAL_SEPERATOR));
  }
  f.close();
  bufferInitialized = false;
  D_PRINTF("%d record%s appended to file\n", measurementsIndex, (measurementsIndex==1)?"":"s");
}

/***************************************
**
**
**/
void setup() {
  D_BEGIN(115200);
  pinMode(LED_PIN, OUTPUT);

  //Increment boot number
  ++bootCount;

  D_PRINTF("\n\n===================================================================================\n");
  D_PRINTF("Build file   [%s]\n", __FILE__);
  D_PRINTF("Build date   [%s %s]\n", __DATE__, __TIME__ );
  D_PRINTF("Boot number  [%d]\n", bootCount);

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  //  start the FFS
  SPIFFS.begin(true);
  D_PRINTLN("Flash file system started");

  //  wakereason button?
  if ( esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0 ) {
    //  yes
    D_PRINTF("ESP_SLEEP_WAKEUP_EXT0 detected. Starting AP\n");
    //  write all intermediate data to the file
    appendBufferToFile();

    //  start access point
    startAP( );
  }

  //  measure data
  handleMeasurements();

  //  measurement buffer full?
  if ( measurementsIndex == MEASUREMENT_BUFFER_SIZE ) {
    //  yes
    // append buffer to data file
    appendBufferToFile();
  }

  //Go to sleep now
  goIntoDeepSleep();
}


/***************************************
**
**  should never happen
**/
void loop() {}
