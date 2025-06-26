#ifndef SINEWAVE_H
#define SINEWAVE_H
#include "main.h" 

extern void createSineTable(void);      // Creates the sine wave lookup table
extern void pinConfigSinewave(void);    // Configures any sinewave-specific pins
extern void setupSinewave(void);        // Initializes the Sinewave generator
extern void loopSinewave(void);         // Sinewave-specific loop logic (e.g., non-ISR tasks)
extern void processSinewaveAudio(int inputSample); // Audio processing function, called by universal ISR
#endif
