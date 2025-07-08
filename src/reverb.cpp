#include "reverb.h"
#include <Arduino.h>

/*********************************************FUNCTION DEFINITIONS****************************************************/
void pinConfigReverb(){
    // No specific pins for Reverb, common pins configured in main.cpp
}

void setUpReverb(){
    for (int i = 0; i < MAX_DELAY; i++) {
        delayBuffer[i] = 0;
    }
    Serial.println("Reverb Pedal Ready!");
}

void loopReverb(){
    // This loop logic specifically handles the TOGGLE switch for Reverb sub-modes.
    // It only applies if the current global mode is one of the REVERB sub-modes.
    if (currentActiveMode == REVERB_ECHO_MODE || currentActiveMode == DELAY_MODE) {
        bool toggleState = digitalRead(TOGGLE);
        EffectMode targetMode = (toggleState == HIGH) ? REVERB_ECHO_MODE : DELAY_MODE;

        if (currentActiveMode != targetMode) {
            if (millis() - lastToggleSwitchStateChange > DEBOUNCE_DELAY_MS) {
                lastToggleSwitchStateChange = millis();
                currentActiveMode = targetMode; // Update the global current effect mode.
                Serial.print("Reverb Sub-Mode: ");
                Serial.println((currentActiveMode == REVERB_ECHO_MODE) ? "REVERB (Echo)" : "DELAY (Repeats)");
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
 * Uses centered floating-point math to prevent clipping and buzzing.
 * @param inputSample The raw 10-bit input audio sample (0-1023).
 */
void processReverbAudio(int inputSample) {
    float outputSampleFloat; // Use float for processing to avoid clipping
    float centered_input = (float)inputSample - 511.5; // Center input to -511.5 to 511.5

    if (effectActive) {
        // --- Fixed Effect Parameters ---
        const int fixedDelayTimeValue = 500; // Maps to approx MAX_DELAY/2
        const float fixedFeedbackValue = 0.75; // 75% feedback (0.0 to 0.95 range)
        const float fixedWetDryMix = 0.70; // 70% wet for reverb-like mode

        delayReadOffset = map(fixedDelayTimeValue, 0, 1023, 1, MAX_DELAY - 1);
        int delayReadPointer = (delayWritePointer + (MAX_DELAY - delayReadOffset)) % MAX_DELAY;

        // Get delayed sample from buffer and center it
        float delayedSample_Centered = (float)delayBuffer[delayReadPointer] - 511.5;

        float mixedSampleForBuffer_Centered;
        float currentDelayedSample_Centered;

        switch (currentActiveMode) {
            case REVERB_ECHO_MODE: {
                // Mix current input with delayed sample for the buffer itself (feedback loop for smearing)
                mixedSampleForBuffer_Centered = centered_input * (1.0 - fixedFeedbackValue) + delayedSample_Centered * fixedFeedbackValue;
                
                // For final output, blend dry input and wet delayed signal
                outputSampleFloat = centered_input * (1.0 - fixedWetDryMix) + delayedSample_Centered * fixedWetDryMix;
                break;
            }

            case DELAY_MODE: {
                // Calculate new sample to store in buffer: current input + portion of delayed signal (for repeats)
                mixedSampleForBuffer_Centered = centered_input + (delayedSample_Centered * fixedFeedbackValue);
                
                // Final output sample is sum of dry input and delayed signal
                outputSampleFloat = centered_input + delayedSample_Centered;
                break;
            }

            case CLEAN_MODE:
            default: {
                outputSampleFloat = centered_input; // Pure bypass (centered)
                for (int i = 0; i < MAX_DELAY; i++) { delayBuffer[i] = 0; } // Clear buffer
                delayWritePointer = 0; // Reset write pointer
                break;
            }
        }
        
        // Store mixedSampleForBuffer_Centered back into buffer after re-biasing
        delayBuffer[delayWritePointer] = (uint16_t)constrain(mixedSampleForBuffer_Centered + 511.5, 0, 1023);

    } else { // If effect is not active, pass through clean signal
        outputSampleFloat = centered_input;
        for (int i = 0; i < MAX_DELAY; i++) { delayBuffer[i] = 0; } // Always clear buffer on bypass
        delayWritePointer = 0;
    }

    // Update Delay Buffer Write Pointer
    delayWritePointer++;
    if (delayWritePointer >= MAX_DELAY) {
        delayWritePointer = 0;
    }

    // --- Final Output Processing ---
    // Re-bias the processed sample to 0-1023 range
    int finalOutputSample = (int)(outputSampleFloat + 511.5);

    // Apply global master volume controlled by PUSHBUTTON_1/2
    float volume_factor = pot2_value / 1023.0;
    finalOutputSample = (int)(finalOutputSample * volume_factor);

    // Constrain the final output to the valid 10-bit range (0-1023)
    finalOutputSample = constrain(finalOutputSample, 0, 1023);

    // // Split for dual PWM output
    // analogWrite(AUDIO_OUT_A, finalOutputSample / 4);
    // analogWrite(AUDIO_OUT_B, map(finalOutputSample % 4, 0, 3, 0, 255));

    /*write the PWM output signal*/
    OCR1AL = ((finalOutputSample + 0x8000) >> 8); // convert to unsigned, send out high byte
    OCR1BL = finalOutputSample; // send out low byte
}
