/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7 */
uint8_t slave_mask; /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void
i8259_init(void)
{
	//Start with all interrupts disabled  (except signal from slave)
	master_mask = BASE_MASK;
	slave_mask = BASE_MASK;
	

	outb(master_mask,MASTER_8259_DATA);
	outb(slave_mask,SLAVE_8259_DATA);
	
	outb(ICW1,MASTER_8259_COMMAND); //Set to single cascade mode
	outb(ICW2_MASTER,MASTER_8259_DATA); //Pass interrupt vector
	outb(ICW3_MASTER,MASTER_8259_DATA); //Inform master of slave position
	outb(ICW4,MASTER_8259_DATA); //8086 mode, normal EOI, not buffered, not fully nested

	outb(ICW1,SLAVE_8259_COMMAND); //Set to single cascade mode
	outb(ICW2_SLAVE,SLAVE_8259_DATA); //Pass interrupt vector
	outb(ICW3_SLAVE,SLAVE_8259_DATA); //Inform slave of its position
	outb(ICW4,SLAVE_8259_DATA); //8086 mode, normal EOI, not buffered, not fully nested



}

/* Enable (unmask) the specified IRQ */
void
enable_irq(uint32_t irq_num)
{
	if(irq_num < PIC_SIZE){
		//Update mask, then send mask to master
		master_mask &= ~(1 << irq_num);
		outb(master_mask,MASTER_8259_DATA);
	}else{
		//Update mask, then send mask to slave
		slave_mask &= ~(1 << (irq_num-PIC_SIZE));
		outb(slave_mask,SLAVE_8259_DATA);

	}
}

/* Disable (mask) the specified IRQ */
void
disable_irq(uint32_t irq_num)
{
	if(irq_num < PIC_SIZE){
		//Update mask, then send mask to master	
		master_mask |= (1 << irq_num);
		outb(master_mask,MASTER_8259_DATA);
	}else{
		//Update mask, then send mask to slave
		slave_mask |= (1 << (irq_num-PIC_SIZE));
		outb(slave_mask,SLAVE_8259_DATA);
	}
}

/* Send end-of-interrupt signal for the specified IRQ */
void
send_eoi(uint32_t irq_num)
{
	if(irq_num >= PIC_SIZE){
		//Send EOI to slave and master
		outb(EOI |  (SLAVE_IRQ),MASTER_8259_COMMAND);
		outb(EOI |  (irq_num-PIC_SIZE),SLAVE_8259_COMMAND);

	}else{
		//Send EOI to master
		outb(EOI |  (irq_num),MASTER_8259_COMMAND);

	}
}

