#include <OneWire.h>
#include <Metro.h>

#include "print.h"
#include "ds18b20.h"
#include "state_hal.h"
#include "command_hal.h"
#include "preset_hal.h"
#include "persist.h"
#include "parse.h"
#include "dump_config.h"
#include "util.h"

//------- READ TEMPERATURE------

DS18B20 ds(A2); // use pin A2

//------- CHECK ACTIVE/INACTIVE TIME/TEMP -------

long inactiveStartMillis;
int inactiveStartTemp;
int inactiveMinutes;
int inactiveDt;

long activeStartMillis;
int activeStartTemp;
int activeMinutes;
int activeDt;

boolean wasInactive;
boolean wasActive;

void checkInactive() {
  long time = millis();
  boolean active = getActiveBits() != 0;
  int temp = ds.value();
  if (temp == DS18B20_NONE)
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
#define MAX_HISTORY 240 

struct HistoryItem {
  byte work;
  int temp;
};

HistoryItem h[MAX_HISTORY];

byte hHead = 0;
byte hTail = 0;
byte hSize = 0;
int hSumWork = 0;
int hWorkMinutes = 0;
int hDeltaTemp = 0;
Metro hPeriod(15000, true); // 15 sec

inline void saveHistory() {
  // note: only save history with valid temperature measurements
  int temp = ds.value();
  if (temp != DS18B20_NONE && hPeriod.check()) {
    byte work = getActiveBits() != 0 ? 1 : 0;
    hSumWork += work;
    hSize++;
    // enqueue to tail
    h[hTail].work = work;
    h[hTail].temp = ds.value();
    // recompute stats
    hWorkMinutes = hSumWork * 60 / hSize;
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

#define HIGHLIGHT_CHAR '*'

boolean firstDump = true; 
Metro dump(5000);
char dumpLine[] = "[C:0 s0000000+??.? d+0.00 p00.0 q0.0 w00 i000-0.0 a000+0.0 u00000000]* ";

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
POSITIONS('d', ' ', deltaPos, deltaSize)
POSITIONS('p', ' ', presetTempPos, presetTempSize)
POSITIONS('q', ' ', presetTimePos, presetTimeSize)
POSITIONS('w', ' ', workPos, workSize)
POSITIONS('i', '-', inactivePos, inactiveSize)
POSITIONS0(inactivePos + inactiveSize, ' ', inactiveDtPos, inactiveDtSize)
POSITIONS('a', '+', activePos, activeSize)
POSITIONS0(activePos + activeSize, ' ', activeDtPos, activeDtSize)
POSITIONS('u', ']', uptimePos, uptimeSize)

#define DAY_LENGTH_MS (24 * 60 * 60000L)

long daystart = 0;
int updays = 0;

inline void prepareDecimal(int x, int pos, byte size, byte fmt = 0) {
  formatDecimal(x, &dumpLine[pos], size, fmt);
}

int roundTemp1(int x) {
  return (x + (x > 0 ? 5 : -5)) / 10;
}

inline void prepareTemp1(int x, int pos, int size) {
  prepareDecimal(roundTemp1(x), pos, size, 1 | FMT_SIGN);
}

inline void prepareTemp2(int x, int pos, int size) {
  prepareDecimal(x, pos, size, 2 | FMT_SIGN);
}

#define DUMP_REGULAR               0
#define DUMP_FIRST                 HIGHLIGHT_CHAR
#define DUMP_EXTERNAL_MODE_CHANGE  'b'
#define DUMP_RESTORE_OFF_MODE      'r'
#define DUMP_HOTWATER_TIMEOUT      'h'
#define DUMP_CMD_RESPONSE          '?'
#define DUMP_CMD_MODE_CHANGE       'c'
#define DUMP_POWER_LOST            '0'
#define DUMP_POWER_BACK            '1'
#define DUMP_ERROR                 'e'
#define DUMP_NORMAL                'n'

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
  int temp = ds.value();
  if (temp != DS18B20_NONE)
    prepareTemp1(temp, tempPos, tempSize);

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
  long time = millis();
  while ((time - daystart) > DAY_LENGTH_MS) {
    daystart += DAY_LENGTH_MS;
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
  println(dumpLine);
  dump.reset();
  firstDump = false;
}

inline void dumpState() {
  if (dump.check())
    makeDump(firstDump ? DUMP_FIRST : DUMP_REGULAR);
}

//------- WRITE VALUES -------

#define WRITE_BUF_SIZE     60
#define WRITE_BUF_START    3
#define WRITE_BUF_MAX_ITEM 6

#define INITIAL_WRITE_INTERVAL   2000L // 2 sec
#define PERIODIC_WRITE_INTERVAL 60000L // 1 min

Metro writeInterval(INITIAL_WRITE_INTERVAL);
char writeBuf[WRITE_BUF_SIZE] = "!C=";
byte writeBufPos = WRITE_BUF_START;

void flushWriteBuffer() {
  if (writeBufPos == WRITE_BUF_START)
    return;
  writeBuf[writeBufPos] = 0;
  println(&writeBuf[0]);
  writeBufPos = WRITE_BUF_START;
}

void appendToBuffer(char tag, int value, byte fmt = 0) {
  if (writeBufPos + 1 + WRITE_BUF_MAX_ITEM >= WRITE_BUF_SIZE)
    flushWriteBuffer();
  writeBuf[writeBufPos++] = tag;
  byte size = formatDecimal(value, &writeBuf[writeBufPos], WRITE_BUF_MAX_ITEM, fmt | FMT_LEFT | FMT_SPACE);
  writeBufPos += size;
}

inline void writeToBuffer() {
  int temp = ds.value();
  if (temp != DS18B20_NONE)
    appendToBuffer('a', roundTemp1(temp), 1 | FMT_SIGN);
  appendToBuffer('b', hDeltaTemp, 2 | FMT_SIGN);
  appendToBuffer('c', hWorkMinutes);
  appendToBuffer('d', getMode());
  appendToBuffer('e', getErrorBits());
  appendToBuffer('f', getActiveBits());
  appendToBuffer('g', getPresetTemp(), 1);
  appendToBuffer('h', getPresetTime(), 1);
}

inline void writeValues() {
  if (writeInterval.check()) {
    writeToBuffer();
    flushWriteBuffer();
    writeInterval.interval(PERIODIC_WRITE_INTERVAL);
    writeInterval.reset();
  }
}

//------- SAVE MODE --------

byte prevMode;

void saveMode() {
  byte mode = getMode(); // atomic read
  if (mode != 0 && mode != getSavedMode())
    setSavedMode(mode);
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
  case '1':
  case '2':
  case '3':
  case '4':
    changeMode(cmd - '0');
    saveMode();
    makeDump(DUMP_CMD_MODE_CHANGE);
    break;
  }
}

//------- UPDATE/RESTORE MODE -------

inline void updateMode() {
  byte mode = getMode(); // read current mode atomically
  byte savedMode = getSavedMode();
  if (mode != 0 && mode != savedMode) {
    // forbit direct transition from OFF to WORKING
    if (savedMode == MODE_OFF && mode == MODE_WORKING) {
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
  uint8_t hotwaterTimeoutMins = getSavedHotwater();
  if (hotwaterTimeoutMins != 0 &&
      mode == MODE_HOTWATER &&
      (long)(millis() - getModeTime(MODE_HOTWATER)) > (hotwaterTimeoutMins * 60000L))
  {
    // HOTWATER mode for too long... switch to WORKING
    changeMode(MODE_WORKING);
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

//------- CHECK FORCED MODE -------

inline void checkForceAuto() {
  uint8_t period = getSavedPeriod();
  uint8_t duration = getSavedDuration();
  if (period == 0 || duration == 0)
    return; // force-auto is not configured
  if (getActiveBits() != 0) {
    if (activeMinutes >= duration) // is active for duration -- force off
      setForceOn(false);
  } else {
    if (inactiveMinutes >= period) // inactive for period -- force on
      setForceOn(true);
  }
}

inline void checkForce() {
  switch (getSavedForce()) {
  case FORCE_ON:
    setForceOn(true);
    break;
  case FORCE_AUTO:
    checkForceAuto();
    break;
  default:
    setForceOn(false);
  }
}

//------- BLINKING LED -------

#define LED_PIN 13

Metro led(1000, true);
boolean ledState = false;

inline void blinkLed() {
  if (led.check()) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }
}

//------- SETUP & MAIN -------

void setup() {
  setupPrint();
  println("{C:ControlHeater started}*");
  ds.setup();
  setupState();
  setupCommand();
  makeConfigDump();
}

void loop() {
  ds.read();
  checkInactive();
  checkState();
  updateMode();
  saveHistory();
  executeCommand(parseCommand());
  checkError();
  checkForce();
  blinkLed();
  dumpState();
  writeValues();
}

