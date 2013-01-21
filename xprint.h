#ifndef PRINT_H
#define PRINT_H

#include <Arduino.h>
#include <avr/pgmspace.h>

void setupPrint();

void waitPrint();
void waitPrintln(const char* s);

void printOn_P(Print& out, PGM_P str);

inline void print_P(PGM_P str) { printOn_P(Serial, str); }

#define printOn_C(out, str) { static const char _s[] PROGMEM = str; printOn_P(out, &_s[0]); }
#define print_C(str)        { static const char _s[] PROGMEM = str; print_P(&_s[0]); }

template<typename T> inline void print(T val) {
  Serial.print(val);
}

template<typename T> inline void print(T val, int base) {
  Serial.print(val, base);
} 


#endif

