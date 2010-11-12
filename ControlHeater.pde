#include <OneWire.h>
#include <EEPROM.h>
#include <Metro.h>

#include "ds18b20.h"

//------- READ TEMPERATURE------

DS18B20 ds(A2); // use pin A2

//------- READ STATE -------

#define STATE_INTERRUPT 0
#define STATE_SIZE 6
#define MAX_MODE 4
#define MODE_MASK ((1 << MAX_MODE) - 1)
#define WORK_MASK (1 << 5)

volatile byte curMode = 0;
volatile byte curState = 0;
volatile int readCounter = 0;

byte statePins[STATE_SIZE] = { 3, 7, 6, 5, 4, 8 };

void readState() {
	byte newState = 0;
	for (byte i = 0; i < STATE_SIZE; i++)
		bitWrite(newState, i, !digitalRead(statePins[i]));
	if (curState != newState) {
		for (int i = 0; i < MAX_MODE; i++)
			if ((newState & MODE_MASK) == (1 << i)) {
				curMode = i + 1;
				break;
			}
		curState = newState;
	}
	readCounter++;
}

void setupRead() {
	attachInterrupt(STATE_INTERRUPT, readState, FALLING);
}

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
	boolean active = curState & WORK_MASK;
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
		byte work = (curState & WORK_MASK) != 0 ? 1 : 0;
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

//------- READ SETTINGS -------

#define PRESET_TEMP_PIN A0
#define PRESET_TIME_PIN A1

int getPresetTemp() {
	int in = analogRead(PRESET_TEMP_PIN);
	return ((361241L + 500) - 324L * in) / 1000;
}

int getPresetTime() {
	int in = analogRead(PRESET_TIME_PIN);
	return ((19571L + 500) - 19L * in) / 1000;
}

//------- DUMP STATE -------

#define TURNED_ON_STATE 6
#define TURNED_ON_PIN A3

boolean firstDump = true; 
Metro dump(5000);
char dumpLine[] = "[C:0 s0000000+??.? d+0.00 p00.0 q0.0 w00 i0000-00.0 a0000+00.0 u00000000]*\n";

int indexOf(int start, char c) {
	for (int i = start; dumpLine[i] != 0; i++)
		if (dumpLine[i] == c)
			return i;
	return 0;
}

#define POSITIONS0(P0,C2,POS,SIZE)             \
	int POS = P0;                              \
	int SIZE = indexOf(POS, C2) - POS;

#define POSITIONS(C1,C2,POS,SIZE)              \
	POSITIONS0(indexOf(0, C1) + 1,C2,POS,SIZE)

int modePos = indexOf(0, ':') + 1;
int statePos = indexOf(0, 's') + 1;
int highlightPos = indexOf(0, '*');

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

void prepareDecimal(int x, int pos, int size, 
		byte prec = 0, boolean sign = false) 
{
	char sc = '+';
	if (x < 0) {
		x = -x;
		sc = '-';
	} else if (x == 0) {
		sc = dumpLine[pos]; // retain previous sign char
	}
	for (int i = 0; i < size; i++) {
		int index = pos + size - 1 - i;
		if (prec != 0 && i == prec) {
			dumpLine[index] = '.';
		} else if (sign && i == size - 1) {
			dumpLine[index] = sc;
		} else {
			dumpLine[index] = '0' + x % 10;
			x /= 10;
		}
	}
}

void prepareTemp1(int x, int pos, int size) {
	prepareDecimal((x + (x > 0 ? 5 : -5)) / 10, pos, size, 1, true);
}

void prepareTemp2(int x, int pos, int size) {
	prepareDecimal(x, pos, size, 2, true);
}

void makeDump(boolean highlight) {
	// atomically read everything
	noInterrupts();
	byte mode = curMode;
	byte state = curState;
	interrupts();

	// prepare state bits
	dumpLine[modePos] = '0' + mode;
	for (byte i = 0; i < STATE_SIZE; i++)
		dumpLine[statePos + i] = '0' + bitRead(state, i);
	dumpLine[statePos + TURNED_ON_STATE] = '0' + digitalRead(TURNED_ON_PIN);
	
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
		dumpLine[highlightPos + 1] = '\n';
	} else {
		dumpLine[highlightPos] = '\n';
		dumpLine[highlightPos + 1] = 0;
	}
	Serial.print(dumpLine);
	dump.reset();
	delay(100); // wait 100 ms to make sure serial data is properly transmitted
	firstDump = false;
}

void dumpState() {
	if (dump.check())
		makeDump(firstDump);
}

//------- CHECK READ COUNTER -------

Metro checkRCPeriod(100, true); // 100 ms

void checkReadCounter() {
	if (checkRCPeriod.check()) {
		boolean cleared = false;
		// atomically check & reset mode if not ticking
		noInterrupts();
		if (readCounter == 0 && curMode != 0) { 
			curMode = 0;
			cleared = true;
		} else
			readCounter = 0;
		interrupts();
		if (cleared)
			makeDump(true);
	}
}

//------- SAVE MODE --------

byte savedMode = 0;

#define EEPROM_MODE 1

void setupSavedMode() {
	savedMode = EEPROM.read(EEPROM_MODE);
}

void saveMode(byte mode) {
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

#define COMMAND_PIN_COUNT 4

byte commandPins[COMMAND_PIN_COUNT] = { 10, 9, 11, 12 };

byte parseState = PARSE_0;
char parseCmd;

Metro changeTimeout(300);

void changeMode(byte mode) {
	byte pin = commandPins[mode - 1];
	// try to press button at most 3 times 
	for (byte att = 0; att < 3 && curMode != mode; att++) {
		digitalWrite(pin, 1); // push button
		// wait until mode changes to desired or timeout
		changeTimeout.reset();
		while (curMode != mode && !changeTimeout.check())
			; // just spin
		digitalWrite(pin, 0); // release button
		// spin again with released button 
		changeTimeout.reset();
		while (curMode != mode && !changeTimeout.check())
			; // just spin
	}
	// save current mode anyway (even if we failed to set the mode we wanted to)
	saveMode(curMode);
}

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
			} else
				parseState = PARSE_ANY;
			break;
		case PARSE_ANY:
			parseState = eol ? PARSE_0 : PARSE_ANY;
			break;
		}
	}
}

void setupCommand() {
	for (byte i = 0; i < COMMAND_PIN_COUNT; i++)
		pinMode(commandPins[i], OUTPUT);
}

//------- UPDATE/RESTORE MODE -------

void updateMode() {
	byte mode = curMode; // read current mode atomically
	if (mode != 0 && mode != savedMode) {
		// mode has changed
		// forbit direct transition from mode 3 (OFF) to mode 1 (WORKING)
		if (savedMode == 3 && mode == 1) {
			changeMode(savedMode);
		} else
			saveMode(mode); // allow all other transitions
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
	// wait 0.5s (make sure XBee on serial port starts)
	delay(500);
	Serial.begin(57600);
	Serial.print("[C STARTING]*\n");
	ds.setup();
	setupRead();
	setupSavedMode();
	setupCommand();
}

void loop() {
	ds.read();
	checkInactive();
	dumpState();
	checkReadCounter();
	updateMode();
	saveHistory();
	parseCommand();
	blinkLed();
}
