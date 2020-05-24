#include <Arduino.h>

#include <BleGamepad.h>

BleGamepad bleGamepad;
const int ledPin = 2;

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  bleGamepad.begin();
  pinMode(ledPin, OUTPUT);
}

void loop()
{
  if (bleGamepad.isConnected())
  {
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
}
