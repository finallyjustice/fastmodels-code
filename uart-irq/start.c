typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

#ifndef NULL
#define NULL ((void*)0)
#endif

#define UART0           0x1c090000
#define UART_CLK        24000000    // Clock rate for UART

#define UART_DR		0	// data register
#define UART_RSR	1	// receive status register/error clear register
#define UART_FR		6	// flag register
#define	UART_IBRD	9	// integer baud rate register
#define UART_FBRD	10	// Fractional baud rate register
#define UART_LCR	11	// line control register
#define UART_CR		12	// control register
#define UART_IMSC	14	// interrupt mask set/clear register
#define UART_MIS	16	// masked interrupt status register
#define	UART_ICR	17	// interrupt clear register
// bits in registers
#define UARTFR_TXFF	(1 << 5)	// tramit FIFO full
#define UARTFR_RXFE	(1 << 4)	// receive FIFO empty
#define	UARTCR_RXE	(1 << 9)	// enable receive
#define UARTCR_TXE	(1 << 8)	// enable transmit
#define	UARTCR_EN	(1 << 0)	// enable UART
#define UARTLCR_FEN	(1 << 4)	// enable FIFO
#define UART_RXI	(1 << 4)	// receive interrupt
#define UART_TXI	(1 << 5)	// transmit interrupt
#define UART_BITRATE 19200

#define VIC_BASE        0x2c000000
#define PIC_TIMER01     2
#define PIC_TIMER23     3
#define PIC_UART0       5
#define PIC_GRAPHIC     19

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
#define NO_INT      0xc0
#define DIS_INT     0x80

volatile unsigned char * const UART0_BASE = (unsigned char *)0x1c090000;
static volatile uint *uart_base;

void cli (void);
void sti (void);

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

//poll the UART for data
int uartgetc (void)
{
	if (uart_base[UART_FR] & UARTFR_RXFE) {
		return -1;
	}
	
	return uart_base[UART_DR];
}

void handle_uart_irq(void)
{
	if (uart_base[UART_MIS] & UART_RXI) 
	{
		int c = uartgetc();
		cprintf("uart input: %x\n", c);
	}

	// clear the interrupt
	uart_base[UART_ICR] = UART_RXI | UART_TXI;
}

void irq_handler()
{
	int intid, intn;
	intid = gic_getack(); /* iack */
	intn = intid - 32;
	handle_uart_irq();
	cprintf("IRQ Number : %d\n", intn);
	gic_eoi(intn);
	sti();
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

	gic_configure(SPI_TYPE, PIC_UART0);
	gic_enable();
}

void cli (void)
{
	uint val;
	
	asm volatile("MRS %[v], cpsr": [v]"=r" (val)::);
	val |= DIS_INT;
	asm volatile("MSR cpsr_cxsf, %[v]": :[v]"r" (val):);
}

void sti (void)
{   
	uint val;

	// ok, enable paging using read/modify/write
	asm volatile("MRS %[v], cpsr": [v]"=r" (val)::);
	val &= ~DIS_INT;            
	asm volatile("MSR cpsr_cxsf, %[v]": :[v]"r" (val):);
}

// enable the receive (interrupt) for uart (after PIC has initialized)
void uart_enable_rx ()
{
	uart_base[UART_IMSC] = UART_RXI;
	//pic_enable(PIC_UART0, isr_uart);
}

// enable uart
void uart_init (void *addr)
{
    uint left;

    uart_base = addr;

    // set the bit rate: integer/fractional baud rate registers
    uart_base[UART_IBRD] = UART_CLK / (16 * UART_BITRATE);

    left = UART_CLK % (16 * UART_BITRATE);
    uart_base[UART_FBRD] = (left * 4 + UART_BITRATE / 2) / UART_BITRATE;

    // enable trasmit and receive
    uart_base[UART_CR] |= (UARTCR_EN | UARTCR_RXE | UARTCR_TXE);

    // enable FIFO
    uart_base[UART_LCR] |= UARTLCR_FEN;
}

void c_start(void)
{
	uart_init((void *)UART0);
	git_init((uint *)VIC_BASE);
	uart_enable_rx();
	sti();
	cprintf("Start the test!\n");

	while(1);
}
