#ifndef PERSIST_H_
#define PERSIST_H_

#include <WProgram.h>

byte getSavedMode();
void setSavedMode(byte mode);

#define FORCE_OFF    0
#define FORCE_ON     1
#define FORCE_AUTO   2

byte getSavedForce();
void setSavedForce(byte force);

#endif /* PERSIST_H_ */
