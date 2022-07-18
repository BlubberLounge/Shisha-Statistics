#define DEFINE_GLOBAL_VARS
#include "blss.h"

// ADC_MODE(ADC_VCC);
// + ESP.getVcc();

// Create AsyncWebServer object on port 80
// AsyncWebServer server(80);
// AsyncWebSocket ws("/ws");

// This is an array of leds.  One item for each led in your strip.
CRGB leds[16];

uint8_t connectionCounter = 0;
char testSSID[33];
char testPASS[65];

// battery read time
// unsigned long nextBatteryReadTime = 0;
// unsigned long lastBatteryReadTime = 0;
// sensor read time
unsigned long nextSensorReadTime = 0;
unsigned long lastSensorReadTime = 0;
// timer
unsigned long startTimer = 0;
unsigned long endTimer = 0;
// raw analog reading 
float rawValue = 0.0;
// calculated voltage            
float voltage = 0.0;
float tmpVoltage = 0.0;
// mapped battery level based on voltage
// long batteryLevel = 0;
// bool batteryIsCharging = false;

uint8_t sensorValue = 0;
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

/*
 * Read all configuration from flash
 */
extern void loadSettingsFromEEPROM()
{
  readStringFromEEPROM(  0, clientSSID, 32);
  readStringFromEEPROM( 32, clientPass, 64);
  readStringFromEEPROM( 96,     apSSID, 32);
  readStringFromEEPROM(160,     apPass, 64);
}

// void setNotifyClients(bool notify)
// {
//   if(notifyClients)
//     return;
  
//   notifyClients = notify;
// }

// void notify()
// {
//   StaticJsonDocument<JSON_OBJECT_SIZE(10)> json;

//   JsonObject battery = json.createNestedObject("battery");
//   battery["voltage"] = voltage;
//   battery["capacity"] = BATTERY_CAPACITY;
//   battery["percent"] = batteryLevel;
//   battery["isCharging"] = batteryIsCharging;
//   battery["nextRead"] = nextBatteryReadTime - millis();

//   json["airflow"] = sensorValue;
//   json["startTimer"] = startTimer;
//   json["endTimer"] = endTimer;
//   json["time"] = millis();

//   char data[200];
//   size_t len = serializeJson(json, data);
//   ws.textAll(data, len);
// }

// void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) 
// {
//   AwsFrameInfo *info = (AwsFrameInfo*)arg;
//   if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
//     data[len] = 0;
//     if (strcmp((char*)data, "update") == 0) {
//       notify();
//     }
//   }
// }

// void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
// {
//     switch (type) {
//       case WS_EVT_CONNECT:
//         Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
//         break;
//       case WS_EVT_DISCONNECT:
//         Serial.printf("WebSocket client #%u disconnected\n", client->id());
//         break;
//       case WS_EVT_DATA:
//         handleWebSocketMessage(arg, data, len);
//         break;
//       case WS_EVT_PONG:
//       case WS_EVT_ERROR:
//         Serial.printf("WebSocket client #%u Error\n", client->id());
//         break;
//   }
// }

// void readSensor()
// {
//   uint8_t tmpSensorValue = digitalRead(SENSOR_PIN);

//   if(tmpSensorValue != sensorValue) {

//     sensorValue = tmpSensorValue;

//     setNotifyClients(true);
    
//     if(sensorValue == 1) {
//       startTimer = millis();
//     } else if(sensorValue == 0) {
//       endTimer = millis();
//     }

//   }
// }


// void readBatteryLevel()
// {
//   // read battery raw input
//   rawValue = analogRead(BATTERY_MEASUREMENT_PIN);

//   // calculate the voltage     
//   voltage = (rawValue / BATTERY_ADC_PRECISION) * BATTERY_MAX_VOLTAGE;
//   // check if voltage is within specified voltage range
//   voltage = voltage < BATTERY_MIN_VOLTAGE || voltage > BATTERY_MAX_VOLTAGE ? -1.0f : voltage;

//   // translate battery voltage into percentage
//   /*
//   the standard "map" function doesn't work
//   https://www.arduino.cc/reference/en/language/functions/math/map/  notes and warnings at the bottom
//   */
//   batteryLevel = mapf(voltage, BATTERY_MIN_VOLTAGE, BATTERY_MAX_VOLTAGE, 0, 100);

//   setNotifyClients(true);
// }


// String processor(const String& var)
// {
//   // return "no data.";
  
//   if(var == "BATTERY_VOLTAGE") {
//     return "0.00 v";
//   }
//   else if(var == "BATTERY_LEVEL") {
//     return "0 &#x25;";
//   }
//   else if(var == "BATTERY_CAPACITY") {
//     return String(BATTERY_CAPACITY) + "mAh (" + String((3.7f * BATTERY_CAPACITY)/1000) + " Wh)";
//   }
//   else if(var == "AIRFLOW") {
//     return String(sensorValue);
//   }
//   else if(var == "VERSION") {
//     return String(BLSS_VERSION);
//   }
//   else if(var == "ESP_CHIPID") {
//     return String(ESP.getChipId());
//   }
//   else if(var== "WIFI_MAC") {
//     return String(WiFi.macAddress());
//   }

//   return String();
// }


// void notFound(AsyncWebServerRequest *request)
// {
//   request->send(404, "text/plain", "Not found");
// }

void setup()
{
  // sanity check delay - allows reprogramming if accidently blowing power w/leds
  delay(2000);

  Serial.begin(115200);
  Serial.println("Booting");
  
  EEPROM.begin(EEPSIZE);
  loadSettingsFromEEPROM();
  EEPROM.end();

  Serial.println("loading settings");

  WiFi.mode(WIFI_STA);
  WiFi.begin(BL_SSID, BL_PASSWORD);
  // WiFi.begin(BL_SSID, BL_PASSWORD);

  // Initialize SPIFFS
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.print("Connection Failed! SSID: ");
    Serial.println(clientSSID);

    if(connectionCounter >= 5) {
      Serial.println("Starting a Access Point...");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(apSSID);

      // mobilephone AP
      // WiFi.begin("MAAAAAXimum Internet", "12345678");

      IPAddress IP = WiFi.softAPIP();
      Serial.print("AP IP address: ");
      Serial.println(IP);
      break;
      // Serial.println("Connection Failed! Rebooting...");
      // ESP.restart();
    }

    delay(2500);
    connectionCounter++;
  }

  // pinMode(BATTERY_MEASUREMENT_PIN, INPUT);
  // pinMode(SENSOR_PIN, INPUT);

  // nextBatteryReadTime = millis() + BATTERY_MEASUREMENT_INTERVAL;
  // lastBatteryReadTime = millis();

  nextSensorReadTime = millis() + SENSOR_MEASUREMENT_INTERVAL;
  lastSensorReadTime = millis();

  FastLED.setMaxPowerInVoltsAndMilliamps( 5, 500);
  FastLED.addLeds<WS2811, LED_PIN, GRB>(leds, NUM_LEDS)
    .setCorrection(TypicalLEDStrip);;

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
  // ws.onEvent(onEvent);
  // server.addHandler(&ws);

  /* ================================================
   * |
   * |  Web Server
   * |
   * ================================================
   */

  // root route
  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  //   // request->send_P(200, "text/html", index_html, processor);
  //   request->send(LittleFS, "/index.html", "text/html", false, processor);
  // });
  
  // // style route
  // server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send(LittleFS, "/style.css", "text/css");
  // });

  // // style route
  // server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send(LittleFS, "/script.js", "text/javascript");
  // });

  // // add WiFi credentails
  // server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){
  //   uint8_t code = 200;

  //   EEPROM.begin(EEPSIZE);
  //   for(uint8_t i = 0; i <= 32; i++ ) {
  //     EEPROM.write(i, request->arg("ssid")[i]);
  //   }
  //   for(uint8_t i = 0; i <= 64; i++ ) {
  //     EEPROM.write(i+32, request->arg("pass")[i]);
  //   }
  //   EEPROM.end();

  //   Serial.print("SSID: ");
  //   Serial.println(clientSSID);
  //   Serial.print("Pass: ");
  //   Serial.println(clientPass);

  //   request->send(code, "text/html", "Restarting...");
  //   // ESP.reset();
  // });

  // server.onNotFound(notFound);
  
  // Start OTA service
  ArduinoOTA.begin();

  // Start server
  // server.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  ArduinoOTA.handle();

  // Move a single white led 
  for(int i=0; i < 16; i++) {
    // Turn our current led on to white, then show the leds
    leds[i] = CRGB::Green;    
  }

  FastLED.show();
  delay(500);

  for(int i=0; i < 16; i++) {
    // Turn our current led on to white, then show the leds
    leds[i] = CRGB::Black;    
  }

  FastLED.show();
  delay(1000);


  // check the battery level every BATTERY_MEASUREMENT_INTERVAL (ms)
  // if (millis() >= nextBatteryReadTime) {

  //   nextBatteryReadTime = millis() + BATTERY_MEASUREMENT_INTERVAL;
  //   lastBatteryReadTime = millis();

  //   readBatteryLevel();
  // }
  
  
  // if (millis() >= nextSensorReadTime) {

  //   nextSensorReadTime = millis() + SENSOR_MEASUREMENT_INTERVAL;
  //   lastSensorReadTime = millis();

  //   readSensor();
  // }

  // if(notifyClients) {
  //   notify();

  //   notifyClients = false;
  // }
}
