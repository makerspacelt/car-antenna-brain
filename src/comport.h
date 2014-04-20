#include "avr/io.h"
#include "avr/interrupt.h"

// defines for 8Mhz
#define BAUD_4800 103
#define BAUD_9600 51
#define BAUD_19200 25
#define BAUD_38400 12
#define BAUD_250000 1
#define BAUD_500000 0


void comport_init(uint8_t baud, uint8_t recvint, uint8_t tranint);
uint8_t comport_recv();
void comport_send(uint8_t dt);
void comport_space();
void comport_newline();
void comport_writeln(char *str);
void comport_write(char *str);
void comport_writenum(int num, uint8_t base);



void comport_readln(char *str, uint8_t maxlen, uint8_t echo);

uint8_t comport_handlerx(uint8_t eback);
char *comport_linepending();
void comport_lineconsume();


