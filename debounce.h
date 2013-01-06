#ifndef DEBOUNCE_H_
#define DEBOUNCE_H_

#include <Arduino.h>

#define DEBOUNCE_TIME 1000L // 1 second

/**
 * Debouncing class that assumes that "update" method is continuously called. 
 */
template<class T> class Debounce {
private:
  T _stableValue;
  T _candidateValue;
  long _lastChangeTime;
public:
  T update(T value);
};

template<class T> T Debounce<T>::update(T value) {
  if (value == _stableValue) {
    // nothing special to do 
  } else if (value != _candidateValue) {
    // value != stableValue && value != candidateValue
    // also happens first time when value changes after stability
    _lastChangeTime = millis();        
  } else { 
    // value == candidateValue != stableValue
    if (millis() - _lastChangeTime >= DEBOUNCE_TIME) {
      _stableValue = value;
    }
  }  
  _candidateValue = value;
  return _stableValue;
}

#endif
