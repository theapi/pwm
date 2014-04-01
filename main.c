//#define F_CPU 16000000L

// based on http://www.youtube.com/watch?v=ZhIRRyhfhLM

#include <avr/io.h>
#include <avr/interrupt.h>



#define PIN_LED_DEBUG PB5 // The led to blink
#define PIN_LED_FADE  PB3 // The led to fade - pwm (NB: 8 bit timer 2)

#define COMPARE_REG 249 // OCR0A when to interupt (datasheet: 14.9.4)
#define T1 500 // timeout value for the blink (mSec)


/********************************************************************************
Function Prototypes
********************************************************************************/

void initTimer(void);


/********************************************************************************
Global Variables
********************************************************************************/

volatile double dutyCycle = 0;

volatile unsigned int time1;


/********************************************************************************
Interupt Routines
********************************************************************************/

//timer 0 compare ISR
ISR(TIMER0_COMPA_vect)
{
    if (time1 > 0)  --time1;
}

//timer 2 overflow ISR
ISR(TIMER2_OVF_vect)
{
    OCR2A = (dutyCycle/100) * 255;
}

/********************************************************************************
Main
********************************************************************************/
int main (void)
{
    //DDRB = 0xFF; // set all to output
    //PORTB = 0; // all off

    // Set the pins as outputs.
    DDRB = (1 << PIN_LED_DEBUG);

    DDRB = (1 << PIN_LED_FADE);

    PORTB = 0; // all off

    // Fast PWM on timer 2 - 0xFF BOTTOM MAX
    TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20);

    // Timer/Counter2 Overflow Interrupt Enable
    TIMSK2 = (1 << TOIE2);

    OCR2A = (dutyCycle/100) * 255;



    initTimer();

    // crank up the ISRs
    sei();

    // Start the timer (after interupts enabled) - No prescaling
    //TCCR2B = (1 << CS20); // @todo see why this kills timer0

    // main loop
    while(1) {


        if (time1 == 0) {
            // reset the timer
            time1 = T1;
            // toggle the led
            PORTB ^= (1 << PIN_LED_DEBUG);

            // Increase the pwm by 10 %
            dutyCycle += 10;
            if (dutyCycle > 100) {
                dutyCycle = 0;
            }

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
