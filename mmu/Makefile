CROSS_PREFIX = arm-linux-gnueabihf-

AS = $(CROSS_PREFIX)as
CC = $(CROSS_PREFIX)gcc
LD = $(CROSS_PREFIX)ld
OBJDUMP = $(CROSS_PREFIX)objdump

CFLAGS += -DSMP
CFLAGS += -march=armv7-a -marm

OBJS = \
	entry.o \
	start.o \
	main.o \
	uart.o \
	asm.o

all: kernel.elf

kernel.elf: $(OBJS) kernel.ld
	$(LD) $(OBJS) libgcc.a -T kernel.ld -o kernel.elf
	$(OBJDUMP) -D kernel.elf > kernel.asm
	$(OBJDUMP) -t kernel.elf | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel.sym

#entry.o: entry.S
#	$(CC) $(CFLAGS) -c -o entry.o $<

%.o: %.c
	$(CC) $(CFLAGS) -O2 -ffreestanding -c -o $@ $<
	
clean distclean:
	rm -f $(OBJS) kernel.elf kernel.asm kernel.sym


