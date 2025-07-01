#include "distortion.h"
#include <Arduino.h>

// Global variables (declared as extern in main.h, defined in main.cpp)
// No need to redefine them here.

/*********************************************FUNCTION DEFINITIONS****************************************************/
void pinConfigDistortion(){
    // No specific pins for Distortion, common pins configured in main.cpp
}

void setupDistortion(){
    Serial.println("Distortion Pedal Ready!");
}

void loopDistortion(){
    // No specific loop logic for Distortion, controls handled in main.cpp
}

/**
 * @brief: Audio processing function for Distortion effect.
 * Uses centered floating-point math for accurate clipping.
 * @param inputSample The raw 10-bit input audio sample (0-1023).
 */
void processDistortionAudio(int inputSample) {
    float outputSampleFloat; // Use float for processing
    float centered_input = (float)inputSample - 511.5; // Center input to -511.5 to 511.5

    if (effectActive) {
        // --- Fixed Effect Parameters ---
        const float fixedPreGainFactor = 3.5; // Example: 3.5x pre-gain
        const int fixedDistortionThreshold = 150; // Example: 150 for threshold (smaller = more distortion)

        // Apply fixed pre-gain to the centered input
        float gained_input = centered_input * fixedPreGainFactor;

        /* Apply Symmetrical Hard Clipping based on fixed threshold.
         * Values outside this range are clamped.
         */
        if (gained_input > fixedDistortionThreshold) {
            outputSampleFloat = fixedDistortionThreshold;
        } else if (gained_input < -fixedDistortionThreshold) {
            outputSampleFloat = -fixedDistortionThreshold;
        } else {
            outputSampleFloat = gained_input; // Signal is within threshold, no clipping
        }
    }
    else {
        outputSampleFloat = centered_input; // Pass through clean signal if effect is bypassed
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
