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
#include "motor.h"

Device::Device() : motor(nullptr) {}

Device device;

const int sensorIn = 34; // pin where the OUT pin from sensor is connected on Arduino

// Define static member variables
bool Device::payloadReady = false;
char Device::globalBuf[256] = {0};

void Device::setup()
{
    static const char *subscription_list[] = {
        "beacon"};
    mqtt.set_subscriptions(subscription_list, 1);
    mqtt.set_callback(message_handler);
    _SampleTime = 60*1000; // 1 minute default sample time
    // Publish initial status
    mqtt.publish("filterchlorine/status", "online");
    mqtt.publish("filterchlorine/ota/state", "ready");
    if (!this->ina219.begin())
    {
        telnet.println("Failed to find INA219 chip mark as down");
        _Down = true;
    }
    else
    {
        // Configure INA219 for 0-16V, 0-400mA range (higher precision for low current)
       //this->ina219.setCalibration_32V_2A();
        telnet.println("INA219 initialized and calibrated (16V/400mA)");
    }
    // Initialize motor: PWM pin 25, DIR pin 26, channel 0, 5kHz, 8-bit
    motor = new MD135(26, 25, 0, 5000, 8);
    motor->begin();
    motor->reverse(255); // Start in reverse to prevent fouling of sensor on startup
    
    _last_power_update = millis();  // Initialize power measurement timer
}

void Device::loop()
{
    // Generate sensor data
    // Generate sensor data
    if(millis() < _LastMillis) {
    
        return;
    }
    
    _LastMillis = millis() + _SampleTime;
    _MinuteCount++;
    // this should run for multiples of 1 minute, then reverse for 6 seconds, then return to forward 
    //  - this is to prevent fouling of the chlorine sensor by keeping water moving in both directions over time
    if((float) _MinuteCount*_ReverseRatio >= 1.00) {
        motor->reverse(255); // Run motor in reverse at speed 200 (0-255)
        _ReverseCount++;
        _MinuteCount = 0;


    } 
    
    if(_MinuteCount >= 1 && !motor->isForward() ) { //reset to forward after reverse duration has passed
        motor->forward(255); // Run motor forward at speed 200 (0-255)
        _MinuteCount = 0;

    }  

    _direction = !_direction; // Toggle direction for next loop 

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
    doc["reversecount"] = _ReverseCount;

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
    // Skip if INA219 failed to initialize
    if (_Down) 
    { 
        telnet.println("\tSkipping power update due to INA219 initialization failure");
        return;
    }
    
    // Calculate time since last measurement
    unsigned long currentMillis = millis();
    float elapsed_seconds = (currentMillis - _last_power_update) / 1000.0;
    _last_power_update = currentMillis;
    
    // Read voltage and current from INA219
    _shuntvoltage = this->ina219.getShuntVoltage_mV();
    _busvoltage = this->ina219.getBusVoltage_V();
    _current_mA = this->ina219.getCurrent_mA();
    
    // Debug: print immediately after reading
    telnet.print("[RAW] Bus: ");
    telnet.print(_busvoltage);
    telnet.print("V, Shunt: ");
    telnet.print(_shuntvoltage);
    telnet.print("mV, Current: ");
    telnet.print(_current_mA);
    telnet.println("mA");

    // Compute load voltage and power
    _loadvoltage = _busvoltage + (_shuntvoltage / 1000);
    _power_mW = _loadvoltage * _current_mA;

    // Accumulate current over time for mAh calculation
    // mAh = (mA * seconds) / 3600
    _total_mA += _current_mA * elapsed_seconds;
    _total_sec += elapsed_seconds;
    _total_mAH = _total_mA / 3600.0;

    _resistance = (_busvoltage * 1000) / _current_mA; // Ohm's law: R = V/I, convert V to mV for mA

    // Only print to telnet occasionally to avoid spam
    static unsigned long last_print = 0;
    if (currentMillis - last_print > 10000) {  // Print every 10 seconds max
        telnet.print("Bus: ");
        telnet.print(_busvoltage);
        telnet.print("V Shunt: ");
        telnet.print(_shuntvoltage);
        telnet.print("mV | I: ");
        telnet.print(_current_mA);
        telnet.print(" mA, V: ");
        telnet.print(_loadvoltage);
        telnet.print(" V, P: ");
        telnet.print(_power_mW);
        telnet.print(" mW, mAH: ");
        telnet.print(_total_mAH);
        telnet.println(" mAH");
        last_print = currentMillis;
    }
}
