#include <avr/eeprom.h>

#include "persist.h"

#define MODE       ((byte*)1)
#define FORCE      ((byte*)2)

byte getSavedMode() {
  return eeprom_read_byte(MODE);
}

void setSavedMode(byte mode) {
  eeprom_write_byte(MODE, mode);
}

byte getSavedForce() {
  return eeprom_read_byte(FORCE);

}

void setSavedForce(byte force) {
  eeprom_write_byte(FORCE, force);
}

