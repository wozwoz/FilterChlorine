#pragma once


class Telnet {

    public:

        Telnet();

        void setup();
        void loop();
        void print( String);
        void print( int i);
        void println( String);

    private:

};


extern Telnet telnet;