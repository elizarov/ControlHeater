
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
  
  /** Returns true when focing heater ON because temp is too low */
  boolean check();

  byte getForcedZone();
  
private:
  enum AutoReason {
    AUTO_NONE,
    AUTO_TEMP_LOW,
    AUTO_PERIODIC
  };

  boolean       _wasActive;
  unsigned long _lastActiveChangeTime;
  boolean       _wasForced;
  boolean       _wasForcedOff;
  State::Mode   _wasForcedMode;
  Force::Mode   _wasForcedSavedForce;

  byte getForcedZoneImpl();
  boolean isTempBelowForceThreshold();
  boolean isTempBelowPeriodicThreshold();  
  AutoReason checkAuto();
  boolean checkDuration();
};

extern Force force;

#endif

