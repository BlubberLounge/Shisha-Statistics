#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiUdp.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

#include "credentials.h"

// pin defaults
#define SENSOR_PIN 14

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

// the frequency to check the Sensor, 1 sec
#ifndef SENSOR_MEASUREMENT_INTERVAL
  #define SENSOR_MEASUREMENT_INTERVAL 250
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


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

uint8_t connectionCounter = 0;

// battery read time
unsigned long nextBatteryReadTime = 0;
unsigned long lastBatteryReadTime = 0;
// sensor read time
unsigned long nextSensorReadTime = 0;
unsigned long lastSensorReadTime = 0;
// timer
unsigned long timer = 0; 
// raw analog reading 
float rawValue = 0.0;
// calculated voltage            
float voltage = 0.0;
// mapped battery level based on voltage
long batteryLevel = 0;

uint8_t sensorValue = 0;
uint8_t tmpSensorValue = 0;
bool notifyClients = false;


// custom map function
double mapf(double x, double in_min, double in_max, double out_min, double out_max) 
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float truncate(float val, byte dec) 
{
  float x = val * pow(10, dec);
  float y = round(x);
  float z = x - y;
  if ((int)z == 5)
  {
      y++;
  }
  x = y / pow(10, dec);
  return x;
}

void setNotifyClients(bool notify)
{
  if(notifyClients)
    return;
  
  notifyClients = notify;
}

void notify()
{
  const uint8_t size = JSON_OBJECT_SIZE(5);
  StaticJsonDocument<size> json;
  json["battery_level"] = batteryLevel;
  json["battery_voltage"] = voltage;
  json["next_battery_read"] = nextBatteryReadTime;
  json["airflow"] = sensorValue;
  json["time"] = millis();

  char data[200];
  size_t len = serializeJson(json, data);
  ws.textAll(data, len);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) 
{
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "update") == 0) {
      notify();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        Serial.printf("WebSocket client #%u Error\n", client->id());
        break;
  }
}


void readSensor()
{
  tmpSensorValue = digitalRead(SENSOR_PIN);
  setNotifyClients(tmpSensorValue != sensorValue);
  sensorValue = tmpSensorValue;
}


void readBatteryLevel()
{
  // read battery raw input
  rawValue = analogRead(BATTERY_MEASUREMENT_PIN);

  // calculate the voltage     
  voltage = (rawValue / BATTERY_ADC_PRECISION) * BATTERY_MAX_VOLTAGE ;
  // check if voltage is within specified voltage range
  voltage = voltage < BATTERY_MIN_VOLTAGE || voltage > BATTERY_MAX_VOLTAGE ? -1.0f : voltage;

  // translate battery voltage into percentage
  /*
  the standard "map" function doesn't work
  https://www.arduino.cc/reference/en/language/functions/math/map/  notes and warnings at the bottom
  */
  batteryLevel = mapf(voltage, BATTERY_MIN_VOLTAGE, BATTERY_MAX_VOLTAGE, 0, 100);

  setNotifyClients(true);
}


String processor(const String& var)
{
  return "no Data";
  
  // if(var == "BATTERY_VOLTAGE") {
  //   return String(voltage);
  // }
  // else if(var == "BATTERY_LEVEL") {
  //   return String(batteryLevel);
  // }
  // else if(var == "AIRFLOW") {
  //   return String(sensorValue);
  // }

  return String();
}


void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Booting");

  WiFi.mode(WIFI_STA);
  WiFi.begin(BL_SSID, BL_PASSWORD);

  // Initialize SPIFFS
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.print("Connection Failed! SSID: ");
    Serial.println(BL_SSID);

    if(connectionCounter >= 5) {
      // Serial.println("Starting a Access Point...");
      // WiFi.mode(WIFI_AP);
      // WiFi.softAP("BlubberLounge StatTrak™");

      // mobilephone AP
      WiFi.begin("MAAAAAXimum Internet", "12345678");

      // IPAddress IP = WiFi.softAPIP();
      // Serial.print("AP IP address: ");
      // Serial.println(IP);
      break;
      // Serial.println("Connection Failed! Rebooting...");
      // ESP.restart();
    }

    delay(5000);
    connectionCounter++;
  }

  pinMode(BATTERY_MEASUREMENT_PIN, INPUT);
  pinMode(SENSOR_PIN, INPUT);

  nextBatteryReadTime = millis() + BATTERY_MEASUREMENT_INTERVAL;
  lastBatteryReadTime = millis();

  nextSensorReadTime = millis() + SENSOR_MEASUREMENT_INTERVAL;
  lastSensorReadTime = millis();

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  // Websocket  
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  /* ================================================
   * |
   * |  Web Server
   * |
   * ================================================
   */

  // root route
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    // request->send_P(200, "text/html", index_html, processor);
    request->send(LittleFS, "/index.html", "text/html", false, processor);
  });
  
  // style route
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/style.css", "text/css");
  });

  // style route
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/script.js", "text/javascript");
  });

  server.onNotFound(notFound);
  
  // Start OTA service
  ArduinoOTA.begin();

  // Start server
  server.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  ArduinoOTA.handle();


  // check the battery level every BATTERY_MEASUREMENT_INTERVAL (ms)
  if (millis() >= nextBatteryReadTime) {

    nextBatteryReadTime = millis() + BATTERY_MEASUREMENT_INTERVAL;
    lastBatteryReadTime = millis();

    readBatteryLevel();
  }
  
  
  if (millis() >= nextSensorReadTime) {

    nextSensorReadTime = millis() + SENSOR_MEASUREMENT_INTERVAL;
    lastSensorReadTime = millis();

    readSensor();
  }

  if(notifyClients) {
    notify();

    notifyClients = false;
  }
}
