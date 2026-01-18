#include "wifi_tools.h"
#include "provisioner.h"
#include "storage.h"
#include "credentials.h"
#include "mqtt.h"
#include "device.h"
#include "telnet.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

//#define CLEAR_CREDS

void setupOTA() {
    // Start mDNS for hostname resolution
    if (!MDNS.begin("esp32-device")) {
        telnet.println("\tError setting up mDNS");
    } else {
        telnet.println("\tmDNS started: esp32-device.local");
    }
    
    ArduinoOTA.setHostname("ESP32-Device");
    ArduinoOTA.setPassword("admin");
    
    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        telnet.print("\n\tOTA: Updating " + type);
    });
    
    ArduinoOTA.onEnd([]() {
        telnet.print("\n\tOTA: Complete");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        telnet.print("\tOTA Progress: " + String((progress / (total / 100))) + "%\r");
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        telnet.print("\tOTA Error[ " + error);
        if (error == OTA_AUTH_ERROR) telnet.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) telnet.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) telnet.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) telnet.println("Receive Failed");
        else if (error == OTA_END_ERROR) telnet.println("End Failed");
    });
    
    ArduinoOTA.begin();
    telnet.println("\tOTA ready");
}


void setup() {
    Serial.begin(115200);
    telnet.println("\n\tESP32 Starting...\n");
    
    // Handle credentials
    char ssid[32];
    char pass[32];
#ifdef CLEAR_CREDS
    telnet.println("\tClearing credentials");
    storage.clear_creds();
#endif
    if (!storage.creds_already_exist(ssid, pass)) {
        provisioner.get_creds(ssid, pass);
        storage.store_creds(ssid, pass);
    }
    
    // Connect WiFi
    WiFi.mode(WIFI_STA);
    wifi_tools.log_events();
    wifi_tools.begin(ssid, pass);
    
    telnet.print("\tConnecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        telnet.print(".");
    }
    telnet.println(" connected!");
    telnet.println("\tIP Address: ");
    telnet.println(String(WiFi.localIP()));
    
    // Setup services
    setupOTA();
    
    mqtt.setup(MQTT_HOST, mqtt_user, mqtt_password, MQTT_PORT);
    device.setup();
    telnet.setup();

    
    telnet.println("\tSetup complete\n");
}

void loop() {
    if (wifi_tools.is_connected) {
        ArduinoOTA.handle();
        mqtt.maintain();
        device.loop();
        telnet.print("\r\n");
    }
    else {
        wifi_tools.reconnect();
        mqtt.report_disconnect();
    }
    telnet.loop();
}
    
