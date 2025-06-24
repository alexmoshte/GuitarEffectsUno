#ifndef MAIN_H
#define MAIN_H
#include <Arduino.h>
#include <TimerOne.h>

/*Enable effect by uncommenting. One at a time! */
// #define NORMAL
// #define REVERB
// #define ECHO
#define OCTAVER
// #define SINEWAVE
// #define DISTORTION

/*Hardware interface resource definitions*/
#define LED_EFFECT_ON 13
#define FOOTSWITCH 12
#define TOGGLE 2
#define PUSHBUTTON_1 A5
#define PUSHBUTTON_2 A4
#define POT0 A1 // Analog input for Potentiometer 0 (e.g., Delay Time)
#define POT1 A2 // Analog input for Potentiometer 1 (e.g., Wet/Mix)
#define POT2 A3  // Analog input for Potentiometer 2 (e.g., Feedback)

/*Audio input/output pin definitions*/
#define AUDIO_IN A0 // Audio input pin
#define AUDIO_OUT_A 9 // Audio output pin A
#define AUDIO_OUT_B 10 // Audio output pin B

/*PWM parameters definition*/
#define PWM_FREQ 0x00FF // pwm frequency - 31.3KHz
#define PWM_MODE 0 // Fast (1) or Phase Correct (0)
#define PWM_QTY 2 // 2 PWMs in parallel

/*Effect buffer size. Maximum value - 500*/
#define MAX_DELAY 350 

/*General variables*/
extern int input, vol_variable;
extern int counter;
extern uint8_t ADC_low, ADC_high;

extern uint16_t delayBuffer[MAX_DELAY];
extern uint32_t delayWritePointer;  
extern uint32_t delayReadOffset; 
extern uint32_t delayDepth;

extern volatile int pot0_value; // Delay Time
extern volatile int pot1_value; // Feedback amount
extern volatile int pot2_value; // Master Volume

extern volatile bool effectActive;

/* Debouncing Variables*/
extern volatile unsigned long lastFootswitchPressTime;
extern volatile unsigned long lastToggleSwitchStateChange;
extern const unsigned long DEBOUNCE_DELAY_MS; /*Button Debouncing and Counter Variables*/
extern unsigned long lastPushButton1PressTime;
extern volatile unsigned long lastPushButton2PressTime;

/*Counter to periodically check pushbuttons (saves CPU in ISR)*/
extern volatile int button_check_counter;
extern const int BUTTON_CHECK_INTERVAL; // Check buttons every 500 samples
 
/* Configure audio parameters.50 microseconds for 20kHz sample rate (1,000,000 / 20,000 = 50).
 * Adjust if needed, lower rate = more processing time, but worse quality.
 * Higher rate = less processing time, better quality but might not work on Uno*/
extern const long SAMPLE_RATE_MICROS; 

/*********************************************FUNCTION DECLARATIONS****************************************************/
extern void adcSetup();
extern void pinConfig ();
extern void pmwSetup(void);
#endif