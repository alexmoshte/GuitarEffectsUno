#include "distortion.h"
#include <Arduino.h> 

/*********************************************FUNCTION DEFINITIONS****************************************************/

/**
 * @brief: Pin configuration for the distortion effect.
 * This function is primarily for any additional pins *unique* to the Distortion module.
 * Common pins (audio I/O, effect selection buttons, volume buttons, footswitch, LED)
 * are configured in main.cpp's pinConfig().
 */
void pinConfigDistortion(){
    // Extra pin configurations
}

/**
 * @brief: Setup function for the distortion effect.
 * Performs any initializations required for the Distortion.
 */
void setupDistortion(){
    // pinConfigDistortion(); // No longer needed here, main.cpp handles general pins, and this is for additional.
    // Timer1 is now initialized and its ISR attached universally in main.cpp.
    // NO LONGER NEEDED:
    // Timer1.initialize(SAMPLE_RATE_MICROS);
    // Timer1.attachInterrupt(audioIsrDistortion);
    Serial.println("Distortion Pedal Ready!");
}

/**
 * @brief: Main loop for the distortion effect.
 * This function currently has no specific logic, as control (footswitch, volume, mode selection)
 * is now handled universally in main.cpp. If the Distortion effect ever needs non-time-critical
 * adjustments (e.g., via another toggle or pushbutton unique to Distortion), that logic would go here.
 */
void loopDistortion(){
    // No specific loop logic yet
}

/**
 * @brief: Audio processing function for Distortion effect.
 * This function is called by the universal ISR (TIMER1_CAPT_vect) from main.cpp.
 * It applies the distortion algorithm using fixed parameters.
 * @param inputSample The raw 10-bit input audio sample (0-1023).
 */
void processDistortionAudio(int inputSample) {
    int outputSample;

    if (effectActive) { // Only process effect if globally active (footswitch state)
        // --- Fixed Effect Parameters (formerly POT0 and POT1) ---
        // These constants are chosen to make the effect clearly perceivable.
        const float fixedPreGainFactor = 3.5; // Example: 3.5x pre-gain
        const int fixedDistortionThreshold = 150; // Example: 150 for threshold (smaller = more distortion)

        /* Center the input signal around 0 (range: -512 to 511) */
        float centered_input = (float)inputSample - 512.0;

        // Apply fixed pre-gain
        centered_input *= fixedPreGainFactor;

        /* Apply Symmetrical Hard Clipping based on fixed threshold.
         * Values outside this range are clamped.
         */
        if (centered_input > fixedDistortionThreshold) {
            centered_input = fixedDistortionThreshold;
        } else if (centered_input < -fixedDistortionThreshold) {
            centered_input = -fixedDistortionThreshold;
        }

        /* Re-bias the signal back to 0-1023 range */
        outputSample = (int)(centered_input + 512.0);

    }
    else {
        /* If the effect is not active, simply pass the input sample directly to the output (bypass) */
        outputSample = inputSample;
    }

    /* Final Output Processing */
    outputSample = constrain(outputSample, 0, 1023); // Constrain to valid 10-bit range

    /* Apply global master volume controlled by PUSHBUTTON_1/2 */
    float volume_factor = pot2_value / 1023.0; // pot2_value is controlled by pushbuttons in main.cpp
    outputSample = (int)(outputSample * volume_factor);

    /* Split the final 10-bit outputSample into two 8-bit PWM values for Pins 9 and 10 */
    analogWrite(AUDIO_OUT_A, outputSample / 4); // Coarse 8 bits (MSB)
    analogWrite(AUDIO_OUT_B, map(outputSample % 4, 0, 3, 0, 255)); // Fine 2 bits (LSB)
}
