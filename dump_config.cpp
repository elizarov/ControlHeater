#include "dump_config.h"
#include "persist.h"

void makeConfigDump() {
  Serial.print("[C M");
  Serial.print(getSavedMode(), DEC);
  Serial.print(" F");
  Serial.print(getSavedForce(), DEC);
  Serial.println("]*");
}

