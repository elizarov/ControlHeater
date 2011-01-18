#include "state_hal.h"

#include <Metro.h>

#define STATE_INTERRUPT 0

#define INVERSE_STATES 6
#define ERROR_TIMEOUT 2000 // 2 sec

volatile byte curState; // current STATE_XXX bits
volatile byte curMode;  // current MODE_XXX
volatile long modeTime[MAX_MODE + 1]; // last time mode was active
volatile int readCounter; // number of times interrupt pin was triggered
volatile long lastErrorTime = -2 * ERROR_TIMEOUT; // last time error state was seen

//------- READ STATE ------

const byte statePins[STATE_SIZE] = { 3, 7, 6, 5, 4, 8, A3 };

void readState() {
  byte newState = 0;
  for (byte i = 0; i < STATE_SIZE; i++) {
    byte v = digitalRead(statePins[i]);
    if (i < INVERSE_STATES)
      v = !v;
    bitWrite(newState, i, v);
  }
  if (curState != newState) {
    if (curState & (1 << STATE_ERROR))
      lastErrorTime = millis();
    byte newMode = curMode;
    for (int i = 0; i < MAX_MODE; i++)
      if ((newState & MODE_MASK) == (1 << i)) {
        newMode = i + 1;
        break;
      }
    curState = newState;
    if (curMode != newMode) {
      modeTime[newMode] = millis();
      curMode = newMode;
    }
  }
  readCounter++;
}

void setupState() {
  attachInterrupt(STATE_INTERRUPT, readState, FALLING);
}

//------- CHECK READ COUNTER -------

Metro checkStatePeriod(100, true); // 100 ms

boolean checkState() {
  boolean updated = false;
  long time = millis();
  // atomically check & reset mode if not ticking
  if (checkStatePeriod.check()) {
    noInterrupts();
    if (readCounter == 0 && curMode != 0) { 
      modeTime[0] = time;
      curMode = 0;
      updated = true;
    } else
      readCounter = 0;
    interrupts();
  }
  // make sure errorTime is not far "behind" current time to avoid "rollover" problems
  noInterrupts();
  if (time - lastErrorTime > 2 * ERROR_TIMEOUT)
    lastErrorTime = time - 2 * ERROR_TIMEOUT;
  interrupts();
  return updated;
}

//------- ACCESSORS -------

byte getState() {
  noInterrupts();
  byte state = curState;
  long errorTime = lastErrorTime;
  interrupts();
  // error state is "blinking". We correct this blinking here
  if (millis() - errorTime < ERROR_TIMEOUT)
    state |= 1 << STATE_ERROR;
  return state;
}

byte getMode() {
  return curMode;  
}

long getModeTime(byte mode) {
  return modeTime[mode];  
}

byte getErrorBits() {
  return (getState() >> STATE_ERROR) & 1;
}

byte getActiveBits() {
  return (curState >> STATE_ACTIVE) & 3;
}

boolean isActive() {
  return (curState & (1 << STATE_ACTIVE)) != 0;
}
