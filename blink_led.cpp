#include <Arduino.h>
#include <Metro.h>

#include "blink_led.h"

#define  BLINK_LED_PIN 13

Metro blinkLedTime(1000, true);
boolean blinkLedState = false;

void blinkLed() {
  if (blinkLedTime.check()) {
    blinkLedState = !blinkLedState;
    digitalWrite(BLINK_LED_PIN, blinkLedState);
  }
}


