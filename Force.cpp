#include "Force.h"
#include "Config.h"
#include "Timeout.h"

Force force;

void Force::checkDuration() {
  if (_wasActive) {
    if (_wasForcedOff)
      return; // force was canceled by some event (like mode change) during this active cycle 
    // keed forced on util it is active for specifed duration 
    boolean force = millis() - _lastActiveChangeTime < config.duration.read() * Timeout::MINUTE;
    // also track changes in operation mode & saved mode and cancel force if any of them changes
    if (force) {
      if (!_wasForced) {
        _wasForcedMode = getMode();
        _wasForcedSavedForce = config.force.read();
      } else if (_wasForcedMode != getMode() || _wasForcedSavedForce != config.force.read()) {
        // something has changed -- cancel force
        force = false;
        _wasForcedOff = true;
      }
    }
    setForceOn(force);
    _wasForced = force;
  } else {
    // cleanup state when inactive
    _wasForced = false;
    _wasForcedOff = false;
  }
}

void Force::checkAuto() {
  byte period = config.period.read();
  byte duration = config.duration.read();
  if (period == 0 || duration == 0)
    return; // force-auto is not configured
  // when inactive for period -- force on
  if (!_wasActive && millis() - _lastActiveChangeTime >= period * Timeout::MINUTE) 
      setForceOn(true);
  // checkForceDuration method will turn it off when duration passes    
}

void Force::check() {
  boolean isActive = getActiveBits() != 0;
  if (isActive != _wasActive) {
    _lastActiveChangeTime = millis();
    _wasActive = isActive;
  }
  
  switch (config.force.read()) {
  case Force::ON:
    setForceOn(true);
    break;
  case Force::AUTO:
    checkAuto();
    // !!! falls through to force active for min duration !!!
  default:
    checkDuration();
  }
}

