#include "mqtt.h"
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_INA219.h>
#include <ArduinoJson.h>
#include "device.h"

#include "../telnet/telnet.h"
#include "ACS712.h"
#define On_Board_LED_PIN 2
#define SampleTime 5000


Device::Device() {}

Device device;

const int sensorIn = 34; // pin where the OUT pin from sensor is connected on Arduino


Adafruit_INA219 ina219;
// Define static member variables
bool Device::payloadReady = false;
char Device::globalBuf[256] = {0};

void Device::setup()
{
    static const char *subscription_list[] = {
        "beacon"};
    mqtt.set_subscriptions(subscription_list, 1);
    mqtt.set_callback(message_handler);

    // Publish initial status
    mqtt.publish("filterchlorine/status", "online");
    mqtt.publish("filterchlorine/ota/state", "ready");
    if (!ina219.begin())
    {
        telnet.println("Failed to find INA219 chip mark as down");
    }
}

void Device::loop()
{
    // Generate sensor data

    int rssi = WiFi.RSSI(); // Get WiFi signal strength

    update_power(); // Read power data from INA219

    // Build JSON payload using ArduinoJson
    JsonDocument doc;
    doc["resistance"] = _resistance;
    doc["current"] = _current_mA;
    doc["rssi"] = rssi;
    doc["uptime"] = millis() / 1000;
    doc["down"] = IsDown() ? "offline" : "online";
    doc["busvoltage"] = _busvoltage;
    doc["shuntvoltage"] = _shuntvoltage;
    doc["loadvoltage"] = _loadvoltage;
    doc["power_mW"] = _power_mW;

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);

    // Publish JSON to single topic
    mqtt.publish("filterchlorine/sensors", jsonBuffer);

    delay(2000);
    if (payloadReady)
    {
        telnet.print("\tprocessed payload: ");
        telnet.println(globalBuf);
        payloadReady = false;
    }
}

void Device::message_handler(char *topic, char *payload)
{
    telnet.print("\treceived topic: ");
    telnet.print(topic);
    telnet.print(" / payload: ");
    telnet.println(payload);

    // Handle OTA update trigger
    if (strcmp(payload, "OTA_UPDATE") == 0 || strcmp(payload, "UPDATE") == 0)
    {
        telnet.println("\tOTA Update triggered via MQTT");
        mqtt.publish("filterchlorine/ota/state", "updating");
        telnet.println("\tWaiting for OTA upload...");
        telnet.println("\tDevice is ready for OTA updates");
        mqtt.publish("filterchlorine/ota/state", "ready");
        return;
    }

    strcpy(globalBuf, payload);
    payloadReady = true;
}
bool Device::IsDown()
{
    return _Down;
}
void Device::update_power()
{
    // Read voltage and current from INA219.
     _shuntvoltage = ina219.getShuntVoltage_mV();
     _busvoltage = ina219.getBusVoltage_V();
     _current_mA = ina219.getCurrent_mA();

    // Compute load voltage, power, and milliamp-hours.
     _loadvoltage = _busvoltage + (_shuntvoltage / 1000);
     _power_mW = _loadvoltage * _current_mA;

    _total_mA += _current_mA;
    _total_sec += 1;
    _total_mAH = _total_mA / 3600.0;

    telnet.print("I: ");
    telnet.print(_current_mA);
    telnet.print(" mA, V: ");
    telnet.print(_loadvoltage);
    telnet.print(" V, P: ");
    telnet.print(_power_mW);
    telnet.print(" mW, mAH: ");
    telnet.print(_total_mAH);
    telnet.println(" mAH");
}
