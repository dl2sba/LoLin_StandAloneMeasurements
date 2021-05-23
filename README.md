# LoLin_StandAloneMeasurements
## Intro
Simple PoC to show, how an ESP32 can be used to measure and store various sensor data independent of any external memory powered by a small LiPo battery.

## Data aquisition and storage
A number of data points is stored in RTC-memory of the ESP32. If its limit is exceeded the data is appended to the flash filesystem in the ESP32.
The data is stored as CSV-based text file in the file "data.csv".

## Data access
The file can be accessed via an integrated simple webserver. The webserver is started via a hardware pushbutton and listens on http://192.168.4.1.
To revert back to data aquisition in low power mode, the webserver must be terminated via the web interface.

## Credits
Some code is based on the work of Jens Fleischer https://fipsok.de/

