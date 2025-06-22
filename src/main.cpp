/*Conditionally compiles the choosen effect*/
#include <main.h>
#include <reverb.h>
#include <echo.h>
#include <octaver.h>
#include <sinewave.h>
#include <distortion.h>

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