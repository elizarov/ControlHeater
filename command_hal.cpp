#include "command_hal.h"
#include "state_hal.h"
#include "Timeout.h"

const byte COMMAND_PIN_COUNT = 4;
const byte commandPins[COMMAND_PIN_COUNT] = { 10, 9, 11, 12 };
const long CHANGE_INTERVAL = 300;

void changeMode(State::Mode mode) {
  byte pin = commandPins[mode - 1];
  // try to press button at most 3 times 
  for (byte att = 0; att < 3 && getMode() != mode; att++) {
    digitalWrite(pin, 1); // push button
    // wait until mode changes to desired or timeout
    Timeout changeTimeout(CHANGE_INTERVAL);
    while (getMode() != mode && !changeTimeout.check())
      ; // just spin
    digitalWrite(pin, 0); // release button
    // spin again with released button 
    changeTimeout.reset(CHANGE_INTERVAL);
    while (getMode() != mode && !changeTimeout.check())
      ; // just spin
  }
  // will save current mode anyway (even if we failed to set the mode we wanted to)
}

void setupCommand() {
  for (byte i = 0; i < COMMAND_PIN_COUNT; i++)
    pinMode(commandPins[i], OUTPUT);
}

