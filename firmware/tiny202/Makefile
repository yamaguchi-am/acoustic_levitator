CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
MCU = attiny202

NAME = main
OBJ = main.o 

CFLAGS = -Wall -mmcu=$(MCU) -L/usr/lib/ -Os -mcall-prologues -mtiny-stack
LDFLAGS = -Wl,-Map,$(NAME).map


all: $(NAME).hex $(NAME).lst

$(NAME).elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

clean:
	rm -rf *.o $(NAME).elf
	rm -rf *.lst *.map
	rm -rf $(NAME).hex
