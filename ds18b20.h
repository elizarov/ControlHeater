#include <WProgram.h>
#include <OneWire.h>
#include <Metro.h>

#define DS18B20_PERIOD 1000
#define DS18B20_NONE 0x7fff
#define DS18B20_SIZE 10

class DS18B20 {
private:
	OneWire wire;
	Metro period;
	byte head;
	byte tail;
	byte size;
	int queue[DS18B20_SIZE];
	int sum; // sum of size values, each value is in 1/16 of degree Centigrade
	void enqueue(int val);
	int readScratchPad();
	void startConversion();
public:
	DS18B20(uint8_t pin);

	/* Returns value in 1/100 of degree Centigrade */
	inline int value() { return size == 0 ? DS18B20_NONE : sum * 100L / (16 * size); }
	void setup();
	void read();
};
