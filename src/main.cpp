#include <Arduino.h>

#include <BleGamepad.h>
#include "gamepad_input.h"
#include "Blinker.h"

using namespace std;

BleGamepad bleGamepad("ESP32 Arcade", "Peluko", 100);

Blinker connectedBlinker(1000, 1000);
Blinker disconnectedBlinker(1000, 100);

#define POWER_LED GPIO_NUM_23
#define BT_LED GPIO_NUM_22

#define ONBOARD_LED 2

#define SLEEP_TIME (5*60*1000)
#define SLEEP_LED_TIME_uS (5 * 1000000)
void setup()
{
  pinMode(POWER_LED, OUTPUT);
  pinMode(BT_LED, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT);

  esp_sleep_enable_ext0_wakeup(INPUT_B, 0);
  esp_sleep_enable_timer_wakeup(SLEEP_LED_TIME_uS);

  if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
    digitalWrite(POWER_LED, HIGH);
    delay(100);
    esp_deep_sleep_start();
    return; // never reached
  }

  gamepad_setup(&bleGamepad);

  bleGamepad.begin();

  Serial.begin(115200);
  Serial.println("");
  Serial.println("ESP32 Gamepad started");
}

void loop()
{
  static unsigned long last_active = millis();

  bool activity = false;

  digitalWrite(POWER_LED, HIGH);

  activity = gamepad_read(bleGamepad.isConnected()) != 0;
  if (bleGamepad.isConnected())
  {
    digitalWrite(ONBOARD_LED, connectedBlinker.checkState() ? HIGH : LOW);
    digitalWrite(BT_LED, connectedBlinker.checkState() ? HIGH : LOW);
  }
  else
  {
    digitalWrite(ONBOARD_LED, disconnectedBlinker.checkState() ? HIGH : LOW);
    digitalWrite(BT_LED, disconnectedBlinker.checkState() ? HIGH : LOW);
  }

  auto current_ms = millis();
  if(activity) {
    last_active = current_ms;
  } else if((current_ms - last_active) > SLEEP_TIME) {
    Serial.println("Going to sleep...");
    esp_deep_sleep_start();
  }

  delay(4);
}
