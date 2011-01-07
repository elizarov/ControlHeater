#include "util.h"

const char HEX_CHARS[17] = "0123456789ABCDEF";

byte formatDecimal(int x, char* pos, byte size, byte fmt) {
  char sc = (fmt & FMT_SPACE) ? ' ' : '+';
  if (x < 0) {
    x = -x;
    sc = '-';
  }
  byte actualSize = 0;
  byte first = (fmt & FMT_PREC) ? (fmt & FMT_PREC) + 1 : 0;
  char* ptr = pos + size;
  for (byte i = 0; i < size; i++) {
    ptr--;
    if (i + 1 == first) {
      *ptr = '.';
      actualSize++;
    } else if ((fmt & FMT_SPACE) && x == 0 && i > first) {
      *ptr = sc;
      if (sc != ' ')
        actualSize++;
      sc = ' '; // will the rest with space with this branch  
    } else if ((fmt & FMT_SIGN) && i == size - 1) {
      *ptr = sc;
      actualSize++;
    } else {
      *ptr = '0' + x % 10;
      x /= 10;
      actualSize++;
    }
  }
  if ((fmt & FMT_LEFT) && actualSize < size) {
    for (byte i = 0; i < actualSize; i++)
      pos[i] = pos[i + size - actualSize];
    for (byte i = actualSize; i < size; i++)
      pos[i] = ' ';    
  }
  return actualSize;
}

byte formatDecimal(long x, char* pos, byte size, byte fmt) {
  char sc = (fmt & FMT_SPACE) ? ' ' : '+';
  if (x < 0) {
    x = -x;
    sc = '-';
  }
  byte actualSize = 0;
  byte first = (fmt & FMT_PREC) ? (fmt & FMT_PREC) + 1 : 0;
  char* ptr = pos + size;
  for (byte i = 0; i < size; i++) {
    ptr--;
    if (i + 1 == first) {
      *ptr = '.';
      actualSize++;
    } else if ((fmt & FMT_SPACE) && x == 0 && i > first) {
      *ptr = sc;
      if (sc != ' ')
        actualSize++;
      sc = ' '; // will the rest with space with this branch  
    } else if ((fmt & FMT_SIGN) && i == size - 1) {
      *ptr = sc;
      actualSize++;
    } else {
      *ptr = '0' + x % 10;
      x /= 10;
      actualSize++;
    }
  }
  if ((fmt & FMT_LEFT) && actualSize < size) {
    for (byte i = 0; i < actualSize; i++)
      pos[i] = pos[i + size - actualSize];
    for (byte i = actualSize; i < size; i++)
      pos[i] = ' ';    
  }
  return actualSize;
}
