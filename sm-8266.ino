// #define ARDUINO 100

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
// #include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <DoubleResetDetector.h>
#include <ThingsBoard.h>
#include <ArduinoJson.h>

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 10

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

#define TOKEN ""

char thingsboardServer[] = "";

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

WiFiClient wifiClient;

//Setup configuration portal on demand
void startConfigPortal(const char *apName, AsyncWiFiManager *manager);
void configModeCallback(AsyncWiFiManager *myWiFiManager);
boolean setupThings();
void updateThings();
boolean reconnect();

struct SoilMoistureSensorResult
{
    long moisture;
    int moistureReading;
};

class CustomLogger
{
public:
    static void log(const char *error)
    {
        Serial.print("[Custom Logger] ");
        Serial.println(error);
    }
};

ThingsBoardSized<128, 32, CustomLogger> tbClient(wifiClient);

//Read Soil moisture Reading
SoilMoistureSensorResult readSoilMoisture();

int sensor_pin = 0;

void setup()
{

    Serial.begin(115200);
    AsyncWebServer server(80);
    DNSServer dns;
    AsyncWiFiManager wifiManager(&server, &dns);
    wifiManager.setDebugOutput(true);

    String apNameStr = "SM-ESP-" + String(ESP.getChipId());
    const char *apName = apNameStr.c_str();
    if (drd.detectDoubleReset())
    {
        Serial.println("Starting setup mode using AP: ");
        Serial.print(apName);
        startConfigPortal(apNameStr.c_str(), &wifiManager);
    }

    if (wifiManager.autoConnect(apName))
    {
        delay(5000);
        if (!reconnect())
        {
            Serial.println("Connection to thingsboard failed...");
            ESP.restart();
        }
        if (!setupThings())
        {
            Serial.println("Setting up things failed");
            ESP.restart();
        }
    }
    drd.stop();
}

void loop()
{
    Serial.println("Pushing readings...");
    updateThings();
    Serial.println("Going into deep sleep mode");
    ESP.deepSleep(300e6);
}

boolean reconnect()
{
    // Loop until we're reconnected
    while (!tbClient.connected())
    {
        //tbClient.setServer(thingsboardServer, 1883);
        Serial.print("Connecting to ThingsBoard node ...");
        if (tbClient.connect(thingsboardServer, TOKEN, 1883))
        {
            Serial.println("[DONE]");
            // break;
        }
        else
        {
            Serial.print("[FAILED] to connect to thingsboard node");
            Serial.println(" : retrying in 5 seconds]");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
    return true;
}

boolean setupThings()
{
    Serial.println("Setting up things adapter");
    String thingName = "SM-ESP-" + String(ESP.getChipId());
    DynamicJsonDocument attributes(256);

    attributes["name"] = thingName;
    attributes["fromDevice"] = true;

    String attributesJSON;
    serializeJson(attributes, attributesJSON);
    tbClient.sendAttributeJSON(attributesJSON.c_str());
    return true;
}

void updateThings()
{
    SoilMoistureSensorResult result = readSoilMoisture();

    Serial.println("Moisture Reading: ");
    Serial.print(result.moistureReading);
    Serial.println("\nMoisture %: ");
    Serial.print(result.moisture);

    DynamicJsonDocument telemetry(256);

    telemetry["moisture"] = result.moisture;
    telemetry["moistureReading"] = result.moistureReading;
    String telemetryJSON;
    serializeJson(telemetry, telemetryJSON);

    
    tbClient.sendTelemetryJson(telemetryJSON.c_str());

    delay(1000);
}

//gets called when WiFiManager enters configuration mode
void configModeCallback(AsyncWiFiManager *wiFiManager)
{
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    //if you used auto generated SSID, print it
    Serial.println(wiFiManager->getConfigPortalSSID());

    // We don't want the next time the boar resets to be considered a double reset
    // so we remove the flag
    drd.stop();
}

void startConfigPortal(const char *apName, AsyncWiFiManager *wifiManager)
{
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    wifiManager->setDebugOutput(true);

    wifiManager->setAPCallback(configModeCallback);

    //exit after config instead of connecting
    // wifiManager.setBreakAfterConfig(true);

    //reset settings - for testing
    wifiManager->resetSettings();

    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    //in seconds
    wifiManager->setTimeout(300);

    //it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration

    //WITHOUT THIS THE AP DOES NOT SEEM TO WORK PROPERLY WITH SDK 1.5 , update to at least 1.5.1
    //WiFi.mode(WIFI_STA);

    if (!wifiManager->startConfigPortal(apName))
    {
        Serial.println("failed to connect and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(5000);
    }

    if (WiFi.reconnect())
    {
        //if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");
    }
    drd.stop();
}

SoilMoistureSensorResult readSoilMoisture()
{
    Serial.println("Getting soil moisture reading...");
    int moistureReading = analogRead(sensor_pin);
    Serial.println("Soil moisture reading: ");
    Serial.print(moistureReading);
    long moisture = map(moistureReading, 800, 350, 0, 100);
    Serial.println("Soil moisture mapped: ");
    Serial.print(moisture);

    SoilMoistureSensorResult result = {.moisture = moisture, .moistureReading = moistureReading};
    return result;
}
