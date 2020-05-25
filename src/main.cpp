#include <Arduino.h>

#include <BleGamepad.h>
#include "gamepad_input.h"

using namespace std;

BleGamepad bleGamepad("ESP32 Arcade", "Peluko", 100);

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  gamepad_setup(&bleGamepad);

  bleGamepad.begin();
}

void loop()
{
  if (bleGamepad.isConnected())
  {
    gamepad_read();
    delay(4);
  }
}

void test2()
{
  static int hatPos = 0;

  hatPos++;

  int button = 1 << (hatPos & 0x000F);

  bleGamepad.press(button);

  bleGamepad.setHat((hatPos & 7) + 1);
  Serial.printf("Hat: %d\n, button: %d\n", hatPos, button);

  delay(1000);
  bleGamepad.release(button);
}

void test1()
{
  int ledPin = 2;
  Serial.println("Press buttons 1 and 14. Move all axes to max. Set DPAD to down right.");
  bleGamepad.press(BUTTON_14);
  bleGamepad.press(BUTTON_1);
  bleGamepad.setAxes(127, 127, 127, 127, 127, 127, DPAD_DOWN_RIGHT);
  delay(500);
  digitalWrite(ledPin, HIGH);
  Serial.println("on");

  Serial.println("Release button 14. Move all axes to min. Set DPAD to centred.");
  bleGamepad.release(BUTTON_14);
  bleGamepad.setAxes(-127, -127, -127, -127, -127, -127, DPAD_CENTERED);
  delay(500);
  digitalWrite(ledPin, LOW);
  Serial.println("off");
}
