#include "mqtt.h"
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_INA219.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "device.h"

#include "../telnet/telnet.h"
#include "ACS712.h"
#define On_Board_LED_PIN 38
#define RGB_LED_PIN 48
#define NUM_PIXELS 1
#define SampleTime 5000
#include "motor.h"

Device::Device() : motor(nullptr), pixel(nullptr) {}

Device device;
#define CurrentCalibrationFactor 18.150 // Adjust this factor based on calibration results
const int sensorIn = 4; // pin where the OUT pin from sensor is connected on ESP32-S3 (ADC1)

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
    _LastSampleTime = millis();
    
    // Initialize NeoPixel RGB LED
    pixel = new Adafruit_NeoPixel(NUM_PIXELS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);
    if (pixel) {
        pixel->begin();
        pixel->setBrightness(20); // Low brightness to avoid blinding
        pixel->setPixelColor(0, pixel->Color(0, 255, 0)); // Start green
        pixel->show();
        telnet.println("NeoPixel initialized (GPIO 48)");
    }
    
    // Publish initial status
    mqtt.publish("filterchlorine/status", "online");
    mqtt.publish("filterchlorine/ota/state", "ready");
    
    // Initialize I2C with custom pins: SDA = GPIO 8, SCL = GPIO 9
    Wire.begin(9, 8);
    
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
    // Initialize motor: PWM pin 26, DIR pin 25, channel 0, 5kHz, 8-bit
    motor = new MD135(35, 36, 0, 5000, 8);
    if (!motor) {
        telnet.println("CRITICAL: Failed to allocate motor object!");
        Serial.println("CRITICAL: Failed to allocate motor object!");
    } else {
        motor->begin();
        motor->forward(255); // Start in forward - will auto-reverse after 10 minutes
        telnet.println("Motor initialized successfully (forward mode)");
    }
    
    _last_power_update = millis();  // Initialize power measurement timer
}

void Device::loop()
{
    // Update LED color gradient continuously
    updateLED();
    
    // Check if motor is initialized
    if (!motor) {
        telnet.println("Error: Motor not initialized, skipping device loop");
        return;
    }
    
    // Generate sensor data
    // Generate sensor data
    if(millis() < _LastMillis) {
    
        return;
    }
    
    _LastMillis = millis() + _SampleTime;
    _LastSampleTime = millis(); // Record when sample was taken
    _MinuteCount++;
    
    // Motor control: Run forward for 10 minutes, then reverse for 1 minute, repeat
    // _ReverseRatio = 0.1 means reverse 1 out of every 10 minutes
    if(motor->isForward()) {
        // Currently running forward - check if it's time to reverse
        if((float) _MinuteCount * _ReverseRatio >= 1.00) {
            motor->reverse(255);
            _ReverseCount++;
            _MinuteCount = 0;
        }
    } else {
        // Currently running reverse - check if it's time to go back to forward
        if(_MinuteCount >= 1) {
            motor->forward(255);
            _MinuteCount = 0;
        }
    }

    int rssi = WiFi.RSSI(); // Get WiFi signal strength

    update_power(); // Read power data from INA219

    // Build JSON payload using ArduinoJson
    JsonDocument doc;
    doc["resistance"] = _resistance;
    doc["current"] = _current_mA;
    doc["rssi"] = rssi;
    doc["direction"] = motor->isForward() ? "forward" : "reverse";
    doc["ip"] = WiFi.localIP().toString();
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

    // Removed blocking delay(2000) - was killing WiFi performance
    if (payloadReady)
    {
        telnet.print("\tprocessed payload: ");
        telnet.println(globalBuf);
        payloadReady = false;
    }
}

void Device::updateLED()
{
    if (!pixel) return;
    
    // Calculate elapsed time since last sample
    unsigned long elapsed = millis() - _LastSampleTime;
    
    // Calculate ratio (0.0 = just sampled/green, 1.0 = about to sample/red)
    float ratio = (float)elapsed / (float)_SampleTime;
    if (ratio > 1.0) ratio = 1.0;
    
    // Interpolate color from green to red
    // Green: (0, 255, 0) -> Red: (255, 0, 0)
    uint8_t red = (uint8_t)(ratio * 255);
    uint8_t green = (uint8_t)((1.0 - ratio) * 255);
    
    pixel->setPixelColor(0, pixel->Color(red, green, 0));
    pixel->show();
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
    _current_mA = this->ina219.getCurrent_mA() * CurrentCalibrationFactor; // Apply calibration factor if needed

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
