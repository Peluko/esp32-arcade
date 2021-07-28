
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

struct button_map_t
{
    gpio_num_t gpio_number;
    uint16_t button;
    gpio_num_t led;
};

const button_map_t buttonMap[] = {
    {INPUT_A, BUTTON_1, LED_A},
    {INPUT_B, BUTTON_2, LED_B},
    {INPUT_X, BUTTON_3, LED_X},
    {INPUT_Y, BUTTON_4, LED_Y},
    {INPUT_L, BUTTON_5, LED_L},
    {INPUT_R, BUTTON_6, LED_R},
    {INPUT_SELECT, BUTTON_7, GPIO_NUM_MAX}, // GPIO_NUM_MAX means no led
    {INPUT_START, BUTTON_8, GPIO_NUM_MAX}};

const button_map_t altButtonMap[] = {
    {INPUT_A, BUTTON_9, LED_A},
    {INPUT_B, BUTTON_10, LED_B},
    {INPUT_X, BUTTON_11, LED_X},
    {INPUT_Y, BUTTON_12, LED_Y},
    {INPUT_L, BUTTON_13, LED_L},
    {INPUT_R, BUTTON_14, LED_R},
    {INPUT_SELECT, BUTTON_7, GPIO_NUM_MAX}, // GPIO_NUM_MAX means no led
    {INPUT_START, BUTTON_8, GPIO_NUM_MAX}};

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

uint16_t *button_state;
uint8_t dpad_state;

Slider ledSlider(sizeof(buttonLedList) / sizeof(gpio_num_t));

BleGamepad *bleHid;

void gamepad_setup(BleGamepad *bt_hid)
{
    bleHid = bt_hid;

    button_state = new uint16_t[sizeof(buttonMap) / sizeof(*buttonMap)];
    auto btpt = button_state;
    for (const auto button : buttonMap)
    {
        pinMode(button.gpio_number, INPUT_PULLUP);
        if (button.led != GPIO_NUM_MAX)
        {
            pinMode(button.led, OUTPUT);
        }
        rtc_gpio_hold_en(button.gpio_number);
        *btpt++ = LOW;
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

// x000 -> no changes
// xxx1 -> button pressed
// xx1x -> button released
// x1xx -> direction changed
// 1xxx -> some button is pressed
uint8_t gamepad_read()
{
    static bool menu_mode = true;
    static bool last_menu_pushed = false;
    static bool launch_menu_on_release = false;
    auto menu_pushed = digitalRead(INPUT_MENU) != HIGH;

    // If on menu release it has not been used for
    // key combo, then launch menu
    if(menu_pushed != last_menu_pushed) {
        if(menu_pushed) {
            launch_menu_on_release = true;
        } else {
            menu_mode = launch_menu_on_release;
        }
    }

    auto btpt = button_state;
    uint8_t changes = 0;
    for (const auto button : (menu_pushed ? altButtonMap : buttonMap))
    {
        auto current_state = digitalRead(button.gpio_number);
        if (current_state != *btpt)
        {
            if (current_state == HIGH)
            {
                changes |= 0x02;
                bleHid->release(button.button);
            }
            else
            {
                changes |= 0x01;
                bleHid->press(button.button);
                // Alt button pressed, do not launch menu on button release
                launch_menu_on_release = false;
            }
        }
        if (button.led != GPIO_NUM_MAX)
        {
            digitalWrite(button.led, !current_state);
            if (current_state != HIGH)
            {
                changes |= 0x08;
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
        if (input_state != HIGH)
        {
            changes |= 0x08;
        }
    }
    auto current_dpad_state = dpad_map[dpad_index];
    if (current_dpad_state != dpad_state)
    {
        bleHid->setHat(current_dpad_state);
        dpad_state = current_dpad_state;
        changes |= 0x04;
    }
    ledSlider.doLighting(changes,
                         [](int led, bool lighted)
                         {
                             digitalWrite(buttonLedList[led], lighted);
                         });
    return changes;
}

uint64_t gamepad_get_button_mask()
{
    uint64_t mask = 0;
    for (const auto button : buttonMap)
    {
        mask |= 1 << button.gpio_number;
    }
    return mask;
}
