#include <OneWire.h>
#include <EEPROM.h>
#include <Metro.h>

#include "print.h"
#include "ds18b20.h"
#include "state_hal.h"
#include "command_hal.h"
#include "preset_hal.h"
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
  boolean active = isActive();
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

void saveHistory() {
  // note: only save history with valid temperature measurements
  int temp = ds.value();
  if (temp != DS18B20_NONE && hPeriod.check()) {
    byte work = isActive() ? 1 : 0;
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

boolean firstDump = true; 
Metro dump(5000);
char dumpLine[] = "[C:0 s0000000+??.? d+0.00 p00.0 q0.0 w00 i0000-00.0 a000+00.0 u00000000]*";

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
byte highlightPos = indexOf(0, '*');

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

void makeDump(boolean highlight) {
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
  if (highlight) {
    dumpLine[highlightPos] = '*';
    dumpLine[highlightPos + 1] = 0;
  } 
  else {
    dumpLine[highlightPos] = 0;
  }
  println(dumpLine);
  dump.reset();
  firstDump = false;
}

void dumpState() {
  if (dump.check())
    makeDump(firstDump);
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

void writeToBuffer() {
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

void writeValues() {
  if (writeInterval.check()) {
    writeToBuffer();
    flushWriteBuffer();
    writeInterval.interval(PERIODIC_WRITE_INTERVAL);
    writeInterval.reset();
  }
}

//------- CHECK ERROR --------

#define ERROR_TIMEOUT (60 * 60000L) // 1 hour

Metro errorTimeout(100);

void checkError() {
  if (getErrorBits() != 0 && errorTimeout.check()) {
    println("{C:ERROR}*");
    errorTimeout.interval(ERROR_TIMEOUT);
    errorTimeout.reset();
  }
}

//------- SAVE MODE --------

byte savedMode = 0;

#define EEPROM_MODE 1

void setupSavedMode() {
  savedMode = EEPROM.read(EEPROM_MODE);
}

void saveMode() {
  byte mode = getMode(); // atomic read
  if (mode != savedMode && mode != 0) {
    savedMode = mode;
    EEPROM.write(EEPROM_MODE, mode);
    makeDump(true);
  }
}

//------- PARSE COMMAND -------

#define PARSE_0 0
#define PARSE_1 1
#define PARSE_2 2
#define PARSE_ANY 3

byte parseState = PARSE_0;
char parseCmd;

void executeCommand(char cmd) {
  switch (cmd) {
  case '?':
    makeDump(true);
    break;
  case '1':
  case '2':
  case '3':
  case '4':
    changeMode(cmd - '0');
    saveMode();
    break;
  }
}

void parseCommand() {
  while (Serial.available()) {
    char c = Serial.read();
    boolean eol = (c == '\r' || c == '\n' || c == '!');
    switch (parseState) {
    case PARSE_0:
      parseState = eol ? PARSE_0 : (c == 'C' || c == '*') ? PARSE_1
        : PARSE_ANY;
      break;
    case PARSE_1:
      parseCmd = c;
      parseState = eol ? PARSE_0
        : (c == '?' || (c >= '1' && c <= '4')) ? PARSE_2
        : PARSE_ANY;
      break;
    case PARSE_2:
      if (eol) {
        executeCommand(parseCmd);
        parseState = PARSE_0;
      } 
      else
        parseState = PARSE_ANY;
      break;
    case PARSE_ANY:
      parseState = eol ? PARSE_0 : PARSE_ANY;
      break;
    }
  }
}

//------- UPDATE/RESTORE MODE -------

#define HOTWATER_TIMEOUT (90 * 60000L) // 90 mins = 1.5 hours

void updateMode() {
  byte mode = getMode(); // read current mode atomically
  if (mode != 0 && mode != savedMode) {
    // forbit direct transition from OFF to WORKING
    if (savedMode == MODE_OFF && mode == MODE_WORKING)
      changeMode(savedMode);
    saveMode(); // allow all other transitions
  }
  mode = getMode(); // atomic reread
  if (mode == MODE_HOTWATER && millis() - getModeTime(MODE_HOTWATER) > HOTWATER_TIMEOUT) {
    // HOTWATER mode for too long... switch to WORKING
    changeMode(MODE_WORKING);
    saveMode();
  }
}  

//------- BLINKING LED -------

#define LED_PIN 13

Metro led(1000, true);
boolean ledState = false;

void blinkLed() {
  if (led.check()) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }
}

//------- SETUP & MAIN -------

void setup() {
  setupPrint();
  ds.setup();
  setupState();
  setupSavedMode();
  setupCommand();
  println("{C:ControlHeater started}*");
}

void loop() {
  ds.read();
  checkInactive();
  if (checkState())
    makeDump(true);
  dumpState();
  writeValues();
  updateMode();
  saveHistory();
  parseCommand();
  blinkLed();
}

