#include "wifi_tools.h"
#include "provisioner.h"
#include "storage.h"
#include "credentials.h"
#include "mqtt.h"
#include "device.h"
#include "telnet.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

// #define CLEAR_CREDS
int Delay = 1000; // Main loop delay in ms

void setupOTA()
{
    // Start mDNS for hostname resolution
    if (!MDNS.begin("esp32-device"))
    {
        telnet.println("\tError setting up mDNS");
    }
    else
    {
        telnet.println("\tmDNS started: esp32-device.local");
    }

    ArduinoOTA.setHostname("ESP32-Device");
    ArduinoOTA.setPassword("admin");

    ArduinoOTA.onStart([]()
                       {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        telnet.print("\n\tOTA: Updating " + type); });

    ArduinoOTA.onEnd([]()
                     { telnet.print("\n\tOTA: Complete"); });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { telnet.print("\tOTA Progress: " + String((progress / (total / 100))) + "%\r"); });

    ArduinoOTA.onError([](ota_error_t error)
                       {
        telnet.print("\tOTA Error[ " + error);
        if (error == OTA_AUTH_ERROR) telnet.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) telnet.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) telnet.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) telnet.println("Receive Failed");
        else if (error == OTA_END_ERROR) telnet.println("End Failed"); });

    ArduinoOTA.begin();
    telnet.println("\tOTA ready");
}

void setup()
{
    Serial.begin(115200);
    telnet.println("\n\tChorinator Starting...\n");

    // Handle credentials
    char ssid[32] = {WIFI_SSID};
    char pass[32] = {WIFI_PASS};
    
#ifdef CLEAR_CREDS
    //telnet.println("\tClearing credentials");
    //storage.clear_creds();
#endif
 //   if (!storage.creds_already_exist(ssid, pass))
   // {
     //   provisioner.get_creds(ssid, pass);
       // storage.store_creds(ssid, pass);
   // }

    // Connect WiFi
    wifi_tools.log_events();
    wifi_tools.begin(ssid, pass);

    telnet.print("\tConnecting to WiFi");
    int connect_attempts = 0;
    const int max_attempts = 40;  // 20 seconds total
    while (WiFi.status() != WL_CONNECTED && connect_attempts < max_attempts)
    {
        delay(500);
        telnet.print(".");
        connect_attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED)
    {
        telnet.println(" connected!");
        telnet.print("\tIP Address: ");
        telnet.println(String(WiFi.localIP()));
    }
    else
    {
        telnet.println(" timeout!");
        telnet.println("\tWill continue trying in background...");
    }

    // Setup services
    setupOTA();

    mqtt.setup(MQTT_HOST, mqtt_user, mqtt_password, MQTT_PORT);
    device.setup();
    telnet.setup();

    telnet.println("\tSetup complete\n");
}

void loop()
{
    // Handle telnet constantly to prevent disconnections
    telnet.loop();
    
    static unsigned long lastLoop = 0;
    if (millis() - lastLoop >= Delay)
    {
        lastLoop = millis();
        //telnet.print(".");

        if (wifi_tools.is_connected)
        {
            ArduinoOTA.handle();
            mqtt.maintain();
            device.loop();
            //telnet.print("\r\n");
        }
        else
        {
            wifi_tools.reconnect();
            // Don't force disconnect - maintain() will handle it
        }
    }
}