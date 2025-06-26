#ifndef DISTORTION_H
#define DISTORTION_H
#include "main.h" // Includes common definitions and variables

// Function declarations for the Distortion module
extern void pinConfigDistortion(void);     // Configures any distortion-specific pins
extern void setupDistortion(void);         // Initializes the Distortion effect
extern void loopDistortion(void);          // Distortion-specific loop logic (e.g., non-ISR tasks)
extern void processDistortionAudio(int inputSample); // Audio processing function, called by universal ISR

#endif
