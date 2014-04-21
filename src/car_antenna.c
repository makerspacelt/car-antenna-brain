#define F_CPU 8000000UL
#define MAX_I 800U

#define AUTO_UP     0b110
#define AUTO_DOWN   0b000
#define MANUAL_UP   0b111
#define MANUAL_DOWN 0b010

#define PWM OCR1A
#define ADC ADCW

#include "avr/io.h"
#include "avr/interrupt.h"
#include "stdlib.h"
#include <util/delay.h>
#include "comport.c"


uint8_t time = 100;

volatile uint16_t startup_current_counter = 0;
volatile uint8_t can_move_motor = 0;

volatile uint8_t button_debounce_counter[8] = { 0 };
volatile uint8_t button_state_cutrrent = 0;
volatile uint8_t button_state_last = 0;
volatile uint8_t is_in_transition = 0;



void pullAntennaDown(void) {
	comport_writeln("Pulling antenna down <<<     ");
	PORTD |= 1<<7;
	PORTB &= ~(1<<0);
	PWM = 0xFF;
	can_move_motor = 1;
}

void pushAntennaUp(void) {
	comport_writeln("Pushing antenna up >>>     ");
	PORTD &= ~(1<<7);
	PORTB |= 1<<0;
	PWM = 0xFF;
	can_move_motor = 1;
}

void stopMotor(void) {
	comport_writeln("Stopping the motor ---     ");
	can_move_motor = 0;
}

void printStatus(void) {
	time++;
	if (time>198) time=100;

	comport_writenum(time,10);
	comport_write(" PWM:");
	comport_writenum(PWM,10);
	comport_write(" ADC:");
	comport_writenum(ADC,10);
	comport_write("     ");
	comport_writenum( (PINC & (0b111<<3))>>3 ,2);
	comport_write("    modes: ");
	comport_writenum(button_debounce_counter[AUTO_UP],10);
	comport_write(" ");
	comport_writenum(button_debounce_counter[MANUAL_UP],10);
	comport_write(" -  ");
	comport_writenum(button_debounce_counter[MANUAL_DOWN],10);
	comport_write(" ");
	comport_writenum(button_debounce_counter[AUTO_DOWN],10);
	comport_write("    button states, last: ");
	comport_writenum(button_state_last,10);
	comport_write(" , current: ");
	comport_writenum(button_state_cutrrent,10);

	comport_write("     ");
	comport_send(13);
}

ISR(ADC_vect) {

	// check for counterforce
	if (ADC > MAX_I-100) {
		startup_current_counter++;
	} else {
		startup_current_counter = 0;
	}
	if (startup_current_counter > 0x00ff) {
		stopMotor();
		startup_current_counter = 0;
	}
	// adjust PWM to match desired current
	if (can_move_motor) {
		if (ADC < MAX_I) {
			if (PWM < 255) {
				PWM++;
			}
		} else {
			if (PWM > 0) {
				PWM--;
			}
		}
	} else {
		PWM = 0;
	}
	// read buttons (PC3 PC4 PC5)
	uint8_t tmp = (PINC & (0b111<<3))>>3;
	uint8_t i = 0;
	for (i=0; i<8; i++) {
		if (tmp==i) {
			if (button_debounce_counter[i] < 255) {
				button_debounce_counter[i]++;
			} else {
				// got seteled combination
				if (button_state_cutrrent != i) {
					button_state_last = button_state_cutrrent;
					button_state_cutrrent = i;
					is_in_transition = 1;
				}
			}
		} else {
			button_debounce_counter[i] = 0;
		}
	}
}

void setup() {
	// init comport communications with no interrupts
	comport_init(BAUD_9600,0,0);

	// init PWM stuff in Timer1
	DDRB |= 1<<1;             // setting PWM pin as output
	PWM = 0x00;               // setting PWM to 0
	TCCR1A = 0b10100001;      // page 97 
	TCCR1B = 0b00001001;      // page 98

	// init ADC conversion
	ADMUX = 0b00000001;       // external Vref; ADC1 input
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); // Enable and set prescaler to 128 
	ADCSRA |= (1<<ADFR);      // Free-running mode
	ADCSRA |= (1<<ADSC);      // Start converting
	ADCSRA |= (1<<ADIE);      // Interrupt enabled

	// motor direction outputs (PB0 and PD7)
	DDRB |= 1<<0;             // Antenna direction DOWN
	DDRD |= 1<<7;             // Antenna direction UP

	// control inputs (PC3 PC4 PC5)
	DDRC &= ~(1<<3);
	DDRC &= ~(1<<4);
	DDRC &= ~(1<<5);

	comport_writeln("Automatic Car Antenna by <darius@at.lt>");
	comport_writeln("BOOTED UP!");

	// enable global interrupts
	sei();
}

int main(void) {

	setup();

	while (1) {
		if (is_in_transition) {
			comport_writeln("\ngot transition");
			is_in_transition = 0;

			if ((button_state_last==MANUAL_DOWN && button_state_cutrrent==AUTO_UP)
			||  (button_state_last==MANUAL_DOWN && button_state_cutrrent==AUTO_DOWN)
			||  (button_state_last==MANUAL_UP   && button_state_cutrrent==AUTO_UP)
			||  (button_state_last==MANUAL_UP   && button_state_cutrrent==AUTO_DOWN)
			) {
					stopMotor();
			} else
			if ((button_state_last==AUTO_UP   && button_state_cutrrent==AUTO_DOWN)
			||  (button_state_last==AUTO_UP   && button_state_cutrrent==MANUAL_DOWN)
			||  (button_state_last==AUTO_DOWN && button_state_cutrrent==MANUAL_DOWN)
			||  (button_state_last==MANUAL_UP && button_state_cutrrent==MANUAL_DOWN)
			) {
					pullAntennaDown();
			} else 
			if ((button_state_last==AUTO_UP     && button_state_cutrrent==MANUAL_UP)
			||  (button_state_last==AUTO_DOWN   && button_state_cutrrent==AUTO_UP)
			||  (button_state_last==AUTO_DOWN   && button_state_cutrrent==MANUAL_UP)
			||  (button_state_last==MANUAL_DOWN && button_state_cutrrent==MANUAL_UP)
			) {
					pushAntennaUp();
			}
		} else {
			printStatus();
		}
	}
	return 1;
}



