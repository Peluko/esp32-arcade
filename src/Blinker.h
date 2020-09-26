#ifndef ESP32_ARCADE_BLINKER_H
#define ESP32_ARCADE_BLINKER_H

class Blinker
{
private:
    int base_off_ms;
    int _current_off_ms;
    int base_on_ms;
    int _current_on_ms;

    int speed;

    unsigned long _last_on;

    void recalculateRates(void);

public:
    Blinker(int off_ms, int on_ms);
    void setRate(int off_ms, int on_ms);
    // Increases/decreases the on and off times by 1/16 increments
    // Positive values decreases times / increases speed
    // Negative values increases times / decreases speed
    void setSpeed(int speed);
    bool checkState(void);
};

#endif
