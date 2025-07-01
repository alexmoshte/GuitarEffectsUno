#include "sinewave.h"
#include <Arduino.h>

#define SINE_TABLE_SIZE 256

static const float SAMPLE_RATE_HZ = 1000000.0 / SAMPLE_RATE_MICROS;

static int nSineTable[SINE_TABLE_SIZE];

static float phase_accumulator = 0.0;
static float frequency_step = 0.0;
static int generated_sample = 0;

/*********************************************FUNCTION DEFINITIONS****************************************************/
void createSineTable(void){
    for (uint16_t nIndex = 0; nIndex < SINE_TABLE_SIZE; nIndex++) {
        nSineTable[nIndex] = (uint16_t)(((1.0 + sin(((2.0 * PI) / SINE_TABLE_SIZE) * nIndex)) * 1023.0) / 2.0);
    }
}

void pinConfigSinewave(void){
    // No specific pins for Sinewave, common pins configured in main.cpp
}

void setupSinewave(void){
    createSineTable();
    Serial.println("SineWave Generator Ready!");

    // Set a fixed frequency step for a perceivable sine wave
    const float fixedFrequencyHz = 440.0; // A4 note (440 Hz)
    frequency_step = (fixedFrequencyHz * SINE_TABLE_SIZE) / SAMPLE_RATE_HZ;
}

void loopSinewave(void){
    // No specific loop logic for Sinewave, controls handled in main.cpp
}

/**
 * @brief: Audio processing function for generating the sine wave.
 * This function is called by the universal ISR (TIMER1_CAPT_vect) from main.cpp.
 * It generates a sine wave using DDS and outputs it.
 * @param inputSample The raw 10-bit input audio sample (0-1023). This parameter is ignored
 * as the sine wave is generated internally, not processed from input.
 */
void processSinewaveAudio(int inputSample) { // inputSample parameter included for ISR consistency
    float outputSampleFloat; // Use float for processing

    if (effectActive) {
        phase_accumulator += frequency_step;

        while (phase_accumulator >= SINE_TABLE_SIZE) {
            phase_accumulator -= SINE_TABLE_SIZE;
        }
        while (phase_accumulator < 0.0) { // Robustness, though frequency_step should be positive
            phase_accumulator += SINE_TABLE_SIZE;
        }

        int idx1 = (int)phase_accumulator;
        float fraction = phase_accumulator - idx1;
        int idx2 = (idx1 + 1) % SINE_TABLE_SIZE;

        int sample1 = nSineTable[idx1];
        int sample2 = nSineTable[idx2];

        // Interpolate between sample1 and sample2 to get the final sine sample
        // These samples are already 0-1023 range, which is perfect for direct output.
        generated_sample = (int)(sample1 + fraction * (sample2 - sample1));
        
        outputSampleFloat = (float)generated_sample - 511.5; // Center for volume application
    }
    else {
        // If generator is off, output silence (midpoint of 10-bit range) and reset phase
        outputSampleFloat = 0.0; // Centered silence
        phase_accumulator = 0.0; // Reset phase for clean restart
    }

    // --- Final Output Processing ---
    // Apply Amplitude/Volume control from global pot2_value
    float amplitude_factor = pot2_value / 1023.0;
    outputSampleFloat *= amplitude_factor; // Apply volume to the centered signal

    // Re-bias the processed sample to 0-1023 range
    int finalOutputSample = (int)(outputSampleFloat + 511.5);

    // Constrain the final output to the valid 10-bit range (0-1023)
    finalOutputSample = constrain(finalOutputSample, 0, 1023);

    // Split for dual PWM output
    analogWrite(AUDIO_OUT_A, finalOutputSample / 4);
    analogWrite(AUDIO_OUT_B, map(finalOutputSample % 4, 0, 3, 0, 255));
}
