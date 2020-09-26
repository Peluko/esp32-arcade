#ifndef GAMEPAD_INPUT_H
#define GAMEPAD_INPUT_H

#include <BleGamepad.h>

void gamepad_setup(BleGamepad *bt_hid);
uint8_t gamepad_read();
uint64_t gamepad_get_button_mask();

#endif