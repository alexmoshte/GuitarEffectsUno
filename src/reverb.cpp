#include <reverb.h>

uint16_t delayBuffer[MAX_DELAY];
uint32_t delayWritePointer = 0; 
uint32_t delayReadOffset = 0; 
uint32_t delayDepth;       

volatile unsigned long lastToggleSwitchStateChange = 0;
volatile unsigned long lastEffectButtonPressTime = 0;
const unsigned long DEBOUNCE_DELAY_MS = 100; //(milliseconds)

enum EffectMode {
  CLEAN = 0, 
  REVERB_ECHO_MODE, 
  DELAY_MODE,    
  NUM_EFFECTS // Counting the number of effects available    
};

const long SAMPLE_RATE_MICROS = 50; 

volatile EffectMode currentEffect = CLEAN; // Initialized to CLEAN

/* Effect Parameters. (volatile so ISR can read them)*/
volatile int pot0_value = 0; // Delay amount/depth control
volatile int pot1_value = 0; // Wet/dry mix or feedback control
volatile int pot2_value = 0; // Master volume control

volatile bool effectActive = false; // Controls overall bypass

/*********************************************FUNCTION DEFINITIONS****************************************************/
/**
 * @brief: Pin configuration for the reverb effect
 */
void pinConfigReverb() {
  pinMode(AUDIO_IN, INPUT);
  pinMode(AUDIO_OUT_A, OUTPUT);
  pinMode(AUDIO_OUT_B, OUTPUT);
  pinMode(POT0, INPUT);
  pinMode(POT1, INPUT);
  pinMode(POT2, INPUT);
  pinMode(TOGGLE, INPUT_PULLUP); 
  pinMode(FOOTSWITCH, INPUT_PULLUP);
  pinMode(LED_EFFECT_ON, OUTPUT);
}

/**
 * @brief: Setup function for the reverb effect
 */
void setUpReverb(){
  // pinConfigReverb();
  for (int i = 0; i < MAX_DELAY; i++) {    // Initialize delay buffer
    delayBuffer[i] = 0;
  }
  Timer1.initialize(SAMPLE_RATE_MICROS); // 50ms interval interruots 
  Timer1.attachInterrupt(audioIsrReverb);
  
  Serial.println("Reverb Pedal Ready!");
}

/**
 * @brief: Main loop for the reverb effect
 */
void loopReverb(){
  pot0_value = analogRead(POT0);
  pot1_value = analogRead(POT1);
  pot2_value = analogRead(POT2);

/*Handle the Effect ON/OFF button*/
  if (digitalRead(FOOTSWITCH) == LOW) {
    /*Debouncing: Only register a press if enough time has passed since the last one*/
    if (millis() - lastEffectButtonPressTime > DEBOUNCE_DELAY_MS) {
      lastEffectButtonPressTime = millis();
      effectActive = !effectActive; // Toggle the overall effect state (ON/OFF).
      if (effectActive) {
        Serial.println("Reverb Effect ON");
      } 
      else {
        Serial.println("Bypass");
        /*Clear the delay buffer to prevent any lingering sound from previous effects*/
        for (int i = 0; i < MAX_DELAY; i++) {
          delayBuffer[i] = 0;
        }
        delayWritePointer = 0; // Reset write pointer.
      }
    }
  }

/*Handle the Toggle Switch for Effect Mode Selection. Applies if the main effect is active*/
if (effectActive) {
  bool toggleState = digitalRead(TOGGLE);
  /*toggleState HIGH - REVERB_ECHO_MODE; otherwise - DELAY_MODE*/
  EffectMode targetMode = (toggleState == HIGH) ? REVERB_ECHO_MODE : DELAY_MODE;
  /*Check if the current effect mode is different from the target mode determined by the switch*/
  if (currentEffect != targetMode) {
      // Debounce for toggle switch state changes.
      if (millis() - lastToggleSwitchStateChange > DEBOUNCE_DELAY_MS) {
          lastToggleSwitchStateChange = millis();
          currentEffect = targetMode; // Update the current effect mode.
            // Print status to Serial.
          if (currentEffect == REVERB_ECHO_MODE) {
              Serial.println("Mode: REVERB (Echo)");
          } 
          else {
              Serial.println("Mode: DELAY (Repeats)");
          }
          // Clear buffer and reset pointer when mode changes to prevent audio artifacts.
          for (int i = 0; i < MAX_DELAY; i++) {
            delayBuffer[i] = 0;
          }
          delayWritePointer = 0; // Reset pointer.
      }
  }
  digitalWrite(LED_EFFECT_ON, HIGH); // Turn LED on when any effect is active.
}
else {
  currentEffect = CLEAN; // If the main effect is off, force "Clean" mode.
  digitalWrite(LED_EFFECT_ON, LOW); // Turn LED off when effect is bypassed.
}
}

/**
 * @brief: Audio Interrupt Service Routine (ISR) for processing audio samples with reverb effect. This function is called at a regular interval defined by Timer1
 */
void audioIsrReverb(){
  /*Read the analog input (guitar signal). This is a 10-bit value (0-1023)*/
  int inputSample = analogRead(AUDIO_IN);
  int outputSample = inputSample; // Initialize output as clean signal (pass-through)

  if (effectActive) {
    /*This determines the length of the delay/reverb*/
    delayReadOffset = map(pot0_value, 0, 1023, 1, MAX_DELAY - 1);
    /*Ensures the read pointer is 'delayReadOffset' samples behind the 'delayWritePointer'*/
    int delayReadPointer = (delayWritePointer + (MAX_DELAY - delayReadOffset)) % MAX_DELAY;

    /*Apply the selected effect*/
    switch (currentEffect) {
      /* "Echo" (Reverb-like) Mode: Creates a decaying, diffuse sound.
       Feedback amount derived from POT1 (0-1023 mapped to 0.0-0.95).
       Max feedback is 0.95 to prevent the signal from infinitely oscillating (runaway)*/
      case REVERB_ECHO_MODE:
        float feedback = map(pot1_value, 0, 1023, 0, 95) / 100.0;

        int delayedSample_Reverb = delayBuffer[delayReadPointer];  // Get the sample from the delay buffer at the read pointer.
        int mixedSampleForBuffer = (int)(inputSample * (1.0 - feedback) + delayedSample_Reverb * feedback);  // Mix the current input with the delayed sample for the sample to be WRITTEN into the buffer.
        
        delayBuffer[delayWritePointer] = mixedSampleForBuffer;  // Write the new, mixed sample into the delay buffer.
        
        /* Determine the final output sample: a blend of the dry input and the wet delayed signal.
         The wet/dry mix is also controlled by POT1 for this mode*/
        float wet_dry_mix = map(pot1_value, 0, 1023, 0, 100) / 100.0;
        outputSample = (int)(inputSample * (1.0 - wet_dry_mix) + delayedSample_Reverb * wet_dry_mix);
        break;

        /*"Delay" Mode: Creates distinct, repeating echoes.
         Feedback amount derived from POT1 (0-1023 mapped to 0.0-0.95)*/
        case DELAY_MODE:
        float delayFeedback = map(pot1_value, 0, 1023, 0, 95) / 100.0;

        /*Get the delayed sample from the buffer*/
        int currentDelayedSample_Delay = delayBuffer[delayReadPointer];

        /* Calculate the new sample to store in the buffer.
         This sample includes the current input plus a portion of the delayed signal (for repeats)*/
        delayBuffer[delayWritePointer] = inputSample + (int)(currentDelayedSample_Delay * delayFeedback);
        
        /*The final output sample is a simple sum of the dry input and the delayed signal*/
        outputSample = inputSample + currentDelayedSample_Delay;
        break;
        case CLEAN:
      default:
        outputSample = inputSample;
        break;
    }
  }

  /* Update Delay Buffer Write Pointer. Move the write pointer to the next position in the circular buffer.
   When it reaches the end of the buffer, it wraps around to the beginning*/
  delayWritePointer++;
  if (delayWritePointer >= MAX_DELAY) {
    delayWritePointer = 0;
  }

  /*Final Output Processing*/
  outputSample = constrain(outputSample, 0, 1023); // Constrain the processed audio sample to the valid 10-bit range (0-1023).

  /* Apply master volume control from POT2*/
  float volume_factor = pot2_value / 1023.0;
  outputSample = (int)(outputSample * volume_factor);

  /*Split the final 10-bit outputSample into two 8-bit PWM values*/

  /*AUDIO_OUT_A (Pin 9, with 4.7kΩ resistor): Controls the more significant 8 bits.
  Divide the 10-bit sample by 4 (i.e., right shift by 2 bits) to get a 0-255 range*/
  int pwm9Value = outputSample / 4;

  /* PWM_OUT_PIN_FINE (Pin 10, with 1.2MΩ resistor): Controls the less significant 2 bits.
  Get the remainder when dividing by 4 (this gives 0, 1, 2, or 3)*/
  int pwm10RawFine = outputSample % 4;

  /*Map this 0-3 range to the full 0-255 PWM range for Pin 10*/
  int pwm10Value = map(pwm10RawFine, 0, 3, 0, 255);

  /*Write the calculated PWM values to the respective output pins*/
  analogWrite(AUDIO_OUT_A, pwm9Value);
  analogWrite(AUDIO_OUT_B, pwm10Value);
}
