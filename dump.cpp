#include <Arduino.h>
#include "TempZones.h"
#include "Config.h"
#include "dump.h"
#include "xprint.h"

boolean printConfigTemp(char code, Config::temp_t temp, boolean first) {
  if (!temp.valid())
    return first;
  if (first)
    print(' ');  
  print(code);
  print(temp);
  return false;
}

void makeConfigDump() {
  waitPrint();
  print_C("[CC M");
  print(config.mode.read(), DEC);
  print_C(" H");
  print(config.hotwater.read(), DEC);
  print_C(" F");
  print(config.force.read(), DEC);
  print_C(" P");
  print(config.period.read(), DEC);
  print_C(" D");
  print(config.duration.read(), DEC);
  for (byte i = 0; i < TempZones::N_ZONES; i++) {
    Config::Zone& zone = config.zone[i];
    Config::temp_t ta = zone.tempA.read();
    Config::temp_t tb = zone.tempB.read();
    Config::temp_t tp = zone.tempP.read();
    if (ta.valid() || tb.valid() || tp.valid()) {
      print_C(" T");
      print(i, DEC);
      print('{');
      boolean first = true;
      first = printConfigTemp('A', ta, first);
      first = printConfigTemp('B', tb, first);
      first = printConfigTemp('P', tp, first);
      print('}');
    }
  }
  print_C("]*\r\n");
}

void makeZonesDump() {
  waitPrint();
  print_C("[CZ");
  for (byte i = 0; i < TempZones::N_ZONES; i++) {
    TempZones::temp_t temp = tempZones.temp[i];
    if (temp.valid()) {
      print(' ');
      print(i, DEC);
      print(':');
      print(temp);
    }
  }
  print_C("]*\r\n");
}
