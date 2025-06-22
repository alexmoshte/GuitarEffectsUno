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

/* Configure audio parameters.50 microseconds for 20kHz sample rate (1,000,000 / 20,000 = 50).
 * Adjust if needed, lower rate = more processing time, but worse quality.
 * Higher rate = less processing time, better quality but might not work on Uno*/
extern const long SAMPLE_RATE_MICROS; 

/*********************************************FUNCTION DECLARATIONS****************************************************/
extern void adcSetup();
extern void pinConfig ();
extern void pmwSetup(void);

/*********************************************FUNCTION DEFINITIONS****************************************************/
/**
* @brief: Configure hardware interfaces
*/
void pinConfig (void){
  pinMode(FOOTSWITCH, INPUT_PULLUP);
  pinMode(TOGGLE, INPUT_PULLUP);
  pinMode(PUSHBUTTON_1, INPUT_PULLUP);
  pinMode(PUSHBUTTON_2, INPUT_PULLUP);
  pinMode(LED_EFFECT_ON, OUTPUT);
}

/**
* @brief: Setup ADC. Configured to be reading automatically the hole time
*/
void adcSetup(void){
  ADMUX = 0x60; // left adjust, adc0, internal vcc
  ADCSRA = 0xe5; // turn on adc, ck/32, auto trigger
  ADCSRB = 0x07; // t1 capture for trigger
  DIDR0 = 0x01; // turn off digital inputs for adc0
}

/** 
* @brief: Setup PWM. For more info about this config check the forum
*/
void pmwSetup(void){
  TCCR1A = (((PWM_QTY - 1) << 5) | 0x80 | (PWM_MODE << 1));
  TCCR1B = ((PWM_MODE << 3) | 0x11); // ck/1
  TIMSK1 = 0x20; // Interrupt on capture interrupt
  ICR1H = (PWM_FREQ >> 8);
  ICR1L = (PWM_FREQ & 0xff);
  DDRB |= ((PWM_QTY << 1) | 0x02); // Turn on outputs
  sei(); // Turn on interrupts. Not really necessary with arduino
}
#endif