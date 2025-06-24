#include "octaver.h"

unsigned int write_pointer = 0; // Current write position in the buffer
volatile float read_pointer_float = 0.0; // Floating-point read pointer for pitch shifting
volatile float pitch_step = 1.0;  // How much read_pointer_float advances per sample (0.5 for octave down, 1.0 for original, 2.0 for octave up)

/*********************************************FUNCTION DEFINITIONS****************************************************/
/**
 * @brief: Pin configuration for the octaver effect
 */
void pinConfigOctaver(){
    // Set pin modes
  pinMode(AUDIO_IN, INPUT);
  pinMode(AUDIO_OUT_A, OUTPUT);
  pinMode(AUDIO_OUT_B, OUTPUT);
  pinMode(POT0, INPUT);
  pinMode(POT1, INPUT);
  pinMode(POT2, INPUT);
  pinMode(FOOTSWITCH, INPUT_PULLUP); // Use internal pull-up for footswitch
  pinMode(LED_EFFECT_ON, OUTPUT);
}

/**
 * @brief: Setup function for the octaver effect
 */
void setupOctaver() {
    // pinConfigOctaver();
  for (int i = 0; i < MAX_DELAY; i++) {
    delayBuffer[i] = 0;
  }
  Timer1.initialize(SAMPLE_RATE_MICROS);
  Timer1.attachInterrupt(audioIsrOctaver);
  Serial.println("Arduino Uno Octaver Pedal Ready!");
}

/**
 * @brief: Main loop for the octaver effect
 */
void loopOctaver() {
  // Read potentiometer values (non-time-critical, done outside ISR)
  pot0_value = analogRead(POT0);
  pot1_value = analogRead(POT1);
  pot2_value = analogRead(POT2);

  /*Footswitch handling*/
  if (digitalRead(FOOTSWITCH) == LOW) {
    if (millis() - lastFootswitchPressTime > DEBOUNCE_DELAY_MS) {
      lastFootswitchPressTime = millis();
      effectActive = !effectActive;

      if (effectActive) {
        Serial.println("Octaver ON");
      } else {
        Serial.println("Bypass");
        for (int i = 0; i < MAX_DELAY; i++) {
          delayBuffer[i] = 0;
        }
        write_pointer = 0;
        read_pointer_float = 0.0;
      }
    }
  }
  digitalWrite(LED_EFFECT_ON, effectActive ? HIGH : LOW);
}

/**
 * @brief: Audio Interrupt Service Routine (ISR) for processing audio samples with octaver effect
 */
void audioIsrOctaver() {
  int inputSample = analogRead(AUDIO_IN); // Read 10-bit input (0-1023)
  int outputSample = inputSample;              // Default to dry signal for bypass

 /*Octaver Effect Processing*/
  if (effectActive) {
    delayBuffer[write_pointer] = inputSample; // Write current input sample into the delay buffer.

    /*Determine Pitch Shift Step based on POT0*/
    if (pot0_value > 700) { // Top ~1/3 of range (originally >2700 for 12-bit)
      pitch_step = 2.0; // Octave Up (read twice as fast)
    } else if (pot0_value > 300) { // Middle ~1/3 (originally >1350 for 12-bit)
      pitch_step = 1.0; // Original Pitch (read at normal speed)
    } else { // Bottom ~1/3 (originally <=1350 for 12-bit)
      pitch_step = 0.5; // Octave Down (read half as fast)
    }

    read_pointer_float += pitch_step; // Advance the floating-point read pointer.

    /*Handles Circular Buffer Wrap-Around for Floating-Point Pointer*/
    while (read_pointer_float >= MAX_DELAY) {
      read_pointer_float -= MAX_DELAY;
    }
    while (read_pointer_float < 0) { // Should not happen with positive pitch_step, but for safety
      read_pointer_float += MAX_DELAY;
    }

    /*Linear Interpolation for Smooth Playback. Get the integer part of the read pointer*/
    int read_idx1 = (int)read_pointer_float;
    /*Get the fractional part for interpolation*/
    float fraction = read_pointer_float - read_idx1;

    /*Get the next sample index for interpolation (wrap around if needed)*/
    int read_idx2 = (read_idx1 + 1) % MAX_DELAY;

    /*Read the two samples from the buffer*/
    int sample1 = delayBuffer[read_idx1];
    int sample2 = delayBuffer[read_idx2];

    /*Interpolate between sample1 and sample2*/
    int octavedSample = (int)(sample1 + fraction * (sample2 - sample1));

    /* Wet/Dry Mix Control with POT1. Scale POT1 (0-1023) to a float 0.0 to 1.0 for wet_mix_factor*/
    float wet_mix_factor = pot1_value / 1023.0;
    /*The dry mix factor is the complement*/
    float dry_mix_factor = 1.0 - wet_mix_factor;

    /*Final output is a blend of the original input and the octaved signal*/
    outputSample = (int)(inputSample * dry_mix_factor + octavedSample * wet_mix_factor);
  }

  /*Increment write pointer (circular buffer)*/
  write_pointer++;
  if (write_pointer >= MAX_DELAY) {
    write_pointer = 0;
  }

  /*Final Output Processing*/
  outputSample = constrain(outputSample, 0, 1023);

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