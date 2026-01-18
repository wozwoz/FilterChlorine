#pragma once

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

#define AP_SSID "DEVICE PROVISIONING"

class Provisioner {
    public:
        Provisioner();
        void get_creds(char * ssid, char * pass);

    private:
        WebServer _web_server{80};
        DNSServer _dns_server;
        bool _provisioning_complete = false;
        char _ssid[32];
        char _pass[32];
        void _start_provisioner();
        void _wait_for_completion();
        
        void _set_base_routes();
        void _set_portal_routes();
        void _set_other_routes();
};

extern Provisioner provisioner;