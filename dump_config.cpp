#include <Arduino.h>

#include "Config.h"

#include "dump_config.h"

void makeConfigDump() {
  Serial.print("[C M");
  Serial.print(config.mode, DEC);
  Serial.print(" H");
  Serial.print(config.hotwater, DEC);
  Serial.print(" F");
  Serial.print(config.force, DEC);
  Serial.print(" P");
  Serial.print(config.period, DEC);
  Serial.print(" D");
  Serial.print(config.duration, DEC);
  Serial.println("]*");
}
