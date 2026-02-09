#pragma once

#include <WiFi.h>

#define RECONNECT_INTERVAL 10000
#define AUTH_FAIL_RETRY_INTERVAL 30000  // 30 seconds after auth failures
#define STATUS_LOG_INTERVAL 1000

class WiFi_Tools {

    public:

        WiFi_Tools();

        void begin(const char *, const char *);
        void reconnect();

        void log_events();
        void log_status();

        bool is_connected = false;

    private:

        bool _should_reconnect = true;
        bool _first_disconnect = true;
        bool _event_logging_enabled = false;
        bool _last_was_auth_fail = false;
        unsigned int _auth_fail_count = 0;

        char _ssid[32];
        char _password[64];

        unsigned long _reconnect_timer;
        unsigned long _status_timer;

        static void _event_handler(WiFiEvent_t, WiFiEventInfo_t);
        void _log_event(WiFiEvent_t, WiFiEventInfo_t);

};

extern WiFi_Tools wifi_tools;