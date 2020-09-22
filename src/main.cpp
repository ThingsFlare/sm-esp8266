#define ARDUINOJSON_USE_LONG_LONG 1
#include <Arduino.h>
#include "Thing.h"
#include "WebThingAdapter.h"
// #include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP_DoubleResetDetector.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <LittleFS.h>

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 10
// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

char thing_id[50];
char thing_name[50];
const int SENSOR_PIN = 0;

DoubleResetDetector *drd;
AsyncWebServer server(80);
DNSServer dns;
WebThingAdapter *adapter;
const char *apName;

const char *soilMoistureSensorTypes[] = {"Soil Moisture", nullptr};
ThingDevice soilMoistureSensor("sm", "Soil Moisture", soilMoistureSensorTypes);
ThingProperty moistureLevel("moistureLevel", "", NUMBER, "MoistureLevelProperty");

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

//callback notifying us of the need to save config
void onConfigMode(AsyncWiFiManager *manager)
{
  drd->stop();
}

void saveConfig(DynamicJsonDocument &json)
{
  Serial.println("saving config");

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile)
  {
    Serial.println("failed to open config file for writing");
  }

  serializeJson(json, Serial);

  serializeJson(json, configFile);
  configFile.close();
}

void configure(bool resetConfig)
{
  strcpy(thing_id, apName);

  if (LittleFS.begin())
  {
    Serial.println("mounted file system");
    if (LittleFS.exists("/config.json"))
    {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = LittleFS.open("/config.json", "r");
      if (configFile)
      {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument json(100);
        DeserializationError error = deserializeJson(json, buf.get());

        serializeJson(json, Serial);
        if (error)
        {
          Serial.println("failed to load json config");
        }
        else
        {
          Serial.println("\nparsed json");
          strcpy(thing_id, json["thing_id"]);
          strcpy(thing_name, json["thing_name"]);
        }
      }
    }
  }
  else
  {
    Serial.println("failed to mount FS");
  }

  // Create custom parameters to be displayed in wifi configuration page
  AsyncWiFiManagerParameter thing_id_param("thing_id", "Thing Id", thing_id, 50);
  AsyncWiFiManagerParameter thing_name_param("thing_name", "Thing Display Name", thing_name, 50);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  AsyncWiFiManager wifiManager(&server, &dns);

  wifiManager.setAPCallback(onConfigMode);
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.addParameter(&thing_id_param);
  wifiManager.addParameter(&thing_name_param);
  if (resetConfig)
  {
    wifiManager.resetSettings();
    drd->stop();
    ESP.reset();
  }

  if (!wifiManager.autoConnect(apName))
  {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  strcpy(thing_id, thing_id_param.getValue());
  strcpy(thing_name, thing_name_param.getValue());

  if (thing_name_param.getValueLength() < 1)
  {
    strcpy(thing_name, thing_id_param.getValue());
  }

  //save the custom parameters to FS
  if (shouldSaveConfig)
  {
    Serial.println("saving config");

    DynamicJsonDocument json(150);
    json["thing_id"] = thing_id;
    json["thing_name"] = thing_name;

    saveConfig(json);
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
}

void setup(void)
{
  String adapterName = "SM-" + String(ESP.getChipId());
  apName = adapterName.c_str();

  Serial.begin(115200);

  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
  bool resetRequested = drd->detectDoubleReset();
  configure(resetRequested);

  adapter = new WebThingAdapter(adapterName, WiFi.localIP());
  soilMoistureSensor.id = thing_id;
  soilMoistureSensor.title = thing_name;
  soilMoistureSensor.addProperty(&moistureLevel);
  adapter->addDevice(&soilMoistureSensor);
  adapter->begin();
  Serial.println("HTTP server started");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/things/");
  Serial.println(soilMoistureSensor.id);
}

void loop(void)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    drd->stop();
    delay(10000);
    ESP.reset();
    return;
  }

  Serial.println("Getting soil moisture reading...");
  int moistureReading = analogRead(SENSOR_PIN);
  Serial.println("Soil moisture reading: ");
  Serial.print(moistureReading);
  long moisture = map(moistureReading, 800, 350, 0, 100);
  Serial.println("Soil moisture mapped: ");
  Serial.print(moisture);
  ThingPropertyValue soilMoistureValue;
  soilMoistureValue.number = moisture;
  moistureLevel.setValue(soilMoistureValue);
  adapter->update();
  delay(5000);
}
