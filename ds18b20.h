#include <WProgram.h>
#include <OneWire.h>
#include <Metro.h>

#define DS18B20_NONE 0x7fff
#define DS18B20_SIZE 10

class DS18B20 {
public:
  DS18B20(byte pin);

  void setup();
  void read();
  int value(); // Returns value in 1/100 of degree Centigrade

private:
  OneWire _wire;
  Metro _period;
  byte _head;
  byte _tail;
  byte _size;
  int _queue[DS18B20_SIZE];
  int _value;

  void enqueue(int val);
  int readScratchPad();
  void startConversion();
  void computeValue();
};
