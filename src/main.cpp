#include "wifi_tools.h"
#include "provisioner.h"
#include "storage.h"
#include "credentials.h"
#include "mqtt.h"
#include "device.h"
#include "telnet.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <esp_task_wdt.h> // For watchdog control

// #define CLEAR_CREDS
int Delay = 100; // Main loop delay in ms (faster for better OTA response)
bool otaInProgress = false; // Flag to pause operations during OTA

void setupOTA()
{
    // Start mDNS for hostname resolution
    if (!MDNS.begin("filter-chlorine"))
    {
        telnet.println("\tError setting up mDNS");
    }
    else
    {
        telnet.println("\tmDNS started: filter-chlorine.local");
    }

    ArduinoOTA.setHostname("filter-chlorine");
    ArduinoOTA.setPassword("admin");
    ArduinoOTA.setPort(3232);

    ArduinoOTA.onStart([]()
                       {
        otaInProgress = true; // Pause other operations
        mqtt.report_disconnect(); // Disconnect MQTT before OTA
        
        // Boost WiFi power for stable OTA transfer
        WiFi.setTxPower(WIFI_POWER_19_5dBm);
        
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.println("\n\tOTA: Updating " + type);
        Serial.println("\tOTA: Disconnecting MQTT...");
        Serial.printf("\tOTA: Free heap: %u bytes\n", ESP.getFreeHeap()); });

    ArduinoOTA.onEnd([]()
                     { 
        otaInProgress = false;
        Serial.println("\n\tOTA: Complete");
        Serial.println("\tOTA: Rebooting..."); });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { 
        static unsigned long lastPrint = 0;
        unsigned long now = millis();
        if (now - lastPrint > 1000) {
            Serial.printf("OTA Progress: %u%% (Heap: %u)\r", 
                (progress / (total / 100)), 
                ESP.getFreeHeap());
            lastPrint = now;
        }
        yield(); // Allow WiFi stack to process packets
    });

    ArduinoOTA.onError([](ota_error_t error)
                       {
        otaInProgress = false; // Reset flag on error
        Serial.print("\tOTA Error: ");
        switch(error) {
            case OTA_AUTH_ERROR: 
                Serial.println("Auth Failed"); 
                break;
            case OTA_BEGIN_ERROR: 
                Serial.println("Begin Failed - Check partition size"); 
                break;
            case OTA_CONNECT_ERROR: 
                Serial.println("Connect Failed"); 
                break;
            case OTA_RECEIVE_ERROR: 
                Serial.println("Receive Failed - Upload interrupted"); 
                break;
            case OTA_END_ERROR: 
                Serial.println("End Failed - Verification error"); 
                break;
            default:
                Serial.println("Unknown error");
                break;
        }
    });

    ArduinoOTA.begin();
    telnet.println("\tOTA ready");
}

void setup()
{
    Serial.begin(115200);
    Serial.println("\n\tChorinator Starting...\n");

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

    Serial.print("\tConnecting to WiFi");
    int connect_attempts = 0;
    const int max_attempts = 40;  // 20 seconds total
    while (WiFi.status() != WL_CONNECTED && connect_attempts < max_attempts)
    {
        delay(500);
        Serial.print(".");
        connect_attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println(" connected!");
        Serial.print("\tIP Address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println(" timeout!");
        Serial.println("\tWill continue trying in background...");
    }

    // Setup services
    setupOTA();

    mqtt.setup(MQTT_HOST, mqtt_user, mqtt_password, MQTT_PORT);
    device.setup();
    telnet.setup();  // Initialize telnet server after WiFi is connected

    Serial.println("\tSetup complete\n");
    telnet.println("\tSetup complete - Telnet ready\n");
}

void loop()
{
    // OTA has highest priority - dedicated fast loop during upload
    if (otaInProgress) {
        ArduinoOTA.handle();
        yield(); // Allow ESP32 to handle background tasks
        return;
    }
    
    // Always handle OTA with high priority
    ArduinoOTA.handle();
    
    // Handle telnet constantly to prevent disconnections
    telnet.loop();
    
    static unsigned long lastLoop = 0;
    if (millis() - lastLoop >= Delay)
    {
        lastLoop = millis();
        //telnet.print(".");

        if (wifi_tools.is_connected)
        {
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