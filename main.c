//#define F_CPU 16000000L


#include <avr/io.h>
#include <avr/interrupt.h>



#define PIN_LED_DEBUG PB5 // The led to blink
#define PIN_LED_FADE  PB3 // The led to fade - pwm (NB: 8 bit timer 2)
#define PIN_LED_FADE2  PD3

#define COMPARE_REG 249 // OCR0A when to interupt (datasheet: 14.9.4)
#define T1 1000 // timeout value for the blink (mSec)
#define T2 10 // Speed of one pin's fade
#define T3 10 // Speed of the other pin's fade

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

/********************************************************************************
Interupt Routines
********************************************************************************/

//timer 0 compare ISR
ISR(TIMER0_COMPA_vect)
{
    if (time1 > 0)  --time1;
    if (time2 > 0)  --time2;
    if (time3 > 0)  --time3;
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
        if (time1 == 0) {
            // reset the timer
            time1 = T1;
            // toggle the led
            PORTB ^= (1 << PIN_LED_DEBUG);
        }

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
    TIMSK0 = (1 << OCIE0A); // (2) turn on timer 0 cmp match ISR

    // Compare register - when to interupt (datasheet: 14.9.4)
    // OCR0A = 249; // set the compare reg to 250 time ticks = 1ms
    //OCR0A = 124; // 0.5 ms
    //OCR0A = 50; // 1/5 of 1ms = 0.0002s
    //OCR0A = 30; // 0.00012s = 0.12ms
    //OCR0A = 25; // 0.0001s = 0.1ms
    OCR0A = COMPARE_REG;

    // Timer mode (datasheet: 14.9.1)
    TCCR0A = (1 << WGM01); // (0b00000010) turn on clear-on-match

    // Prescaler (datasheet: 14.9.2)
    // 16MHz/64=250kHz so precision of 0.000004 = 4us
    // calculation to show why 64 is required the prescaler:
    // 1 / (16000000 / 64 / 250) = 0.001 = 1ms
    TCCR0B = ((1 << CS10) | (1 << CS11)); // (0b00000011)(3) clock prescalar to 64

    // Timer initialization
    time1 = T1;
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
