#ifndef REVERB_H
#define REVERB_H
#include "main.h" 

extern void pinConfigReverb(void); // Configures any additional reverb-specific pins
extern void setUpReverb(void);     // Initializes the reverb effect (e.g., buffer init)
extern void loopReverb(void);      // Handles reverb-specific loop logic (e.g., sub-mode toggle)
extern void processReverbAudio(int inputSample); // Audio processing function, called by universal ISR
#endif
