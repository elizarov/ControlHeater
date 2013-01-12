#include <Metro.h>
#include "state_hal.h"

const int STATE_INTERRUPT = 0;

const int SCAN_STATES = 6;

const int TURN_ON_PIN       = A3;
const int TURN_ON_THRESHOLD = 100;
const int UNKNOWN           = 0xff;

const int ERROR_TIMEOUT = 2000L; // 2 sec

volatile byte scanState; // current STATE_XXX bits from scanState
volatile State::Mode curMode;  // current MODE_XXX
volatile long modeTime[MAX_MODE + 1]; // last time mode was active
volatile int readCounter; // number of times interrupt pin was triggered
volatile byte errorCnt; // # of times error seen in ERROR_TIMEOUT = 0, 1, or 2+
volatile long lastErrorTime; // last time error state was seen
volatile byte turnedOnCache; // cached value of turned on state or UNKNOWN

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
    State::Mode newMode = curMode;
    for (int i = 0; i < MAX_MODE; i++)
      if ((newState & MODE_MASK) == (1 << i)) {
        newMode = (State::Mode)(i + 1);
        break;
      }
    if (curMode != newMode) {
      modeTime[newMode] = time;
      curMode = newMode;
    }
  }
  if (newState & (1 << State::ERROR_LED)) {
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
      curMode = State::MODE_UNKNOWN;
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

// internal check -- report error when blinked error light twice in ERROR_TIMEOUT and use debouncing
byte getErrorBits() {
  return errorCnt > 1 ? 1 : 0;
}

byte getActiveSignalBits() {
  return forcedOn || isTurnedOnCached(); 
}

// use debouncing to provide a more stable measurement base
byte getActiveBits() {
  return ((scanState >> State::ACTIVE_LED) & 1) | (getActiveSignalBits() << 1);
}

byte getState() {
  byte state = scanState; // atomic read
  // We rebuild error state here based on internal logic
  state &= ~(1 << State::ERROR_LED);
  state |= getErrorBits() << State::ERROR_LED;
  // add turned on state (not part of scan state)
  state |= getActiveSignalBits() << State::ACTIVE_SIGNAL;
  return state;
}

State::Mode getMode() {
  return curMode;  
}

long getModeTime(State::Mode mode) {
  return modeTime[mode];  
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
