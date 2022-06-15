#ifndef BLSS_H
#define BLSS_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiUdp.h>
#include <LittleFS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

#include "fcn_declare.h"
#include "const.h"
#include "credentials.h"



#define EEPSIZE 200  //Maximum is 4096

#ifndef CLIENT_SSID
  #define CLIENT_SSID DEFAULT_CLIENT_SSID
#endif

#ifndef CLIENT_PASS
  #define CLIENT_PASS ""
#endif

#ifndef BLSS_VERSION
  #define BLSS_VERSION "dev"
#endif

// for the esp32 it is best to use the ADC1: GPIO32 - GPIO39
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html
#ifndef BATTERY_MEASUREMENT_PIN
  #ifdef ARDUINO_ARCH_ESP32
    #define BATTERY_MEASUREMENT_PIN 32
  #else //ESP8266 boards
    #define BATTERY_MEASUREMENT_PIN A0
  #endif
#endif

// esp32 has a 12bit adc resolution
// esp8266 only 10bit
#ifndef BATTERY_ADC_PRECISION
  #ifdef ARDUINO_ARCH_ESP32
    // 12 bits
    #define BATTERY_ADC_PRECISION 4095.0f
  #else
    // 10 bits
    #define BATTERY_ADC_PRECISION 1024.0f
  #endif
#endif

// the frequency to check the battery, 30 sec
#ifndef BATTERY_MEASUREMENT_INTERVAL
  #define BATTERY_MEASUREMENT_INTERVAL 30000
#endif

// the frequency to check the Sensor, 100ms => 0,100 sec
#ifndef SENSOR_MEASUREMENT_INTERVAL
  #define SENSOR_MEASUREMENT_INTERVAL 100
#endif

// default for 18650 battery
// https://batterybro.com/blogs/18650-wholesale-battery-reviews/18852515-when-to-recycle-18650-batteries-and-how-to-start-a-collection-center-in-your-vape-shop
// Discharge voltage: 2.5 volt + .1 for personal safety
#ifndef BATTERY_MIN_VOLTAGE
  #define BATTERY_MIN_VOLTAGE 2.6f
#endif

#ifndef BATTERY_MAX_VOLTAGE
  #define BATTERY_MAX_VOLTAGE 4.2f
#endif

#ifndef BATTERY_CAPACITY
  #define BATTERY_CAPACITY 550
#endif

// GLOBAL VARIABLES
// both declared and defined in header (solution from http://www.keil.com/support/docs/1868.htm)
//
//e.g. byte test = 2 becomes BLSS_GLOBAL byte test _INIT(2);
//     int arr[]{0,1,2} becomes BLSS_GLOBAL int arr[] _INIT_N(({0,1,2}));
#ifndef DEFINE_GLOBAL_VARS
    #define BLSS_GLOBAL extern
    #define _INIT(x)
    #define _INIT_N(x)
#else
    #define BLSS_GLOBAL
    #define _INIT(x)  = x
#endif

BLSS_GLOBAL char clientSSID[33] _INIT(CLIENT_SSID);
BLSS_GLOBAL char clientPass[65] _INIT(CLIENT_PASS);

BLSS_GLOBAL char apSSID[33] _INIT("BlubberLounge StatTrak™");
BLSS_GLOBAL char apPass[65]  _INIT(DEFAULT_AP_PASS);
BLSS_GLOBAL char otaPass[33] _INIT(DEFAULT_OTA_PASS);

#endif // BLSS_H