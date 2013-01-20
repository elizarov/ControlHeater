#ifndef COMMAND_HAL_H
#define COMMAND_HAL_H

#include <Arduino.h>
#include "state_hal.h"

void changeMode(State::Mode mode);
void setupCommand();

#endif

