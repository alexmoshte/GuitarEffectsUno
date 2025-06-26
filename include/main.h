#ifndef MAIN_H
#define MAIN_H
#include <Arduino.h>
#include <TimerOne.h> 

/*Can be chosen independently*/
#define NORMAL
#define OCTAVER

/*Enable effect by uncommenting. One at a time! */
// #define REVERB
#define ECHO
// #define DISTORTION
// #define SINEWAVE 

/*Hardware interface resource definitions*/
#define LED_EFFECT_ON 13
#define FOOTSWITCH 12    
#define TOGGLE 2         

/*Digital inputs for effect selection*/
#define SELECT_NORMAL_BUTTON A1 
#define SELECT_EFFECT_BUTTON_A2 A2 
#define SELECT_OCTAVER_BUTTON A3 

/*Volume control*/
#define PUSHBUTTON_1 A5 
#define PUSHBUTTON_2 A4 

/*Audio input/output pin definitions*/
#define AUDIO_IN A0 
#define AUDIO_OUT_A 9 // coarse 8 bits of 10-bit output
#define AUDIO_OUT_B 10 // fine 2 bits of 10-bit output

/*PWM parameters definition*/
#define PWM_FREQ 0x00FF // PWM frequency - 31.3KHz
#define PWM_MODE 0      // Phase Correct (0) or Fast (1) PWM
#define PWM_QTY 2       // 2 PWMs in parallel for higher resolution

/*Effect buffer size. Maximum value - 500*/
#define MAX_DELAY 350

/*General variables*/
extern int input_sample; // Holds the raw 10-bit ADC value
extern uint8_t ADC_low, ADC_high; // For direct ADC read in ISR

extern uint16_t delayBuffer[MAX_DELAY]; // Buffer for delay-based effects
extern uint32_t delayWritePointer;      // Current write position in delayBuffer
extern uint32_t delayReadOffset;        // Calculated read offset (used in effect processing)
extern uint32_t delayDepth;             // Not explicitly used in current logic, can be removed if unused

extern volatile int pot2_value; // Master Volume, now controlled by PUSHBUTTON_1/2 globally

extern volatile bool effectActive; // Global ON/OFF state of the selected effect

/* Debouncing Variables - Now universally declared and initialized in main.cpp*/
extern volatile unsigned long lastFootswitchPressTime;
extern volatile unsigned long lastToggleSwitchStateChange;
extern const unsigned long DEBOUNCE_DELAY_MS;
extern volatile unsigned long lastPushButton1PressTime;
extern volatile unsigned long lastPushButton2PressTime;

// Debouncing for effect selection buttons (A1, A2, A3)
extern volatile unsigned long lastSelectNormalPressTime;
extern volatile unsigned long lastSelectEffectButtonA2PressTime; // Renamed for A2 button
extern volatile unsigned long lastSelectOctaverPressTime;

/*Counter to periodically check pushbuttons (saves CPU in ISR) - now just for NORMAL's legacy volume*/
extern volatile int button_check_counter;
extern const int BUTTON_CHECK_INTERVAL; // Check buttons every 100 samples

/* Configure audio parameters - Consistent 20kHz sample rate */
extern const long SAMPLE_RATE_MICROS;


/*Enum for universal effect mode management*/
enum EffectMode {
    CLEAN_MODE = 0,         
    NORMAL_MODE,          
    REVERB_MODE,       
    DELAY_MODE,            
    ECHO_MODE,            
    OCTAVER_MODE,           
    DISTORTION_MODE,     
    SINEWAVE_MODE,          
    NUM_EFFECTS_ENUM        // Helper to count total modes (always last)
};
extern volatile EffectMode currentActiveMode; 

/*********************************************FUNCTION DECLARATIONS****************************************************/
/*Core hardware setup functions*/
extern void adcSetup();
extern void pinConfig ();
extern void pmwSetup(void);

// Audio processing functions for each effect (called by the universal ISR)
extern void processNormalAudio(int inputSample);
extern void processReverbAudio(int inputSample);
extern void processEchoAudio(int inputSample);
extern void processOctaverAudio(int inputSample);
extern void processDistortionAudio(int inputSample);
extern void processSinewaveAudio();
#endif
