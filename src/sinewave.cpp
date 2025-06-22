#include <main.h> 
#include <math.h> 

#define SINE_TABLE_SIZE 256// 256 samples * 2 bytes/uint16_t = 512 bytes

/*Audio Configuration*/
const float SAMPLE_RATE_HZ = 1000000.0 / SAMPLE_RATE_MICROS;

/*Sine Wave Lookup Table*/
uint16_t nSineTable[SINE_TABLE_SIZE]; // Stores 10-bit samples (0-1023)

/*Sine Wave Generation Variables (volatile for ISR access)*/
volatile float phase_accumulator = 0.0; // Floating-point accumulator for smooth phase
volatile float frequency_step = 0.0;    // How much 'phase_accumulator' advances per sample
volatile int generated_sample = 0;      // The current sine wave sample to output

/*Effect Parameters*/
volatile int pot0_value = 0; // Frequency control
volatile int pot2_value = 0; // Amplitude/Volume control

volatile bool generatorActive = false; 

/*Button Debouncing Variables*/
volatile unsigned long lastFootswitchPressTime = 0;
const unsigned long DEBOUNCE_DELAY_MS = 100; 

/*********************************************FUNCTION DEFINITIONS****************************************************/
/**
 * @brief: Function to create the sine wave lookup table. Calculate sine value, normalized to Uno's 10-bit range (0-1023). 
 * (1 + sin(angle)) shifts -1 to 1 range to 0 to 2. Then scale by 1023.0 / 2 to map to 0-1023 range
 */
void createSineTable(void){
  for (uint16_t nIndex = 0; nIndex < SINE_TABLE_SIZE; nIndex++) {
    nSineTable[nIndex] = (uint16_t)(((1.0 + sin(((2.0 * PI) / SINE_TABLE_SIZE) * nIndex)) * 1023.0) / 2.0);
  }
}

/**
 * @brief: Pin configuration for the sinewave effect
 */
void pinConfigSinewave(void){
  pinMode(AUDIO_OUT_A, OUTPUT);
  pinMode(AUDIO_OUT_B, OUTPUT);
  pinMode(POT0, INPUT);
  pinMode(POT2, INPUT);
  pinMode(FOOTSWITCH, INPUT_PULLUP); 
  pinMode(LED_EFFECT_ON, OUTPUT);
}

/**
 * @brief: Setup function for the sinewave generator
 */
void setupSinewave(void){
  createSineTable(); 
  //pinConfigSinewave();
  Timer1.initialize(SAMPLE_RATE_MICROS);
  Timer1.attachInterrupt(audioIsrSinewave);
  Serial.println("SineWave Generator Ready!");
}

/**
 * @brief: Main loop for the sinewave generator
 */
void loopSinewave(void){
  pot0_value = analogRead(POT0); 
  int pot1_dummy = analogRead(A2); 
  pot2_value = analogRead(POT2); 

/*Calculate frequency step based on POT0*/
  float min_freq_hz = 20.0;
  float max_freq_hz = 2000.0;
  float desired_freq = map(pot0_value, 0, 1023, (int)(min_freq_hz * 10), (int)(max_freq_hz * 10)) / 10.0;
  frequency_step = (desired_freq * SINE_TABLE_SIZE) / SAMPLE_RATE_HZ;

  /*Handle the Footswitch (Generator ON/OFF)*/
  if (digitalRead(FOOTSWITCH) == LOW) { 
    if (millis() - lastFootswitchPressTime > DEBOUNCE_DELAY_MS) {
      lastFootswitchPressTime = millis();
      generatorActive = !generatorActive;

      if (generatorActive) {
        Serial.println("Generator ON");
      } else {
        Serial.println("Generator OFF"); 
        phase_accumulator = 0.0;
      }
    }
  }

  // Update LED status based on 'generatorActive' flag.
  digitalWrite(LED_EFFECT_ON, generatorActive ? HIGH : LOW);
}

/**
 * @brief: Audio Interrupt Service Routine (ISR) for generating the sine wave.
 */
void audioIsrSinewave() {
  if (generatorActive) {
    phase_accumulator += frequency_step; // Advance the phase accumulator

    /*Wrap the phase accumulator around the table size. Use 'while' loop for safety in case 'frequency_step' is large (e.g., high frequencies)*/
    while (phase_accumulator >= SINE_TABLE_SIZE) {
      phase_accumulator -= SINE_TABLE_SIZE;
    }
    while (phase_accumulator < 0.0) {
      phase_accumulator += SINE_TABLE_SIZE;
    }

    /*Linear Interpolation for Smoother Waveform. Get the integer part of the read pointer*/
    int idx1 = (int)phase_accumulator;

    /*Get the fractional part for interpolation*/
    float fraction = phase_accumulator - idx1;

    /*Get the next sample index (wrap around if needed)*/
    int idx2 = (idx1 + 1) % SINE_TABLE_SIZE;

    /*Read the two samples from the table*/
    int sample1 = nSineTable[idx1];
    int sample2 = nSineTable[idx2];

    /*Interpolate between sample1 and sample2 to get the final sine sample*/
    generated_sample = (int)(sample1 + fraction * (sample2 - sample1));

    /* Apply Amplitude/Volume control from POT2*/
    float amplitude_factor = pot2_value / 1023.0;
    
    /*Scale the generated sample by the amplitude factor around the midpoint*/
    float centered_sample = (float)generated_sample - 511.5; // Center around 0
    centered_sample *= amplitude_factor; // Apply amplitude
    generated_sample = (int)(centered_sample + 511.5); // Re-center to 0-1023

  } 
  else {
    /* If generator is off, output silence (midpoint of 10-bit range)*/
    generated_sample = 1023 / 2; // Roughly 511 or 512
  }

  /*Final Output Processing*/
  generated_sample = constrain(generated_sample, 0, 1023); // Constrain the generated audio sample to the valid 10-bit range (0-1023)

  /*Split the final 10-bit generated_sample into two 8-bit PWM values for Pins 9 and 10*/
  int pwm9Value = generated_sample / 4;
  int pwm10RawFine = generated_sample % 4;
  int pwm10Value = map(pwm10RawFine, 0, 3, 0, 255);

  /*Write the calculated PWM values to the respective output pins*/
  analogWrite(AUDIO_OUT_A, pwm9Value);
  analogWrite(AUDIO_OUT_B, pwm10Value);
}