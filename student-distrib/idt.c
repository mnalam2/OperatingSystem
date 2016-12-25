#include "x86_desc.h"
#include "lib.h"
#include "RTC.h"
#include "idt.h"
#include "keyboard.h"
#include "i8259.h"
#include "debug.h"
#include "multiboot.h"
#include "types.h"
#include "syscall_linkage.h"
#include "syscall.h"
#include "scheduler.h"
void initialize_idt()
{
	int i;
	/*
	exceptions/interrupts = kernel cs, dpl = 0 
	system calls = trap gates , dpl = 3 
	*/

	for (i=0;i<32;i++)
		{
		/*
			set up proper bits to make into an trap gate exception 
		*/
			idt[i].present = 1;
			idt[i].seg_selector = KERNEL_CS;
			idt[i].dpl = 0;
			
			idt[i].reserved4 = 0;
			idt[i].reserved3 = 1;
			idt[i].reserved2 = 1;
			idt[i].reserved1 = 1;
			idt[i].size = 1;
			idt[i].reserved0 = 0;

			SET_IDT_ENTRY(idt[i],&generic_handler);
		
		}	
	/*
	interrupt handlers
	all are interrupt gates w kernel CS and priv = 0 

	idt[32] = timer chip
	idt[33] = keys
	idt[40] = rtc
	*/

	for (i=32;i<NUM_VEC;i++)
	{
			idt[i].present = 1;
			idt[i].seg_selector = KERNEL_CS;
			idt[i].dpl = 0;
		
			idt[i].reserved4 = 0;
			idt[i].reserved3 = 0;
			idt[i].reserved2 = 1;
			idt[i].reserved1 = 1;
			
			
			idt[i].size = 1;
			idt[i].reserved0 = 0;
		
			
	}


	/*

	set idt entries 
	first 32 = exceptions
	next (256-32) = interrupts
	0x80 will have the system call jump table function, but not yet
	be sure to set dpl to 3 for that
	*/
	SET_IDT_ENTRY(idt[0],&de_handler);
	SET_IDT_ENTRY(idt[1],&db_handler);
	SET_IDT_ENTRY(idt[2],&nmi_handler);
	SET_IDT_ENTRY(idt[3],&bp_handler);
	SET_IDT_ENTRY(idt[4],&of_handler);
	SET_IDT_ENTRY(idt[5],&br_handler);
	SET_IDT_ENTRY(idt[6],&ud_handler);
	SET_IDT_ENTRY(idt[7],&nm_handler);
	SET_IDT_ENTRY(idt[8],&df_handler);
	SET_IDT_ENTRY(idt[9],&co_segment_overrun_handler);
	SET_IDT_ENTRY(idt[10],&ts_handler);
	SET_IDT_ENTRY(idt[11],&np_handler);
	SET_IDT_ENTRY(idt[12],&ss_handler);
	SET_IDT_ENTRY(idt[13],&gp_handler);
	SET_IDT_ENTRY(idt[14],&pf_handler);
	SET_IDT_ENTRY(idt[16],&mf_handler);
	SET_IDT_ENTRY(idt[17],&ac_handler);
	SET_IDT_ENTRY(idt[18],&mc_handler);
	SET_IDT_ENTRY(idt[19],&xf_handler);

	SET_IDT_ENTRY(idt[32],&pit_irq_handler);
	SET_IDT_ENTRY(idt[33],&key_irq_handler);
	SET_IDT_ENTRY(idt[40],&rtc_irq_handler);
	
	SET_IDT_ENTRY(idt[128],&systemcall_linkage);
	idt[128].present=1;
	idt[128].seg_selector = KERNEL_CS;
	idt[128].dpl = 3;
	idt[128].reserved4=0;
	idt[128].reserved3=1;
	idt[128].reserved2=1;
	idt[128].reserved1=1;
	idt[128].size=1;
	idt[128].reserved0=0;
}
void de_handler()
{
	cli();
	printf("divide by zero exception");
	halt(-1);

}
void db_handler()
{
	cli();
	printf("db exception");
	halt(-1);
}
void nmi_handler()
{
	cli();
	printf("nmi exception");
	halt(-1);
}
void bp_handler()
{
	cli();
	printf("bp exception");
	halt(-1);
}
void of_handler()
{
	cli();
	printf("of exception");
	halt(-1);

}
void br_handler()
{
	cli();
	printf("br exception");
	halt(-1);
}
void ud_handler()
{
	cli();
	printf("ud exception");
	halt(-1);
}
void nm_handler()
{
	cli();
	printf("nm exception");
	halt(-1);

}
void df_handler()
{
	cli();
	printf("df exception");
	halt(-1);
}
void co_segment_overrun_handler()
{
	cli();
	printf("co_segment_overrun exception");
	halt(-1);
}
void ts_handler()
{
	cli();
	printf("ts exception");
	halt(-1);
}
void np_handler()
{
	cli();
	printf("np exception");
	halt(-1);
}
void ss_handler()
{
	cli();
	printf("ss exception");
	halt(-1);
}
void gp_handler()
{
	cli();
	printf("gp exception");
	halt(-1);
}
void pf_handler()
{
	cli();
	uint32_t cr2;
	uint32_t eec;
	asm volatile("mov %%cr2,%0; popl %1":"=r"(cr2),"=r"(eec)::"memory");
	printf("pf exception at %x, %x\n",cr2,eec);
	halt(-1);
}
void mf_handler()
{
	cli();
	printf("mf exception");
	halt(-1);
}
void ac_handler()
{
	cli();
	printf("ac exception");
	halt(-1);
}
void mc_handler()
{
	cli();
	printf("mc exception");
	halt(-1);
}
void xf_handler()
{
	cli();
	printf("xf exception");
	halt(-1);
}
void generic_handler()
{
	cli();
	printf("Unknown exception");
	halt(-1);
}
void pit_irq_handler()
{
	send_eoi(0);
	sched();
 	asm volatile("leave;iret;");
}
