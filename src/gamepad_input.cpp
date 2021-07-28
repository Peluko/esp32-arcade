
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
    bool ph_pressed_state; // Last pressed state
    bool ph_changed_state; // State changed last time checked
    bool autofire;
    bool auto_pressed_state;
    bool auto_changed_state;
    unsigned long autofire_rate;
    unsigned long last_millis;
};

// Definitions and state storage
physical_button_t buttons[] = {
    {INPUT_A, BUTTON_1, LED_A, false, false, false, false, false, 10, 0},
    {INPUT_B, BUTTON_2, LED_B, false, false, false, false, false, 10, 0},
    {INPUT_X, BUTTON_3, LED_X, false, false, false, false, false, 10, 0},
    {INPUT_Y, BUTTON_4, LED_Y, false, false, false, false, false, 10, 0},
    {INPUT_L, BUTTON_5, LED_L, false, false, false, false, false, 10, 0},
    {INPUT_R, BUTTON_6, LED_R, false, false, false, false, false, 10, 0}};

physical_button_t select_button = { INPUT_SELECT, BUTTON_7, GPIO_NUM_MAX, false, false, false, false, false, 10, 0}; // GPIO_NUM_MAX means no led
physical_button_t start_button = { INPUT_START, BUTTON_8, GPIO_NUM_MAX, false, false, false, false, false, 10, 0};
physical_button_t menu_button = {INPUT_MENU, 0, GPIO_NUM_MAX, false, false, false, false, false, 10, 0};

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

    pinMode(INPUT_SELECT, INPUT_PULLUP);
    pinMode(INPUT_START, INPUT_PULLUP);
    pinMode(INPUT_MENU, INPUT_PULLUP);
    rtc_gpio_hold_en(INPUT_MENU);
}

void button_read(physical_button_t *button)
{
    auto pressed = digitalRead(button->gpio_number) != HIGH;
    button->ph_changed_state = pressed != button->ph_pressed_state;
    button->ph_pressed_state = pressed;

    if(button->autofire && pressed) {
        auto now = millis();
        auto ellapsed = now - button->last_millis;
        auto rate = button->autofire_rate > 10 ? button->autofire_rate : 10;
        if(ellapsed > rate) {
            button->auto_changed_state = true;
            button->auto_pressed_state = !button->auto_pressed_state;
            button->last_millis = now;
        }
    } else {
        button->auto_changed_state = button->ph_changed_state;
        button->auto_pressed_state = button->ph_pressed_state;
    }
}

void button_release(physical_button_t *button)
{
    if (!button->ph_changed_state && button->ph_pressed_state)
    {
        button->ph_pressed_state = false;
        button->ph_changed_state = true;
        button->auto_pressed_state = false;
        button->auto_changed_state = true;
    }
}

void button_release_all()
{
    for (auto &button : buttons)
    {
        button_release(&button);
    }
    button_release(&select_button);
    button_release(&start_button);
    button_release(&menu_button);
}

void dpad_release()
{
    if(!dpad_changed && (dpad_state != DPAD_CENTERED)) {
        dpad_changed = true;
        dpad_state = DPAD_CENTERED;
    }
}

void button_led(const physical_button_t *button, bool on)
{
    if (button->led != GPIO_NUM_MAX)
    {
        digitalWrite(button->led, on ? HIGH : LOW);
    }
}

void button_led_all(bool on)
{
    for (auto &button : buttons)
    {
        button_led(&button, on);
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
    button_read(&start_button);
    button_read(&select_button);
    button_read(&menu_button);
    for (auto &button : buttons)
    {
        button_read(&button);
    }
    dpad_changed = dpad_read();
}

bool menu_mode = false;
physical_button_t *last_button_pressed_on_menu = nullptr;

bool menu_loop()
{
    if (menu_button.ph_changed_state && !menu_button.ph_pressed_state)
    {
        menu_mode = false;
        button_release_all();
        dpad_release();
        button_led_all(false);
        return true;
    }

    if(last_button_pressed_on_menu == nullptr) {
        for (auto &button : buttons)
        {
            if(button.ph_changed_state && button.ph_pressed_state) {
                last_button_pressed_on_menu = &button;
                break;
            }
        }
        if(last_button_pressed_on_menu == nullptr) {
            button_led_all(true);
        } else {
            button_led_all(false);
            button_led(last_button_pressed_on_menu, true);
        }
    } else {
        static unsigned long last_millis = 0;
        static bool autofire_lit = false;
        auto now = millis();
        auto ellapsed = now - last_millis;
        auto rate = last_button_pressed_on_menu->autofire_rate > 10 ? last_button_pressed_on_menu->autofire_rate : last_button_pressed_on_menu->autofire_rate;
        // Serial.println("--");
        // Serial.println(autofire_lit);
        // Serial.println(now);
        // Serial.println(last_millis);
        // Serial.println(rate);
        if(ellapsed > rate) {
            autofire_lit = !autofire_lit;
            last_millis = now;
        }

        if(last_button_pressed_on_menu->autofire) {
            button_led(last_button_pressed_on_menu, autofire_lit);
        } else {
            button_led(last_button_pressed_on_menu, true);
        }

        if(last_button_pressed_on_menu->ph_changed_state && last_button_pressed_on_menu->ph_pressed_state) {
            last_button_pressed_on_menu->autofire = !last_button_pressed_on_menu->autofire;
            last_button_pressed_on_menu->autofire_rate = 10;
        }

        if(start_button.ph_pressed_state) {
            static unsigned long last_millis = 0;
            auto ellapsed = now - last_millis;
            if(ellapsed > 100) {
                last_millis = now;
                last_button_pressed_on_menu->autofire_rate -= 1;
                if(last_button_pressed_on_menu->autofire_rate < 10) {
                    last_button_pressed_on_menu->autofire_rate = 10;
                }
            }
        }
        if(select_button.ph_pressed_state) {
            static unsigned long last_millis = 0;
            auto ellapsed = now - last_millis;
            if(ellapsed > 100) {
                last_millis = now;
                last_button_pressed_on_menu->autofire_rate += 1;
                if(last_button_pressed_on_menu->autofire_rate > 10000) {
                    last_button_pressed_on_menu->autofire_rate = 10000;
                }
            }
        }
    }

    return true;
}

void send_button_hid(physical_button_t *button)
{
    if (button->auto_changed_state)
    {
        if (button->auto_pressed_state)
        {
            bleHid->press(button->button);
        }
        else
        {
            bleHid->release(button->button);
        }
    }
}

bool normal_loop(bool connected)
{
    if (menu_button.ph_changed_state && !menu_button.ph_pressed_state)
    {
        menu_mode = true;
        last_button_pressed_on_menu = nullptr;
        button_release_all();
        dpad_release();
    }

    auto some_pressed = false;

    send_button_hid(&select_button);
    send_button_hid(&start_button);
    for (auto &button : buttons)
    {
        send_button_hid(&button);
        button_led(&button, button.auto_pressed_state);
        some_pressed |= button.auto_pressed_state;
    }

    if (dpad_changed)
    {
        bleHid->setHat(dpad_state);
    }
    some_pressed |= dpad_state |= DPAD_CENTERED;

    ledSlider.doLighting(some_pressed || !connected,
                         [](int led, bool lighted)
                         {
                             digitalWrite(buttonLedList[led], lighted);
                         });
    return some_pressed;
}

// returns true if some button is pressed or held
bool gamepad_read(bool connected)
{
    read_physical_buttons();

    if (menu_mode)
    {
        return menu_loop();
    }
    else
    {
        return normal_loop(connected);
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
