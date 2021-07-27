#ifndef ESP32_ARCADE_SLIDER_H
#define ESP32_ARCADE_SLIDER_H

#include <Arduino.h>
#include <functional>

class Slider
{
private:
    unsigned long _start_ms;

    unsigned long _light_ms;
    unsigned long _wait_ms;

    int _num_leds;

    bool _reseted;

    void reset();
    bool checkState(int led);

public:
    Slider(int num_leds);
    void doLighting(bool restart, std::function<void(int, bool)> setLedState);
};

#endif

