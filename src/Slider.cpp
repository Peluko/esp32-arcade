#include "Slider.h"

#include <Arduino.h>

Slider::Slider(int num_leds)
{
    _start_ms = millis();
    _wait_ms = 3000;
    _light_ms = 150;
    _num_leds = num_leds;
    _reseted = true;
}

void Slider::reset()
{
    _start_ms = millis();
}

bool Slider::checkState(int led)
{
    auto duration = _wait_ms + (_num_leds + 2) * _light_ms;
    auto elapsed = (millis() - _start_ms) % duration;

    if(elapsed < _wait_ms) {
        return false;
    }

    auto in_cycle = elapsed - _wait_ms;
    unsigned long led_time_start = (led + 1) * _light_ms - (_light_ms / 2);
    unsigned long led_time_end = led_time_start + _light_ms + (_light_ms / 2);

    return (in_cycle >= led_time_start) && (in_cycle <= led_time_end);
}

void Slider::doLighting(bool restart, std::function<void(int, bool)> setLedState)
{
    if(restart) {
        reset();
        if(_reseted) {
            // don't touch the leds
            return;
        } else {
            // set the leds off
            _reseted = true;
        }
    }

    for (int i = 0; i < _num_leds; i++)
    {
        setLedState(i, checkState(i));
    }
}
