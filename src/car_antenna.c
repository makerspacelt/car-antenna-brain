#define F_CPU 8000000UL
#define MAX_I 800U

#define FULL_UP 6
#define FULL_DOWN 0
#define UP 7
#define DOWN 2


#include "avr/io.h"
#include "avr/interrupt.h"
//#include <avr/eeprom.h>
#include "stdlib.h"
//#include "string.h"
//#include "testas.h"
#include <util/delay.h>
#include "comport.c"


uint8_t time = 100;
volatile uint16_t done=0;
volatile uint8_t go=0;
volatile uint8_t cc[8] = { 0 };
volatile uint8_t pq[2] = { 0 };
volatile uint8_t transition = 0;

uint8_t a,b,c = 0;

void go_down(void) {
		comport_writeln("Pulling in (down) >>>     ");
		PORTD |= 1<<7;
 		PORTB &= ~(1<<0);
		OCR1A=0xFF;
		go=1;
}
void go_up(void) {
		comport_writeln("Pulling out (up) <<<     ");
 		PORTD &= ~(1<<7);
  		PORTB |= 1<<0;
		OCR1A=0xFF;
		go=1;
}
void stop(void) {
	go=0;
}


ISR(TIMER0_OVF_vect) {
	time++;
	if (time>198) time=100;

	comport_writenum(time,10);
	comport_write(" PWM:");
	comport_writenum(OCR1A,10);
	comport_write(" ADC:");
	comport_writenum(ADCW,10);
	comport_write("     ");
	comport_writenum( (PINC & (0b111<<3))>>3 ,2);
	comport_write("    modes: ");
	comport_writenum(cc[FULL_UP],10);
	comport_write(" ");
	comport_writenum(cc[UP],10);
	comport_write(" -  ");
	comport_writenum(cc[DOWN],10);
	comport_write(" ");
	comport_writenum(cc[FULL_DOWN],10);
	comport_write("    pq: ");
	comport_writenum(pq[1],10);
	comport_write(" ");
	comport_writenum(pq[0],10);

	comport_write("     ");
	comport_send(13);
}
ISR(ADC_vect) {
	// check for counterforce
	if (ADCW > MAX_I-100) {
		done++;
	} else {
		done = 0;
	}
	if (done>0x00ff) {
		stop();
		done=0;
	}
	// adjust PWM to match desired current
	if (go) {
		if (ADCW < MAX_I) {
			if (OCR1A < 255) {
				OCR1A++;
			}
		} else {
			if (OCR1A > 0) {
				OCR1A--;
			}
		}
	} else {
		OCR1A = 0;
	}
	// read buttons
	uint8_t tmp = (PINC & (0b111<<3))>>3;
	for (a=0; a<8; a++) {
		if (tmp==a) {
			if (cc[a] < 255) {
				cc[a]++;
			} else {
				// got seteled combination
				if (pq[0] != a) {
					pq[1] = pq[0];
					pq[0] = a;
					transition = 1;
				}
			}
		} else {
			cc[a] = 0;
		}
	}
}

int main(void) {

	// init comport comunications with no interrupts
	comport_init(BAUD_38400,0,0);

	// init PWM stuff in Timer1
	DDRB |= 1<<1;
	OCR1A = 0x00;
	TCCR1A = 0b10100001; // page 97 
	TCCR1B = 0b00001001; // page 98

	// init Timer1 interupt
	TCCR0 |= (1<<CS00)|(1<<CS02); //prescaler 1/1024
	TIMSK |= (1<<TOIE0); //Timer0 Overflow Interrupt ON

	// init ADC conversion
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); // Enable and set prescaler to 128 
	ADMUX = 0b00000001; // external Vref; ADC1 input
	ADCSRA |= (1<<ADFR);                //Free-running mode
	ADCSRA |= (1<<ADSC);                //Start converting
	ADCSRA |= (1<<ADIE);                //Interupt enabled

	// motor direction outputs
	DDRB |= 1<<0;
	DDRD |= 1<<7;

	// controll inputs
	DDRC &= ~(1<<3);
	DDRC &= ~(1<<4);
	DDRC &= ~(1<<5);

	// enable global interupts
	sei();

	comport_writeln("Automatic Car Antenna by <darius@at.lt>");
	comport_writeln("BOOTED UP!");

	while (1) {

		if (transition) {
comport_writeln("got transition");
			transition = 0;
			if ((pq[1]==2 && pq[0]==6)		// DOWN -> FULL UP
			||  (pq[1]==2 && pq[0]==0)		// DOWN -> FULL DOWN
			||  (pq[1]==7 && pq[0]==6)		// UP -> FULL UP
			||  (pq[1]==7 && pq[0]==0) ) {	// UP -> FULL DOWN
					stop();
			} else
			if ((pq[1]==6 && pq[0]==0) 		// FULL UP -> FULL DOWN
			||  (pq[1]==6 && pq[0]==2)		// FULL UP -> DOWN
			||  (pq[1]==0 && pq[0]==2)		// FULL DOWN -> DOWN
			||  (pq[1]==7 && pq[0]==2) ) {	// UP -> DOWN
					go_down();
			} else 
			if ((pq[1]==6 && pq[0]==7)		// FULL UP -> UP
			||  (pq[1]==0 && pq[0]==6)		// FULL DOWN -> FULL UP
			||  (pq[1]==0 && pq[0]==7)		// FULL DOWN -> UP
			||  (pq[1]==2 && pq[0]==7) ) {	// DOWN -> UP
					go_up();
			}
		}

	}

	return 1;
}



