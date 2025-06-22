#include <distortion.h> 

/*Effect Parameters*/
volatile int pot0_value = 0; // Distortion Threshold
volatile int pot1_value = 0; // Pre-Gain
volatile int pot2_value = 0; // Master Volume

/* This threshold will be applied to the centered 10-bit audio signal (-512 to 511)*/
volatile int distortion_threshold_val = 250; // Initial threshold, adjustable by pots/buttons

volatile bool effectActive = false; 

/*Button Debouncing and Counter Variables*/
volatile unsigned long lastFootswitchPressTime = 0;
volatile unsigned long lastPushButton1PressTime = 0;
volatile unsigned long lastPushButton2PressTime = 0;
const unsigned long DEBOUNCE_DELAY_MS = 100; // Minimum time between button state changes

/*Counter to periodically check pushbuttons (saves CPU in ISR)*/
volatile int button_check_counter = 0;
const int BUTTON_CHECK_INTERVAL = 500; // Check buttons every 500 samples

/*********************************************FUNCTION DEFINITIONS****************************************************/

/**
 * @brief: Pin configuration for the distortion effect
 */
void pinConfigDistortion(){
  pinMode(AUDIO_IN, INPUT);
  pinMode(AUDIO_OUT_A, OUTPUT);
  pinMode(AUDIO_OUT_B, OUTPUT);
  pinMode(POT0, INPUT);
  pinMode(POT1, INPUT);
  pinMode(POT2, INPUT);
  pinMode(FOOTSWITCH, INPUT_PULLUP);
  pinMode(PUSHBUTTON_1, INPUT_PULLUP);
  pinMode(PUSHBUTTON_2, INPUT_PULLUP);
  pinMode(LED_EFFECT_ON, OUTPUT);
}

/**
 * @brief: Setup function for the distortion effect
 */
void setupDistortion(){
  // pinConfigDistortion();
  Timer1.initialize(SAMPLE_RATE_MICROS);
  Timer1.attachInterrupt(audioIsrDistortion);
  Serial.println("Distortion Pedal Ready!");
}

/**
 * @brief: Main loop for the distortion effect. This function handles the user interface and effect toggling.
 */
void loopDistortion(){
  // Read potentiometer values (non-time-critical, done outside ISR)
  pot0_value = analogRead(POT0); // For Distortion Threshold
  pot1_value = analogRead(POT1); // For Pre-Gain
  pot2_value = analogRead(POT2); // For Master Volume

  /*Handle the Footswitch (Effect ON/OFF)*/
  if (digitalRead(FOOTSWITCH) == LOW) { 
    if (millis() - lastFootswitchPressTime > DEBOUNCE_DELAY_MS) {
      lastFootswitchPressTime = millis();
      effectActive = !effectActive; // Toggle the effect ON/OFF

      if (effectActive) {
        Serial.println("Distortion ON");
      } else {
        Serial.println("Bypass");
      }
    }
  }

  /* Handle Pushbuttons for Fine-tuning Distortion Threshold. Only check periodically to avoid excessive CPU usage in loop*/
  if (millis() % 25 == 0) { // Check every ~25ms
    if (digitalRead(PUSHBUTTON_2) == LOW) { // Pushbutton 2 pressed (Increase Distortion / Decrease Threshold)
      if (millis() - lastPushButton2PressTime > DEBOUNCE_DELAY_MS) {
        lastPushButton2PressTime = millis();
        if (distortion_threshold_val > 50) { // Min threshold (max distortion)
          distortion_threshold_val -= 5;     // Decrease threshold by a small step
          Serial.print("Threshold: "); Serial.println(distortion_threshold_val);
        }
      }
    }

    if (digitalRead(PUSHBUTTON_1) == LOW) { // Pushbutton 1 pressed (Decrease Distortion / Increase Threshold)
      if (millis() - lastPushButton1PressTime > DEBOUNCE_DELAY_MS) {
        lastPushButton1PressTime = millis();
        if (distortion_threshold_val < 500) { // Max threshold (min distortion, avoid no effect)
          distortion_threshold_val += 5;     // Increase threshold by a small step
          Serial.print("Threshold: "); Serial.println(distortion_threshold_val);
        }
      }
    }
  }

  digitalWrite(LED_EFFECT_ON, effectActive ? HIGH : LOW);
}

/**
 * @brief: Audio Interrupt Service Routine (ISR) for processing audio samples with distortion effect
 */
void audioIsrDistortion() {
  int inputSample = analogRead(AUDIO_IN); // Read 10-bit input (0-1023)
  int outputSample;                  

  if (effectActive) {
    /*Pre-Gain Control (from POT1). Scale POT1 (0-1023) to a gain factor (e.g., 1.0 to 5.0)*/
    float pre_gain_factor = map(pot1_value, 0, 1023, 10, 50) / 10.0; // 1.0 to 5.0

    /*Center the input signal around 0 (range: -512 to 511)*/
    float centered_input = (float)inputSample - 512.0;

    // Apply pre-gain
    centered_input *= pre_gain_factor;

    /* Distortion Threshold Control (from POT0). Maps POT0 (0-1023) inversely to threshold (e.g., 500 to 50 for more distortion). A smaller threshold means more signal gets clipped, resulting in harder distortion*/
    int pot_threshold = map(pot0_value, 0, 1023, 500, 50); // Min 50 (hard clip), Max 500 (soft clip)

    /*Combine POT0 threshold with pushbutton adjustments. The pushbutton adjustments will effectively shift the pot_threshold up/down*/
    int current_clipping_threshold = pot_threshold;

    /*Add logic to combine pot_threshold with button adjustments*/
    if (button_check_counter % BUTTON_CHECK_INTERVAL == 0) {
        // Only update if buttons are not pressed (to let buttons take precedence when active)
        if (digitalRead(PUSHBUTTON_1) == HIGH && digitalRead(PUSHBUTTON_2) == HIGH) {
            distortion_threshold_val = pot_threshold; // POT0 sets the threshold if no buttons are pressed
        }
    }

    /*Apply Symmetrical Hard Clipping. Clip the centered_input based on the current_clipping_threshold. Values outside this range are clamped*/
    if (centered_input > distortion_threshold_val) {
      centered_input = distortion_threshold_val;
    } else if (centered_input < -distortion_threshold_val) {
      centered_input = -distortion_threshold_val;
    }

    /*Re-bias the signal back to 0-1023 range*/
    outputSample = (int)(centered_input + 512.0);

  } 
  else {
    /*If the effect is not active, simply pass the input sample directly to the output (bypass)*/
    outputSample = inputSample;
  }

  /*Update Button Check Counter (for loop-based button reading within ISR)*/
  button_check_counter++;
  if (button_check_counter >= BUTTON_CHECK_INTERVAL) {
    button_check_counter = 0;
  }

  /*Final Output Processing*/
  outputSample = constrain(outputSample, 0, 1023);   // Constrain the processed audio sample to the valid 10-bit range (0-1023).

  
  float volume_factor = pot2_value / 1023.0; // Apply master volume control from POT2. Scale POT2 (0-1023) to 0.0-1.0
  outputSample = (int)(outputSample * volume_factor);

  /*Split the final 10-bit outputSample into two 8-bit PWM values for Pins 9 and 10*/
  int pwm9Value = outputSample / 4; // Effectively takes the top 8 bits (0-255)
  int pwm10RawFine = outputSample % 4;
  int pwm10Value = map(pwm10RawFine, 0, 3, 0, 255);

  /*Write the calculated PWM values to the respective output pins*/
  analogWrite(AUDIO_OUT_A, pwm9Value);
  analogWrite(AUDIO_OUT_B, pwm10Value);
}