#ifndef USONIC_HP
#define USONIC_HP

#include "mbed.h"

class USonic {
    public:
        USonic(PinName trig, PinName echo);

        int distance();
    private:
        DigitalOut trig;
        DigitalIn echo;
        Timer pulse;
};
#endif