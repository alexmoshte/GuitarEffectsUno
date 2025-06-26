#include "octaver.h"
#include <Arduino.h>

/*********************************************FUNCTION DEFINITIONS****************************************************/
/**
 * @brief: Pin configuration for the Octaver effect.
 * This is primarily for any additional pins *unique* to the Octaver module.
 * Common pins are configured in main.cpp's pinConfig().
 */
void pinConfigOctaver() {
    //Extra pin configurations
}

/**
 * @brief: Setup function for the Octaver effect.
 * Performs any initializations required for the Octaver.
 */
void setupOctaver(){
    Serial.println("Octaver Pedal Ready!");
}

/**
 * @brief: Main loop for the Octaver effect.
 * This function handles any non-time-critical logic specific to the Octaver.
 * Currently, it's a placeholder.
 */
void loopOctaver(){
    // No loop logic yet
}

/**
 * @brief: Audio processing function for Octaver effect.
 * This function is called by the universal ISR (TIMER1_CAPT_vect) from main.cpp.
 * Currently, it implements a simple pass-through with global master volume.
 * This is where the actual octaver algorithm would be implemented.
 * @param inputSample The raw 10-bit input audio sample (0-1023).
 */
void processOctaverAudio(int inputSample) {
    int outputSample = inputSample; // Start with the clean input sample

    if (effectActive) { // Only apply effect if globally active (footswitch state)
        // --- Octaver Algorithm Placeholder ---
        // This is where your octaver logic would go.
        // For demonstration, it's a simple pass-through.
        // Example (very basic down-octave using integer division - not good audio!):
        // outputSample = inputSample / 2;
        // Or to mix it: outputSample = (inputSample + (inputSample / 2)) / 2;

        // For a perceivable constant effect without full algorithm,
        // you could add a subtle modulation or a fixed simple pitch shift here
        // (but proper octavers are complex).
        // For now, it will simply pass through unless a more complex algorithm is added.
    } else {
        outputSample = inputSample; // Pass through clean signal if effect is bypassed
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
