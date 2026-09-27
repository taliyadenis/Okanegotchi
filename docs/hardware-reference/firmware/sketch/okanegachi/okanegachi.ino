#include <Arduino.h>
#include "../../../hardware/firmware_pins.h"

#ifndef OKANEGACHI_LOCAL_DEMO
#define OKANEGACHI_LOCAL_DEMO 0
#endif

void setup() {
  Serial.begin(115200);
  pinMode(OkanegachiPins::BUTTON_LEFT, INPUT_PULLUP);
  pinMode(OkanegachiPins::BUTTON_SELECT, INPUT_PULLUP);
  pinMode(OkanegachiPins::BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(OkanegachiPins::BUZZER, OUTPUT);
  digitalWrite(OkanegachiPins::BUZZER, LOW);
  Serial.println(OKANEGACHI_LOCAL_DEMO ? "okanegachi local demo" : "okanegachi production");
}

void loop() {
  delay(50);
}
