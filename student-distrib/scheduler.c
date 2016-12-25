#include "scheduler.h"
#include "syscall.h"
#include "i8259.h"
#include "x86_desc.h"
#include "paging.h"
#include "lib.h"

volatile uint8_t shells_started = 1;

void init_PIT(){
	//possibly configure pit for different frequencies?
	enable_irq(0);

}

/*
 *  Switches to the next active process. 
 */
void sched(){
	cli();
	PCB_t * pcb;
	cur_pcb(pcb);
	uint32_t pid = pcb->pid;

	active[pcb->pid] = true;


	asm volatile("\
		movl	%%esp,%0 	\n\
		movl    %%ebp, %1"
		: "=r"(pcb->esp), "=r"(pcb->ebp)
		: 
		:"memory"); //change segment registers 


	//Make sure that our shells are started
	if(shells_started < 3){
		shells_started++;
		execute((uint8_t *)"shell");
	}

	uint32_t next = get_next_proc(pid);
	show_status();
	PCB_t * next_pcb = 	(PCB_t *)(KERNEL_TOP-KB8 * (next+1));
	loadPageDirectory(proc_page_directory[next]);
	tss.ss0 = KERNEL_DS;
	tss.esp0 = KERNEL_TOP-KB8 * next - 4;
	asm volatile("\
		movl	%0,%%esp 	\n\
		movl    %1,%%ebp"
		:
		: "r"(next_pcb->esp),"r"(next_pcb->ebp)\
		: "memory" );
	sti();
}

/*
 *  Returns the pid of the next active process in round-robin order.
 */
uint32_t get_next_proc(uint32_t curr){
	do {
		curr++;		
		if(curr >= MAX_USER_PROG)
			curr -= MAX_USER_PROG;
	}while(!active[curr]);
	return curr;
}

void show_status(){
	uint8_t * video = (uint8_t *) VIDEO;
	int i;
	for(i=0;i<MAX_USER_PROG;i++){
		if(proc_id_used[i] == true){
			if(active[i]){
				video[((NUM_COLS-MAX_USER_PROG + i)<<1)+1] = 0x20;

			}else{
				video[((NUM_COLS-MAX_USER_PROG + i)<<1)+1] = 0x40;

			}
		}else{
			video[((NUM_COLS-MAX_USER_PROG + i)<<1)+1] = ATTRIB;
		} 
	}
}
