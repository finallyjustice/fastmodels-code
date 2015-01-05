#include <linux/module.h>
#include <linux/kernel.h>

static int __init smc_test_init(void)
{
	printk(KERN_ALERT "[module] init the module\n");
	asm volatile(
			".arch_extension sec\n\t"
			"smc #0\n\t") ;
	return 0;
}
static void __exit smc_test_exit(void)
{
	printk(KERN_ALERT "[module] exit the module\n");
}

module_init(smc_test_init);
module_exit(smc_test_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dongli Zhang");
MODULE_DESCRIPTION("DEV driver for TrustZone");
