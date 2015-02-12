#include "uart.h"

void kmain(void)
{
	uart_send('W');
//cprintf("This is new world!\n");
	while(1);
}
