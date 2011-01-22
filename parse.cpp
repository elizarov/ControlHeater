#include <WProgram.h>

#include "parse.h"
#include "persist.h"

#define PARSE_ANY      0
#define PARSE_ATTN     1      // Attention char '!' received, wait for 'C'
#define PARSE_WCMD     2      // '!C' was read, wait for command char
#define PARSE_FORCE    'F'    // '!CF' was read, wait for arg
#define PARSE_PERIOD   'P'    // '!CP' was read, wait for arg
#define PARSE_DURATION 'D'    // '!CD' was read, wait for arg

byte parseState = PARSE_ANY;
byte parseArg;

inline char parseChar(char ch) {
  switch (parseState) {
  case PARSE_ANY:
    if (ch == '!')
      parseState = PARSE_ATTN;
    break;
  case PARSE_ATTN:
    parseState = (ch == 'C') ? PARSE_WCMD : PARSE_ANY;
    break;
  case PARSE_WCMD:
    switch (ch) {
    case CMD_DUMP_STATE: case CMD_DUMP_CONFIG:
    case '1': case '2': case '3': case '4':
      parseState = PARSE_ANY;
      return ch; // command for external processing
    case 'F':
    case 'P':
    case 'D':
      parseState = ch;
      parseArg = 0;
      break;
    default:
      parseState = PARSE_ANY;
    }
    break;
  case PARSE_FORCE:
  case PARSE_PERIOD:
  case PARSE_DURATION:
     if (ch >= '0' && ch <= '9') {
       parseArg *= 10;
       parseArg += ch - '0';
       break;
     }
     if (ch == '\r' || ch == '\n' || ch == '!') {
       switch (parseState) {
       case PARSE_FORCE:
         setSavedForce(parseArg);
         break;
       case PARSE_PERIOD:
         setSavedPeriod(parseArg);
         break;
       case PARSE_DURATION:
         setSavedDuration(parseArg);
         break;
       }
       parseState = PARSE_ANY;
       return CMD_DUMP_CONFIG;
     }
     parseState = PARSE_ANY;
     break;
  }
  return 0;
}

char parseCommand() {
  while (Serial.available()) {
    char cmd = parseChar(Serial.read());
    if (cmd != 0)
      return cmd;
  }
  return 0;
}
