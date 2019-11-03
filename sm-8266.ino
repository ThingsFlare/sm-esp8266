// #define ARDUINO 100

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
// #include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <Thing.h>
#include <WebThingAdapter.h>
#include <DoubleResetDetector.h>

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 10

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

WebThingAdapter *adapter;

//Setup configuration portal on demand
void startConfigPortalOnDemand(const char *apName);
void configModeCallback(AsyncWiFiManager *myWiFiManager);

//Read Soil moisture Reading
int readSoilMoisture();

int sensor_pin = 0;
int output_value;

AsyncWebServer server(80);
DNSServer dns;

void setup()
{

    Serial.begin(115200);

    String apNameStr = "SM-ESP-" + String(ESP.getChipId());
    const char *apName = apNameStr.c_str();
    if (drd.detectDoubleReset())
    {
        Serial.println("Starting setup mode using AP: ");
        Serial.print(apName);

        startConfigPortalOnDemand(apName);
    }
    else
    {
        AsyncWiFiManager wifiManager(&server, &dns);
        wifiManager.autoConnect(apName);        
    }
}

void loop()
{

    int moisture = readSoilMoisture();

    Serial.println("Moisture Reading: ");
    Serial.print(output_value);
    Serial.println("Moisture %: ");
    Serial.print(moisture);

    Serial.print("\n");
    delay(1000);
}

//gets called when WiFiManager enters configuration mode
void configModeCallback(AsyncWiFiManager *wiFiManager)
{
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    //if you used auto generated SSID, print it
    Serial.println(wiFiManager->getConfigPortalSSID());
}

void startConfigPortalOnDemand(const char *apName)
{
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    AsyncWiFiManager wifiManager(&server, &dns);

    wifiManager.setAPCallback(configModeCallback);

    //exit after config instead of connecting
    wifiManager.setBreakAfterConfig(true);

    //reset settings - for testing
    //wifiManager.resetSettings();

    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    //in seconds
    //wifiManager.setTimeout(300);

    //it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration

    //WITHOUT THIS THE AP DOES NOT SEEM TO WORK PROPERLY WITH SDK 1.5 , update to at least 1.5.1
    //WiFi.mode(WIFI_STA);

    if (!wifiManager.startConfigPortal(apName))
    {
        Serial.println("failed to connect and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
}

int readSoilMoisture()
{
    output_value = analogRead(sensor_pin);
    long moisture = map(output_value, 800, 350, 0, 100);
}

// int output_value;
// void setup()
// {
//     Serial.begin(115200);
//     Serial.println("Reading From the Sensor ...");
//     delay(2000);
// }
// void loop()
// {
//     output_value = analogRead(sensor_pin);
//     Serial.print("Moisture : ");
//     Serial.print(output_value);
//     Serial.print("\n");
//     delay(1000);
// }