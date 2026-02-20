#include "mqtt.h"
#include <WiFi.h>

#define DEVICE_ID "FilterChlorine"

Mqtt::Mqtt() : _stored_handler(nullptr) {} // constructor

Mqtt mqtt; // global mqtt object

void Mqtt::setup(const char * mqtt_host, const char * user, const char * password,int mqtt_port) {

    Serial.print("\tMQTT Client ID: ");
    Serial.println(DEVICE_ID);
    
    _mqtt_client.setBufferSize(2048);
    _mqtt_client.setClient(_wifi_client);
    _mqtt_client.setServer(mqtt_host, mqtt_port);
    _mqtt_client.setKeepAlive(30);  // 30 second keepalive
    _mqtt_client.setSocketTimeout(15);  // 15 second timeout for unreliable links
    strcpy(_user, user);
    strcpy( _password, password);
}

void Mqtt::maintain() {

    // Check if WiFi is connected before attempting MQTT operations
    if (WiFi.status() != WL_CONNECTED) {
        _is_connected = false;
        Serial.println("\tMQTT: WiFi not connected, skipping MQTT operations");
        return;
    }

    if (!_mqtt_client.connected()) {
        _is_connected = false;
        bool should_reconnect = _is_first_connect || millis() - _retry_timer > RETRY_INTERVAL;
        if (should_reconnect) {
            _retry_timer = millis();
            _is_first_connect = false;
            Serial.print("\tMQTT: Attempting connection to broker... ");
            if (_mqtt_client.connect(DEVICE_ID, _user, _password) ) {
                Serial.println("SUCCESS");
                _is_connected = true;

                _subscribe_to_all();

            } else {
                int state = _mqtt_client.state();
                Serial.print("FAILED - state: ");
                Serial.print(state);
                Serial.print(" (");
                switch(state) {
                    case -4: Serial.println("TIMEOUT - broker didn't respond)"); break;
                    case -3: Serial.println("CONNECTION_LOST)"); break;
                    case -2: Serial.println("CONNECT_FAILED - can't reach broker)"); break;
                    case -1: Serial.println("DISCONNECTED)"); break;
                    case 1: Serial.println("BAD_PROTOCOL)"); break;
                    case 2: Serial.println("BAD_CLIENT_ID)"); break;
                    case 3: Serial.println("UNAVAILABLE)"); break;
                    case 4: Serial.println("BAD_CREDENTIALS)"); break;
                    case 5: Serial.println("UNAUTHORIZED)"); break;
                    default: Serial.println("UNKNOWN)"); break;
                }
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
