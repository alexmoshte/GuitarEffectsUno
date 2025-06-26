#include "echo.h"
#include <Arduino.h> 

/*********************************************FUNCTION DEFINITIONS****************************************************/
/**
 * @brief: Pin configuration for the Echo effect.
 * This function is primarily for any additional pins *unique* to the Echo module.
 * Common pins (audio I/O, effect selection buttons, volume buttons, footswitch, LED)
 * are configured in main.cpp's pinConfig().
 */
void pinConfigEcho(){
    // If there are any unique pins for Echo beyond those in main.cpp, configure them here.
    // Example: pinMode(ECHO_SPECIFIC_PIN, OUTPUT);
}

/**
 * @brief: Setup function for the echo effect.
 * Initializes the delay buffer to silence.
 */
void setupEcho(){
    // pinConfigEcho(); // No longer needed here, main.cpp handles general pins, and this is for additional.
    for (int i = 0; i < MAX_DELAY; i++) { // Initialize delay buffer with silence
        delayBuffer[i] = 0;
    }  
    Serial.println("Echo Pedal Ready!");
}

/**
 * @brief: Main loop for the echo effect.
 * This function currently has no specific logic, as control (footswitch, volume, mode selection)
 * is now handled universally in main.cpp. If the Echo effect ever needs non-time-critical
 * adjustments (e.g., via another toggle or pushbutton unique to Echo), that logic would go here.
 */
void loopEcho(){
    // No specific loop logic yet
}

/**
 * @brief: Audio processing function for Echo effect.
 * This function is called by the universal ISR (TIMER1_CAPT_vect) from main.cpp.
 * It applies the echo algorithm using fixed parameters.
 * @param inputSample The raw 10-bit input audio sample (0-1023).
 */
void processEchoAudio(int inputSample) {
    int outputSample = inputSample; // Initialize output as clean signal (pass-through)

    if (effectActive) { // Only process effect if globally active (footswitch state)
        // --- Fixed Effect Parameters (formerly POT0 and POT1) ---
        // These constants are chosen to make the effect clearly perceivable.
        const int fixedEchoDelayTimeValue = 600; // Value for delay time (maps to MAX_DELAY/1.7 approx)
        const float fixedEchoFeedbackFactor = 0.65; // 65% feedback (0.0 to 0.98 range)

        /* Determine the current delay depth based on a fixed value, not potentiometer.
         * Maps fixedEchoDelayTimeValue (e.g., 600) to a valid delay offset (1 to MAX_DELAY - 1).
         */
        int currentDelayDepth = map(fixedEchoDelayTimeValue, 0, 1023, 1, MAX_DELAY - 1);

        /* Calculate the read pointer for the circular buffer */
        int delayReadPointer = (delayWritePointer + (MAX_DELAY - currentDelayDepth)) % MAX_DELAY;

        // Get the previously stored (delayed) sample from the buffer.
        int delayedSample = delayBuffer[delayReadPointer];

        /* The core echo algorithm: current input plus a portion of the delayed signal */
        // For classic echo, the new sample *in the buffer* is the input + feedback of delayed sample
        // This makes distinct repeats.
        int newSampleForBuffer = inputSample + (int)(delayedSample * fixedEchoFeedbackFactor);
        
        /* Write the new sample into the delay buffer at the current write pointer */
        delayBuffer[delayWritePointer] = newSampleForBuffer;

        /* For the final output, mix the dry input signal with the wet delayed signal */
        outputSample = inputSample + delayedSample; // Simple summing for a clear echo
    }
    else { // If effect is not active (bypassed via footswitch)
        outputSample = inputSample; // Pass through clean signal
        // Always clear buffer when bypassing to prevent lingering sound
        for (int i = 0; i < MAX_DELAY; i++) {
            delayBuffer[i] = 0;
        }
        delayWritePointer = 0;
    }

    /* Update Delay Buffer Write Pointer. Wraps to the beginning when it reaches the end */
    delayWritePointer++;
    if (delayWritePointer >= MAX_DELAY) {
        delayWritePointer = 0;
    }

    /* Final Output Processing */
    outputSample = constrain(outputSample, 0, 1023); // Constrain the processed audio sample to the valid 10-bit range (0-1023).

    /* Apply global master volume controlled by PUSHBUTTON_1/2 */
    float volume_factor = pot2_value / 1023.0; // pot2_value is controlled by pushbuttons in main.cpp
    outputSample = (int)(outputSample * volume_factor);

    /* Split the final 10-bit outputSample into two 8-bit PWM values for Pins 9 and 10 */
    analogWrite(AUDIO_OUT_A, outputSample / 4); // Coarse 8 bits
    analogWrite(AUDIO_OUT_B, map(outputSample % 4, 0, 3, 0, 255)); // Fine 2 bits
}
