#ifndef PERSIST_H_
#define PERSIST_H_

#include <Arduino.h>

uint8_t getSavedMode();
void setSavedMode(uint8_t mode);

uint8_t getSavedHotwater();
void setSavedHotwater(uint8_t hotwater);

#define FORCE_OFF    0
#define FORCE_ON     1
#define FORCE_AUTO   2

uint8_t getSavedForce();
void setSavedForce(uint8_t force);

uint8_t getSavedPeriod();
void setSavedPeriod(uint8_t period);

uint8_t getSavedDuration();
void setSavedDuration(uint8_t duration);

#endif /* PERSIST_H_ */
