#ifndef TEMPZONES_H_
#define TEMPZONES_H_

#include <Arduino.h>
#include "FixNum.h"

class TempZones {
public:
  static const int N_ZONES = 10;
  
  typedef FixNum<int, 2> temp_t;
  
  temp_t temp[N_ZONES];

  TempZones();
};

extern TempZones tempZones;

#endif

