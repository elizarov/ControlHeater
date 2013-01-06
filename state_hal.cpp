#include "state_hal.h"
#include "debounce.h"

#include <Metro.h>

#define STATE_INTERRUPT 0

#define SCAN_STATES  6

#define TURN_ON_PIN        A3
#define TURN_ON_THRESHOLD  100
#define UNKNOWN            0xff

#define ERROR_TIMEOUT   2000L // 2 sec

volatile byte scanState; // current STATE_XXX bits from scanState
volatile byte curMode;  // current MODE_XXX
volatile long modeTime[MAX_MODE + 1]; // last time mode was active
volatile int readCounter; // number of times interrupt pin was triggered
volatile byte errorCnt; // # of times error seen in ERROR_TIMEOUT = 0, 1, or 2+
volatile long lastErrorTime; // last time error state was seen
volatile byte turnedOnCache; // cached value of turned on state or UNKNOWN

Debounce<byte> stableErrorBits;
Debounce<byte> stableActiveBits;

boolean forcedOn; // last setForceOn value

//------- READ STATE ------

const byte statePins[SCAN_STATES] = { 3, 7, 6, 5, 4, 8 };

void scanStateInterruptHandler() {
  byte newState = 0;
  for (byte i = 0; i < SCAN_STATES; i++)
    bitWrite(newState, i, !digitalRead(statePins[i]));
  long time = millis();
  if (scanState != newState) {
    scanState = newState;
    byte newMode = curMode;
    for (int i = 0; i < MAX_MODE; i++)
      if ((newState & MODE_MASK) == (1 << i)) {
        newMode = i + 1;
        break;
      }
    if (curMode != newMode) {
      modeTime[newMode] = time;
      curMode = newMode;
    }
  }
  if (newState & (1 << STATE_ERROR)) {
    if (time - lastErrorTime < ERROR_TIMEOUT)
      errorCnt = 2;
    else
      errorCnt = 1;
    lastErrorTime = time;
  } else if (time - lastErrorTime >= ERROR_TIMEOUT) {
    errorCnt = 0;
    lastErrorTime = time - 2 * ERROR_TIMEOUT;
  }
  readCounter++;
  turnedOnCache = UNKNOWN;
}

void setupState() {
  lastErrorTime = millis() - 2 * ERROR_TIMEOUT; // do not report error initially
  attachInterrupt(STATE_INTERRUPT, scanStateInterruptHandler, FALLING);
}

//------- CHECK READ COUNTER -------

Metro checkStatePeriod(100, true); // 100 ms

void checkState() {
  long time = millis();
  // atomically check & reset mode if not ticking
  if (checkStatePeriod.check()) {
    noInterrupts();
    if (readCounter == 0 && curMode != 0) { 
      modeTime[0] = time;
      scanState = 0;
      curMode = 0;
      errorCnt = 0; // do not report error condition for getErrorBits
      lastErrorTime = time - 2 * ERROR_TIMEOUT; // but do not report error when turned on
      turnedOnCache = UNKNOWN;
    } else
      readCounter = 0;
    interrupts();
  }
  // make sure errorTime is not far "behind" current time to avoid "rollover" problems
  noInterrupts();
  if (time - lastErrorTime > 2 * ERROR_TIMEOUT)
    lastErrorTime = time - 2 * ERROR_TIMEOUT;
  interrupts();
}

//------- CACHING READ FOR TURNED ON PIN -------

// analog read turned on pin, but no too often
boolean isTurnedOnCached() {
  boolean on;
  byte cache = turnedOnCache; // atomic read
  if (cache == UNKNOWN) {
    on = analogRead(TURN_ON_PIN) > TURN_ON_THRESHOLD;
    turnedOnCache = on ? 1 : 0;
  } else
    on = cache > 0;
  return on;
}

//------- ACCESSORS -------

byte getState() {
  byte state = scanState; // atomic read
  // We rebuild error state here based on internal logic
  state &= ~(1 << STATE_ERROR);
  state |= getErrorBits() << STATE_ERROR;
  // add turned on state
  if (forcedOn || isTurnedOnCached())
    state |= 1 << STATE_TURNED_ON;
  return state;
}

byte getMode() {
  return curMode;  
}

long getModeTime(byte mode) {
  return modeTime[mode];  
}

// internal check -- report error when blinked error light twice in ERROR_TIMEOUT and use debouncing
byte getErrorBits() {
  return stableErrorBits.update(errorCnt > 1 ? 1 : 0);
}

// use debouncing to provide a more stable measurement base
byte getActiveBits() {
  return stableActiveBits.update((getState() >> STATE_ACTIVE) & 3);
}

//------- FORCED_TURN_ON -------

void setForceOn(boolean on) {
  if (on == forcedOn)
    return;
  forcedOn = on;
  if (on) {
    pinMode(TURN_ON_PIN, OUTPUT);
    digitalWrite(TURN_ON_PIN, 1);
  } else {
    pinMode(TURN_ON_PIN, INPUT);
    digitalWrite(TURN_ON_PIN, 0); // no pullup
  }
}

boolean isForceOn() {
  return forcedOn;  
}
