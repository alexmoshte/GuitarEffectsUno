#ifndef ECHO_H
#define ECHO_H
#include "main.h"

extern void pinConfigEcho(void);
extern void setupEcho(void);
extern void loopEcho(void);
extern void processEchoAudio(int inputSample);

#endif
