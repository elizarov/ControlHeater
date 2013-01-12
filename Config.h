#ifndef CONFIG_H_
#define CONFIG_H_

#include <Arduino.h>
#include <avr/eeprom.h>

#include "Force.h"

#include "state_hal.h"

class Config {
public:
  template<class T> class Byte {
  public:
    operator T ();
    Byte<T>& operator = (T value);
  private:
    byte _placeholder;
  };
  
  Byte<byte>        _reserved0;
  Byte<State::Mode> mode;       // see state_hal.h enum State::Mode 
  Byte<Force::Mode> force;      // see force.h     enum Force::Mode
  Byte<byte>        period;     // minimal period between activations (minutes)
  Byte<byte>        duration;   // minimal activation duration (minutes)
  Byte<byte>        hotwater;   // max hotwater time (minutes)
  Byte<byte>        _reserved6;
  Byte<byte>        _reserved7;
  Byte<byte>        _reserved8;
  Byte<byte>        _reserved9;
};

template<class T> inline Config::Byte<T>::operator T () {
  return (T)eeprom_read_byte((uint8_t*)this);
}

template<class T> inline Config::Byte<T>& Config::Byte<T>::operator = (T value) {
  eeprom_write_byte((uint8_t*)this, (byte)value);
  return *this;
}

extern Config config;

#endif

