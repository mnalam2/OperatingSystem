#include "RTC.h"
#include "syscall.h"
#include "types.h"
volatile int rtc_interrupt_occured[3];

/* Initializes RTC before it can take interrupts */
void RTC_init()
{
	// Initialize rate variable
	unsigned char rate;
	// Initialize 2 Hz frequency by setting rate to 15
	// frequency = 32768 >> (rate-1)
	// 			 = 32768 >> (15-1) = 2 Hz
	rate = RTC_DEFAULT_RATE;

	/* Turn on IRQ 8 */
	// Disable NMI and select Register B
	outb(REGISTER_B, NMI);
	// Read value of Register B
	char val = inb(CMOS);
	// Set index
	outb(REGISTER_B, NMI);
	// Turn on bit 6 of Register B
	outb(val | MASKBIT6, CMOS);

	/* Set Default Interrupt Rate */
	// Disable NMI and select Register A
	outb(REGISTER_A, NMI);
	// Read value of Register A
	val = inb(CMOS);
	// Set index
	outb(REGISTER_A, NMI);
	// Write rate into Register A (bottom 4 bits)
	outb((val & RATEMASK) | rate, CMOS);

	enable_irq(IRQ_NUM);
}

/* Handles RTC interrupts */
void rtc_irq_handler()
{
	// Disable NMI and select Register C
	outb(REGISTER_C, NMI);
	// Throw away contents
	inb(CMOS);
	int i;
	for(i=0;i<3;i++)
		rtc_interrupt_occured[i] = 1;

	// Send end-of-interrupt signal
	send_eoi(IRQ_NUM);

 	asm volatile("leave;iret;");
}


/* Sets up data to handle RTC device */
int32_t RTC_open(struct file * fp)
{
	// Initializes RTC device
	RTC_init();
	return 0;
}

/* Reads data from RTC device */
int32_t RTC_read(struct file * fp, char * buff, uint32_t nbytes)
{

	cli();
	PCB_t * pcb;
	cur_pcb(pcb);
	uint32_t term = pcb->terminal_id;
	rtc_interrupt_occured[term] = 0; 
	sti();
	// Waits for interrupt to occur
	while(rtc_interrupt_occured[term] == 0);
	return 0;
}


/* Sets passed in frequency/writes frequency to RTC */
int32_t RTC_write(struct file * fp, const char * buff, uint32_t nbytes)
{
	// Checks if 4-byte integer is being passed in
	if (nbytes == BYTE4)
	{
		unsigned char rate;
		// Extracts frequency passed in
		int32_t fq = *(int32_t*)buff;
		// Checks which rate should be set according to which frequency was passed in
		// frequency = 32768 >> (rate-1)
		if (fq == FREQ1)
		{
			rate = RATE1;
		}
		else if(fq == FREQ2)
		{
			rate = RATE2;
		}
		else if (fq == FREQ3)
		{
			rate = RATE3;
		}
		else if (fq == FREQ4)
		{
			rate = RATE4;
		}
		else if (fq == FREQ5)
		{
			rate = RATE5;
		}
		else if (fq == FREQ6)
		{
			rate = RATE6;
		}
		else if (fq == FREQ7)
		{
			rate = RATE7;
		}
		else if (fq == FREQ8)
		{
			rate = RATE8;
		}
		else if (fq == FREQ9)
		{
			rate = RATE9;
		}
		else if (fq == RTC_DEFAULT_FREQ)
		{
			rate = RTC_DEFAULT_RATE;
		}

		// Disable NMI and select Register A
		outb(REGISTER_A, NMI);
		// Read value of Register A
		char val = inb(CMOS);
		// Set index
		outb(REGISTER_A, NMI);
		// Write rate into Register A (bottom 4 bits)
		outb((val & RATEMASK) | rate, CMOS);

		return 0;

	}

	return -1;
}

/* Closes RTC device and makes it available for later calls to open */
int32_t RTC_close(struct file * fp)
{
	return 0;
}
