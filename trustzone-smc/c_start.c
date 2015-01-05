#include "semihosting.h"

void Normal_World()
{
	while(1)
	{
		semi_write0("[Fast Model] This is normal world\n");
		asm volatile(
				".arch_extension sec\n\t"
				"smc #0\n\t") ;
	}
}

void c_start(void)
{
	monitorInit(Normal_World);
	semi_write0("[Fast Model] Install Monitor Successfully\n");

	int i;
	for(i=0; i<10; i++)
	{
		semi_write0("[Fast Model] This is secure world\n");
		asm volatile(
				".arch_extension sec\n\t"
				"smc #0\n\t");
	};

	while(1);
}
