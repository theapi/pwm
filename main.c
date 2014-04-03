//#define F_CPU 16000000L


#include <avr/io.h>
#include <avr/interrupt.h>



#define PIN_LED_DEBUG PB5 // The led to blink
#define PIN_LED_FADE  PB3 // The led to fade - pwm (NB: 8 bit timer 2)
#define PIN_LED_FADE2  PD3


#define COMPARE_REG 249 // OCR0A when to interupt (datasheet: 14.9.4)
#define MILLIS_TICKS 10  // number of TIMER0_COMPA_vect ISR calls before a millisecond is counted


#define T1 1000 * MILLIS_TICKS
#define T2 10 * MILLIS_TICKS // Speed of one pin's fade
#define T3 10 * MILLIS_TICKS // Speed of the other pin's fade
#define T4 1000 * MILLIS_TICKS

#define SERVO_COMP1 1 // The max steps for the first servo

/********************************************************************************
Function Prototypes
********************************************************************************/

void initTimer(void);
void initPwm(void);

/********************************************************************************
Global Variables
********************************************************************************/
unsigned char direction;
unsigned char width;
unsigned char directionB = 1;
unsigned char widthB = 254;

volatile unsigned int time1;
volatile unsigned int time2;
volatile unsigned int time3;

volatile unsigned int time4;
volatile unsigned int step;
volatile unsigned int servoCompare1;

/********************************************************************************
Interupt Routines
********************************************************************************/

//timer 0 compare A ISR
ISR(TIMER0_COMPA_vect)
{
    // called every 0.1ms

    if (time1 > 0)  --time1;
    if (time2 > 0)  --time2;
    if (time3 > 0)  --time3;



    if (time1 == 0) {
        // reset the timer
        time1 = T1;
        PORTB ^= (1 << PIN_LED_DEBUG); // toggle
        //PORTB |= (1 << PIN_LED_DEBUG); // on
    }

}

//timer 0 compare B ISR
ISR(TIMER0_COMPB_vect)
{
    // called every 0.005ms - NOPE
    //Called with TIMER0_COMPA_vect!! so no use.

    /*
    if (time4 > 0)  --time4;

    if (time4 == 0) {
        PORTB ^= (1 << PIN_LED_DEBUG); // toggle
        //PORTB &= (0 << PIN_LED_DEBUG); // off
        time4 = T4;
    }
*/

}

//timer 2 overflow ISR
ISR(TIMER2_OVF_vect)
{
    // This is the pwm interupt.
    // Needs to be defined even if it has no code in it.
}

/********************************************************************************
Main
********************************************************************************/
int main (void)
{

    // Pins to output
    DDRB = (1 << PIN_LED_FADE) | (1 << PIN_LED_DEBUG);
    PORTB = 0; // all off

    DDRD |= (1 << PIN_LED_FADE2); // output
    PORTD &= (0 << PIN_LED_FADE2); // LOW


    initTimer();
    initPwm();

    // crank up the ISRs
    sei();

    // main loop
    while(1) {

        /*
        if (time1 == 0) {
            // reset the timer
            time1 = T1;
            // toggle the led
            //PORTB ^= (1 << PIN_LED_DEBUG);
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


void initTimer(void)
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


    OCR0A = 200; // 0.1ms on a prescaler of 8
    //OCR0B = 10; // 0.005ms  = 200 steps - DAMN - CTC only uese OCROA - back to the drawing board...

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


    //servoCompare1 = SERVO_COMP1;

    /*
    Prescaler of 8
    1/(16000000 / 8 / 10)

    16MHz/8 = 2MHz
    0.0000005 = 0.5us precision
    OCR0A 250 = 0.000125 seconds  = 0.125ms
    OCR0A 200 = 0.0001 seconds    = 0.1ms    = 10 steps
    OCR0A 100 = 0.00005 seconds   = 0.05ms   = 20 steps
    OCR0A 50  = 0.000025 seconds  = 0.025ms  = 40 steps
    OCR0A 25  = 0.0000125 seconds = 0.0125ms = 80 steps
    OCR0A 10  = 0.000005 seconds  = 0.005ms  = 200 steps

    for a 180 degree servo ideally there are 180 steps in 1ms
    since the pulse widths need to be between 1ms & 2 ms for every 20ms

     */
}

void initPwm(void)
{

    // Fast PWM on timer 2 - 0xFF BOTTOM MAX - pin A & B
    TCCR2A = (1 << COM2A1) | (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);

    // Timer/Counter2 Overflow Interrupt Enable
    TIMSK2 = (1 << TOIE2);

    // 64 prescaler
    TCCR2B = (1 << CS22);

    time2 = T2;
    time3 = T3;

    OCR2A = width;
    OCR2B = widthB;
}
