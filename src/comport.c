#include <avr/io.h>
#include <avr/interrupt.h>

#include <string.h>
#include <stdio.h>

#include "comport.h"


#define COMBUFF_COUNT 2
#define COMBUFF_SIZE 20

struct {
	char buf[COMBUFF_COUNT][COMBUFF_SIZE];
	uint8_t nbuffs;
} combuff;

/**
 * Always uses 8n2 format
 * @param recvint if 1 receive interrupt is enabled
 * @param tranint if 1 transmit interrupt is enabled
 */
void comport_init(uint8_t baud, uint8_t recvint, uint8_t tranint)
{
	UBRRH = (uint8_t)(baud>>8);
	UBRRL = (uint8_t)baud;

	/* Enable receiver and transmitter */
	UCSRB = (1<<RXEN) | (1<<TXEN) | (recvint<<RXCIE) | (tranint<<TXCIE);

	/* Set frame format: 8data, 2stop bit */
	UCSRC = (1<<URSEL) | (1<<USBS) | (3<<UCSZ0);

	memset(&combuff, 0, sizeof(combuff));
}

uint8_t comport_recv()
{
	// Wait for data to be received
	while ( !(UCSRA & (1<<RXC)) );
	// Get and return received data from buffer
	return UDR;
}

/*!
 * Sends single char/byte
 */
void comport_send(uint8_t dt)
{
	// Wait for data to be received
	while (!(UCSRA & (1<<UDRE)));
	UDR = dt;
	// Wait for transmit to be completed
	while (!(UCSRA & (1<<TXC)));
}

void comport_space()
{
	comport_send(' ');
}

void comport_newline()
{
	comport_send('\n');
	comport_send('\r');
}

/*!
 * Writes number as specified base string
 */
void comport_writenum(int num, uint8_t base)
{
	char buf[33];
	itoa(num, buf, base);
	comport_write(buf);
}

void comport_write(char *str)
{
	uint8_t i = 0;
	while (str[i] != 0) {
		comport_send(str[i]);
		i++;
	}
}

void comport_writeln(char *str)
{
	comport_write(str);
	comport_newline();
}


/**
 * Reads one line from COM port.
 * (line end is marked by carriege return (13) symbol
 * @param echo if 1, echos back every symbol read
 */
void comport_readln(char *str, uint8_t maxlen, uint8_t echo)
{
	uint8_t i = 0;
	uint8_t symb;
	while (i < maxlen - 1) {
		symb = comport_recv();
		if (symb == '\r') break;
		
		if (echo)
			comport_send(symb);

		str[i] = symb;
		i++;
	}

	str[i] = 0;
}

/*!
 * Handle COM receive, places char into buffer,
 * if carriege return is found - marks new available command
 * @return symbol read
 */
uint8_t comport_handlerx(uint8_t eback)
{
	uint8_t cudr = UDR;

	if (combuff.nbuffs >= COMBUFF_COUNT) return cudr;
	if (eback)
		UDR = cudr;

	if (cudr == '\r') {
		combuff.nbuffs++;
		return cudr;
	}

	if (cudr < 32 || cudr > 122) {
		return cudr;
	}

	uint8_t ub = strlen(combuff.buf[combuff.nbuffs]);
	
	if (ub < COMBUFF_SIZE - 2) {
		combuff.buf[combuff.nbuffs][ub] = cudr;
		combuff.buf[combuff.nbuffs][ub+1] = 0;
	} else {
		combuff.nbuffs++;
	}

	return cudr;
}


/*!
 * Checks if there is new line in the COM buffer
 * @return pointer to buffer line, NULL if no new line
 */
char *comport_linepending()
{
	if (combuff.nbuffs < 1) return NULL;
	return combuff.buf[0];
}

/*!
 * Notify that pending line was consumed
 * comport_linepending pointer becomes invalid after this call
 */
void comport_lineconsume()
{
	if (combuff.nbuffs < 1) return;
	
	uint8_t i;
	for (i = 0; i < combuff.nbuffs - 1; i++)
		strncpy(combuff.buf[i], combuff.buf[i+1], COMBUFF_SIZE);

	combuff.buf[i][0] = 0;
	combuff.nbuffs--;
}



