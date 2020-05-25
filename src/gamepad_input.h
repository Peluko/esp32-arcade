#ifndef GAMEPAD_INPUT_H
#define GAMEPAD_INPUT_H

#include <BleGamepad.h>

void gamepad_setup(BleGamepad *bt_hid);
void gamepad_read();

#endif