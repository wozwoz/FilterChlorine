#include "mqtt.h"
#include "../telnet/telnet.h"


void Mqtt::set_subscriptions(const char ** topics, int count) {
    _subscription_list = topics;
    _sub_list_length = count;
}

void Mqtt::_subscribe_to_all() {
    if (_subscription_list) {
        for (int i=0; i<_sub_list_length; i++) {
            //telnet.print("\t.. subscribing to: ");
            //telnet.println(_subscription_list[i]);
            _mqtt_client.subscribe(_subscription_list[i]);
        }
    }
}