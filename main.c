//#define F_CPU 16000000L


#include <avr/io.h>
#include <avr/interrupt.h>



#define PIN_LED_DEBUG PB5 // The led to blink (arduino 13)
#define PIN_LED_FADE  PB3 // hardware pwm - (NB: 8 bit timer 2) (arduino 11)
#define PIN_LED_FADE2  PD3 // hardware pwm (arduino 3)
#define PIN_SERVO1  PB0 // (arduino 8)  - TIMER0
#define PIN_SERVO2  PB1 // (arduino 15) - TIMER1

//#define COMPARE_REG 249 // OCR0A when to interupt (datasheet: 14.9.4)
#define MILLIS_TICKS 100  // number of TIMER0_COMPA_vect ISR calls before a millisecond is counted


#define T1 500UL * MILLIS_TICKS
#define T2 10 * MILLIS_TICKS // Speed of one pin's fade
#define T3 10 * MILLIS_TICKS // Speed of the other pin's fade
#define T4 20 * MILLIS_TICKS // Servo period of 20ms

#define T5 1000UL * MILLIS_TICKS // time5 delay

#define SERVO_COMP1 150 // The pulse width (steps) for the first servo
//#define SERVO_COMP1 100 // 1ms pulse fully left
//#define SERVO_COMP1 150 // 1.5ms pulse, centered
//#define SERVO_COMP1 200 // 2ms pulse fully right

/********************************************************************************
Function Prototypes
********************************************************************************/

void initTimer0(void);
void initTimer1(void);
void initPWM(void);
void initADC(void);

/********************************************************************************
Global Variables
********************************************************************************/
unsigned char direction;
unsigned char width;
unsigned char directionB = 1;
unsigned char widthB = 254;

unsigned char directionSweep;

volatile uint32_t time1;
volatile unsigned int time2;
volatile unsigned int time3;

volatile unsigned int time4;
volatile unsigned int step;
volatile unsigned int servo_compare1;
volatile unsigned long time5;

enum servo2_states {SV2_LEFT, SV2_MID, SV2_RIGHT};
enum servo2_states servo2_state = SV2_MID;

/********************************************************************************
Interupt Routines
********************************************************************************/

//timer 0 compare A ISR
ISR(TIMER0_COMPA_vect)
{
    // 0.01ms = 100 steps (per ms)


    if (time1 > 0)  --time1;
    if (time2 > 0)  --time2;
    if (time3 > 0)  --time3;
    if (time5 > 0)  --time5;

    if (time4 == 0) {
        // reset the timer
        time4 = T4;
        step = 0;

        // Start of the 20ms period so start the pulse.
        PORTB |= (1 << PIN_SERVO1); // on
    } else {
        if (step == servo_compare1)  {
            // Done enough steps, so turn off untill the next period starts.
            PORTB &= ~(1 << PIN_SERVO1); // off
        }
        ++step;
        --time4;
    }


}

//timer 0 compare B ISR
ISR(TIMER0_COMPB_vect)
{
    //Called with TIMER0_COMPA_vect!! so no use.

}

/********************************************************************************
Main
********************************************************************************/
int main (void)
{

    // Pins to output
    DDRB = (1 << PIN_LED_FADE) | (1 << PIN_LED_DEBUG) | (1 << PIN_SERVO1);
    //DDRB = 0xFF; // all output
    PORTB = 0; // all off

    DDRD |= (1 << PIN_LED_FADE2); // output
    PORTD &= (1 << PIN_LED_FADE2); // high

    initTimer0();
    initTimer1();
    initPWM();
    initADC();

    uint8_t servo1_max = 255;
    uint8_t servo1_min = 65;

    // crank up the ISRs
    sei();



    // main loop
    while(1) {


        if (time1 == 0) {
            // reset the timer
            time1 = T1;
            // toggle the led
            PORTB ^= (1 << PIN_LED_DEBUG);
        }


        // Read ADC value of the pot when the conversion has completed.
        if (ADCSRA & ~(1 << ADSC)) {
            uint8_t adc_a = ADCH;
            if (adc_a > servo1_max) {
                servo_compare1 = servo1_max;
            } else if (adc_a < servo1_min) {
                servo_compare1 = servo1_min;
            } else {
                servo_compare1 = adc_a;
            }
            // start another adc reading
            ADCSRA |= (1 << ADSC);
        }


        if (time5 == 0) {
            // reset the timer
            time5 = T5;
            if (servo2_state == SV2_LEFT) {
                OCR1A = 4000;
                servo2_state = SV2_RIGHT;
            } else if (servo2_state == SV2_RIGHT) {
                OCR1A = 3000;
                servo2_state = SV2_MID;
            } else if (servo2_state == SV2_MID) {
                OCR1A = 2000;
                servo2_state = SV2_LEFT;
            }
        }

/*
        // Sweep
        if (time5 == 0) {
            // reset the timer
            time5 = T5;

            // Change the position
            if (directionSweep) {
                servo_compare1--;
            } else {
                servo_compare1++;
            }

            if (servo_compare1 > 250) {
                directionSweep = 1;
            } else if (servo_compare1 < 60) {
                directionSweep = 0;
            }

        }
*/

        // A
        if (time2 == 0) {
            // reset the timer
            time2 = T2;

            // Change the pwm
            if (direction) {
                width--;
            } else {
                width++;
            }

            if (width > 254) {
                direction = 1;
            } else if (width == 0) {
                direction = 0;
            }

            OCR2A = width;
        }

        // B
        if (time3 == 0) {
            // reset the timer
            time3 = T3;

            // Change the pwm
            if (directionB) {
                widthB--;
            } else {
                widthB++;
            }

            if (widthB > 254) {
                directionB = 1;
            } else if (widthB == 0) {
                directionB = 0;
            }

            OCR2B = widthB;
        }

    }
}


/********************************************************************************
Functions
********************************************************************************/


void initTimer0(void)
{
    // set up timer 0 for 1 mSec ticks (timer 0 is an 8 bit timer)

    // Interupt mask register - to enable the interupt (datasheet: 14.9.6)
    // (Bit 1 - OCIE0A: Timer/Counter0 Output Compare Match A Interrupt Enable)
    TIMSK0 = (1 << OCIE0A); // (2) turn on timer 0 compare match ISR

    // Compare register - when to interupt (datasheet: 14.9.4)
    // OCR0A = 249; // set the compare reg to 250 time ticks = 1ms
    //OCR0A = 124; // 0.5 ms
    //OCR0A = 50; // 1/5 of 1ms = 0.0002s
    //OCR0A = 30; // 0.00012s = 0.12ms
    //OCR0A = 25; // 0.0001s = 0.1ms
    //OCR0A = COMPARE_REG

    /*
    Prescaler of 8
    1/(16000000 / 8 / 20)

    16MHz/8 = 2MHz
    0.0000005 = 0.0005ms precision



    OCR0A 250 = 0.000125 seconds  = 0.125ms
    OCR0A 200 = 0.0001 seconds    = 0.1ms    = 10 steps
    OCR0A 100 = 0.00005 seconds   = 0.05ms   = 20 steps
    OCR0A 50  = 0.000025 seconds  = 0.025ms  = 40 steps
    OCR0A 25  = 0.0000125 seconds = 0.0125ms = 80 steps
    OCR0A 20  = 0.00001 seconds   = 0.01ms   = 100 steps (per ms)
    OCR0A 10  = 0.000005 seconds  = 0.005ms  = 200 steps (per ms)

    for a 180 degree servo ideally there are 180 steps in 1ms
    since the pulse widths need to be between 1ms & 2 ms for every 20ms

     */

    //OCR0A = 200; // 0.1ms on a prescaler of 8
    OCR0A = 20; // 0.01ms   = 100 steps (per ms) - cannot get 10/0.005ms to work - too slow.
    //OCR0B = 10; // 0.005ms  = 200 steps - DAMN - CTC only uses OCROA - back to the drawing board...

    // Timer mode (datasheet: 14.9.1)
    TCCR0A = (1 << WGM01); // (0b00000010) turn on clear-on-match

    // Prescaler (datasheet: 14.9.2)
    // 16MHz/64=250kHz so precision of 0.000004 = 4us
    // calculation to show why 64 is required the prescaler:
    // 1 / (16000000 / 64 / 250) = 0.001 = 1ms
    //TCCR0B = ((1 << CS01) | (1 << CS00)); // (0b00000011)(3) clock prescalar to 64
    TCCR0B = ((1 << CS01)); // (0b00000011)(3) clock prescalar to 8

    // Timer initialization
    time1 = T1;
    time4 = T4;
    time5 = T5;

    servo_compare1 = SERVO_COMP1;

}

// Servo PWM on timer 1
void initTimer1(void)
{
    // see http://eliaselectronics.com/atmega-servo-tutorial/
    TCCR1A |= (1 << COM1A1) | (1 << WGM11); // non-inverting mode for OC1A
    TCCR1B |= (1 << WGM13) | (1 << WGM12) | (1 << CS11); // Mode 14, Prescaler 8

    ICR1 = 40000; // 320000 / 8 = 40000

    DDRB |= (1 << PIN_SERVO2); // OC1A set to output
}

void initPWM(void)
{

    // Fast PWM on timer 2 - 0xFF BOTTOM MAX - pin A & B
    TCCR2A = (1 << COM2A1) | (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);

    // Timer/Counter2 Overflow Interrupt Enable
    //TIMSK2 = (1 << TOIE2);

    // 64 prescaler
    TCCR2B = (1 << CS22);

    time2 = T2;
    time3 = T3;

    OCR2A = width;
    OCR2B = widthB;
}

void initADC(void)
{
    // Datasheet 23.9.1
    // ADC (AVCC ref) | (8 bit so only read ADCH) + (Input on A0)
    ADMUX = (1 << REFS0) | (1 << ADLAR); // ob01100000

    // Datasheet 23.9.2
    //       (on) | (start a conversion) | (prescaler 128)
    ADCSRA = (1 << ADEN) | (1 << ADSC) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}
