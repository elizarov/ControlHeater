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
  print(config.mode, DEC);
  print_C(" H");
  print(config.hotwater, DEC);
  print_C(" F");
  print(config.force, DEC);
  print_C(" P");
  print(config.period, DEC);
  print_C(" D");
  print(config.duration, DEC);
  for (byte i = 0; i < TempZones::N_ZONES; i++) {
    Config::Zone& zone = config.zone[i];
    Config::temp_t ta = zone.tempA;
    Config::temp_t tb = zone.tempB;
    Config::temp_t tp = zone.tempP;
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
