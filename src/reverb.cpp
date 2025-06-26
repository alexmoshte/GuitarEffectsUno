#include "reverb.h"
#include <Arduino.h>   

/*********************************************FUNCTION DEFINITIONS****************************************************/
/**
 * @brief: Pin configuration for the reverb effect.
 * This is primarily for any additional pins *unique* to the reverb module.
 * Common pins (audio I/O, effect selection buttons, volume buttons, footswitch, LED)
 * are configured in main.cpp's pinConfig().
 */
void pinConfigReverb() {
    // Additional pin configuration
}

/**
 * @brief: Setup function for the reverb effect.
 * Initializes the delay buffer to silence.
 */
void setUpReverb(){
    for (int i = 0; i < MAX_DELAY; i++) { // Initialize delay buffer with silence
        delayBuffer[i] = 0;
    }    
    Serial.println("Reverb Pedal Ready!");
}

/**
 * @brief: Main loop for the reverb effect.
 * This function now *only* handles the TOGGLE switch for sub-mode selection (Reverb/Echo vs. Delay).
 * It no longer reads potentiometers for parameters, as those are now fixed constants
 * or controlled globally by pushbuttons.
 */
void loopReverb(){
    /* Handles the TOGGLE switch for Reverb sub-modes*/
    if (effectActive && (currentActiveMode == REVERB_MODE || currentActiveMode == DELAY_MODE)) {
        bool toggleState = digitalRead(TOGGLE); 
        /*toggleState HIGH - REVERB_ECHO_MODE; otherwise - DELAY_MODE*/
        EffectMode targetMode = (toggleState == HIGH) ? REVERB_MODE : DELAY_MODE;

        /*Check if the current active mode (global) is different from the target mode determined by the switch*/
        if (currentActiveMode != targetMode) {
            /*Debounce for toggle switch state changes*/
            if (millis() - lastToggleSwitchStateChange > DEBOUNCE_DELAY_MS) {
                lastToggleSwitchStateChange = millis();
                currentActiveMode = targetMode; 
                /*Print status to Serial*/
                if (currentActiveMode == REVERB_MODE) {
                    Serial.println("Reverb Sub-Mode: REVERB (Echo)");
                }
                else {
                    Serial.println("Reverb Sub-Mode: DELAY (Repeats)");
                }
                /*Clear buffer and reset pointer when sub-mode changes to prevent audio artifacts*/
                for (int i = 0; i < MAX_DELAY; i++) {
                    delayBuffer[i] = 0;
                }
                delayWritePointer = 0; 
            }
        }
    }
}

/**
 * @brief: Audio processing function for Reverb/Delay effect.
 * This function is called by the universal ISR (TIMER1_CAPT_vect) from main.cpp.
 * It applies the selected audio effect based on 'currentActiveMode'.
 * Potentiometer values are now replaced with constant values for effect parameters.
 * @param inputSample The raw 10-bit input audio sample (0-1023).
 */
void processReverbAudio(int inputSample) {
    int outputSample = inputSample; // Initialize output as clean signal (pass-through)

    if (effectActive) { // Only process effect if globally active (footswitch state)
        const int fixedDelayTimeValue = 500; // Value for delay time (maps to MAX_DELAY/2 approx)
        const float fixedFeedbackValue = 0.75; // 75% feedback (0.0 to 0.95 range)
        const float fixedWetDryMix = 0.70; // 70% wet for reverb-like mode

        /* Calculate delayReadOffset based on a fixed value, not potentiometer.
         * Maps fixedDelayTimeValue (e.g., 500) to a valid delay offset (1 to MAX_DELAY - 1).
         * This value will be MAX_DELAY * (fixedDelayTimeValue/1023) roughly.
         */
        delayReadOffset = map(fixedDelayTimeValue, 0, 1023, 1, MAX_DELAY - 1);
        
        /* Ensures the read pointer is 'delayReadOffset' samples behind the 'delayWritePointer'
         * This calculates the position to read from in the circular buffer.
         */
        int delayReadPointer = (delayWritePointer + (MAX_DELAY - delayReadOffset)) % MAX_DELAY;

        /* Apply the selected effect based on currentActiveMode (global variable managed by loopReverb/main) */
        switch (currentActiveMode) {
            case REVERB_MODE: {
                /* "Echo" (Reverb-like) Mode: Creates a decaying, diffuse sound. */
                int delayedSample_Reverb = delayBuffer[delayReadPointer];
                /*Mix current input with delayed sample for the buffer itself (feedback loop for smearing)*/
                int mixedSampleForBuffer = (int)(inputSample * (1.0 - fixedFeedbackValue) + delayedSample_Reverb * fixedFeedbackValue);

                delayBuffer[delayWritePointer] = mixedSampleForBuffer; // Store the mixed sample back into the buffer

                /* Determine the final output sample: a blend of the dry input and the wet delayed signal. */
                outputSample = (int)(inputSample * (1.0 - fixedWetDryMix) + delayedSample_Reverb * fixedWetDryMix);
                break;
            }

            case DELAY_MODE: {
                /* "Delay" Mode: Creates distinct, repeating echoes. */
                int currentDelayedSample_Delay = delayBuffer[delayReadPointer];

                /* Calculate the new sample to store in the buffer.
                 * This sample includes the current input plus a portion of the delayed signal (for repeats)
                 */
                delayBuffer[delayWritePointer] = inputSample + (int)(currentDelayedSample_Delay * fixedFeedbackValue);

                /* The final output sample is a simple sum of the dry input and the delayed signal */
                outputSample = inputSample + currentDelayedSample_Delay;
                break;
            }

            case CLEAN_MODE: 
            default: { 
                outputSample = inputSample;     
                for (int i = 0; i < MAX_DELAY; i++) {
                    delayBuffer[i] = 0;
                }
                delayWritePointer = 0;
                break;
            }
        }
    } 
    else { 
        outputSample = inputSample; 
        for (int i = 0; i < MAX_DELAY; i++) {
            delayBuffer[i] = 0;
        }
        delayWritePointer = 0;
    }
    /* Update Delay Buffer Write Pointer. */
    delayWritePointer++;
    if (delayWritePointer >= MAX_DELAY) {
        delayWritePointer = 0;
    }

    /* Final Output Processing */
    outputSample = constrain(outputSample, 0, 1023); // Constrain to valid 10-bit range

    /* Apply global master volume controlled by PUSHBUTTON_1/2 */
    float volume_factor = pot2_value / 1023.0; // pot2_value is controlled by pushbuttons in main.cpp
    outputSample = (int)(outputSample * volume_factor);

    /* Write the calculated PWM values to the respective output pins for higher resolution */
    analogWrite(AUDIO_OUT_A, outputSample / 4); // Coarse 8 bits
    analogWrite(AUDIO_OUT_B, map(outputSample % 4, 0, 3, 0, 255)); // Fine 2 bits
}
