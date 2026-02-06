/*
 * Telnet Server Implementation
 * Provides remote command-line interface for debugging and monitoring
 * the Tank Level Controller over TCP port 23
 */

#include "mqtt.h"
#include <WiFi.h>
#include "telnet.h"
#include <Adafruit_BMP280.h>
#include "ACS712.h"
#include "../device/device.h"
#include <Adafruit_INA219.h>

Telnet::Telnet()
{
    // Constructor
}

// Global telnet instance
Telnet telnet;

// Telnet server listening on standard port 23
WiFiServer telnetServer(23);
// Active client connection
WiFiClient telnetClient;

/**
 * Initialize telnet server
 * Starts listening on port 23 and disables Nagle algorithm for responsive interaction
 */
void Telnet::setup()
{
    telnetServer.begin();
    telnetServer.setNoDelay(true);  // Disable buffering for immediate command response
    Serial.println("\tTelnet server started on port 23");
    Serial.println("IP: " + WiFi.localIP().toString());
}

/**
 * Main telnet loop - handles client connections and processes incoming commands
 * Called repeatedly from main loop
 */
void Telnet::loop()
{
    // Check for new client connection
    if (!telnetClient || !telnetClient.connected())
    {
        telnetClient = telnetServer.available();
        if (telnetClient && telnetClient.connected())
        {
            Serial.println("\tTelnet client connected");
            telnetClient.println("Welcome to Tank Level Controller");
            telnetClient.print("> ");
            commandBuffer = "";
        }
    }

    // Handle incoming data from connected client
    if (telnetClient && telnetClient.connected())
    {
        while (telnetClient.available())
        {
            char c = telnetClient.read();

            // Process newline - execute command
            if (c == '\n' || c == '\r')
            {
                telnetClient.println("");
                
                // If buffer is empty and we have a last command, repeat it
                if (commandBuffer.length() == 0 && lastCommand.length() > 0)
                {
                    telnetClient.print("[Repeating: ");
                    telnetClient.print(lastCommand);
                    telnetClient.println("]");
                    processCommand(lastCommand);
                }
                else if (commandBuffer.length() > 0)
                {
                    processCommand(commandBuffer);
                    lastCommand = commandBuffer;  // Store for repeat
                    commandBuffer = "";
                }
                
                telnetClient.print("> ");
            }
            // Handle backspace (ASCII 8 or DEL 127)
            else if (c == 8 || c == 127)
            {
                if (commandBuffer.length() > 0)
                {
                    commandBuffer.remove(commandBuffer.length() - 1);
                    telnetClient.print("\b \b");  // Backspace, space, backspace to erase
                }
            }
            // Handle printable characters
            else if (c >= 32 && c <= 126)
            {
                commandBuffer += c;
                telnetClient.print(c);  // Echo character back to client
            }
        }
    }
}

/**
 * Process and execute telnet commands
 * @param cmd Command string received from client
 */
void Telnet::processCommand(String cmd)
{
    cmd.trim();
    cmd.toLowerCase();

    Serial.print("\tCommand: ");
    Serial.println(cmd);

    // HELP - Display available commands
    if (cmd == "help" || cmd == "?")
    {
        telnetClient.println("Available commands:");
        telnetClient.println("  help       - Show this help");
        telnetClient.println("  status     - Show device status");
        telnetClient.println("  delay      - Display sample interval time");
        telnetClient.println("  level      - Show current tank level");
        telnetClient.println("  force (f)  - Force immediate measurement");
        telnetClient.println("  remaining (r) - Time remaining to next sample");
        telnetClient.println("  reboot     - Restart device");
    }
    // STATUS - Show device information
    else if (cmd == "status")
    {
        telnetClient.println("Device Status: Running");
        telnetClient.print("IP: ");
        telnetClient.println(WiFi.localIP().toString());
        telnetClient.print("Uptime: ");
        telnetClient.print(millis() / 1000);
        telnetClient.println(" seconds");
    }
    // REBOOT - Restart the ESP32
    else if (cmd == "reboot")
    {
        telnetClient.println("Rebooting...");
        delay(1000);
        ESP.restart();
    }
    // DELAY - Display the configured sample interval
    else if (cmd == "delay")
    {
        telnetClient.print("Sample interval: ");
        //telnetClient.print(device._SampleTime / 1000);
        telnetClient.println(" seconds");
    }
    // REMAINING - Show time until next scheduled measurement
    else if (cmd == "remaining" || cmd=="r")
    {
        unsigned long currentMillis = millis();
        
        // Note: _LastMillis stores the NEXT scheduled measurement time, not the last measurement time
        if (currentMillis ) {//< device._LastMillis) {
            // Calculate remaining time until next measurement
            //unsigned long remainingMillis = device._LastMillis - currentMillis;
            //unsigned long elapsedMillis = device._SampleTime - remainingMillis;
            
            telnetClient.print("Elapsed: ");
            //telnetClient.print(elapsedMillis / 1000);
            telnetClient.println(" seconds");
            
            telnetClient.print("Remaining: ");
            //telnetClient.print(remainingMillis / 1000);
            telnetClient.println(" seconds");
        } else {
            // Measurement is overdue
            telnetClient.println("Sample overdue!");
            //unsigned long overdueMillis = currentMillis - device._LastMillis;
            telnetClient.print("Overdue by: ");
            //telnetClient.print(overdueMillis / 1000);
            telnetClient.println(" seconds");
        }
        
        telnetClient.print("Sample interval: ");
        //  telnetClient.print(device._SampleTime / 1000);
        telnetClient.println(" seconds");
    }
    // FORCE - Trigger immediate measurement by resetting timer
    else if (cmd == "force" || cmd=="f")
    {
       // device._LastMillis = 0;  // Setting to 0 triggers immediate measurement
        telnetClient.println("Measurement forced - will execute on next loop iteration.");
    }
    // LEVEL - Display current average tank level
    else if (cmd == "level")
    {
        telnetClient.print("Average tank level: ");
        // Prevent division by zero
          //   if (device._averagetanklevelCount == 0)
        // {
            //device._averagetanklevelCount = 1;
        //}
       // telnetClient.println(device._averagetanklevel / device._averagetanklevelCount);
    }
    // Unknown command
    else if (cmd.length() > 0)
    {
        telnetClient.println("Unknown command. Type 'help' for available commands.");
    }
}

/**
 * Print string to telnet client (without newline)
 * Automatically handles client connection checking
 * @param Msg Message to send
 */
void Telnet::print(String Msg)
{
    // Check for new client connection if none active
    if (!telnetClient || !telnetClient.connected())
    {
        telnetClient = telnetServer.available();
    }

    // Send data if client is connected
    if (telnetClient && telnetClient.connected())
    {
        telnetClient.print(Msg);
    }
}

/**
 * Print C-string to telnet client (without newline)
 * @param Msg Message to send
 */
void Telnet::print(const char *Msg)
{
    print(String(Msg));
}

/**
 * Print string to telnet client with newline
 * @param Msg Message to send
 */
void Telnet::println(String Msg)
{
    print(Msg + "\r\n");
}

/**
 * Print C-string to telnet client with newline
 * @param Msg Message to send
 */
void Telnet::println(const char *Msg)
{
    println(String(Msg));
}

/**
 * Print integer to telnet client (without newline)
 * @param i Integer value to send
 */
void Telnet::print(int i)
{
    char buffer[12];
    sprintf(buffer, "%d", i);
    print(buffer);
}
