#ifndef TIMED_VALUE_H_
#define TIMED_VALUE_H_

#include "Timeout.h"

/**
 * This class is designed to work with FixNum class as its T or other class
 * that defines "invalid" and "valid" method and comparisons.
 */
template<typename T, unsigned long interval> class TimedValue {
  private:
    T       _value[2];
    byte    _index;
    Timeout _timeout;
    TimedValue(const TimedValue<T, interval>& other); // no copy constructor

    void updateTimeout(T value);

  public:
    TimedValue();

    /** Sets actual value, will be returned by get(). */
    void setValue(T value);

    /** Sets received value, get() will return the max of last two received. */ 
    void setReceived(T value);

    /** Returns max of the last two received values. */
    T get();
};

template<typename T, unsigned long interval> TimedValue<T, interval>::TimedValue() {}

template<typename T, unsigned long interval> void TimedValue<T, interval>::updateTimeout(T value) {
  if (value.valid()) {
    _timeout.reset(interval);
  } else {
    _timeout.disable();
  }
}

template<typename T, unsigned long interval> void TimedValue<T, interval>::setValue(T value) {
  updateTimeout(value);
  _value[0] = value;
  _value[1] = value;
}

template<typename T, unsigned long interval> void TimedValue<T, interval>::setReceived(T value) {
  updateTimeout(value);
  _value[_index] = value;
  _index = 1 - _index;
}

template<typename T, unsigned long interval> T TimedValue<T, interval>::get() {
  if (_timeout.check()) {
    _value[0] = T::invalid();
    _value[1] = T::invalid();
  }
  T result = _value[0];
  if (!(_value[1] < result)) // true when _value[1] is invalid
    result = _value[1];
  return result;  
}

#endif /* TIMED_VALUE_H_ */
