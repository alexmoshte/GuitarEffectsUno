#include "octaver.h"
#include <Arduino.h>



/*********************************************FUNCTION DEFINITIONS****************************************************/
void pinConfigOctaver() {
    // No specific pins for Octaver, common pins configured in main.cpp
}

void setupOctaver(){
    Serial.println("Octaver Pedal Ready!");
}

void loopOctaver(){
    // No specific loop logic for Octaver, controls handled in main.cpp
}

/**
 * @brief: Audio processing function for Octaver effect.
 * Currently a placeholder, but processes centered floating-point samples.
 * @param inputSample The raw 10-bit input audio sample (0-1023).
 */
void processOctaverAudio(int inputSample) {
    float outputSampleFloat; // Use float for processing
    float centered_input = (float)inputSample - 511.5; // Center input to -511.5 to 511.5

    if (effectActive) {
        // --- OCTAVER ALGORITHM PLACEHOLDER ---
        // This is where your actual Octaver DSP would go.
        // For a simple test, you could mix a very basic down-pitched signal.
        // A true octaver requires complex algorithms (e.g., zero-crossing detection, wave shaping).
        // For now, it behaves as a clean pass-through with volume.
        
        outputSampleFloat = centered_input; // Placeholder: currently just passes through centered signal
        // Example: very crude half-wave rectifier for a simple 'fuzz' or sub-octave like sound
        // if (centered_input < 0) centered_input = 0; // Half-wave rectification
        // outputSampleFloat = centered_input;

    } else {
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
