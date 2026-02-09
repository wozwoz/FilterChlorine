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
#include "../motor/motor.h"
#include <Adafruit_INA219.h>

// Command table structure for maintainable menu and dispatch
struct Command {
    const char* name;
    const char* alias;
    const char* description;
    const char* category;
};

// Command table - single source of truth for all available commands
const Command COMMAND_TABLE[] PROGMEM = {
    // Status & Info
    {"help",      "?", "Show this help",           "Info"},
    {"status",    "s", "Show device status",       "Info"},
    {"delay",     "d", "Display sample interval",  "Info"},
    {"minutes",   "m", "Show minute count",        "Info"},
    {"remaining", "r", "Time to next sample",      "Info"},
    
    // Control
    {"force",     "f", "Force measurement now",    "Control"},
    {"power",     "p", "Read power sensor now",     "Control"},
    {"reboot",    "",  "Restart device",           "Control"},
    
    // Motor
    {"chlorine",     "c",  "Show motor status",        "Motor"},
    {"forward",   "f",  "Run forward (speed 200)",  "Motor"},
    {"reverse",   "b",  "Run reverse (speed 200)",  "Motor"},
    {"max",       "x",  "Run at max speed (255)",   "Motor"},
    {"speed <n>", "sp",  "Set speed 0-255",          "Motor"},
    {"stop",      "st",  "Stop motor",               "Motor"},
};

const int COMMAND_COUNT = sizeof(COMMAND_TABLE) / sizeof(Command);

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
// Last activity timestamp for keepalive
unsigned long lastActivityMillis = 0;
#define KEEPALIVE_INTERVAL 30000  // Send keepalive every 30 seconds

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
    lastActivityMillis = millis();
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
            // Configure TCP keepalive on the client socket
            telnetClient.setNoDelay(true);
            telnetClient.setTimeout(5000);  // 5 second timeout for operations
            
            // Flush any telnet negotiation bytes sent by client
            delay(50);  // Wait for any initial negotiation bytes
            while (telnetClient.available())
            {
                telnetClient.read();  // Discard telnet protocol bytes
            }
            
            telnetClient.println("Welcome to Chlorine Tank Controller Telnet Interface");
            telnetClient.print("> ");
            commandBuffer = "";
            lastActivityMillis = millis();
        }
    }

    // Handle incoming data from connected client
    if (telnetClient && telnetClient.connected())
    {
        // Send keepalive if needed (idle for too long)
        unsigned long currentMillis = millis();
        if (currentMillis - lastActivityMillis > KEEPALIVE_INTERVAL)
        {
            // Send a null byte as keepalive to prevent timeouts
            telnetClient.write((uint8_t)0);
            lastActivityMillis = currentMillis;
        }

        while (telnetClient.available())
        {
            char c = telnetClient.read();
            lastActivityMillis = millis();  // Reset keepalive timer on activity

            // Process newline - execute command
            if (c == '\n' || c == '\r')
            {
                // Consume the matching newline character if it follows immediately
                // (e.g., \r\n or \n\r pairs)
                if (telnetClient.available())
                {
                    char next = telnetClient.peek();
                    if ((c == '\r' && next == '\n') || (c == '\n' && next == '\r'))
                    {
                        telnetClient.read();  // Consume the second newline character
                    }
                }
                
                telnetClient.println("");
                
                // If buffer is empty and we have a last command, repeat it silently
                if (commandBuffer.length() == 0 && lastCommand.length() > 0)
                {
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

    // HELP - Display available commands (dynamically generated from command table)
    if (cmd == "help" || cmd == "?" || cmd == "h")
    {
        telnetClient.println("Available commands:");
        telnetClient.println("");
        
        // Track current category to print headers
        String currentCategory = "";
        
        for (int i = 0; i < COMMAND_COUNT; i++)
        {
            // Read command from PROGMEM
            char name[20], alias[5], desc[50], category[20];
            strcpy_P(name, COMMAND_TABLE[i].name);
            strcpy_P(alias, COMMAND_TABLE[i].alias);
            strcpy_P(desc, COMMAND_TABLE[i].description);
            strcpy_P(category, COMMAND_TABLE[i].category);
            
            // Print category header if changed
            if (currentCategory != category)
            {
                if (i > 0) telnetClient.println("");  // Blank line between categories
                telnetClient.print("  \033[1;36m");  // Cyan bold
                telnetClient.print(category);
                telnetClient.println(":\033[0m");    // Reset color
                currentCategory = category;
            }
            
            // Format: "    command  (a) - description"
            telnetClient.print("    ");
            telnetClient.print(name);
            
            // Pad to align aliases and descriptions
            int padding = 12 - strlen(name);
            for (int j = 0; j < padding; j++) telnetClient.print(" ");
            
            if (strlen(alias) > 0)
            {
                telnetClient.print("(");
                telnetClient.print(alias);
                telnetClient.print(") ");
            }
            else
            {
                telnetClient.print("    ");
            }
            
            telnetClient.print("- ");
            telnetClient.println(desc);
        }
        
        telnetClient.println("");
    }
    // STATUS - Show device information
    else if (cmd == "status" || cmd == "s")
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
    // MINUTECOUNT - Display the current minute count    
    else if (cmd == "delay" || cmd == "d")
    {
        telnetClient.print("Sample interval: ");
        telnetClient.print(device._SampleTime / 1000);
        telnetClient.println(" seconds");
    }
    else if (cmd == "minutemount" || cmd == "m")
    {
        telnetClient.print("MinuteCount: ");
        telnetClient.print(device.GetMinuteCount() );
        telnetClient.println(" minutes");
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
            telnetClient.print(device._LastMillis / 1000);
            telnetClient.println(" seconds");
            
            telnetClient.print("Remaining: ");
            telnetClient.print((device._LastMillis - currentMillis)     / 1000);
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
        device._LastMillis = 0;  // Setting to 0 triggers immediate measurement
        telnetClient.println("Measurement forced - will execute on next loop iteration.");
    }
    // POWER - Read INA219 power sensor immediately
    else if (cmd == "power" || cmd == "p")
    {
        if (!device.IsDown()) {
            float shunt = device.ina219.getShuntVoltage_mV();
            float bus = device.ina219.getBusVoltage_V();
            float current = device.ina219.getCurrent_mA();
            float load = bus + (shunt / 1000.0);
            float power = load * current;
            
            telnetClient.println("--- INA219 Power Sensor ---");
            telnetClient.print("Bus Voltage:   ");
            telnetClient.print(bus);
            telnetClient.println(" V");
            telnetClient.print("Shunt Voltage: ");
            telnetClient.print(shunt);
            telnetClient.println(" mV");
            telnetClient.print("Load Voltage:  ");
            telnetClient.print(load);
            telnetClient.println(" V (bus + shunt)");
            telnetClient.print("Current:       ");
            telnetClient.print(current);
            telnetClient.println(" mA");
            telnetClient.print("Power:         ");
            telnetClient.print(power);
            telnetClient.println(" mW");
        } else {
            telnetClient.println("INA219 sensor not available (initialization failed)");
        }
    }
    // LEVEL - Display current average tank level
    else if (cmd == "currentlevel" || cmd == "c")
    {
        telnetClient.print("chlorine current: ");
        telnetClient.println(device.GetAmps());

    }
    // MOTOR STATUS - Display motor state
    else if (cmd == "motor")
    {
        telnetClient.print("Motor status: ");
        if (device.motor->isRunning())
        {
            telnetClient.print("Running ");
            telnetClient.print(device.motor->isForward() ? "FORWARD" : "REVERSE");
            telnetClient.print(" at speed ");
            telnetClient.println(device.motor->getSpeed());
        }
        else
        {
            telnetClient.println("STOPPED");
        }
    }
    // MOTOR FORWARD
    else if (cmd == "forward")
    {
        device.motor->forward(200);
        telnetClient.println("Motor running forward at speed 200");
    }
    // MOTOR REVERSE
    else if (cmd == "reverse")
    {
        device.motor->reverse(200);
        telnetClient.println("Motor running reverse at speed 200");
    }
    // MOTOR STOP
    else if (cmd == "stop")
    {
        device.motor->stop();
        telnetClient.println("Motor stopped");
    }
    // MOTOR MAX SPEED
    else if (cmd == "max")
    {
        if (device.motor->isForward())
        {
            device.motor->forward(255);
            telnetClient.println("Motor running forward at MAX speed (255)");
        }
        else
        {
            device.motor->reverse(255);
            telnetClient.println("Motor running reverse at MAX speed (255)");
        }
    }
    // MOTOR CUSTOM SPEED - format: "speed 150" or "speed 255"
    else if (cmd.startsWith("speed "))
    {
        int speed = cmd.substring(6).toInt();
        if (speed >= 0 && speed <= 255)
        {
            if (device.motor->isForward())
            {
                device.motor->forward(speed);
                telnetClient.print("Motor forward at speed ");
            }
            else
            {
                device.motor->reverse(speed);
                telnetClient.print("Motor reverse at speed ");
            }
            telnetClient.println(speed);
        }
        else
        {
            telnetClient.println("Error: Speed must be 0-255");
        }
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
    // Only send data if client is connected (don't try to accept new clients here)
    if (telnetClient && telnetClient.connected())
    {
        telnetClient.print(Msg);
        lastActivityMillis = millis();  // Reset keepalive timer on activity
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

/**
 * Print float to telnet client (without newline)
 * @param f Float value to send
 */
void Telnet::print(float f)
{
    char buffer[16];
    sprintf(buffer, "%.2f", f);
    print(buffer);
}
