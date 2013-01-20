#ifndef TIMEDVALUE_H_
#define TIMEDVALUD_H_

#include "Timeout.h"

/**
 * This class is designed to work with FixNum class as its T or other class
 * that defines "invalid" and "valid" method.
 */

template<typename T, long interval> class TimedValue {
  private:
    T       _value;
    Timeout _timeout;
  public:
    TimedValue();
    TimedValue& operator = (T value);
    operator T();
};

template<typename T, long interval> TimedValue<T, interval>::TimedValue() {}

template<typename T, long interval> TimedValue<T, interval>& TimedValue<T, interval>::operator = (T value) {
  if (value.valid()) {
    _timeout.reset(interval);
  } else {
    _timeout.disable();
  }
  _value = value;
}

template<typename T, long interval> TimedValue<T, interval>::operator T() {
  if (_timeout.check())
    _value = T::invalid();
  return _value;  
}

#endif

