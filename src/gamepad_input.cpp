
#include <Arduino.h>
#include <driver/rtc_io.h>

#include "Slider.h"

#include "gamepad_input.h"

#define LED_A GPIO_NUM_14
#define LED_B GPIO_NUM_13
#define LED_X GPIO_NUM_27
#define LED_Y GPIO_NUM_12
#define LED_L GPIO_NUM_26
#define LED_R GPIO_NUM_25

#define INPUT_DUP GPIO_NUM_17
#define INPUT_DDOWN GPIO_NUM_15
#define INPUT_DLEFT GPIO_NUM_16
#define INPUT_DRIGHT GPIO_NUM_4

#define INPUT_A GPIO_NUM_35
#define INPUT_B GPIO_NUM_34
#define INPUT_X GPIO_NUM_32
#define INPUT_Y GPIO_NUM_33
#define INPUT_L GPIO_NUM_39
#define INPUT_R GPIO_NUM_36

#define INPUT_SELECT GPIO_NUM_19
#define INPUT_START GPIO_NUM_18

#define INPUT_MENU GPIO_NUM_21

const gpio_num_t buttonLedList[] = {
    LED_Y,
    LED_X,
    LED_R,
    LED_B,
    LED_A,
    LED_L};

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

struct physical_button_t
{
    gpio_num_t gpio_number;
    uint16_t button;
    gpio_num_t led;
    bool pressed_state; // Last pressed stated
    bool changed_state; // State changed last time checked
};

// Definitions and state storage
physical_button_t buttons[] = {
    {INPUT_A, BUTTON_1, LED_A, false},
    {INPUT_B, BUTTON_2, LED_B, false},
    {INPUT_X, BUTTON_3, LED_X, false},
    {INPUT_Y, BUTTON_4, LED_Y, false},
    {INPUT_L, BUTTON_5, LED_L, false},
    {INPUT_R, BUTTON_6, LED_R, false},
    {INPUT_SELECT, BUTTON_7, GPIO_NUM_MAX, false}, // GPIO_NUM_MAX means no led
    {INPUT_START, BUTTON_8, GPIO_NUM_MAX, false}};

physical_button_t menu_button = {INPUT_MENU, 0, GPIO_NUM_MAX, false};

bool dpad_changed;
uint8_t dpad_state;

Slider ledSlider(sizeof(buttonLedList) / sizeof(gpio_num_t));

BleGamepad *bleHid;

void gamepad_setup(BleGamepad *bt_hid)
{
    bleHid = bt_hid;

    for (const auto button : buttons)
    {
        pinMode(button.gpio_number, INPUT_PULLUP);
        if (button.led != GPIO_NUM_MAX)
        {
            pinMode(button.led, OUTPUT);
        }
        rtc_gpio_hold_en(button.gpio_number);
    }

    dpad_state = DPAD_CENTERED;
    for (const auto gpio_number : dpad_gpio_numbers)
    {
        pinMode(gpio_number, INPUT_PULLUP);
        rtc_gpio_hold_en((gpio_num_t)gpio_number);
    }

    pinMode(INPUT_MENU, INPUT_PULLUP);
    rtc_gpio_hold_en(INPUT_MENU);
}

void button_read(physical_button_t &button)
{
    auto pressed = digitalRead(button.gpio_number) != HIGH;
    button.changed_state = pressed != button.pressed_state;
    button.pressed_state = pressed;
}

void button_release_all()
{
    for (auto &button : buttons)
    {
        if (!button.changed_state && button.pressed_state)
        {
            button.pressed_state = false;
            button.changed_state = true;
        }
    }
}

void button_led(const physical_button_t &button, bool on)
{
    if (button.led != GPIO_NUM_MAX)
    {
        digitalWrite(button.led, on ? HIGH : LOW);
    }
}

void button_led_all(bool on)
{
    for (auto &button : buttons)
    {
        button_led(button, on);
    }
}

bool dpad_read()
{
    uint8_t dpad_index = 0;
    for (const auto gpio_number : dpad_gpio_numbers)
    {
        auto input_state = digitalRead(gpio_number);
        dpad_index <<= 1;
        dpad_index |= input_state;
    }
    auto changed = dpad_map[dpad_index] != dpad_state;
    dpad_state = dpad_map[dpad_index];
    return changed;
}

void read_physical_buttons()
{
    button_read(menu_button);
    for (auto &button : buttons)
    {
        button_read(button);
    }
    dpad_changed = dpad_read();
}

bool menu_mode = false;

bool menu_loop()
{
    if (menu_button.changed_state && !menu_button.pressed_state)
    {
        menu_mode = false;
        button_release_all();
        button_led_all(false);
    }
}

bool normal_loop()
{
    if (menu_button.changed_state && !menu_button.pressed_state)
    {
        menu_mode = true;
        button_release_all();
        button_led_all(true);
        return true;
    }

    auto some_pressed = false;

    for (auto &button : buttons)
    {
        if (button.changed_state)
        {
            if (button.pressed_state)
            {
                bleHid->press(button.button);
            }
            else
            {
                bleHid->release(button.button);
            }
        }

        button_led(button, button.pressed_state);
        some_pressed |= button.pressed_state;
    }

    if (dpad_changed)
    {
        bleHid->setHat(dpad_state);
    }
    some_pressed |= dpad_state |= DPAD_CENTERED;

    ledSlider.doLighting(some_pressed,
                         [](int led, bool lighted)
                         {
                             digitalWrite(buttonLedList[led], lighted);
                         });
    return some_pressed;
}

// returns true if some button is pressed or held
bool gamepad_read()
{
    read_physical_buttons();

    if (menu_mode)
    {
        return menu_loop();
    }
    else
    {
        return normal_loop();
    }
}


uint64_t gamepad_get_button_mask()
{
    uint64_t mask = 0;
    for (const auto button : buttons)
    {
        mask |= 1 << button.gpio_number;
    }
    return mask;
}
