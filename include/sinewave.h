#ifndef SINEWAVE_H
#define SINEWAVE_H
#include "main.h"

extern void createSineTable(void);
extern void pinConfigSinewave(void);
extern void setupSinewave(void);
extern void loopSinewave(void);
extern void processSinewaveAudio(int inputSample); 

#endif
