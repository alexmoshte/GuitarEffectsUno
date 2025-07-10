#ifndef MAIN_H
#define MAIN_H
#include <Arduino.h>
#include <TimerOne.h> 

/*Effects selectable through buttons A3 - A6*/
#define NORMAL
#define OCTAVER
#define REVERB
#define ECHO
#define DISTORTION
#define SINEWAVE 

/*Hardware interface resource definitions*/
#define LED_EFFECT_ON 13
#define FOOTSWITCH 12 // Global Momentary Bypass: Press (LOW) for CLEAN_MODE, Release (HIGH) for last selected effect
#define TOGGLE 11  // Reverb sub-mode toggle switch: HIGH for REVERB_ECHO_MODE, LOW for DELAY_MODE

/*Audio input/output pin definitions*/
#define AUDIO_IN A0 // Audio input pin
#define AUDIO_OUT_A 9 // Audio output pin A (coarse 8 bits of 10-bit output)
#define AUDIO_OUT_B 10 // Audio output pin B (fine 2 bits of 10-bit output)

#define PUSHBUTTON_1 A1 // Global Volume Up
#define PUSHBUTTON_2 A2 // Global Volume Down

// Effect selection buttons (momentary activation)
#define SELECT_OCTAVER_BUTTON A3 // OCTAVER_MODE
#define SELECT_NORMAL_BUTTON A4 // NORMAL_MODE
#define SELECT_REVERB_BUTTON A5 // REVERB_ECHO_MODE 
#define SELECT_ECHO_BUTTON 2 // ECHO_MODE 
#define SELECT_DISTORTION_BUTTON 3 // DISTORTION_MODE
#define SELECT_SINEWAVE_BUTTON 4 // SINEWAVE_MODE 

/*PWM parameters definition*/
#define PWM_FREQ 0x00FF // PWM frequency - 31.3KHz
#define PWM_MODE 0      // Phase Correct (0) or Fast (1) PWM
#define PWM_QTY 2       // 2 PWMs in parallel for higher resolution

/*Effect buffer size. Maximum value - 500*/
#define MAX_DELAY 350

/*General variables*/
extern int input_raw_sample; // Will hold the raw 10-bit ADC value (0-1023)
extern uint8_t ADC_low, ADC_high; // For direct ADC read in ISR

extern uint16_t delayBuffer[MAX_DELAY]; // Buffer for delay-based effects
extern uint32_t delayWritePointer;      // Current write position in delayBuffer
extern uint32_t delayReadOffset;        // Calculated read offset (used in effect processing)
extern uint32_t delayDepth;             // Not explicitly used in current logic, can be removed if unused

extern volatile int pot2_value; // Master Volume, now controlled by PUSHBUTTON_1/2 globally

// Enum for universal effect mode management
enum EffectMode {
    CLEAN_MODE = 0,         // Basic bypass (no effect, signal passes through, no processing)
    NORMAL_MODE,            // Original 'Normal' effect (simple volume on clean signal)
    REVERB_ECHO_MODE,       // Reverb-like effect
    DELAY_MODE,             // Distinct delay effect (sub-mode of REVERB module)
    ECHO_MODE,              // Echo effect
    OCTAVER_MODE,           // Octaver effect
    DISTORTION_MODE,        // Distortion effect
    SINEWAVE_MODE,          // Sinewave generator
    NUM_EFFECTS_ENUM        // Helper to count total modes (always last)
};

extern volatile bool effectActive; // Global ON/OFF state (true if any effect is running, false for clean bypass)
extern volatile EffectMode lastSelectedMode; // Stores the last selected effect mode (not CLEAN_MODE)

/* Debouncing Variables*/
extern volatile unsigned long lastFootswitchPressTime;
extern volatile unsigned long lastToggleSwitchStateChange; // Used by Reverb sub-mode
extern const unsigned long DEBOUNCE_DELAY_MS;
extern volatile unsigned long lastPushButton1PressTime;
extern volatile unsigned long lastPushButton2PressTime;

// Debouncing for effect selection buttons (A1, A2, A3)
extern volatile unsigned long lastSelectNormalPressTime;
extern volatile unsigned long lastSelectEffectButtonA2PressTime;
extern volatile unsigned long lastSelectOctaverPressTime;

/*Counter to periodically check pushbuttons (saves CPU in ISR) - now just for NORMAL's legacy volume*/
extern volatile int button_check_counter;
extern const int BUTTON_CHECK_INTERVAL; // Check buttons every 100 samples

/* Configure audio parameters - Consistent 20kHz sample rate */
extern const long SAMPLE_RATE_MICROS;


extern volatile EffectMode currentActiveMode; // Universal variable for the currently active effect mode

/*********************************************FUNCTION DECLARATIONS****************************************************/
/*Core hardware setup functions*/
extern void adcSetup();
extern void pinConfig ();
extern void pmwSetup(void);
extern void volumeControl();

/* Audio processing functions for each effect (called by the universal ISR)
 * All processXAudio functions now accept 'int inputSample' for consistency,
 * even if a generator doesn't use it*/
extern void processNormalAudio(int inputSample);
extern void processReverbAudio(int inputSample);
extern void processEchoAudio(int inputSample);
extern void processOctaverAudio(int inputSample);
extern void processDistortionAudio(int inputSample);
extern void processSinewaveAudio(int inputSample); 
#endif
