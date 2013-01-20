#ifndef TIMEOUT_H_
#define TIMEOUT_H_

#include <Arduino.h>

/**
 * Simple timeout class that "fires" once by returning true from its "check" method when
 * previously spcified time interval passes. Auto-repeat is not supported. Call "reset".
 */
class Timeout {
  private:
    unsigned long _time;
  public:
    static const long SECOND = 1000L;
    static const long MINUTE = 60 * SECOND;
    
    Timeout();
    Timeout(long interval);
    boolean check(); 
    boolean enabled();
    void disable();
    void reset(long interval);
};

inline Timeout::Timeout() {}

inline Timeout::Timeout(long interval) { 
  reset(interval);
}

inline boolean Timeout::enabled() {
  return _time != 0;
}

inline void Timeout::disable() {
  _time = 0;
}

#endif

