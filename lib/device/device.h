#pragma once
#include <Adafruit_INA219.h>

// Forward declaration
class MD135;
class Adafruit_NeoPixel;

class Device {

    public:

        Device();

        void setup();
        void loop();
        static void message_handler(char *, char *);
        static bool payloadReady;
        static char globalBuf[256];
        void CalculateData();

        void update_power();
        void updateLED();
        bool IsDown();
        unsigned long _SampleTime = 0;
        unsigned long _LastMillis = 0;
        unsigned long _LastSampleTime = 0;
        float GetAmps() { return _current_mA; };
        int GetMinuteCount() { return _MinuteCount; };
        MD135* motor; // Motor as pointer - initialized in setup()
        Adafruit_INA219 ina219; // INA219 sensor - public for diagnostics
        Adafruit_NeoPixel* pixel; // NeoPixel RGB LED
    private:
        float _resistance = 0.0;
        bool _Down = false;
        float _total_mA = 0.0;
        float _total_sec = 0.0;

        float _shuntvoltage = 0.0;
        float _busvoltage = 0.0;
        float _current_mA = 0.0;
        float _loadvoltage = 0.0;
        float _power_mW = 0.0;
        float _total_mAH = 0.0;
        bool _direction = true; // true for forward, false for reverse
        unsigned int _MinuteCount = 0;
        float _ReverseRatio = 0.1;
        unsigned int _ReverseCount = 0;
        unsigned long _last_power_update = 0;  // Track time between measurements
};


extern Device device;