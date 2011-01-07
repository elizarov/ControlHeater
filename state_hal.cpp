#include "state_hal.h"

#include <Metro.h>

#define STATE_INTERRUPT 0

volatile byte curState;
volatile byte curMode;
volatile long modeTime[MAX_MODE + 1];
volatile int readCounter;

//------- READ STATE ------

byte statePins[STATE_SIZE] = { 3, 7, 6, 5, 4, 8, A3 };

void readState() {
  byte newState = 0;
  for (byte i = 0; i < STATE_SIZE; i++)
    bitWrite(newState, i, !digitalRead(statePins[i]));
  if (curState != newState) {
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
  // atomically check & reset mode if not ticking
  if (checkStatePeriod.check()) {
    noInterrupts();
    if (readCounter == 0 && curMode != 0) { 
      modeTime[0] = millis();
      curMode = 0;
      updated = true;
    } else
      readCounter = 0;
    interrupts();
  }
  return updated;
}

//------- ACCESSORS -------

byte getState() {
  return curState;  
}

byte getMode() {
  return curMode;  
}

long getModeTime(byte mode) {
  return modeTime[mode];  
}

boolean isActive() {
  return curState & (1 << STATE_ACTIVE) != 0;
}


