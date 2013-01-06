#include <Arduino.h>

#include "blink_led.h"

#define  BLINK_LED_PIN 13

long blinkSwitchTime;
boolean blinkLedState = false;

void blinkLed(int time) {
  long now = millis();
  if (now - blinkSwitchTime > time) {  
    blinkLedState = !blinkLedState;
    blinkSwitchTime = now;
    digitalWrite(BLINK_LED_PIN, blinkLedState);
  }
}


