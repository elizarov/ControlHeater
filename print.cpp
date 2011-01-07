#include "print.h"

#include <Metro.h>

#define PRINT_INTERVAL 300 

Metro printTimeout(PRINT_INTERVAL, true);

void println(const char* s) {
  while (!printTimeout.check()); // just wait...
  Serial.println(s);
}

void setupPrint() {
  Serial.begin(57600);  
}

