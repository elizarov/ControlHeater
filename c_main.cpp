#include <OneWire.h>
#include "Timeout.h"
#include "Force.h"
#include "Config.h"
#include "xprint.h"
#include "ds18b20.h"
#include "state_hal.h"
#include "command_hal.h"
#include "preset_hal.h"
#include "parse.h"
#include "dump.h"
#include "fmt_util.h"
#include "blink_led.h"

//------- ALL TIME DEFS ------

const long STARTUP_DELAY           = 3000L;  // 3 sec

const long INITIAL_DUMP_INTERVAL   = 2000L;  // 2 sec
const long PERIODIC_DUMP_INTERVAL  = 60000L; // 1 min
const long PERIODIC_DUMP_SKEW      = 5000L;  // 5 sec 

const long RESET_CONDITION_WAIT_INTERVAL = 180000L; // 3 min

const int RESET_ACTIVE_MINUTES_THRESHOLD = 50;   // reset when working for 50 mins
const int RESET_TEMP_DROP_THRESHOLD      = -10;  // ... and loosing 0.1 deg C/hour or more
const int RESET_TEMP_ABS_THRESHOLD       = 2100; // ... and temparature is below +21 deg C

const unsigned int BLINK_TIME_FORCED  =  250; // blink two times per second
const unsigned int BLINK_TIME_NORMAL  = 1000; // normal flip one every second

//------- READ TEMPERATURE------

DS18B20 ds(A2); // use pin A2

//------- CHECK ACTIVE/INACTIVE TIME/TEMP -------

unsigned long   inactiveStartMillis;
DS18B20::temp_t inactiveStartTemp;
DS18B20::temp_t inactiveDt(0);
int             inactiveMinutes;

unsigned long   activeStartMillis;
DS18B20::temp_t activeStartTemp;
DS18B20::temp_t activeDt(0);
int             activeMinutes;

boolean         wasInactive;
boolean         wasActive;

void checkInactive() {
  unsigned long time = millis();
  boolean active = getActiveBits() != 0;
  DS18B20::temp_t temp = ds.value();
  if (!temp.valid())
    return;
  if (!wasInactive && !active) {
    inactiveStartMillis = time;
    inactiveStartTemp = temp;
  } 
  if (!active) {
    inactiveMinutes = (time - inactiveStartMillis) / 60000L;
    inactiveDt = temp - inactiveStartTemp;
  }
  if (!wasActive && active) {
    activeStartMillis = time;
    activeStartTemp = temp;
  }
  if (active) {
    activeMinutes = (time - activeStartMillis) / 60000L;
    activeDt = temp - activeStartTemp;
  }
  wasInactive = !active;
  wasActive = active;
}

//------- STATE HISTORY -------

// Keep an hour of history, record every 15 second 
const int MAX_HISTORY = 240; 
const int MAX_WORK_MINUTES = 60;
const long HISTORY_INTERVAL = 15 * Timeout::SECOND;

struct HistoryItem {
  byte            work;
  DS18B20::temp_t temp;
};

HistoryItem     h[MAX_HISTORY];
byte            hHead = 0;
byte            hTail = 0;
byte            hSize = 0;
int             hSumWork = 0;
int             hWorkMinutes = 0;
DS18B20::temp_t hDeltaTemp = 0;
Timeout         hTimeout(HISTORY_INTERVAL);

inline void saveHistory() {
  // note: only save history with valid temperature measurements
  DS18B20::temp_t temp = ds.value();
  if (!temp.valid())
    return;
  if (hTimeout.check()) {
    hTimeout.reset(HISTORY_INTERVAL);
    byte work = getActiveBits() != 0 ? 1 : 0;
    hSumWork += work;
    hSize++;
    // enqueue to tail
    h[hTail].work = work;
    h[hTail].temp = temp;
    // recompute stats
    hWorkMinutes = hSumWork * MAX_WORK_MINUTES / hSize;
    hDeltaTemp = temp - h[hHead].temp;
    // move queue tail
    hTail++;
    if (hTail == MAX_HISTORY)
      hTail = 0;
    if (hTail == hHead) {
      hSumWork -= h[hHead].work;
      hHead++;
      if (hHead == MAX_HISTORY)
        hHead = 0;
      hSize--;
    }
  }
}

//------- DUMP STATE -------

const char HIGHLIGHT_CHAR = '*';

boolean firstDump = true; 
Timeout dumpTimeout(INITIAL_DUMP_INTERVAL);
char dumpLine[] = "[C:0 +??.? e0o0z0;s0000000 d+0.00p00.0q0.0w00i000-0.0a000+0.0u00000000]* ";

byte indexOf(byte start, char c) {
  for (byte i = start; dumpLine[i] != 0; i++)
    if (dumpLine[i] == c)
      return i;
  return 0;
}

#define POSITIONS0(P0,C2,POS,SIZE)                 \
        byte POS = P0;                             \
      	byte SIZE = indexOf(POS, C2) - POS;

#define POSITIONS(C1,C2,POS,SIZE)                  \
        POSITIONS0(indexOf(0, C1) + 1,C2,POS,SIZE)

byte modePos = indexOf(0, ':') + 1;
byte statePos = indexOf(0, 's') + 1;
byte highlightPos = indexOf(0, HIGHLIGHT_CHAR);

POSITIONS0(indexOf(0, '+'), ' ', tempPos, tempSize)

POSITIONS('e', 'o', errorPos, errorSize)
POSITIONS('o', 'z', operationPos, operationSize) 
POSITIONS('z', ';', zonePos, zoneSize) 

POSITIONS('d', 'p', deltaPos, deltaSize)
POSITIONS('p', 'q', presetTempPos, presetTempSize)
POSITIONS('q', 'w', presetTimePos, presetTimeSize)
POSITIONS('w', 'i', workPos, workSize)
POSITIONS('i', '-', inactivePos, inactiveSize)
POSITIONS0(inactivePos + inactiveSize, 'a', inactiveDtPos, inactiveDtSize)
POSITIONS('a', '+', activePos, activeSize)
POSITIONS0(activePos + activeSize, 'u', activeDtPos, activeDtSize)
POSITIONS('u', ']', uptimePos, uptimeSize)

unsigned long daystart = 0;
int updays = 0;

inline void prepareDecimal(int x, int pos, byte size, byte fmt = 0) {
  formatDecimal(x, &dumpLine[pos], size, fmt);
}

typedef FixNum<int, 1> temp1_t;

inline temp1_t roundTemp1(DS18B20::temp_t x) {
  return x;
}

inline void prepareTemp1(DS18B20::temp_t x, int pos, int size) {
  temp1_t x1 = x;
  x1.format(&dumpLine[pos], size, temp1_t::SIGN);
}

inline void prepareTemp2(DS18B20::temp_t x, int pos, int size) {
  x.format(&dumpLine[pos], size, DS18B20::temp_t::SIGN);
}

const char DUMP_REGULAR              = 0;
const char DUMP_FIRST                = HIGHLIGHT_CHAR;
const char DUMP_EXTERNAL_MODE_CHANGE = 'b';
const char DUMP_RESTORE_OFF_MODE     = 'r';
const char DUMP_HOTWATER_TIMEOUT     = 'h';
const char DUMP_CMD_RESPONSE         = '?';
const char DUMP_CMD_MODE_CHANGE      = 'c';
const char DUMP_POWER_LOST           = '0';
const char DUMP_POWER_BACK           = '1';
const char DUMP_ERROR                = 'e';
const char DUMP_NORMAL               = 'n';
const char DUMP_FORCED_ON            = 'f';

void makeDump(char dumpType) {
  // atomically read everything
  noInterrupts();
  byte mode = getMode();
  byte state = getState();
  interrupts();

  // prepare state bits
  dumpLine[modePos] = '0' + mode;
  for (byte i = 0; i < STATE_SIZE; i++)
    dumpLine[statePos + i] = '0' + bitRead(state, i);

  // prepare temperature
  DS18B20::temp_t temp = ds.value();
  if (temp.valid())
    prepareTemp1(temp, tempPos, tempSize);

  // prepeare state info
  prepareDecimal(getErrorBits(), errorPos, errorSize);
  prepareDecimal(getActiveBits(), operationPos, operationSize);
  prepareDecimal(force.getForcedZone(), zonePos, zoneSize);

  // prepare other stuff
  prepareTemp2(hDeltaTemp, deltaPos, deltaSize);
  prepareDecimal(hWorkMinutes, workPos, workSize);
  prepareDecimal(inactiveMinutes, inactivePos, inactiveSize);
  prepareTemp1(inactiveDt, inactiveDtPos, inactiveDtSize);
  prepareDecimal(activeMinutes, activePos, activeSize);
  prepareTemp1(activeDt, activeDtPos, activeDtSize);
  
  // prepare presets
  prepareDecimal(getPresetTemp(), presetTempPos, presetTempSize, 1);
  prepareDecimal(getPresetTime(), presetTimePos, presetTimeSize, 1);

  // prepare uptime
  unsigned long time = millis();
  while (time - daystart > Timeout::DAY) {
    daystart += Timeout::DAY;
    updays++;
  }
  prepareDecimal(updays, uptimePos, uptimeSize - 6);
  time -= daystart;
  time /= 1000; // convert seconds
  prepareDecimal(time % 60, uptimePos + uptimeSize - 2, 2);
  time /= 60; // minutes
  prepareDecimal(time % 60, uptimePos + uptimeSize - 4, 2);
  time /= 60; // hours
  prepareDecimal((int) time, uptimePos + uptimeSize - 6, 2);

  // print
  if (dumpType == DUMP_REGULAR) {
    dumpLine[highlightPos] = 0;
  } else {
    byte i = highlightPos;
    dumpLine[i++] = dumpType;
    if (dumpType != HIGHLIGHT_CHAR)
      dumpLine[i++] = HIGHLIGHT_CHAR; // must end with highlight (signal) char
    dumpLine[i++] = 0; // and the very last char must be zero
  }
  waitPrintln(dumpLine);
  dumpTimeout.reset(PERIODIC_DUMP_INTERVAL + random(-PERIODIC_DUMP_SKEW, PERIODIC_DUMP_SKEW));
  firstDump = false;
}

inline void dumpState() {
  if (dumpTimeout.check())
    makeDump(firstDump ? DUMP_FIRST : DUMP_REGULAR);
}

//------- SAVE MODE --------

State::Mode prevMode;

void saveMode() {
  State::Mode mode = getMode(); // atomic read
  if (mode != 0 && mode != config.mode.read())
    config.mode = mode;
  prevMode = mode; // also store as "previous mode" to track updates
}

//------- EXECUTE COMMANDS -------

void executeCommand(char cmd) {
  switch (cmd) {
  case CMD_DUMP_STATE:
    makeDump(DUMP_CMD_RESPONSE);
    break;
  case CMD_DUMP_CONFIG:
    makeConfigDump();
    break;
  case CMD_DUMP_ZONES:
    makeZonesDump();
    break;
  case '1':
  case '2':
  case '3':
  case '4':
    changeMode((State::Mode)(cmd - '0'));
    saveMode();
    makeDump(DUMP_CMD_MODE_CHANGE);
    break;
  }
}

//------- UPDATE/RESTORE MODE -------

inline void updateMode() {
  State::Mode mode = getMode(); // read current mode atomically
  State::Mode savedMode = config.mode.read();
  if (mode != 0 && mode != savedMode) {
    // forbit direct transition from OFF to WORKING
    if (savedMode == State::MODE_OFF && mode == State::MODE_WORKING) {
      changeMode(savedMode);
      saveMode();
      makeDump(DUMP_RESTORE_OFF_MODE);
    } else {
      // allow all other transitions
      saveMode();
      makeDump(DUMP_EXTERNAL_MODE_CHANGE);
    }
  } else if (mode != prevMode) {
     // transition to zero mode or from zero mode
    makeDump(mode == 0 ? DUMP_POWER_LOST : DUMP_POWER_BACK);
    prevMode = mode;
  }
  mode = getMode(); // atomic reread
  byte hotwaterTimeoutMins = config.hotwater.read();
  if (hotwaterTimeoutMins != 0 &&
      mode == State::MODE_HOTWATER &&
      millis() - getModeTime(State::MODE_HOTWATER) > hotwaterTimeoutMins * Timeout::MINUTE)
  {
    // HOTWATER mode for too long... switch to WORKING
    changeMode(State::MODE_WORKING);
    saveMode();
    makeDump(DUMP_HOTWATER_TIMEOUT);
  }
}  

//------- CHECK ERROR -------

boolean wasError = false;

inline void checkError() {
  boolean isError = getErrorBits() != 0;
  if (wasError != isError) {
    wasError = isError;
    makeDump(isError ? DUMP_ERROR : DUMP_NORMAL);
  }
}

//------- CHECK FOR RESET -------

inline boolean hasResetCondition() {
  if (getErrorBits() != 0) 
    return true; // reset when error 
  DS18B20::temp_t temp = ds.value();
  if (wasActive && activeMinutes >= RESET_ACTIVE_MINUTES_THRESHOLD && 
      hDeltaTemp < RESET_TEMP_DROP_THRESHOLD && 
      temp.valid() && temp < RESET_TEMP_ABS_THRESHOLD)
    return true; // reset when supposed to be working for 30 min, but loosing temperature, and temp is low
  return false;  
}

long lastResetConditionTime;
long lastOkConditionTime;
long resetConditionWaitInterval = RESET_CONDITION_WAIT_INTERVAL;

void checkReset() {
  long now = millis();
  if (!hasResetCondition()) {
    // Ok condition
    lastResetConditionTime = 0;
    if (lastOkConditionTime == 0) // for a first time after reset condition
      lastOkConditionTime = now;
    else if (now - lastOkConditionTime > resetConditionWaitInterval)
      resetConditionWaitInterval = RESET_CONDITION_WAIT_INTERVAL; // ok for long enough -- set interval to default 
    return;
  }
  // Reset condition 
  if (lastResetConditionTime == 0) {
    // for a first time after ok condition
    lastResetConditionTime = now;
    return;  
  }
  long wasResetConditionInterval = now - lastResetConditionTime;
  if (wasResetConditionInterval < resetConditionWaitInterval)
    return; // not long enough... wait
  // long enough -> perform reset
  waitPrint();
  print_C("!RR\r\n"); // send reset signal
  resetConditionWaitInterval *= 2; // next time wait longer
}

//------- SETUP & MAIN -------

void setup() {
  setupPrint();
  ds.setup();
  setupState();
  setupCommand();
  delay(STARTUP_DELAY);
  waitPrint();
  print_C("{C:ControlHeater started}*\r\n");
  makeConfigDump();
}

void loop() {
  ds.read();
  tempZones.temp[0].setValue(ds.value());
  checkInactive();
  checkState();
  updateMode();
  saveHistory();
  executeCommand(parseCommand());
  checkError();
  checkReset();
  if (force.check())
    makeDump(DUMP_FORCED_ON);
  dumpState();
  blinkLed(isForceOn() ? BLINK_TIME_FORCED : BLINK_TIME_NORMAL);
}

