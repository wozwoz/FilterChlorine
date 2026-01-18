#pragma once
#include <Adafruit_INA219.h>

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
        bool IsDown();
    private:
        float _amps = 0.0;
        float _resistance = 0.0;
        Adafruit_INA219 ina219;
        bool _Down = false;
        float _total_mA = 0.0;
        float _total_sec = 0.0;

        float _shuntvoltage = 0.0;
        float _busvoltage = 0.0;
        float _current_mA = 0.0;
        float _loadvoltage = 0.0;
        float _power_mW = 0.0;
        float _total_mAH = 0.0;

};


extern Device device;