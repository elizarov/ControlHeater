#include "fmt_util.h"

const char HEX_CHARS[17] = "0123456789ABCDEF";

// Replaces all digits with '9' on overflow
static void fillOverflow(char* pos, byte size) {
  for (byte i = 0; i < size; i++)
    if (pos[i] >= '0' && pos[i] < '9')
      pos[i] = '9';
}

// Moves to the left and fills with spaces on the right
static void moveLeft(char* pos, byte size, byte actualSize) {
  for (byte i = 0; i < actualSize; i++)
    pos[i] = pos[i + size - actualSize];
  for (byte i = actualSize; i < size; i++)
    pos[i] = ' ';
}

template<class T> inline byte formatDecimalTemplate(T x, char* pos, byte size, byte fmt) {
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
      sc = ' '; // fill the rest with spaces
    } else if ((fmt & FMT_SIGN) && i == size - 1) {
      *ptr = sc;
      actualSize++;
    } else {
      *ptr = '0' + x % 10;
      x /= 10;
      actualSize++;
    }
  }
  if (x != 0)
    fillOverflow(pos, size);
  if ((fmt & FMT_LEFT) && actualSize < size)
    moveLeft(pos, size, actualSize);
  return actualSize;
}

byte formatDecimal(int x, char* pos, byte size, byte fmt) {
  return formatDecimalTemplate(x, pos, size, fmt);
}

byte formatDecimal(long x, char* pos, byte size, byte fmt) {
  return formatDecimalTemplate(x, pos, size, fmt);
}

