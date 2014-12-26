#include "semihosting.h"

void c_start(void)
{
	semi_write0("[Fast Model] Hello World!\n");
	while(1);
}
