#include "Timeout.h"

boolean Timeout::check() {
  if (!enabled())
    return false;
  if ((long)(millis() - _time) >= 0) {
    disable();
    return true;
  }
  return false;
}

void Timeout::reset(long interval) {
  if (interval < 0) // just in case
    interval = 0; 
  _time = millis() + interval;  
  if (_time == 0) // just in case
    _time = 1;
}

