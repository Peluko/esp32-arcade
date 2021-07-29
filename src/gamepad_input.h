#ifndef GAMEPAD_INPUT_H
#define GAMEPAD_INPUT_H

#include <Arduino.h>
#include <BleGamepad.h>

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

void gamepad_setup(BleGamepad *bt_hid);
bool gamepad_read(bool connected);
uint64_t gamepad_get_button_mask();

#endif