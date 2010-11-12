#include "ds18b20.h"

DS18B20::DS18B20(uint8_t pin) :
	wire(pin),
	period(DS18B20_PERIOD, true),
	head(0),
	tail(0),
	size(0),
	sum(0)
{}

void DS18B20::enqueue(int val) {
	if (val == DS18B20_NONE)
		return;
	// dequeue previous value
	if (size == DS18B20_SIZE) {
		sum -= queue[head++];
		if (head == DS18B20_SIZE) head = 0;
		size--;
	}

	// enqueue new value
	sum += val;
	queue[tail++] = val;
	if (tail == DS18B20_SIZE) tail = 0;
	size++;
}

int DS18B20::readScratchPad() {
	if (!wire.reset())
		return DS18B20_NONE;
	wire.skip();
	wire.write(0xBE); // Read Scratchpad
	byte data[2];
	for (byte i = 0; i < 2; i++) // we need 2 bytes
		data[i] = wire.read();
	return (data[1] << 8) + data[0]; // take the two bytes from the response relating to temperature
}

void DS18B20::startConversion() {
	if (!wire.reset())
		return;
	wire.skip();
	wire.write(0x44, 0); // start conversion
}

void DS18B20::setup() {
	startConversion();
	delay(DS18B20_PERIOD);
}

void DS18B20::read() {
	if (period.check()) {
		enqueue(readScratchPad());
		startConversion();
	}
}
