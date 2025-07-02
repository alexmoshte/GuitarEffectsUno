#include "echo.h"
#include <Arduino.h>

/*********************************************FUNCTION DEFINITIONS****************************************************/
void pinConfigEcho(){
    // No specific pins for Echo, common pins configured in main.cpp
}

void setupEcho(){
    for (int i = 0; i < MAX_DELAY; i++) {
        delayBuffer[i] = 0;
    }
    Serial.println("Echo Pedal Ready!");
}

void loopEcho(){
    // No specific loop logic for Echo, controls handled in main.cpp
}

/**
 * @brief: Audio processing function for Echo effect.
 * Uses centered floating-point math to prevent clipping and buzzing.
 * @param inputSample The raw 10-bit input audio sample (0-1023).
 */
void processEchoAudio(int inputSample) {
    float outputSampleFloat; // Use float for processing
    float centered_input = (float)inputSample - 511.5; // Center input to -511.5 to 511.5

    if (effectActive) {
        // --- Fixed Effect Parameters ---
        const int fixedEchoDelayTimeValue = 600; // Delay time (e.g., 600 maps to ~MAX_DELAY/1.7)
        const float fixedEchoFeedbackFactor = 0.65; // 65% feedback

        int currentDelayDepth = map(fixedEchoDelayTimeValue, 0, 1023, 1, MAX_DELAY - 1);
        int delayReadPointer = (delayWritePointer + (MAX_DELAY - currentDelayDepth)) % MAX_DELAY;

        // Get delayed sample from buffer and center it
        float delayedSample_Centered = (float)delayBuffer[delayReadPointer] - 511.5;

        // The core echo algorithm: new sample in buffer is input + feedback of delayed sample
        float newSampleForBuffer_Centered = centered_input + (delayedSample_Centered * fixedEchoFeedbackFactor);
        
        // Store newSampleForBuffer_Centered back into buffer after re-biasing
        delayBuffer[delayWritePointer] = (uint16_t)constrain(newSampleForBuffer_Centered + 511.5, 0, 1023);

        // Final output is sum of dry input and delayed signal (for clear echo)
        outputSampleFloat = centered_input + delayedSample_Centered;

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

    // Split for dual PWM output
    analogWrite(AUDIO_OUT_A, finalOutputSample / 4);
    analogWrite(AUDIO_OUT_B, map(finalOutputSample % 4, 0, 3, 0, 255));
}
