#pragma once


class Telnet {

    public:

        Telnet();

        void setup();
        void loop();
        void print(String);
        void print(const char*);
        void print(int i);
        void print(float f);
        void println(String);
        void println(const char*);
        void processCommand(String cmd);

    private:
        String commandBuffer;
        String lastCommand;  // Store last executed command for repeat functionality

};


extern Telnet telnet;