#ifndef DISTORTION_H
#define DISTORTION_H
#include "main.h"

extern void pinConfigDistortion(void);
extern void setupDistortion(void);
extern void loopDistortion(void);
extern void processDistortionAudio(int inputSample); 

#endif
