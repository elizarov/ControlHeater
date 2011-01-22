#include <WProgram.h>

#include "dump_config.h"
#include "persist.h"

void makeConfigDump() {
  Serial.print("[C M");
  Serial.print(getSavedMode(), DEC);
  Serial.print(" F");
  Serial.print(getSavedForce(), DEC);
  Serial.print(" P");
  Serial.print(getSavedPeriod(), DEC);
  Serial.print(" D");
  Serial.print(getSavedDuration(), DEC);
  Serial.println("]*");
}

