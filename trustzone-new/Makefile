CPPFLAGS += -DSMP
CPPFLAGS += -march=armv7-a -marm

LIBFDTOBJS = libfdt/fdt.o libfdt/fdt_ro.o libfdt/fdt_wip.o \
			libfdt/fdt_sw.o libfdt/fdt_rw.o libfdt/fdt_strerror.o
BOOTLOADER = boot.S
# use LIBFDTOBJS and semi_loader.o if you want to load image
#OBJS       = boot.o c_start.o semihosting.o string.o semi_loader.o $(LIBFDTOBJS)
OBJS       = boot.o c_start.o semihosting.o string.o

SEMIIMG    = linux-system-semi.axf
LD_SCRIPT  = model.ld

CC		= $(CROSS_COMPILE)gcc
LD		= $(CROSS_COMPILE)ld
OBJDUMP    = $(CROSS_COMPILE)objdump

export CROSS_COMPILE ARCH

all: $(SEMIIMG)

clean distclean:
	rm -f $(SEMIIMG) $(OBJS) *.asm *.sym

$(SEMIIMG): $(OBJS) $(LD_SCRIPT)
	$(LD) -o $@ $(OBJS) libgcc.a -script=$(LD_SCRIPT)
	$(OBJDUMP) -D linux-system-semi.axf > linux-system-semi.asm
	$(OBJDUMP) -t linux-system-semi.axf | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > linux-system-semi.sym

boot.o: $(BOOTLOADER)
	$(CC) $(CPPFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CPPFLAGS) -O2 -ffreestanding -I. -Ilibfdt -c -o $@ $<

