#include "main.h"
#include "reverb.h"
#include "echo.h"
#include "octaver.h"
#include "distortion.h"
#include "sinewave.h"

int input_sample;
uint8_t ADC_low, ADC_high;

uint16_t delayBuffer[MAX_DELAY]; 
uint32_t delayWritePointer = 0;
uint32_t delayReadOffset;
uint32_t delayDepth; 

/*Initialized to a mid-range value (0-1023)*/
volatile int pot2_value = 512; // 

/*Global effect ON/OFF state (controlled by FOOTSWITCH)*/
volatile bool effectActive = false; 

/* Debouncing Variables - Universal for all buttons */
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

/*Universal current effect mode, starts in NORMAL_MODE by default*/
volatile EffectMode currentActiveMode = NORMAL_MODE;

void setup() {
    Serial.begin(9600);

    /* Configure core hardware for audio input/output*/
    pinConfig(); 
    adcSetup();  
    pmwSetup();  

    /*Initialize effect-specific components based on selected effect*/
    #ifdef REVERB
    setUpReverb(); 
    currentActiveMode = REVERB_MODE; 
    #elif defined(ECHO)
    setupEcho();
    currentActiveMode = ECHO_MODE; 
    #elif defined(OCTAVER)
    setupOctaver(); 
    currentActiveMode = OCTAVER_MODE; 
    #elif defined(DISTORTION)
    setupDistortion();
    currentActiveMode = DISTORTION_MODE; 
    #elif defined(SINEWAVE) 
    setupSinewave();
    currentActiveMode = SINEWAVE_MODE; 
    #else
    currentActiveMode = NORMAL_MODE;
    #endif

    Serial.println("Arduino Audio Pedal Ready!");
}

void loop() {
    
    /*LED OFF when FOOTSWITCH is NOT pressed (HIGH), ON when pressed (LOW)*/
    bool footswitchState = digitalRead(FOOTSWITCH);
    effectActive = (footswitchState == LOW); // effectActive true if footswitch is pressed (LOW), false if not pressed (HIGH)
    digitalWrite(LED_EFFECT_ON, effectActive ? HIGH : LOW); 

    /*Volume Control via PUSHBUTTON_1 and PUSHBUTTON_2. Controls 'pot2_value' globally, which acts as master volume*/
    if (digitalRead(PUSHBUTTON_1) == LOW) { 
        if (millis() - lastPushButton1PressTime > DEBOUNCE_DELAY_MS) {
            lastPushButton1PressTime = millis();
            /*Ensure volume doesn't exceed max 10-bit value*/
            if (pot2_value < 1023) { // 
                pot2_value = constrain(pot2_value + 20, 0, 1023); //Increase volume
                Serial.print("Volume: "); Serial.println(pot2_value);
            }
        }
    }

    if (digitalRead(PUSHBUTTON_2) == LOW) { 
        if (millis() - lastPushButton2PressTime > DEBOUNCE_DELAY_MS) {
            lastPushButton2PressTime = millis();
            /* Ensure volume doesn't go below min*/
            if (pot2_value > 0) { //
                pot2_value = constrain(pot2_value - 20, 0, 1023); // Decrease volume
                Serial.print("Volume: "); Serial.println(pot2_value);
            }
        }
    }

    /*Effect Mode Selection via digital buttons. Selects NORMAL_MODE*/
    if (digitalRead(SELECT_NORMAL_BUTTON) == LOW) { // Active LOW
        if (millis() - lastSelectNormalPressTime > DEBOUNCE_DELAY_MS) {
            lastSelectNormalPressTime = millis();
            if (currentActiveMode != NORMAL_MODE) {
                currentActiveMode = NORMAL_MODE;
                Serial.println("Mode Selected: NORMAL");
                for (int i = 0; i < MAX_DELAY; i++) {
                    delayBuffer[i] = 0;
                }
                delayWritePointer = 0;
            }
        }
    }

    /*Selects REVERB_MODE, ECHO_MODE, DISTORTION_MODE, OR SINEWAVE_MODE*/
    if (digitalRead(SELECT_EFFECT_BUTTON_A2) == LOW) { 
        if (millis() - lastSelectEffectButtonA2PressTime > DEBOUNCE_DELAY_MS) {
            lastSelectEffectButtonA2PressTime = millis();
            /*Button's behavior depends on which effect is defined at compile time*/
            #ifdef REVERB
            if (currentActiveMode != REVERB_MODE && currentActiveMode != DELAY_MODE) {
                currentActiveMode = REVERB_ECHO_MODE; // Default to REVERB_MODE
                Serial.println("Mode Selected: REVERB (Echo)");
                for (int i = 0; i < MAX_DELAY; i++) {
                    delayBuffer[i] = 0;
                }
                delayWritePointer = 0;
            }
            #elif defined(ECHO)
            if (currentActiveMode != ECHO_MODE) {
                currentActiveMode = ECHO_MODE;
                Serial.println("Mode Selected: ECHO");
                for (int i = 0; i < MAX_DELAY; i++) {
                    delayBuffer[i] = 0;
                }
                delayWritePointer = 0;
            }
            #elif defined(DISTORTION)
            if (currentActiveMode != DISTORTION_MODE) {
                currentActiveMode = DISTORTION_MODE;
                Serial.println("Mode Selected: DISTORTION");
                for (int i = 0; i < MAX_DELAY; i++) { 
                    delayBuffer[i] = 0;
                }
                delayWritePointer = 0;
            }
            #elif defined(SINEWAVE) 
            if (currentActiveMode != SINEWAVE_MODE) {
                currentActiveMode = SINEWAVE_MODE;
                Serial.println("Mode Selected: SINEWAVE");.
                for (int i = 0; i < MAX_DELAY; i++) {
                    delayBuffer[i] = 0;
                }
                delayWritePointer = 0;
            }
            #endif
        }
    }

    /*Selects OCTAVER_MODE*/
    if (digitalRead(SELECT_OCTAVER_BUTTON) == LOW) { 
        if (millis() - lastSelectOctaverPressTime > DEBOUNCE_DELAY_MS) {
            lastSelectOctaverPressTime = millis();
            // Only switch if OCTAVER is enabled for compilation
            #ifdef OCTAVER
            if (currentActiveMode != OCTAVER_MODE) {
                currentActiveMode = OCTAVER_MODE;
                Serial.println("Mode Selected: OCTAVER");
                for (int i = 0; i < MAX_DELAY; i++) { // Octaver might use a buffer, so clear for safety
                    delayBuffer[i] = 0;
                }
                delayWritePointer = 0;
            }
            #endif
        }
    }

    /*Effect-specific loop functions*/
    #ifdef REVERB
    loopReverb(); 
    #elif defined(ECHO)
    loopEcho();
    #elif defined(OCTAVER)
    loopOctaver(); 
    #elif defined(DISTORTION)
    loopDistortion();
    #elif defined(SINEWAVE)
    loopSinewave();
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
    input_sample = (ADC_high << 8) | ADC_low;

    // Dispatch the input sample to the active effect's audio processing function
    switch (currentActiveMode) {
        case NORMAL_MODE:
            processNormalAudio(input_sample);
            break;
            /*Both REVERB_ECHO_MODE and DELAY_MODE use processReverbAudio*/
        case REVERB_MODE: // 
        case DELAY_MODE:       
            processReverbAudio(input_sample);
            break;
        case ECHO_MODE:
            processEchoAudio(input_sample);
            break;
        case OCTAVER_MODE:
            processOctaverAudio(input_sample);
            break;
        case DISTORTION_MODE:
            processDistortionAudio(input_sample);
            break;
        case SINEWAVE_MODE: 
            processSinewaveAudio();
            break;
        case CLEAN_MODE: 
        default:
            int output_val_default = constrain(input_sample, 0, 1023);
            analogWrite(AUDIO_OUT_A, output_val_default / 4); // Coarse 8 bits
            analogWrite(AUDIO_OUT_B, map(output_val_default % 4, 0, 3, 0, 255)); // Fine 2 bits
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
    // This directly sets DDRB |= (1<<PB1) | (1<<PB2) for pins 9 and 10.
    DDRB |= ((PWM_QTY << 1) | 0x02);

    sei(); // Enable global interrupts (also done by Arduino's init, but explicit is clear)
}
