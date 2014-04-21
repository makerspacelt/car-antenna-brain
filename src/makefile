SRCS=car_antenna.c
OBJS=$(SRCS:.c=.o)
CC=avr-gcc
PROJECT=car_antenna
TARGET=atmega8
PROGRAMER=usbasp
DEV=/dev/ttyACM0

## The C compiler flags
CFLAGS = -mmcu=$(TARGET) -Wall -fomit-frame-pointer -Os -fshort-enums -g 

## The linker flags. The -Wl,-Map,program.map option says to generate a
## "program map" (i.e.,  what goes where in the AVR memory), while the
## -Wl,--cref option generates a  "cross reference" file which tells you where
## each variable and function lives in  the AVR's memory.
LFLAGS = -Wl,-Map,program.map -Wl,--cref

%.o: %.c
	$(CC) -c $(CFLAGS) -o $*.o $<

all: $(PROJECT).elf
	avr-objcopy -O ihex $(PROJECT).elf $(PROJECT).hex
	ls -lsha $(PROJECT).hex

$(PROJECT).elf: $(OBJS)
	$(CC) $(CFLAGS) $(LFLAGS) -o $(PROJECT).elf $(OBJS)

upload: $(PROJECT).elf
	avr-objcopy -O ihex $(PROJECT).elf $(PROJECT).hex
	avrdude -c $(PROGRAMER) -p $(TARGET) -P $(DEV) -U flash:w:$(PROJECT).hex

clean:
	rm -f program.map *.elf *.cof *.hex *.o

