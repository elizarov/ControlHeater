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
  print_P(PSTR("[CC M"));
  print(config.mode, DEC);
  print_P(PSTR(" H"));
  print(config.hotwater, DEC);
  print_P(PSTR(" F"));
  print(config.force, DEC);
  print_P(PSTR(" P"));
  print(config.period, DEC);
  print_P(PSTR(" D"));
  print(config.duration, DEC);
  for (byte i = 0; i < TempZones::N_ZONES; i++) {
    Config::Zone& zone = config.zone[i];
    Config::temp_t ta = zone.tempA;
    Config::temp_t tb = zone.tempB;
    Config::temp_t tp = zone.tempP;
    if (ta.valid() || tb.valid() || tp.valid()) {
      print_P(PSTR(" T"));
      print(i, DEC);
      print('{');
      boolean first = true;
      first = printConfigTemp('A', ta, first);
      first = printConfigTemp('B', tb, first);
      first = printConfigTemp('P', tp, first);
      print('}');
    }
  }
  print_P(PSTR("]*\r\n"));
}

void makeZonesDump() {
  waitPrint();
  print_P(PSTR("[CZ"));
  for (byte i = 0; i < TempZones::N_ZONES; i++) {
    TempZones::temp_t temp = tempZones.temp[i];
    if (temp.valid()) {
      print(' ');
      print(i, DEC);
      print(':');
      print(temp);
    }
  }
  print_P(PSTR("]*\r\n"));
}
