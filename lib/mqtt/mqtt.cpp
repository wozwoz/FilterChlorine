#include "mqtt.h"

#define DEVICE_ID "temp_device_id"

Mqtt::Mqtt() : _stored_handler(nullptr) {} // constructor

Mqtt mqtt; // global mqtt object

void Mqtt::setup(const char * mqtt_host, const char * user, const char * password,int mqtt_port) {

    _mqtt_client.setBufferSize(2048);
    _mqtt_client.setClient(_wifi_client);
    _mqtt_client.setServer(mqtt_host, mqtt_port);
    strcpy(_user, user);
    strcpy( _password, password);
}

void Mqtt::maintain() {

    if (!_mqtt_client.connected()) {
        _is_connected = false;
        bool should_reconnect = _is_first_connect || millis() - _retry_timer > RETRY_INTERVAL;
        if (should_reconnect) {
            _retry_timer = millis();
            _is_first_connect = false;
            Serial.println("\tmqtt connecting...");
            if (_mqtt_client.connect((DEVICE_ID), _user, _password) ) {
                Serial.println("\tmqtt connected...");
                _is_connected = true;

                _subscribe_to_all();

            } else {
                Serial.println("\txx failed to connect to mqtt");
                _is_connected = false;
            }
        }
    } else {
        _mqtt_client.loop();       
    }
  
}


void Mqtt::report_disconnect() {
    _is_connected = false;
}
