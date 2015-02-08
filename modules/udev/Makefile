obj-m += udev.o

all:
	make -C /home/zhang/kvm/linux-kvm-arm/ M=$(PWD) modules
	arm-none-linux-gnueabi-gcc read.c -o read

clean:
	make -C /home/zhang/kvm/linux-kvm-arm/ M=$(PWD) clean
	rm -rf read
