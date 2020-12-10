# Weather Station


## Overview
This Project consists of solar powered weather station based on arduino code adapted to a NodeMCU (ESP8266) microcontroller using two environmental sensors:
1. HTU21D (SparkFun)
2. BMP180 (Bosch)


## Necessary Changes
To use the code provided on this repo, you need to change the ssid and password of the default wireless network, and add the keys to WeatherUnderground API and IFTTT.

The WeatherUndergound is used to POST the weather data, making it public to everyone on the website.

The IFTTT is used with WebHooks to trigger two alerts, one when the battery is overcharged (never happened xD) and the other when the battery is low on voltage.
```c
Line
 08  const char* ssid     = "ID"; 
 09  const char* password = "PASS";
 . 
 .
 .
 17  char ID [] = "ID";
 18  char PASSWORD [] = "PASS";
 .
 .
 .
 25  const String Maker_Key = "KEY";
```

Currently, the weather station is powered by a 5W solar panel using a iPhone 7 Plus Lithium Battery.
