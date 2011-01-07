#include <WProgram.h>

void __cxa_pure_virtual()
{
  cli();
  for (;;);
}

void main() __attribute__ ((noreturn));

void main(void)
{
	init();

	setup();
    
	for (;;)
		loop();
}

