# Weather Station


## Overview
This Project consists of solar powered weather station based on arduino code adapted to a NodeMCU (ESP8266) microcontroller using two environmental sensors:
1. HTU21D (SparkFun)
2. BMP180 (Bosch)
3. Generic NTC 10K @ 25 Degree Celsius


## Necessary Changes
To use the code provided on this repo, you need to change the ssid and password of the default wireless network, and add the keys to WeatherUnderground API and IFTTT.

The WeatherUndergound is used to POST the weather data, making it public to everyone on the website.

```c
const char* ssid     = "YOUR_SSID"; 
const char* password = "YOUR_WIFI_PSWD";
 . 
 .
 .
char ID [] = "WEATHER_STATION_ID";
char PASSWORD [] = "WEATHER_STATION_KEY";
```

Currently, the weather station is powered by a 5W solar panel using a iPhone 7 Plus Lithium Battery.
