#include "mqtt.h"
#include <WiFi.h>
#include "telnet.h"

Telnet::Telnet() 
{
    // Constructor
}

Telnet telnet;

WiFiServer telnetServer(23);
WiFiClient telnetClient;

void Telnet::setup() {
    telnetServer.begin();
    telnetServer.setNoDelay(true);
    Serial.println("\tTelnet server started on port 23");
	Serial.println("IP: " + WiFi.localIP().toString()	);

}


void Telnet::loop() {
    // Generate sensor data
    // Handle incoming data from client
    if (telnetClient && telnetClient.connected()) {
        while (telnetClient.available()) {
            String line = telnetClient.readStringUntil('\n');
            line.trim();
            Serial.print("\tTelnet RX: ");
            Serial.println(line);
        }
    }
}

void Telnet::print( String Msg) {
    // Check for new client connection
    if (!telnetClient || !telnetClient.connected()) {
        telnetClient = telnetServer.available();
    }
    
    // Send data if client is connected
    if (telnetClient && telnetClient.connected()) {
        telnetClient.print(Msg);
    }
}
void Telnet::println( String Msg) {
    // Check for new client connection
    print(Msg + "\r\n");
    

}
void Telnet::print( int i) {
    char buffer[12];
    sprintf(buffer, "%d", i);
    print(buffer);

}
