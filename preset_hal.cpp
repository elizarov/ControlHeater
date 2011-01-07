#include "preset_hal.h"

//------- READ SETTINGS -------

#define PRESET_TEMP_PIN A0
#define PRESET_TIME_PIN A1

int getPresetTemp() {
  int in = analogRead(PRESET_TEMP_PIN);
  return ((361241L + 500) - 324L * in) / 1000;
}

int getPresetTime() {
  int in = analogRead(PRESET_TIME_PIN);
  return ((19571L + 500) - 19L * in) / 1000;
}


