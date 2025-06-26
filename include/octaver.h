#ifndef OCTAVER_H
#define OCTAVER_H
#include "main.h" 

extern void pinConfigOctaver(void);    // Configures any octaver-specific pins
extern void setupOctaver(void);        // Initializes the Octaver effect
extern void loopOctaver(void);         // Octaver-specific loop logic (e.g., non-ISR tasks)
extern void processOctaverAudio(int inputSample); // Audio processing function, called by universal ISR
#endif
