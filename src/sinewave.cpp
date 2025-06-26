#include "sinewave.h"
#include <Arduino.h>

#define SINE_TABLE_SIZE 256 // 256 samples * 2 bytes/uint16_t = 512 bytes

/*Audio Configuration*/
static const float SAMPLE_RATE_HZ = 1000000.0 / SAMPLE_RATE_MICROS;

/*Sine Wave Lookup Table*/
static int nSineTable[SINE_TABLE_SIZE]; // Stores 10-bit samples (0-1023)

/*Sine Wave Generation Variables (volatile for ISR access)*/
static float phase_accumulator = 0.0; // Floating-point accumulator for smooth phase
static float frequency_step = 0.0;    // How much 'phase_accumulator' advances per sample
static int generated_sample = 0;      // The current sine wave sample to output

/*********************************************FUNCTION DEFINITIONS****************************************************/
/**
 * @brief: Function to create the sine wave lookup table.
 * Calculates sine value, normalized to Uno's 10-bit range (0-1023).
 * (1 + sin(angle)) shifts -1 to 1 range to 0 to 2.
 * Then scale by 1023.0 / 2 to map to 0-1023 range.
 */
void createSineTable(void){
    for (uint16_t nIndex = 0; nIndex < SINE_TABLE_SIZE; nIndex++) {
        nSineTable[nIndex] = (uint16_t)(((1.0 + sin(((2.0 * PI) / SINE_TABLE_SIZE) * nIndex)) * 1023.0) / 2.0);
    }
}

/**
 * @brief: Pin configuration for the sinewave effect.
 * This is primarily for any additional pins *unique* to the Sinewave module.
 * Common pins (audio I/O, effect selection buttons, volume buttons, footswitch, LED)
 * are configured in main.cpp's pinConfig().
 */
void pinConfigSinewave(void){
    // If there are any unique pins for Sinewave beyond those in main.cpp, configure them here.
    // Example: pinMode(SINEWAVE_SPECIFIC_PIN, OUTPUT);
}

/**
 * @brief: Setup function for the sinewave generator.
 * Creates the sine wave lookup table.
 */
void setupSinewave(void){
    createSineTable();
    // pinConfigSinewave(); // No longer needed here, main.cpp handles general pins, and this is for additional.
    // Timer1 is now initialized and its ISR attached universally in main.cpp.
    // NO LONGER NEEDED:
    // Timer1.initialize(SAMPLE_RATE_MICROS);
    // Timer1.attachInterrupt(audioIsrSinewave);
    Serial.println("SineWave Generator Ready!");

    // Set a fixed frequency step for a perceivable sine wave
    const float fixedFrequencyHz = 440.0; // A4 note (can be changed to any desired frequency)
    frequency_step = (fixedFrequencyHz * SINE_TABLE_SIZE) / SAMPLE_RATE_HZ;
}

/**
 * @brief: Main loop for the sinewave generator.
 * This function currently has no specific logic, as control (footswitch, volume, mode selection)
 * is now handled universally in main.cpp.
 */
void loopSinewave(void){
    // No specific loop logic for Sinewave implemented yet.
    // The main controls (effect on/off, volume, effect selection) are in main.cpp.
}

/**
 * @brief: Audio processing function for generating the sine wave.
 * This function is called by the universal ISR (TIMER1_CAPT_vect) from main.cpp.
 * It generates a sine wave using DDS and outputs it.
 * @param inputSample The raw 10-bit input audio sample (0-1023). This parameter is ignored
 * as the sine wave is generated internally, not processed from input.
 */
void processSinewaveAudio() { // inputSample parameter removed as it's not used by a generator
    // Check global 'effectActive' flag to determine if generator should be active
    if (effectActive) {
        phase_accumulator += frequency_step; // Advance the phase accumulator

        /* Wrap the phase accumulator around the table size.
         * Use 'while' loop for safety in case 'frequency_step' is large (e.g., high frequencies).
         */
        while (phase_accumulator >= SINE_TABLE_SIZE) {
            phase_accumulator -= SINE_TABLE_SIZE;
        }
        while (phase_accumulator < 0.0) { // Should not happen with positive frequency_step, but good for robustness
            phase_accumulator += SINE_TABLE_SIZE;
        }

        /* Linear Interpolation for Smoother Waveform. */
        int idx1 = (int)phase_accumulator;     // Get the integer part of the read pointer
        float fraction = phase_accumulator - idx1; // Get the fractional part for interpolation
        int idx2 = (idx1 + 1) % SINE_TABLE_SIZE; // Get the next sample index (wrap around if needed)

        /* Read the two samples from the table */
        int sample1 = nSineTable[idx1];
        int sample2 = nSineTable[idx2];

        /* Interpolate between sample1 and sample2 to get the final sine sample */
        generated_sample = (int)(sample1 + fraction * (sample2 - sample1));

        /* Apply Amplitude/Volume control from global pot2_value */
        float amplitude_factor = pot2_value / 1023.0; // pot2_value is controlled by pushbuttons in main.cpp

        /* Scale the generated sample by the amplitude factor around the midpoint */
        float centered_sample = (float)generated_sample - 511.5; // Center around 0
        centered_sample *= amplitude_factor; // Apply amplitude
        generated_sample = (int)(centered_sample + 511.5); // Re-center to 0-1023
    }
    else {
        /* If generator is off, output silence (midpoint of 10-bit range)*/
        generated_sample = 1023 / 2; // Roughly 511 or 512
        phase_accumulator = 0.0; // Reset phase for clean restart
    }

    /*Final Output Processing*/
    generated_sample = constrain(generated_sample, 0, 1023); // Constrain to valid 10-bit range

    /*Split the final 10-bit generated_sample into two 8-bit PWM values for Pins 9 and 10*/
    analogWrite(AUDIO_OUT_A, generated_sample / 4); // Coarse 8 bits
    analogWrite(AUDIO_OUT_B, map(generated_sample % 4, 0, 3, 0, 255)); // Fine 2 bits
}
