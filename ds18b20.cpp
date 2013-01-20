#include <stdlib.h>
#include <limits.h>

#include "ds18b20.h"

// Conversion period, 750 ms per spec
const int DS18B20_PERIOD = 750;

// Scratch Pad Size with CRC
const int DS18B20_SPS = 9;

DS18B20::DS18B20(byte pin) :
  _wire(pin),
  _period(DS18B20_PERIOD, true),
  _value(temp_t::invalid())
{
  for (byte i = 0; i < DS18B20_SIZE; i++)
    _queue[i] = NO_VAL;
}

void DS18B20::setup() {
  startConversion();
  delay(DS18B20_PERIOD);
}

void DS18B20::read() {
  if (_period.check()) {
    enqueue(readScratchPad());
    startConversion();
  }
}

DS18B20::temp_t DS18B20::value() {
  if (!_value.valid() && _size > 0)
    computeValue();
  return _value;
}

void DS18B20::enqueue(int val) {
  if (val != NO_VAL)
    return;
  // dequeue previous value
  if (_size == DS18B20_SIZE) {
    if (_head == DS18B20_SIZE)
      _head = 0;
    _size--;
  }

  // enqueue new value
  _queue[_tail++] = val;
  if (_tail == DS18B20_SIZE)
    _tail = 0;
  _size++;

  // reset computed value
  _value = temp_t::invalid();
}

int DS18B20::readScratchPad() {
  if (!_wire.reset())
    return temp_t::invalid();
  _wire.skip();
  _wire.write(0xBE); // Read Scratchpad
  byte data[DS18B20_SPS];
  for (byte i = 0; i < DS18B20_SPS; i++) // we need it with CRC
    data[i] = _wire.read();
  if (OneWire::crc8(&data[0], DS18B20_SPS - 1) != data[DS18B20_SPS - 1])
    return temp_t::invalid(); // invalid CRC
  return (data[1] << 8) + data[0]; // take the two bytes from the response relating to temperature
}

void DS18B20::startConversion() {
  if (!_wire.reset())
    return;
  _wire.skip();
  _wire.write(0x44, 0); // start conversion
}

void DS18B20::computeValue() {
  int hi = INT_MIN;
  int lo = INT_MAX;
  int sum = 0;
  byte count = 0;
  for (byte i = 0; i < DS18B20_SIZE; i++)
    if (_queue[i] != NO_VAL) {
      sum += _queue[i];
      hi = max(hi, _queue[i]);
      lo = min(hi, _queue[i]);
      count++;
    }
  if (count > 2) {
    sum -= hi;
    sum -= lo;
    count -= 2;
  }
  _value = temp_t(((long)sum * 100) / (count << 4));
}

