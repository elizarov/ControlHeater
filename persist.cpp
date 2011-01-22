#include <avr/eeprom.h>

#include "persist.h"

#define MODE       ((uint8_t*)1)
#define FORCE      ((uint8_t*)2)
#define PERIOD     ((uint8_t*)3)
#define DURATION   ((uint8_t*)4)

uint8_t getSavedMode() {
  return eeprom_read_byte(MODE);
}

void setSavedMode(uint8_t mode) {
  eeprom_write_byte(MODE, mode);
}

uint8_t getSavedForce() {
  return eeprom_read_byte(FORCE);

}

void setSavedForce(uint8_t force) {
  eeprom_write_byte(FORCE, force);
}

uint8_t getSavedPeriod() {
  return eeprom_read_byte(PERIOD);
}

void setSavedPeriod(uint8_t period) {
  eeprom_write_byte(PERIOD, period);
}

uint8_t getSavedDuration() {
  return eeprom_read_byte(DURATION);
}

void setSavedDuration(uint8_t duration) {
  eeprom_write_byte(DURATION, duration);
}
