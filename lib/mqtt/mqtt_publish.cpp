#include "mqtt.h"
#include "../telnet/telnet.h"


void Mqtt::publish(const char * topic, const char * payload) {
    _publish(topic, payload);
}

void Mqtt::publish(const char * topic, float number) {
    char payload[32];
    dtostrf(number, 0, 2, payload);
    _publish(topic, payload);
}

void Mqtt::publish(const char * topic, int number) {
    char payload[32];
    itoa(number, payload, 10);
    _publish(topic, payload);
}

void Mqtt::publish(const char * topic) {
    _publish(topic, "");
}


void Mqtt::_publish(const char * topic, const char * payload) {
    if (_is_connected) {
        telnet.print("\tsending: ");
        telnet.print(topic);
        telnet.print(" / ");
        telnet.println(payload);        
        /*Serial.print("\tsending: ");
        Serial.print(topic);
        Serial.print(" / ");
        Serial.println(payload);*/
        _mqtt_client.publish(topic, payload);
    }
}
