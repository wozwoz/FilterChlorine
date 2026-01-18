#pragma once

#include <WiFiClient.h>
#include <PubSubClient.h>

#define RETRY_INTERVAL 3000

using MessageHandler = void (*)(char *, char *);

class Mqtt {

    public:

        Mqtt();

        // connect
        void setup(const char *, const char *, const char *, int);
        void maintain();
        void report_disconnect();

        // publish
        void publish(const char *, const char *);
        void publish(const char *, float);
        void publish(const char *, int);
        void publish(const char *);

        // subscribe
        void set_subscriptions(const char **, int);

        // handle
        void set_callback(MessageHandler);

    private:

        WiFiClient _wifi_client;
        PubSubClient _mqtt_client;

        // connect
        unsigned long _retry_timer;
        bool _is_first_connect = true;
        bool _is_connected = false;
        char _device_id[9];
        char _user[15];
        char _password[15];

        // publish
        void _publish(const char *, const char *);

        // subscribe
        const char ** _subscription_list = nullptr;
        int _sub_list_length = 0;
        void _subscribe_to_all();

        // handle
        MessageHandler _stored_handler;
        static void _callback_wrapper(char *, byte *, unsigned int);

};

extern Mqtt mqtt;