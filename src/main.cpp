#include <Arduino.h>

const int ledPin = 2;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(ledPin, HIGH);
  Serial.println("on");
  delay(500);
  digitalWrite(ledPin, LOW);
  delay(500);
  Serial.println("off");
}
