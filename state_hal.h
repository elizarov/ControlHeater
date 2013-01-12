#ifndef STATE_HAL_H
#define STATE_HAL_H

#include <Arduino.h>

const int MAX_MODE   = 4;
const int STATE_SIZE = 7;
const int MODE_MASK  = ((1 << MAX_MODE) - 1);

// Valid values for mode

enum Mode {
  MODE_UNKNOWN  = 0;
  MODE_WORKING  = 1,
  MODE_TIMER    = 2,
  MODE_OFF      = 3,
  MODE_HOTWATER = 4
};

// Valid bits for state

enum State {
  STATE_WORKING_MODE_LED  = 0,
  STATE_TIMER_MODE_LED    = 1,
  STATE_OFF_MODE_LED      = 2,
  STATE_HOTWATER_MODE_LED = 3,
  STATE_ERROR_LED         = 4,
  STATE_ACTIVE_LED        = 5,
  STATE_ACTIVE_SIGNAL     = 6
};

void setupState();
void checkState();

// Returns value of STATE_ERROR_LED
byte getErrorBits();

// Retruns value of  STATE_ACTIVE_LED and STATE_ACTIVE_SIGNAL bits
byte getActiveBits();

// Returns a combination of STATE_xxx bits
byte getState();

// Returns one of MODE_xxx constants
Mode getMode();

// Returns time of the last change to the specified mode
long getModeTime(Mode mode);

// force heater on
void setForceOn(boolean on);
boolean isForceOn();

#endif
