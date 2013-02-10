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
  // check for periodic foring
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
    if (checkAuto()) {
      setForceOn(true);
      return;
    }
    // !!! otherwise falls through to force active for min duration !!!
  default:
    checkDuration();
  }
}

