#include "semihosting.h"

volatile unsigned char * const UART0_BASE = (unsigned char *)0x1c090000;

void uart_send(unsigned int c)
{
	*UART0_BASE = c;
	if(c == '\n')
		*UART0_BASE = '\r';
}

void printstr(char *s)
{
	int i = 0;
	while(s[i])
	{
		uart_send(s[i]);
		i++;
	}
}

void c_start(void)
{
	printstr("hello1\n");
	printstr("hello2\n");
	semi_write0("[Fast Model] Hello World!\n");
	while(1);
}
