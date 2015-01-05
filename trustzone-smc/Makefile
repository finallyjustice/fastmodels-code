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

export CROSS_COMPILE ARCH

all: $(SEMIIMG)

clean distclean:
	rm -f $(SEMIIMG) $(OBJS)

$(SEMIIMG): $(OBJS) $(LD_SCRIPT)
	$(LD) -o $@ $(OBJS) --script=$(LD_SCRIPT)

boot.o: $(BOOTLOADER)
	$(CC) $(CPPFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CPPFLAGS) -O2 -ffreestanding -I. -Ilibfdt -c -o $@ $<

