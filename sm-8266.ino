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
const char *deviceTypes[] = {"CapacitivieSoilMoistureSensor", "SoilMoistureSensor", "Sensor", nullptr};
ThingDevice soilMoistureSensor("SoildMoisture-Plant-1", "Soil Moisture Sensor", deviceTypes);
ThingProperty soilMoistureProperty("moisture", "Soil Moisture", NUMBER, "SoilMoistureCalibrated");
ThingProperty soilMoistureReadingProperty("moistureReading", "Soil Moisture Reading", NUMBER, "SoilMoistureReading");

//Setup configuration portal on demand
void startConfigPortal(const char *apName, AsyncWiFiManager *manager);
void configModeCallback(AsyncWiFiManager *myWiFiManager);
void setupThings();
void updateThings();

struct SoilMoistureSensorResult
{
    long moisture;
    int moistureReading;
};

//Read Soil moisture Reading
SoilMoistureSensorResult readSoilMoisture();

int sensor_pin = 0;
int moistureReading;

AsyncWebServer server(80);
DNSServer dns;

void setup()
{

    Serial.begin(115200);
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
        setupThings();
    }
}

void loop()
{
    updateThings();
    delay(60000);
}

void setupThings()
{
    Serial.println("Setting up things adapter");
    adapter = new WebThingAdapter("w25", WiFi.localIP());
    soilMoistureSensor.addProperty(&soilMoistureProperty);
    soilMoistureSensor.addProperty(&soilMoistureReadingProperty);
    adapter->addDevice(&soilMoistureSensor);
    adapter->begin();

    Serial.println("Moisture sensor server started");
    Serial.print("http://");
    Serial.print(WiFi.localIP());
    Serial.print("/things/");
    Serial.println(soilMoistureSensor.id);
}

void updateThings()
{
    Serial.println("Updatig thing properties...");

    SoilMoistureSensorResult result = readSoilMoisture();

    Serial.println("Moisture Reading: ");
    Serial.print(result.moistureReading);
    Serial.println("\nMoisture %: ");
    Serial.print(result.moisture);

    ThingPropertyValue soilMoistureValue;
    ThingPropertyValue soilMoistureReadingValue;

    soilMoistureValue.number = result.moisture;
    soilMoistureReadingValue.number = result.moistureReading;

    soilMoistureProperty.setValue(soilMoistureValue);
    soilMoistureReadingProperty.setValue(soilMoistureReadingValue);

    adapter->update();
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

    // We don't want the next time the boar resets to be considered a double reset
    // so we remove the flag
    drd.stop();
}

void startConfigPortal(const char *apName, AsyncWiFiManager *manager)
{
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    AsyncWiFiManager wifiManager(&server, &dns);
    wifiManager.setDebugOutput(true);

    wifiManager.setAPCallback(configModeCallback);

    //exit after config instead of connecting
    // wifiManager.setBreakAfterConfig(true);

    //reset settings - for testing
    //wifiManager.resetSettings();

    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    //in seconds
    wifiManager.setTimeout(300);

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

    if (WiFi.reconnect())
    {
        //if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");
    }
    drd.stop();
}

SoilMoistureSensorResult readSoilMoisture()
{
    Serial.println("Getting soild moisture reading...");
    moistureReading = analogRead(sensor_pin);
    Serial.println("Soil moisture reading: ");
    Serial.print(moistureReading);
    long moisture = map(moistureReading, 800, 350, 0, 100);
    Serial.println("Soil moisture mapped: ");
    Serial.print(moisture);

    SoilMoistureSensorResult result = {.moisture = moisture, .moistureReading = moistureReading};
    return result;
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