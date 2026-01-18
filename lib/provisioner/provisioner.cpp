

#include "provisioner.h"

Provisioner::Provisioner() {}
Provisioner provisioner;

void Provisioner::get_creds(char * ssid, char * pass) {
    _start_provisioner();
    _wait_for_completion();
    strcpy(ssid, _ssid);
    strcpy(pass, _pass);
}

void Provisioner::_start_provisioner() {

    Serial.println("\n  >> entering provisioning mode...\n");



    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID);

    _set_base_routes();
    _set_portal_routes();
    _set_other_routes();

    _web_server.begin();
    _dns_server.start(53, "*", WiFi.softAPIP());

    // These four lines below were added after the video.  
    // I should have included them a long time ago.

    Serial.print("     attach to network:    ");
    Serial.println(AP_SSID);

    Serial.print("     navigate browser to:  ");
    Serial.println(WiFi.softAPIP());

}

void Provisioner::_wait_for_completion() {
    while (!_provisioning_complete) {
        _dns_server.processNextRequest();
        _web_server.handleClient();
        delay(100);
    }
    Serial.println("\n  >> exiting provisioning mode...");
}