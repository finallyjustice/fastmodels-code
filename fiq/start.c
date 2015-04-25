typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

#ifndef NULL
#define NULL ((void*)0)
#endif

#define VIC_BASE        0x2c000000
#define PIC_TIMER01     2
#define PIC_TIMER23     3
#define PIC_UART0       5
#define PIC_GRAPHIC     19

#define HZ           100

#define TIMER0          0x1c110000
#define TIMER1          0x1c120000
#define CLK_HZ          1000000     // the clock is 1MHZ

// A SP804 has two timers, we only use the first one, and as perodic timer

// define registers (in units of 4-bytes)
#define TIMER_LOAD     0    // load register, for perodic timer
#define TIMER_CURVAL   1    // current value of the counter
#define TIMER_CONTROL  2    // control register
#define TIMER_INTCLR   3    // clear (ack) the interrupt (any write clear it)
#define TIMER_MIS      5    // masked interrupt status

// control register bit definitions
#define TIMER_ONESHOT  0x01 // wrap or one shot
#define TIMER_32BIT    0x02 // 16-bit/32-bit counter
#define TIMER_INTEN    0x20 // enable/disable interrupt
#define TIMER_PERIODIC 0x40 // enable periodic mode
#define TIMER_EN       0x80 // enable the timer

#define SGI_TYPE        1
#define PPI_TYPE        2
#define SPI_TYPE        3

#define GICD_CTLR       0x000
#define GICD_TYPER      0x004
#define GICD_IIDR       0x008

#define GICD_IGROUP     0x080
#define GICD_ISENABLE       0x100
#define GICD_ICENABLE       0x180
#define GICD_ISPEND     0x200
#define GICD_ICPEND     0x280
#define GICD_ISACTIVE       0x300
#define GICD_ICACTIVE       0x380
#define GICD_IPRIORITY      0x400
#define GICD_ITARGET        0x800
#define GICD_ICFG       0xC00

#define GICC_CTLR       0x000
#define GICC_PMR        0x004
#define GICC_BPR        0x008
#define GICC_IAR        0x00C
#define GICC_EOIR       0x010
#define GICC_RRR        0x014
#define GICC_HPPIR      0x018

#define GICC_ABPR       0x01C
#define GICC_AIAR       0x020
#define GICC_AEOIR      0x024
#define GICC_AHPPIR     0x028

#define GICC_APR        0x0D0
#define GICC_NSAPR      0x0E0
#define GICC_IIDR       0x0FC
#define GICC_DIR        0x1000

static volatile uint* gic_base;

#define GICD_REG(o)     (*(uint *)(((uint) gic_base) + 0x1000 + o))
#define GICC_REG(o)     (*(uint *)(((uint) gic_base) + 0x2000 + o))

// cpsr/spsr bits
#define DIS_IRQ     0x80  // IRQ bit
#define DIS_FIQ     0x40  // FIQ bit

volatile unsigned char * const UART0_BASE = (unsigned char *)0x1c090000;

void cli_irq (void);
void sti_irq (void);
void cli_fiq (void);
void sti_fiq (void);

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

void swi_handler()
{
	cprintf("In Secure World's SWI Handler\n");
}

int gic_getack()
{
	return GICC_REG(GICC_IAR);
}

static int spi2id(int spi)
{
	return spi+32;
}

void gic_eoi(int intn)
{
	GICC_REG(GICC_EOIR) = spi2id(intn);
}

void irq_handler()
{
	int intid, intn;
	volatile uint * timer0 = (uint *)TIMER0;
	intid = gic_getack(); /* iack */
	intn = intid - 32;
	cprintf("IRQ Number : %d\n", intn);
	timer0[TIMER_INTCLR] = 1;
	gic_eoi(intn);
	sti_irq();
}

void fiq_handler()
{
	int intid, intn;
	volatile uint * timer0 = (uint *)TIMER0;
	intid = gic_getack(); /* iack */
	intn = intid - 32;
	cprintf("FIQ Number : %d\n", intn);
	timer0[TIMER_INTCLR] = 1;
	gic_eoi(intn);
	sti_fiq();
}

static void gicd_set_bit(int base, int id, int bval) 
{
	int offset = id/32;
	int bitpos = id%32;
	uint rval = GICD_REG(base+4*offset);
	if(bval)
		rval |= 1 << bitpos;
	else
		rval &= ~(1<< bitpos);
	GICD_REG(base+ 4*offset) = rval;
}

static void gd_spi_setcfg(int spi, int is_edge)
{
	int id=spi2id(spi);
	int offset = id/16;
	int bitpos = (id%16)*2;
	uint rval = GICD_REG(GICD_ICFG+4*offset);
	uint vmask=0x03;
	rval &= ~(vmask << bitpos);
	if (is_edge)
		rval |= 0x02 << bitpos;
	GICD_REG(GICD_ICFG+ 4*offset) = rval;
}

static void gd_spi_enable(int spi)
{
	int id = spi2id(spi);
	gicd_set_bit(GICD_ISENABLE, id, 1);
}

static void gd_spi_group0(int spi)
{
	return;
}

static void gd_spi_target0(int spi)
{
	int id=spi2id(spi);
	int offset = id/4;
	int bitpos = (id%4)*8;
	uint rval = GICD_REG(GICD_ITARGET+4*offset);
	unsigned char tcpu=0x01;
	rval |= tcpu << bitpos;
	GICD_REG(GICD_ITARGET+ 4*offset) = rval;
}

static void gic_dist_configure(int itype, int num)
{
	int spi= num;
	gd_spi_setcfg(spi, 1);
	gd_spi_enable(spi);
	gd_spi_group0(spi);
	gd_spi_target0(spi);
}

static void gic_configure(int itype, int num)
{
	gic_dist_configure(itype, num);
}

static void gic_enable()
{
	GICD_REG(GICD_CTLR) |= 1;
	GICC_REG(GICC_CTLR) |= 1;
}

void git_init(uint* base)
{
	gic_base = base;
	GICC_REG(GICC_PMR) = 0x0f;

	gic_configure(SPI_TYPE, PIC_TIMER01);
	gic_enable();
}

void timer_init(int hz)
{   
	volatile uint * timer0 = (uint *)TIMER0;

	timer0[TIMER_LOAD] = CLK_HZ / hz;
	timer0[TIMER_CONTROL] = TIMER_EN|TIMER_PERIODIC|TIMER_32BIT|TIMER_INTEN;
}

void cli_irq (void)
{
	uint val;
	
	asm volatile("MRS %[v], cpsr": [v]"=r" (val)::);
	val |= DIS_IRQ;
	asm volatile("MSR cpsr_cxsf, %[v]": :[v]"r" (val):);
}

void sti_irq (void)
{   
	uint val;

	// ok, enable paging using read/modify/write
	asm volatile("MRS %[v], cpsr": [v]"=r" (val)::);
	val &= ~DIS_IRQ;            
	asm volatile("MSR cpsr_cxsf, %[v]": :[v]"r" (val):);
}

void cli_fiq (void)
{
	uint val;
	
	asm volatile("MRS %[v], cpsr": [v]"=r" (val)::);
	val |= DIS_FIQ;
	asm volatile("MSR cpsr_cxsf, %[v]": :[v]"r" (val):);
}

void sti_fiq (void)
{   
	uint val;

	// ok, enable paging using read/modify/write
	asm volatile("MRS %[v], cpsr": [v]"=r" (val)::);
	val &= ~DIS_FIQ;            
	asm volatile("MSR cpsr_cxsf, %[v]": :[v]"r" (val):);
}

unsigned int ret_addr;

void c_start(void)
{
	printstr("hello1\n");
	asm volatile("swi #7");
	cprintf("It is working!\n");

	git_init((uint *)VIC_BASE);
	timer_init(HZ);

	// set timer as fiq
	// base addr of GIC Distr is 0x2c001000
	// base addr of GIC Interface is 0x2c002000
	// ICCICR Interface Control Register (banked) is at offset 0x0
	unsigned int *iccicr_base = (unsigned int *)0x2c002000;

	/*
	 * [0] ENABLES  enable secure interrupt
	 * [1] ENABLENS enable non-secure interrupt
	 * [3] FIQEn 1-signal secure interrupt as FIQ, 0-IRQ. non-secure interrupt is always IRQ
	 */
	*iccicr_base |= 0x8;

	sti_fiq();

	while(1);
}
