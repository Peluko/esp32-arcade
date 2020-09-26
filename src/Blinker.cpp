#include "Blinker.h"

#include <Arduino.h>

Blinker::Blinker(int off_ms, int on_ms)
{
    base_off_ms = off_ms;
    base_on_ms = on_ms;
    _current_off_ms = off_ms;
    _current_on_ms = on_ms;
    speed = 0;
}

void Blinker::setRate(int off_ms, int on_ms)
{
    base_off_ms = off_ms;
    base_on_ms = on_ms;
    recalculateRates();
}

void Blinker::setSpeed(int speed)
{
    if(this->speed != speed) {
        this->speed = speed;
        recalculateRates();
    }
}

void Blinker::recalculateRates(void)
{
    _current_off_ms = base_off_ms - ((base_off_ms >> 4) * speed);
    _current_on_ms = base_on_ms - ((base_on_ms >> 4) * speed);

    _current_off_ms = _current_off_ms < 0 ? 0 : _current_off_ms;
    _current_on_ms = _current_on_ms < 0 ? 0 : _current_on_ms;
}

bool Blinker::checkState(void)
{
    auto current_ms = millis();
    auto diff = current_ms - _last_on;

    if (diff > _current_off_ms)
    {
        if (diff > (_current_off_ms + _current_on_ms))
        {
            _last_on = current_ms;
            return false;
        }
        else
        {
            return true;
        }
    }
    return false;
}
