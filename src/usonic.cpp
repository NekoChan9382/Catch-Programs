#include "mbed.h"
#include "usonic.hpp"

USonic::USonic(PinName trig, PinName echo) : trig(trig), echo(echo) {
}

int USonic::distance() {
    echo.mode(PullUp);

    long duration;
    trig = 0;
    wait_us(2);
    trig = 1;
    wait_us(5);
    trig = 0;
    pulse.reset();
    pulse.start();
    while(echo);
    pulse.stop();
    duration = pulse.elapsed_time().count();
    return duration / 58;
}