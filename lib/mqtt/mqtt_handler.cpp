#include "mqtt.h"
#include "../telnet/telnet.h"


void Mqtt::set_callback(MessageHandler handler) {
    _stored_handler = handler;
    telnet.println("\tCallback handler registered");
    _mqtt_client.setCallback(Mqtt::_callback_wrapper);
}

void Mqtt::_callback_wrapper(char * topic, byte * payload, unsigned int length) {
    telnet.print("\tCallback wrapper triggered! Length: ");
    telnet.print((int)length);
    telnet.println("");
    char message[length+1];
    memcpy(message, payload, length);
    message[length] = 0;
    if (mqtt._stored_handler) {
        telnet.println("\tCalling stored handler");
        mqtt._stored_handler(topic, message);
    } else {
        telnet.println("\tERROR: No stored handler!");
    }
}