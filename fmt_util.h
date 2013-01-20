#ifndef UTIL_H
#define UTIL_H

#include <Arduino.h>

const byte FMT_PREC  = 0x0f;  // define number precision in lower bits
const byte FMT_SIGN  = 0x10;  // print sign at the first position 
const byte FMT_SPACE = 0x20;  // fill with spaces (with zeroes by default)
const byte FMT_LEFT  = 0x40;  // left-align the result (use in combination with FMT_SPACE)

extern const char HEX_CHARS[];

extern byte formatDecimal(int x, char* pos, byte size, byte fmt = 0);
extern byte formatDecimal(long x, char* pos, byte size, byte fmt = 0);

#endif

