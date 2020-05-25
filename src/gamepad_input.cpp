
#include <Arduino.h>

#include "gamepad_input.h"

#define OUTPUT_LED 15

#define INPUT_DUP 17
#define INPUT_DDOWN 18
#define INPUT_DLEFT 19
#define INPUT_DRIGHT 21

#define INPUT_BUTTON_1 22
#define INPUT_BUTTON_2 23

struct button_map_t
{
    uint8_t gpio_number;
    uint16_t button;
};

const button_map_t buttonMap[] = {
    {INPUT_BUTTON_1, BUTTON_1},
    {INPUT_BUTTON_2, BUTTON_2}};

/* The four directional inputs are used to construct a four-bit index over the
   'dpad_map' table to obtain the Gamepad direction code.
   The bit order, MSB first is:
      UP, RIGHT, DOWN, LEFT
*/
const uint8_t dpad_map[] = {
    DPAD_CENTERED,   // 0000
    DPAD_LEFT,       // 0001
    DPAD_DOWN,       // 0010
    DPAD_DOWN_LEFT,  // 0011
    DPAD_RIGHT,      // 0100
    DPAD_CENTERED,   // 0101 -> left + right
    DPAD_DOWN_RIGHT, // 0110
    DPAD_DOWN,       // 0111 -> left + down + right
    DPAD_UP,         // 1000
    DPAD_UP_LEFT,    // 1001
    DPAD_CENTERED,   // 1010 -> up + down
    DPAD_LEFT,       // 1011 -> up + down + left
    DPAD_UP_RIGHT,   // 1100
    DPAD_UP,         // 1101 -> up + right + left
    DPAD_RIGHT,      // 1110 -> up + right + down
    DPAD_CENTERED    // 1111 -> up + right + down + left
};

/* DPAD input pins, used to construct the index to 'dpad_map'. MSB is first.
*/
const uint8_t dpad_gpio_numbers[] = {
    INPUT_DUP,
    INPUT_DRIGHT,
    INPUT_DDOWN,
    INPUT_DLEFT};

uint16_t *button_state;
uint8_t dpad_state;

BleGamepad *bleHid;

void gamepad_setup(BleGamepad *bt_hid)
{
    bleHid = bt_hid;

    button_state = new uint16_t[sizeof(buttonMap) / sizeof(*buttonMap)];
    auto btpt = button_state;
    for (const auto button : buttonMap)
    {
        pinMode(button.gpio_number, INPUT_PULLDOWN);
        *btpt++ = LOW;
    }

    dpad_state = DPAD_CENTERED;
    for (const auto gpio_number : dpad_gpio_numbers)
    {
        pinMode(gpio_number, INPUT_PULLDOWN);
    }

    pinMode(OUTPUT_LED, OUTPUT);
}

void gamepad_read()
{
    auto btpt = button_state;
    for (const auto button : buttonMap)
    {
        auto current_state = digitalRead(button.gpio_number);
        if (current_state != *btpt)
        {
            if (current_state == HIGH)
            {
                bleHid->press(button.button);
                digitalWrite(OUTPUT_LED, HIGH);
            }
            else
            {
                bleHid->release(button.button);
                digitalWrite(OUTPUT_LED, LOW);
            }
        }
        *btpt++ = current_state;
    }

    uint8_t dpad_index = 0;
    uint8_t tmp[4];
    uint8_t *tmpp = tmp;
    for (const auto gpio_number : dpad_gpio_numbers)
    {
        auto input_state = digitalRead(gpio_number);
        dpad_index <<= 1;
        dpad_index |= input_state;
        *tmpp++ = input_state;
    }
    auto current_dpad_state = dpad_map[dpad_index];
    if (current_dpad_state != dpad_state)
    {
        for (const auto s : tmp)
        {
            Serial.printf("Input: %d\n", s);
        }
        Serial.printf("DPad index: %04X, Dpad direction: %d\n", dpad_index, current_dpad_state);
        bleHid->setHat(current_dpad_state);
        dpad_state = current_dpad_state;
    }
}
