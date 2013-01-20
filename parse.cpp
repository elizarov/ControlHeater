#include <Arduino.h>
#include "FixNum.h"
#include "Config.h"
#include "parse.h"

const byte PARSE_ANY    = 0;
const byte PARSE_ATTN   = 1;      // Attention char '!' received, wait for 'C'
const byte PARSE_CMD    = 2;      // '!C' was read, wait for command char
const byte PARSE_TVAL   = 3;      // '!CT'<arg><type> was read, wait for temp value
const byte PARSE_X_ARG  = 4;      // '[' was read, wait for zone id (in parseArg)
const byte PARSE_X_VAL0 = 5;      // '['<arg>':' was read, wait for temp value (skip spaces)
const byte PARSE_X_VAL  = 6;      // .. continues to read value
const byte PARSE_X_FIN  = 7;      // wait for final ']'

const byte PARSE_HOTWATER = 'H';    // '!CH' was read, wait for arg
const byte PARSE_FORCE    = 'F';    // '!CF' was read, wait for arg
const byte PARSE_PERIOD   = 'P';    // '!CP' was read, wait for arg
const byte PARSE_DURATION = 'D';    // '!CD' was read, wait for arg
const byte PARSE_TEMP     = 'T';    // '!CT' was read, wait for arg

const byte TEMP_TYPE_A     = 'A';
const byte TEMP_TYPE_B     = 'B';
const byte TEMP_TYPE_P     = 'P';

byte parseState = PARSE_ANY;
byte parseArg;
byte parseTempType;

typedef FixNumParser<int> temp_parser_t;
temp_parser_t parseTempVal;

inline char parseChar(char ch) {
  boolean eoln = ch == '\r' || ch == '\n';
  switch (parseState) {
    case PARSE_X_FIN:
      if (ch == ']') {
        // temperature packet '['<arg>':' ... <temp> ... ']' is over
        tempZones.temp[parseArg] = parseTempVal;        
        parseState = PARSE_ANY;
        break;
      }
      if (eoln) {
        // line over w/o closing brace!
        parseState = PARSE_ANY;
        break;
      }
      if (ch != '[' && ch != '!')
        break; // wait for more chars
      parseState = PARSE_ANY;  
      // !!! fall through to parse any -- some other packet begin while old one is not over yet
    case PARSE_ANY:
      switch (ch) {
        case '!':
          parseState = PARSE_ATTN;
          break;
        case '[':
          parseState = PARSE_X_ARG;
          parseArg = 0;
          break;     
      }
      break;
    case PARSE_ATTN:
      parseState = (ch == 'C') ? PARSE_CMD : PARSE_ANY;
      break;
    case PARSE_CMD:
      switch (ch) {
        case CMD_DUMP_STATE: 
        case CMD_DUMP_CONFIG:
        case CMD_DUMP_ZONES:
        case '1': 
        case '2': 
        case '3': 
        case '4':
          parseState = PARSE_ANY;
          return ch; // command for external processing
        case PARSE_HOTWATER:
        case PARSE_FORCE:
        case PARSE_PERIOD:
        case PARSE_DURATION:
        case PARSE_TEMP:
          parseState = ch;
          parseArg = 0;
          break;
        default:
          parseState = PARSE_ANY;
      }
      break;
    case PARSE_TEMP:
      switch (ch) {
        case TEMP_TYPE_A: 
        case TEMP_TYPE_B: 
        case TEMP_TYPE_P:
          if (parseArg > TempZones::N_ZONES) {
            parseState = PARSE_ANY;
          } else {
            parseTempType = ch;
            parseState = PARSE_TVAL;
            parseTempVal.reset();
          }
          return 0;        
      }
      // falls through to parse arg
    case PARSE_HOTWATER:
    case PARSE_FORCE:
    case PARSE_PERIOD:
    case PARSE_DURATION:
    case PARSE_X_ARG:
      if (ch >= '0' && ch <= '9') {
        parseArg *= 10;
        parseArg += ch - '0';
        break;
      }
      if (parseState == PARSE_X_ARG) {
        if (ch == ':' && parseArg > 0 && parseArg < TempZones::N_ZONES) {
          parseState = PARSE_X_VAL0;
          parseTempVal.reset();
          break;
        } else {
          parseState = PARSE_ANY;
          break;
        }
      }
      // for states other than PARSE_X_xxx
      if (eoln) {
        switch (parseState) {
          case PARSE_HOTWATER:
            config.hotwater = parseArg;
            break;
          case PARSE_FORCE:
            config.force = (Force::Mode)parseArg;
            break;
          case PARSE_PERIOD:
            config.period = parseArg;
            break;
          case PARSE_DURATION:
            config.duration = parseArg;
            break;
        }
        parseState = PARSE_ANY;
        return CMD_DUMP_CONFIG;
      }
      parseState = PARSE_ANY;
      break;
    case PARSE_TVAL:
      { // block to encapsulate result var
        temp_parser_t::Result result = parseTempVal.parse(ch);
        if (result != temp_parser_t::NUM) {
          if (eoln) {
            Config::temp_t temp = result == temp_parser_t::BAD ? Config::temp_t::invalid() : parseTempVal;
            switch (parseTempType) {
              case TEMP_TYPE_A:
                config.zone[parseArg].tempA = temp;
                break;
              case TEMP_TYPE_B:
                config.zone[parseArg].tempB = temp;
                break;
              case TEMP_TYPE_P:
                config.zone[parseArg].tempP = temp;
                break;
            }
            parseState = PARSE_ANY;
            return CMD_DUMP_CONFIG;
          } else
            parseState = PARSE_ANY;
        }
      }
      break;    
    case PARSE_X_VAL0:
      if (ch == ' ') 
        break; // skip spaces
      parseState = PARSE_X_VAL;  
      // fall through to read number
    case PARSE_X_VAL:
      switch (parseTempVal.parse(ch)) {
        case temp_parser_t::BAD:
          parseState = PARSE_ANY;
          break;
        case temp_parser_t::DONE:
          parseState = PARSE_X_FIN;
          break;  
        case temp_parser_t::NUM:
          break; // continue parsing number
      }
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


