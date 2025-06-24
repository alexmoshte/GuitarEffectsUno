/*Conditionally compiles the choosen effect. Chosen from main.h*/
#include "main.h"
#include "reverb.h"
#include "echo.h"
#include "octaver.h"
#include "sinewave.h"
#include "distortion.h"

uint16_t delayBuffer[MAX_DELAY];
uint32_t delayWritePointer = 0; 

/*Effect Parameters*/
volatile int pot0_value = 0; // Delay Time
volatile int pot1_value = 0; // Feedback amount
volatile int pot2_value = 0; // Master Volume

volatile bool effectActive = false;

/* Debouncing Variables*/
volatile unsigned long lastFootswitchPressTime = 0;
unsigned long lastPushButton1PressTime = 0;
volatile unsigned long lastPushButton2PressTime = 0;
const unsigned long DEBOUNCE_DELAY_MS = 100;

#ifdef NORMAL
/*General variables*/
int input, vol_variable=512;
int counter=0;
uint8_t ADC_low, ADC_high;
#endif

void setup() {
  Serial.begin(9600); 

  #ifdef NORMAL
  pinConfig();  
  adcSetup();
  pmwSetup();
  #endif

  #ifdef REVERB
  pinConfigReverb();
  setUpReverb(); 
  #endif

  #ifdef ECHO
  pinConfigEcho();
  setupEcho();
  #endif

  #ifdef OCTAVER
  pinConfigOctaver();
  setupOctaver();
  #endif

  #ifdef SINEWAVE
  pinConfigSinewave();
  setupSinewave();
  #endif

  #ifdef DISTORTION
  pinConfigDistortion();
  setupDistortion();
  #endif  
}

void loop() {

  /*Turn on LED when footswitch is pressed (effect is on)*/
  #ifdef NORMAL
  if (digitalRead(FOOTSWITCH)) digitalWrite(LED_EFFECT_ON, HIGH);
  else  digitalWrite(LED_EFFECT_ON, LOW);
  #endif

  #ifdef REVERB
  loopReverb(); 
  #endif

  #ifdef ECHO
  loopEcho();
  #endif

  #ifdef OCTAVER
  loopOctaver();
  #endif

  #ifdef SINEWAVE
  loopSinewave();
  #endif

  #ifdef DISTORTION
  loopDistortion();
  #endif
}

#ifdef NORMAL
/** 
* @brief: Timer 1 interruption for normal.
*/
ISR(TIMER1_CAPT_vect)
{
  /*Low byte need to be fetched first. Read the ADC input signal data: 2 bytes Low and High.*/
  ADC_low = ADCL; 
  ADC_high = ADCH;
  /*Construct the input sample summing the ADC low and high byte*/
  input = ((ADC_high << 8) | ADC_low) + 0x8000; // Make a signed 16b value

  /*Push button check. To save resources, the push-buttons are checked every 100 times*/
counter++;
if(counter==100)
{
counter=0;
if (!digitalRead(PUSHBUTTON_1)) {
  if (vol_variable<1024) vol_variable=vol_variable+1; // Increase vol
}
if (!digitalRead(PUSHBUTTON_2)) {
  if (vol_variable>0) vol_variable=vol_variable-1; // Decrease vol
}
}
/*The amplitude of the signal is modified following the vol_variable using the Arduino map fucntion*/
  input = map(input, 0, 1024, 0, vol_variable);
 
/*Write the PWM output signal*/
  OCR1AL = ((input + 0x8000) >> 8); // Convert to unsigned, send out high byte
  OCR1BL = input; // Send out low byte
}
#endif

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