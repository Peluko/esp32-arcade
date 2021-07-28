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

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  print_wakeup_reason();

  gamepad_setup(&bleGamepad);

  bleGamepad.begin();

  pinMode(POWER_LED, OUTPUT);
  pinMode(BT_LED, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT);
  esp_sleep_enable_ext1_wakeup(gamepad_get_button_mask(), ESP_EXT1_WAKEUP_ANY_HIGH);
}

#define SLEEP_TIME (10*60*1000)
void loop()
{
  static unsigned long last_active = millis();

  bool activity = false;

  digitalWrite(POWER_LED, HIGH);

  activity = gamepad_read() != 0;
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
