#include <echo.h>

uint16_t delayBuffer[MAX_DELAY];
unsigned int delayWritePointer = 0; 

volatile int pot0_value = 0; // Delay Time
volatile int pot1_value = 0; // Feedback amount
volatile int pot2_value = 0; // Master Volume

volatile bool effectActive = false;

/* Debouncing Variables*/
volatile unsigned long lastFootswitchPressTime = 0;
const unsigned long DEBOUNCE_DELAY_MS = 100; 

/*********************************************FUNCTION DEFINITIONS****************************************************/
/**
 * @brief: Pin configuration for the reverb effect
 */
void pinConfigEcho(){
  pinMode(AUDIO_IN, INPUT);
  pinMode(AUDIO_OUT_A, OUTPUT);
  pinMode(AUDIO_OUT_B, OUTPUT);
  pinMode(POT0, INPUT);
  pinMode(POT1, INPUT);
  pinMode(POT2, INPUT);
  pinMode(FOOTSWITCH, INPUT_PULLUP);
  pinMode(LED_EFFECT_ON, OUTPUT);  
}

/**
 * @brief: Setup function for the echo effect
 */
void setupEcho(){
// pinConfigEcho();
for (int i = 0; i < MAX_DELAY; i++) {
delayBuffer[i] = 0;
}
  Timer1.initialize(SAMPLE_RATE_MICROS);
  Timer1.attachInterrupt(audioIsrEcho);      
  Serial.println("Echo Pedal Ready!");
}

/**
 * @brief: Main loop for the echo effect
 */
void loopEcho(){
  /*To be used in the audioISR*/
  pot0_value = analogRead(POT0);
  pot1_value = analogRead(POT1);
  pot2_value = analogRead(POT2);

  /*Footswitch handling*/
  if (digitalRead(FOOTSWITCH) == LOW) {
    if (millis() - lastFootswitchPressTime > DEBOUNCE_DELAY_MS) {
      lastFootswitchPressTime = millis();
      effectActive = !effectActive; 

      if (effectActive) {
        Serial.println("Echo Effect ON");
      } else {
        Serial.println("Bypass");
        for (int i = 0; i < MAX_DELAY; i++) {
          delayBuffer[i] = 0;
        }
        delayWritePointer = 0;
      }
    }
  }
  digitalWrite(LED_EFFECT_ON, effectActive ? HIGH : LOW);
}

/**
 * @brief: Audio Interrupt Service Routine (ISR) for processing audio samples with echo effect
 */
void audioIsrEcho() {
  int inputSample = analogRead(AUDIO_IN); //10-bit value (0-1023)
  int outputSample; 

  /*Echo Effect Processing*/
  if (effectActive) {
    /*Determine the current delay depth based on POT0*/
    int currentDelayDepth = map(pot0_value, 0, 1023, 1, MAX_DELAY - 1);

    /*Calculate the read pointer for the circular buffer*/
    int delayReadPointer = (delayWritePointer + (MAX_DELAY - currentDelayDepth)) % MAX_DELAY;

    // Get the previously stored (delayed) sample from the buffer.
    int delayedSample = delayBuffer[delayReadPointer];

    // Determine feedback amount from POT1 (0-1023 mapped to 0.0-0.98).
    // Max feedback is 0.98 to prevent infinite oscillations (runaway).
    float feedbackFactor = map(pot1_value, 0, 1023, 0, 98) / 100.0;

    /*The core echo algorithm*/
    int newSampleForBuffer = (int)(inputSample * (1.0 - feedbackFactor) + delayedSample * feedbackFactor);
    
    /*Write the new sample into the delay buffer at the current write pointer*/
    delayBuffer[delayWritePointer] = newSampleForBuffer;

    /*For the final output, mix the dry input signal with the wet delayed signal*/
    outputSample = inputSample + delayedSample; // Simple summing.
  } 
  else {
    outputSample = inputSample;
  }

  /*Update Delay Buffer Write Pointer.Wraps to the beginning when it reaches the end*/
  delayWritePointer++;
  if (delayWritePointer >= MAX_DELAY) {
    delayWritePointer = 0;
  }

  /*Final Output Processing*/
  outputSample = constrain(outputSample, 0, 1023); // Constrain the processed audio sample to the valid 10-bit range (0-1023).

  /*Apply master volume control from POT2*/
  float volume_factor = pot2_value / 1023.0;
  outputSample = (int)(outputSample * volume_factor);

  /*Split the final 10-bit outputSample into two 8-bit PWM values for Pins 9 and 10*/
  int pwm9Value = outputSample / 4;
  int pwm10RawFine = outputSample % 4;
  int pwm10Value = map(pwm10RawFine, 0, 3, 0, 255);

  analogWrite(AUDIO_OUT_A, pwm9Value);
  analogWrite(AUDIO_OUT_B, pwm10Value);
}