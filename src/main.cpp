#include "main.h"
#include "reverb.h"
#include "echo.h"
#include "octaver.h"
#include "distortion.h"
#include "sinewave.h"

int input_raw_sample;
uint8_t ADC_low, ADC_high;

uint16_t delayBuffer[MAX_DELAY]; 
uint32_t delayWritePointer = 0;
uint32_t delayReadOffset;
uint32_t delayDepth;

int counter = 0; // For volume control logic

volatile int pot2_value = 512; //initialized to mid-range (0-1023)

volatile bool effectActive = false; 
volatile EffectMode currentActiveMode = NORMAL_MODE; // Initially set, will be updated by setup
volatile EffectMode lastSelectedMode = NORMAL_MODE;  // Stores the last non-CLEAN effect mode

/*Initializing debouncing Variables*/
volatile unsigned long lastFootswitchPressTime = 0;
volatile unsigned long lastToggleSwitchStateChange = 0;
const unsigned long DEBOUNCE_DELAY_MS = 100;
volatile unsigned long lastPushButton1PressTime = 0;
volatile unsigned long lastPushButton2PressTime = 0;
volatile unsigned long lastSelectNormalPressTime = 0;
volatile unsigned long lastSelectEffectButtonA2PressTime = 0;
volatile unsigned long lastSelectOctaverPressTime = 0;

/* Counter to periodically check pushbuttons (for volume control) */
volatile int button_check_counter = 0;
const int BUTTON_CHECK_INTERVAL = 100;

const long SAMPLE_RATE_MICROS = 50;

void setup() {
    Serial.begin(9600);

    pinConfig(); // Configure all I/O pins
    adcSetup();  // Configure ADC
    pmwSetup();  // Configure PWM and Timer1 ISR

    // Initialize specific effect modules
    #ifdef REVERB
    setUpReverb();
    lastSelectedMode = REVERB_ECHO_MODE; // Default startup mode if REVERB compiled
    #elif defined(ECHO)
    setupEcho();
    lastSelectedMode = ECHO_MODE;
    #elif defined(OCTAVER)
    setupOctaver();
    lastSelectedMode = OCTAVER_MODE;
    #elif defined(DISTORTION)
    setupDistortion();
    lastSelectedMode = DISTORTION_MODE;
    #elif defined(SINEWAVE)
    setupSinewave();
    lastSelectedMode = SINEWAVE_MODE;
    #else
    lastSelectedMode = NORMAL_MODE; // Default if no specific effect is compiled
    #endif

    // Initial state after setup: go to lastSelectedMode unless FOOTSWITCH is pressed for CLEAN
    currentActiveMode = lastSelectedMode;
    effectActive = true; // Assume effect is active initially, can be overridden by footswitch

    Serial.println("Arduino Audio Pedal Ready!");
}

void loop() {
    // --- Momentary Global Bypass (FOOTSWITCH) ---
    //FOOTSWITCH is pressed (LOW), force CLEAN_MODE. Otherwise, run the last selected effect*/
    if (digitalRead(FOOTSWITCH) == LOW) { // FOOTSWITCH IS PRESSED
        // Debounce for the initial press
        if (millis() - lastFootswitchPressTime > DEBOUNCE_DELAY_MS) {
            lastFootswitchPressTime = millis(); // Update timestamp only if debounced
            // No action needed here beyond setting currentActiveMode
        }
        currentActiveMode = CLEAN_MODE; // Force bypass
        effectActive = false; // Indicate effect is not active
        digitalWrite(LED_EFFECT_ON, LOW); // LED OFF when in clean mode via footswitch
    } else { // FOOTSWITCH IS NOT PRESSED (HIGH)
        currentActiveMode = lastSelectedMode; // Revert to last selected effect
        effectActive = true; // Indicate effect is active
        digitalWrite(LED_EFFECT_ON, HIGH); // LED ON when effect is active
    }

    // --- Momentary Effect Selection/Activation via A1, A2, A3 ---
    // These buttons override the FOOTSWITCH if pressed.
    bool buttonA1Pressed = (digitalRead(SELECT_NORMAL_BUTTON) == LOW);
    bool buttonA2Pressed = (digitalRead(SELECT_EFFECT_BUTTON_A2) == LOW);
    bool buttonA3Pressed = (digitalRead(SELECT_OCTAVER_BUTTON) == LOW);

    // If any selection button is pressed, it takes precedence over FOOTSWITCH and activates its effect momentarily.
    if (buttonA1Pressed || buttonA2Pressed || buttonA3Pressed) {
        // Debounce for A1 button
        if (buttonA1Pressed && (millis() - lastSelectNormalPressTime > DEBOUNCE_DELAY_MS)) {
            lastSelectNormalPressTime = millis();
            lastSelectedMode = NORMAL_MODE; // Set as last selected
            Serial.println("Momentary Mode: NORMAL");
            currentActiveMode = NORMAL_MODE;
            effectActive = true;
            digitalWrite(LED_EFFECT_ON, HIGH);
        }
        // Debounce for A2 button (Reverb/Echo/Distortion/Sinewave)
        if (buttonA2Pressed && (millis() - lastSelectEffectButtonA2PressTime > DEBOUNCE_DELAY_MS)) {
            lastSelectEffectButtonA2PressTime = millis();
            #ifdef REVERB
            lastSelectedMode = REVERB_ECHO_MODE;
            Serial.println("Momentary Mode: REVERB (Echo)");
            currentActiveMode = REVERB_ECHO_MODE;
            #elif defined(ECHO)
            lastSelectedMode = ECHO_MODE;
            Serial.println("Momentary Mode: ECHO");
            currentActiveMode = ECHO_MODE;
            #elif defined(DISTORTION)
            lastSelectedMode = DISTORTION_MODE;
            Serial.println("Momentary Mode: DISTORTION");
            currentActiveMode = DISTORTION_MODE;
            #elif defined(SINEWAVE)
            lastSelectedMode = SINEWAVE_MODE;
            Serial.println("Momentary Mode: SINEWAVE");
            currentActiveMode = SINEWAVE_MODE;
            #endif
            effectActive = true;
            digitalWrite(LED_EFFECT_ON, HIGH);
        }
        // Debounce for A3 button (Octaver)
        if (buttonA3Pressed && (millis() - lastSelectOctaverPressTime > DEBOUNCE_DELAY_MS)) {
            lastSelectOctaverPressTime = millis();
            #ifdef OCTAVER
            lastSelectedMode = OCTAVER_MODE;
            Serial.println("Momentary Mode: OCTAVER");
            currentActiveMode = OCTAVER_MODE;
            #endif
            effectActive = true;
            digitalWrite(LED_EFFECT_ON, HIGH);
        }

        // When any effect selection button is pressed, always clear delay buffer
        // to prevent sound artifacts from previous modes.
        for (int i = 0; i < MAX_DELAY; i++) {
            delayBuffer[i] = 0;
        }
        delayWritePointer = 0;

    } else { // No effect selection button is pressed
        // If no momentary button is pressed, effectActive and currentActiveMode
        // are determined by the FOOTSWITCH state, as set at the start of loop().
        // Nothing to do here explicitly, as it was handled already.
    }


    // --- Effect-specific loop functions ---
    // These functions now primarily handle sub-mode selection (like REVERB's TOGGLE)
    // and any other non-time-critical logic specific to their effect.
    // They are called only if their respective #define is active.
    #ifdef REVERB
    // If the current active mode is a REVERB sub-mode (set by A2 or lastSelectedMode),
    // then allow its loop function to manage its internal TOGGLE switch.
    if (currentActiveMode == REVERB_ECHO_MODE || currentActiveMode == DELAY_MODE) {
        loopReverb();
    }
    #elif defined(ECHO)
    if (currentActiveMode == ECHO_MODE) {
        loopEcho();
    }
    #elif defined(OCTAVER)
    if (currentActiveMode == OCTAVER_MODE) {
        loopOctaver();
    }
    #elif defined(DISTORTION)
    if (currentActiveMode == DISTORTION_MODE) {
        loopDistortion();
    }
    #elif defined(SINEWAVE)
    if (currentActiveMode == SINEWAVE_MODE) {
        loopSinewave();
    }
    #endif
}

/**
 * @brief: Universal Timer 1 interrupt for audio sampling and dispatch.
 * This ISR is responsible for reading the ADC at a consistent high frequency
 * and then dispatching the raw audio sample to the currently active effect's
 * processing function. This ensures real-time performance regardless of the effect.
 */
ISR(TIMER1_CAPT_vect)
{
    /* Low byte must be fetched first. Read the 10-bit ADC input signal data. */
    ADC_low = ADCL;
    ADC_high = ADCH;
    /* Construct the 10-bit input sample (0-1023) from ADC high and low bytes. */
    input_raw_sample = (ADC_high << 8) | ADCL; // Ensure ADCL is used correctly
    /*Apply master volume control to the raw input sample*/
    volumeControl();
    input_raw_sample = map(input_raw_sample, 0, 1024, 0, pot2_value);
    // Dispatch the input sample to the active effect's audio processing function
    switch (currentActiveMode) {
        case NORMAL_MODE:
            processNormalAudio(input_raw_sample);
            break;
        case REVERB_ECHO_MODE:
        case DELAY_MODE:
            processReverbAudio(input_raw_sample);
            break;
        case ECHO_MODE:
            processEchoAudio(input_raw_sample);
            break;
        case OCTAVER_MODE:
            processOctaverAudio(input_raw_sample);
            break;
        case DISTORTION_MODE:
            processDistortionAudio(input_raw_sample);
            break;
        case SINEWAVE_MODE:
            processSinewaveAudio(input_raw_sample); // Pass raw input, though generator may ignore
            break;
        case CLEAN_MODE: // Explicit CLEAN_MODE selected via effect bypass logic or momentary release
        default:
            // Simple pass-through. Apply master volume here too for consistency.
            int output_val_clean = input_raw_sample;
            // output_val_clean = (int)(output_val_clean * (pot2_value / 1023.0));
            // output_val_clean = constrain(output_val_clean, 0, 1023);
            analogWrite(AUDIO_OUT_A, output_val_clean / 4);
            analogWrite(AUDIO_OUT_B, map(output_val_clean % 4, 0, 3, 0, 255));
            break;
    }
}

/**
 * @brief: Audio processing function for NORMAL mode.
 * This function provides a simple pass-through signal.
 * It is called by the universal ISR (TIMER1_CAPT_vect).
 * @param input_val The raw 10-bit input audio sample (0-1023).
 */
void processNormalAudio(int input_val) {
    // For NORMAL_MODE, we simply pass the signal through and apply volume.
    // No complex DSP or centering needed if it's just bypass with volume.
    int output_val = input_val;

    // Apply master volume controlled by PUSHBUTTON_1/2
    output_val = (int)(output_val * (pot2_value / 1023.0));

    // Constrain the output to the valid 10-bit range (0-1023)
    output_val = constrain(output_val, 0, 1023);

    /* Write the PWM output signal using the 10-bit to dual 8-bit PWM technique */
    analogWrite(AUDIO_OUT_A, output_val / 4); // Coarse 8 bits (MSB)
    analogWrite(AUDIO_OUT_B, map(output_val % 4, 0, 3, 0, 255)); // Fine 2 bits (LSB)
}


/**
 * @brief: Configures common hardware interfaces used by the pedal.
 * This function is called once in setup().
 * Pins A1, A2, A3 (formerly POTs) are now configured as digital inputs with pullups.
 */
void pinConfig (void){
    pinMode(FOOTSWITCH, INPUT_PULLUP);
    pinMode(TOGGLE, INPUT_PULLUP);
    pinMode(PUSHBUTTON_1, INPUT_PULLUP);
    pinMode(PUSHBUTTON_2, INPUT_PULLUP);

    // Configure former POT pins as digital inputs for effect selection
    pinMode(SELECT_NORMAL_BUTTON, INPUT_PULLUP);
    pinMode(SELECT_EFFECT_BUTTON_A2, INPUT_PULLUP);
    pinMode(SELECT_OCTAVER_BUTTON, INPUT_PULLUP);

    pinMode(LED_EFFECT_ON, OUTPUT);
}

/**
 * @brief: Sets up the Analog-to-Digital Converter (ADC) for continuous,
 * auto-triggered conversions for real-time audio input.
 * This function is called once in setup().
 */
void adcSetup(void){
    ADMUX = 0x60;  // ADMUX: Reference (AVcc), Left Adjust Result, ADC0 (A0) selected
    ADCSRA = 0xe5; // ADCSRA: ADC Enable, ADC Start Conversion, ADC Auto Trigger Enable, Prescaler CK/32
    ADCSRB = 0x07; // ADCSRB: ADC Auto Trigger Source: Timer/Counter1 Capture Event
    DIDR0 = 0x01;  // DIDR0: Disable Digital Input Buffer for ADC0 (A0) to reduce noise.
}

/**
 * @brief: Sets up Timer/Counter1 for Pulse Width Modulation (PWM) output.
 * Used for generating the audio output signal on pins AUDIO_OUT_A (9) and AUDIO_OUT_B (10).
 * This function is called once in setup().
 */
void pmwSetup(void){
    // TCCR1A: Configure Waveform Generation Mode (WGM) and Compare Output Mode (COM).
    TCCR1A = (((PWM_QTY - 1) << 5) | 0x80 | (PWM_MODE << 1));

    // TCCR1B: Configure Waveform Generation Mode (WGM) and Clock Select (CS).
    TCCR1B = ((PWM_MODE << 3) | 0x11); // ck/1 prescaler

    // TIMSK1: Timer/Counter1 Interrupt Mask Register.
    TIMSK1 = 0x20; // Enable Input Capture Interrupt (ICIE1) - this is the ISR trigger.

    // ICR1: Input Capture Register. Used as TOP value for PWM frequency.
    ICR1H = (PWM_FREQ >> 8); // High byte of ICR1
    ICR1L = (PWM_FREQ & 0xff); // Low byte of ICR1

    // DDRB: Data Direction Register for Port B. Set pins 9 and 10 as outputs.
    DDRB |= ((PWM_QTY << 1) | 0x02);

    sei(); // Enable global interrupts
}

/**
 * @brief: Periodically checks the volume control push-buttons (PUSHBUTTON_1 and PUSHBUTTON_2).
 * This function is called in the main loop to adjust the global volume (pot2_value).
 * It uses a counter to limit checks to every 100 iterations for efficiency.
 */
void volumeControl(void) {
    // The push-buttons are checked now:
counter++; //to save resources, the push-buttons are checked every 100 times.
if(counter==100)
{
counter=0;
if (!digitalRead(PUSHBUTTON_1)) {
  if (pot2_value<1024) pot2_value=pot2_value+1; //increase the vol
    }
if (!digitalRead(PUSHBUTTON_2)) {
  if (pot2_value>0) pot2_value=pot2_value-1; //decrease vol
    }
}
}

