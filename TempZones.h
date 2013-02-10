#ifndef TEMPZONES_H_
#define TEMPZONES_H_

#include <Arduino.h>
#include "FixNum.h"
#include "TimedValue.h"

class TempZones {
public:
  static const int  N_ZONES = 10;
  static const long TIMEOUT = 10 * Timeout::MINUTE;
  
  typedef FixNum<int, 1> temp_t;
  
  TimedValue<temp_t, TIMEOUT> temp[N_ZONES];

  TempZones();
};

extern TempZones tempZones;

#endif

