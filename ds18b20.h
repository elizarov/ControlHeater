#include <Arduino.h>
#include <OneWire.h>
#include "Timeout.h"
#include "FixNum.h"

class DS18B20 {
  public:
    typedef FixNum<int, 2> temp_t;
    
    DS18B20(byte pin);
  
    void setup();
    void read();
    temp_t value(); // Returns value in 1/100 of degree Centigrade
  
  private:
    static const byte DS18B20_SIZE = 10;
    static const int NO_VAL = INT_MAX;
    
    OneWire _wire;
    Timeout _timeout;
    byte _head;
    byte _tail;
    byte _size;
    int _queue[DS18B20_SIZE]; // queue of raw reads in 1/16 of degree Centigrade
    
    temp_t _value; // computed value
  
    void enqueue(int val);
    int readScratchPad();
    void startConversion();
    void computeValue();
};

