#ifndef PRINT_H
#define PRINT_H

#include <Arduino.h>
#include <avr/pgmspace.h>

void setupPrint();

void waitPrint();
void waitPrintln(const char* s);

void print_P(Print& out, PGM_P str);
void print_P(PGM_P str);

#endif

