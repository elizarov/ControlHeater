
/**
 * Main logic that forces heater to work under specific conditions.
 */

#ifndef FORCE_H_
#define FORCE_H_

#include <Arduino.h>
#include "state_hal.h"

class Force {
public:  
  enum Mode {
    OFF   = 0,
    ON    = 1,
    AUTO  = 2,
  };
  
  void check();
  
private:
  boolean       _wasActive;
  unsigned long _lastActiveChangeTime;
  boolean       _wasForced;
  boolean       _wasForcedOff;
  State::Mode   _wasForcedMode;
  Force::Mode   _wasForcedSavedForce;

  boolean isTempBelowForceThreshold();
  boolean isTempBelowPeriodicThreshold();  
  boolean checkAuto();
  boolean checkDuration();
};

extern Force force;

#endif

