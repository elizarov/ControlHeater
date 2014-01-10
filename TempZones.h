#ifndef TEMP_ZONES_H_
#define TEMP_ZONES_H_

#include <Arduino.h>
#include "FixNum.h"
#include "TimedValue.h"

class TempZones {
  public:
    static const int  N_ZONES = 10;
    static const long TIMEOUT = 5 * Timeout::MINUTE;
    
    typedef FixNum<int, 1> temp_t;
    
    TimedValue<temp_t, TIMEOUT> temp[N_ZONES];

    TempZones();
};

extern TempZones tempZones;

#endif /* TEMP_ZONES_H_ */

