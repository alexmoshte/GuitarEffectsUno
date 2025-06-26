#ifndef ECHO_H
#define ECHO_H
#include "main.h" 

extern void pinConfigEcho(void);    // Configures any echo-specific pins
extern void setupEcho(void);        // Initializes the Echo effect
extern void loopEcho(void);         // Echo-specific loop logic (e.g., non-ISR tasks)
extern void processEchoAudio(int inputSample); // Audio processing function, called by universal ISR
#endif
