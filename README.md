# Soil Moisture Sketch
Soil Moisture Sensor firmware for ESP8266 and Thingsboard

## Dependencies
1. ESP8266 libraries
2. Pubsub client (https://github.com/knolleary/pubsubclient)
3. AsyncWebServer (https://github.com/me-no-dev/ESPAsyncWebServer)
4. AsyncTCP (https://github.com/me-no-dev/AsyncTCP)


## TO-DO
- [x] Make SSID configuration through AP mode using built-in reset button
- [ ] Make deep sleep time and telemetry push interval configurable
- [ ] Add things board host and access token to be configurable
- [ ] Auto register device with things board through configuration mode
- [ ] Cleanup code