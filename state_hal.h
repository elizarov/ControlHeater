#ifndef STATE_HAL_H
#define STATE_HAL_H

#include <WProgram.h>

#define MAX_MODE   4
#define STATE_SIZE 7
#define MODE_MASK  ((1 << MAX_MODE) - 1)

// Valid values for mode

#define MODE_WORKING  1
#define MODE_TIMER    2
#define MODE_OFF      3
#define MODE_HOTWATER 4

// Valid bits for state

#define STATE_WORKING_MODE   0
#define STATE_TIMER_MODE     1
#define STATE_OFF_MODE       2
#define STATE_HOTWATER_MODE  3
#define STATE_ERROR          4
#define STATE_ACTIVE         5
#define STATE_TURNED_ON      6

void setupState();
boolean checkState();

// Returns a combination of STATE_xxx bits
byte getState();

// Returns one of MODE_xxx constants
byte getMode();

// Returns time of the last change to the specified mode
long getModeTime(byte mode);

byte getErrorBits();
byte getActiveBits();

// force heater on
void setForceOn(boolean on);

#endif
