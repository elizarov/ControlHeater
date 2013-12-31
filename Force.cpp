#include "Force.h"
#include "Config.h"
#include "Timeout.h"
#include "TempZones.h"
#include "state_hal.h"

Force force;

boolean Force::isTempBelowForceThreshold() {
  State::Mode mode = getMode();
  boolean activeMode;
  switch (mode) {
  case State::MODE_WORKING:
  case State::MODE_TIMER:
    activeMode = true;
    break;
  case State::MODE_OFF:
    activeMode = false;
    break;
  default:
    return false; // unsupported mode  
  }
  for (byte i = 0; i < TempZones::N_ZONES; i++) {
    Config::temp_t configTemp = activeMode ? config.zone[i].tempA.read() : config.zone[i].tempB.read();
    if (tempZones.temp[i].get() < configTemp) 
      return true;
  }
  return false;
}

boolean Force::isTempBelowPeriodicThreshold() {
  for (byte i = 0; i < TempZones::N_ZONES; i++)
    if (tempZones.temp[i].get() < config.zone[i].tempP.read()) 
      return true;
  return false;
}

boolean Force::checkAuto() {
  if (_wasActive)
    return false; // already was active -- nothing to do
  if (isTempBelowForceThreshold())
    return true; // force on because temp is too low
  // check for periodic forcing
  byte period = config.period.read();
  byte duration = config.duration.read();
  if (period == 0 || duration == 0)
    return false; // periodic forcing is not configured
  if (!isTempBelowPeriodicThreshold())
    return false; // don't do periodic forcing as temp is not low enough
  // when inactive for period -- force on
  if (millis() - _lastActiveChangeTime >= period * Timeout::MINUTE) 
      return true; // checkDuration method will turn it off when duration passes    
  return false;    
}

boolean Force::checkDuration() {
  if (_wasActive) {
    if (_wasForcedOff)
      return false; // force was already canceled by some event (like mode change) during this active cycle 
    // keed forced on util it is active for specifed duration 
    boolean keepForced = millis() - _lastActiveChangeTime < config.duration.read() * Timeout::MINUTE;
    // also track changes in operation mode & saved mode and cancel force if any of them changes
    if (keepForced) {
      if (!_wasForced) {
        _wasForcedMode = getMode();
        _wasForcedSavedForce = config.force.read();
      } else if (_wasForcedMode != getMode() || _wasForcedSavedForce != config.force.read()) {
        // something has changed -- cancel force
        keepForced = false;
        _wasForcedOff = true;
      }
    }
    _wasForced = keepForced;
    return !keepForced;
  } else {
    // cleanup state when inactive
    _wasForced = false;
    _wasForcedOff = false;
  }
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
    if (checkAuto())
      setForceOn(true); // turn on when needed
    else if (checkDuration())
      setForceOn(false); // will turn force off after timeout or mode change
    break;
  default:
    // no force -- turn it off;
    setForceOn(false);
  }
}

