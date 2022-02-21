
#include <Arduino.h>
#include <driver/rtc_io.h>

#include "Slider.h"

#include "gamepad_input.h"

#define AUTOFIRE_PCT_CHANGE 5 // (AUTOFIRE_INITIAL_RATE * AUTOFIRE_PCT_CHANGE) / 100 must be >= 1
#define AUTOFIRE_INITIAL_RATE 20
#define AUTOFIRE_MIN_RATE 10
#define AUTOFIRE_MAX_RATE 2000

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


#if !defined USE_DPAD_HAT
// If we want to use joy axis instead of DPAD (recommended for compatibility)
// we use the direction definitions for DPAD and then translate to joy axis
// using this table

// #define DPAD_CENTERED 	0
// #define DPAD_UP 		1
// #define DPAD_UP_RIGHT 	2
// #define DPAD_RIGHT 		3
// #define DPAD_DOWN_RIGHT 4
// #define DPAD_DOWN 		5
// #define DPAD_DOWN_LEFT 	6
// #define DPAD_LEFT 		7
// #define DPAD_UP_LEFT 	8

const signed char dpad_to_xy[9][2] =
    {
        { 0, 0},
        { 0, -128},
        { 127, -128},
        { 127, 0},
        { 127, 127},
        { 0, 127},
        { -128, 127},
        { -128, 0},
        { -128, -128}
    };

#endif

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
    uint16_t alt_button;
    gpio_num_t led;
    unsigned long autofire_rate;
    bool ph_pressed_state; // Last pressed state
    bool ph_changed_state; // State changed last time checked
    bool alt_pressed_state;
    bool alt_changed_state;
    bool autofire;
    bool auto_pressed_state;
    bool auto_changed_state;
    unsigned long last_millis;
};

// Definitions and state storage
physical_button_t buttons[] = {
    {INPUT_B, BUTTON_1, BUTTON_9, LED_B, AUTOFIRE_INITIAL_RATE},
    {INPUT_A, BUTTON_2, BUTTON_10, LED_A, AUTOFIRE_INITIAL_RATE},
    {INPUT_Y, BUTTON_3, BUTTON_11, LED_Y, AUTOFIRE_INITIAL_RATE},
    {INPUT_X, BUTTON_4, BUTTON_12, LED_X, AUTOFIRE_INITIAL_RATE},
    {INPUT_L, BUTTON_5, BUTTON_13, LED_L, AUTOFIRE_INITIAL_RATE},
    {INPUT_R, BUTTON_6, BUTTON_14, LED_R, AUTOFIRE_INITIAL_RATE}};

physical_button_t select_button = { INPUT_SELECT, BUTTON_7, BUTTON_15, GPIO_NUM_MAX, AUTOFIRE_INITIAL_RATE}; // GPIO_NUM_MAX means no led
physical_button_t start_button = { INPUT_START, BUTTON_8, BUTTON_16, GPIO_NUM_MAX, AUTOFIRE_INITIAL_RATE};
physical_button_t menu_button = {INPUT_MENU, 0, 0, GPIO_NUM_MAX, AUTOFIRE_INITIAL_RATE};

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
        // esp_sleep_enable_ext0_wakeup(button.gpio_number, GPIO_INTR_HIGH_LEVEL);
        // rtc_gpio_hold_en(button.gpio_number);
    }

    dpad_state = DPAD_CENTERED;
    for (const auto gpio_number : dpad_gpio_numbers)
    {
        pinMode(gpio_number, INPUT_PULLUP);
        // rtc_gpio_hold_en((gpio_num_t)gpio_number);
    }

    pinMode(INPUT_SELECT, INPUT_PULLUP);
    pinMode(INPUT_START, INPUT_PULLUP);
    pinMode(INPUT_MENU, INPUT_PULLUP);
    // rtc_gpio_hold_en(INPUT_SELECT);
    // rtc_gpio_hold_en(INPUT_START);
    // rtc_gpio_hold_en(INPUT_MENU);
}

void button_read(physical_button_t *button, bool alt)
{
    auto readed = digitalRead(button->gpio_number) != HIGH;
    auto pressed = readed && !alt;
    button->ph_changed_state = pressed != button->ph_pressed_state;
    button->ph_pressed_state = pressed;

    auto alt_pressed = readed & alt;
    button->alt_changed_state = alt_pressed != button->alt_pressed_state;
    button->alt_pressed_state = alt_pressed;

    if(button->autofire && pressed) {
        auto now = millis();
        auto ellapsed = now - button->last_millis;
        auto rate = button->autofire_rate > AUTOFIRE_MIN_RATE ? button->autofire_rate : AUTOFIRE_MIN_RATE;
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
    if (!button->alt_changed_state && button->alt_pressed_state)
    {
        button->alt_pressed_state = false;
        button->alt_changed_state = true;
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
    button_read(&menu_button, false);
    button_read(&start_button, menu_button.ph_pressed_state);
    button_read(&select_button, menu_button.ph_pressed_state);
    for (auto &button : buttons)
    {
        button_read(&button, menu_button.ph_pressed_state);
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
        auto rate = last_button_pressed_on_menu->autofire_rate > AUTOFIRE_MIN_RATE ? last_button_pressed_on_menu->autofire_rate : AUTOFIRE_MIN_RATE;
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
            last_button_pressed_on_menu->autofire_rate = AUTOFIRE_INITIAL_RATE;
        }

        if(start_button.ph_pressed_state) {
            static unsigned long last_millis = 0;
            auto ellapsed = now - last_millis;
            if(ellapsed > 100) {
                last_millis = now;
                auto inc = (last_button_pressed_on_menu->autofire_rate * AUTOFIRE_PCT_CHANGE) / 100;
                inc = inc > 0 ? inc : 1;
                last_button_pressed_on_menu->autofire_rate -= inc;
                if(last_button_pressed_on_menu->autofire_rate < AUTOFIRE_MIN_RATE) {
                    last_button_pressed_on_menu->autofire_rate = AUTOFIRE_MIN_RATE;
                }
            }
        }
        if(select_button.ph_pressed_state) {
            static unsigned long last_millis = 0;
            auto ellapsed = now - last_millis;
            if(ellapsed > 100) {
                last_millis = now;
                auto inc = (last_button_pressed_on_menu->autofire_rate * AUTOFIRE_PCT_CHANGE) / 100;
                inc = inc > 0 ? inc : 1;
                last_button_pressed_on_menu->autofire_rate += inc;
                if(last_button_pressed_on_menu->autofire_rate > AUTOFIRE_MAX_RATE) {
                    last_button_pressed_on_menu->autofire_rate = AUTOFIRE_MAX_RATE;
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
    if (button->alt_changed_state)
    {
        if (button->alt_pressed_state)
        {
            bleHid->press(button->alt_button);
        }
        else
        {
            bleHid->release(button->alt_button);
        }
    }
}

bool normal_loop(bool connected)
{
    static bool some_pressed_while_menu_pressed = false;
    if (menu_button.ph_changed_state)
    {
        if(menu_button.ph_pressed_state) {
            some_pressed_while_menu_pressed = false;
        } else if(!some_pressed_while_menu_pressed) {
            menu_mode = true;
            last_button_pressed_on_menu = nullptr;
            button_release_all();
            dpad_release();
        }
    }

    send_button_hid(&select_button);
    send_button_hid(&start_button);

    auto some_pressed = select_button.auto_pressed_state || select_button.alt_pressed_state || start_button.auto_pressed_state || start_button.alt_pressed_state;
    for (auto &button : buttons)
    {
        send_button_hid(&button);
        button_led(&button, button.auto_pressed_state || button.alt_pressed_state);
        some_pressed |= button.auto_pressed_state || button.alt_pressed_state;
    }

    if (dpad_changed)
    {
#if defined USE_DPAD_HAT
        bleHid->setHat(dpad_state);
#else
        bleHid->setAxes(dpad_to_xy[dpad_state][0], dpad_to_xy[dpad_state][1], 0, 0, 0, DPAD_CENTERED);
#endif
    }
    some_pressed |= dpad_state |= DPAD_CENTERED;

    ledSlider.doLighting(some_pressed || !connected,
                         [](int led, bool lighted)
                         {
                             digitalWrite(buttonLedList[led], lighted);
                         });

    some_pressed_while_menu_pressed |= some_pressed;
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
        mask |= 1ULL << button.gpio_number;
    }
    return mask;
}
