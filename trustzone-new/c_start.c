#include "semihosting.h"

#define __REG(x)    (*((volatile unsigned int *)(x)))
#define UART0           0x1c090000

void uart_send(const char c)
{
	__REG(UART0) = c;
	if(c == '\n')
		__REG(UART0) = '\r';
}

void printint(int xx, int base, int sign)
{
	static char digits[] = "0123456789abcdef";
	char buf[16];
	int i;
	unsigned int x;

	if(sign && (sign = xx < 0))
		x = -xx;
	else
		x = xx;

	i = 0;
	do{
		buf[i++] = digits[x % base];
	}while((x /= base) != 0);

	if(sign)
		buf[i++] = '-';

	while(--i >= 0)
		uart_send(buf[i]);
}

void cprintf(char *fmt, ...)
{
	int i, c;
	unsigned int *argp;
	char *s;

	argp = (unsigned int*)(void*)(&fmt + 1);
	for(i = 0; (c = fmt[i] & 0xff) != 0; i++)
	{
		if(c != '%')
		{
			uart_send(c);
			continue;
		}
		c = fmt[++i] & 0xff;
		if(c == 0)
			break;
		switch(c)
		{
		case 'd':
			printint(*argp++, 10, 1);
			break;
		case 'x':
		case 'p':
			printint(*argp++, 16, 0);
			break;
		case 's':
			if((s = (char*)*argp++) == 0)
				s = "(null)";
			for(; *s; s++)
				uart_send(*s);
			break;
		case '%':
			uart_send('%');
			break;
		default:
			// Print unknown % sequence to draw attention.
			uart_send('%');
			uart_send(c);
			break;
		}
	}
}

void Normal_World()
{
	asm volatile("ldr r13, =ns_svc_stacktop");
	while(1)
	{
		cprintf("[Fast Model] This is normal world\n");
		asm volatile(
				".arch_extension sec\n\t"
				"smc #0\n\t") ;
	}
}

void Secure_World()
{
	cprintf("[Fast Model] Install Monitor Successfully\n");

	int i;
	for(i=0; i<10; i++)
	{
		cprintf("[Fast Model] This is secure world: %d\n", i);
		asm volatile(
				".arch_extension sec\n\t"
				"smc #0\n\t");
	};

	while(1);
}

void c_start(void)
{
	monitorInit(Normal_World);
	cprintf("[Fast Model] We should never come to here\n");
	while(1);
}
