/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "RTC.h"
#include "idt.h"
#include "paging.h"
#include "keyboard.h"
#include "filesystem.h"
#include "syscall.h"
#include "scheduler.h"
/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

void fill_interrupts();
/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void
entry (unsigned long magic, unsigned long addr)
{
	multiboot_info_t *mbi;

	/* Clear the screen. */
	clear();

	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		printf ("Invalid magic number: 0x%#x\n", (unsigned) magic);
		return;
	}

	/* Set MBI to the address of the Multiboot information structure. */
	mbi = (multiboot_info_t *) addr;

	/* Print out the flags. */
	printf ("flags = 0x%#x\n", (unsigned) mbi->flags);

	/* Are mem_* valid? */
	if (CHECK_FLAG (mbi->flags, 0))
		printf ("mem_lower = %uKB, mem_upper = %uKB\n",
				(unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

	/* Is boot_device valid? */
	if (CHECK_FLAG (mbi->flags, 1))
		printf ("boot_device = 0x%#x\n", (unsigned) mbi->boot_device);

	/* Is the command line passed? */
	if (CHECK_FLAG (mbi->flags, 2))
		printf ("cmdline = %s\n", (char *) mbi->cmdline);

	if (CHECK_FLAG (mbi->flags, 3)) {
		int mod_count = 0;
		int i;
		module_t* mod = (module_t*)mbi->mods_addr;
		while(mod_count < mbi->mods_count) {
			fs_base = (boot_block_t *) mod->mod_start;
			fs_end = (boot_block_t *) mod->mod_end;

			printf("Module %d has string: 0x%#x\n", mod_count, (unsigned int)mod->string);			
			printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
			printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
			printf("First few bytes of module:\n");
			for(i = 0; i<16; i++) {
				printf("0x%x ", *((char*)(mod->mod_start+i)));
			}
			printf("\n");
			mod_count++;
			mod++;
		}
	}
	/* Bits 4 and 5 are mutually exclusive! */
	if (CHECK_FLAG (mbi->flags, 4) && CHECK_FLAG (mbi->flags, 5))
	{
		printf ("Both bits 4 and 5 are set.\n");
		return;
	}

	/* Is the section header table of ELF valid? */
	if (CHECK_FLAG (mbi->flags, 5))
	{
		elf_section_header_table_t *elf_sec = &(mbi->elf_sec);

		printf ("elf_sec: num = %u, size = 0x%#x,"
				" addr = 0x%#x, shndx = 0x%#x\n",
				(unsigned) elf_sec->num, (unsigned) elf_sec->size,
				(unsigned) elf_sec->addr, (unsigned) elf_sec->shndx);
	}

	/* Are mmap_* valid? */
	if (CHECK_FLAG (mbi->flags, 6))
	{
		memory_map_t *mmap;

		printf ("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
				(unsigned) mbi->mmap_addr, (unsigned) mbi->mmap_length);
		for (mmap = (memory_map_t *) mbi->mmap_addr;
				(unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
				mmap = (memory_map_t *) ((unsigned long) mmap
					+ mmap->size + sizeof (mmap->size)))
			printf (" size = 0x%x,     base_addr = 0x%#x%#x\n"
					"     type = 0x%x,  length    = 0x%#x%#x\n",
					(unsigned) mmap->size,
					(unsigned) mmap->base_addr_high,
					(unsigned) mmap->base_addr_low,
					(unsigned) mmap->type,
					(unsigned) mmap->length_high,
					(unsigned) mmap->length_low);
	}

	/* Construct an LDT entry in the GDT */
	{
		seg_desc_t the_ldt_desc;
		the_ldt_desc.granularity    = 0;
		the_ldt_desc.opsize         = 1;
		the_ldt_desc.reserved       = 0;
		the_ldt_desc.avail          = 0;
		the_ldt_desc.present        = 1;
		the_ldt_desc.dpl            = 0x0;
		the_ldt_desc.sys            = 0;
		the_ldt_desc.type           = 0x2;

		SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
		ldt_desc_ptr = the_ldt_desc;
		lldt(KERNEL_LDT);
	}

	/* Construct a TSS entry in the GDT */
	{
		seg_desc_t the_tss_desc;
		the_tss_desc.granularity    = 0;
		the_tss_desc.opsize         = 0;
		the_tss_desc.reserved       = 0;
		the_tss_desc.avail          = 0;
		the_tss_desc.seg_lim_19_16  = TSS_SIZE & 0x000F0000;
		the_tss_desc.present        = 1;
		the_tss_desc.dpl            = 0x0;
		the_tss_desc.sys            = 0;
		the_tss_desc.type           = 0x9;
		the_tss_desc.seg_lim_15_00  = TSS_SIZE & 0x0000FFFF;

		SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

		tss_desc_ptr = the_tss_desc;

		tss.ldt_segment_selector = KERNEL_LDT;
		tss.ss0 = KERNEL_DS;
		tss.esp0 = 0x800000;
		ltr(KERNEL_TSS);
	}

	/* Init the PIC */

	initialize_idt();

	lidt(idt_desc_ptr);
	i8259_init();
	init_PIT();
	enable_irq(2);

	//RTC_init();
	
	/* Test RTC write and read functions */
	// RTC_open(NULL);
	// int32_t inbuff[1] = {2};
	// struct file* temp;
	// RTC_write(temp,(const char*)&inbuff,4);
	// clear();
	// int i=0;
	// int x=0;
	// for (i=0;i<6;i++)
	// {
	// 	x++;
	// 	RTC_read(temp,NULL,0);
	// 	if (x % (80*25) == 0)
	// 	{
	// 		clear();
	// 		x = 0;
	// 	}
	// }
	// clear();
	// inbuff[0] = 4;
	// RTC_write(temp,(const char*)&inbuff,4);
	// x = 0;
	// for (i=0;i<6;i++)
	// {
	// 	x++;
	// 	RTC_read(temp,NULL,0);
	// 	if (x % (80*25) == 0)
	// 	{
	// 		clear();
	// 		x = 0;
	// 	}
	// }
	// clear();
	// inbuff[0] = 8;
	// RTC_write(temp,(const char*)&inbuff,4);
	// x = 0;
	// for (i=0;i<8;i++)
	// {
	// 	x++;
	// 	RTC_read(temp,NULL,0);
	// 	if (x % (80*25) == 0)
	// 	{
	// 		clear();
	// 		x = 0;
	// 	}
	// }
	// clear();
	// inbuff[0] = 16;
	// RTC_write(temp,(const char*)&inbuff,4);
	// x = 0;
	// for (i=0;i<16;i++)
	// {
	// 	x++;
	// 	RTC_read(temp,NULL,0);
	// 	if (x % (80*25) == 0)
	// 	{
	// 		clear();
	// 		x = 0;
	// 	}
	// }
	// clear();
	// inbuff[0] = 32;
	// RTC_write(temp,(const char*)&inbuff,4);
	// x = 0;
	// for (i=0;i<32;i++)
	// {
	// 	x++;
	// 	RTC_read(temp,NULL,0);
	// 	if (x % (80*25) == 0)
	// 	{
	// 		clear();
	// 		x = 0;
	// 	}
	// }
	// clear();
	// inbuff[0] = 64;
	// RTC_write(temp,(const char*)&inbuff,4);
	// x = 0;
	// for (i=0;i<64;i++)
	// {
	// 	x++;
	// 	RTC_read(temp,NULL,0);
	// 	if (x % (80*25) == 0)
	// 	{
	// 		clear();
	// 		x = 0;
	// 	}
	// }
	// clear();
	// inbuff[0] = 128;
	// RTC_write(temp,(const char*)&inbuff,4);
	// x = 0;
	// for (i=0;i<128;i++)
	// {
	// 	x++;
	// 	RTC_read(temp,NULL,0);
	// 	if (x % (80*25) == 0)
	// 	{
	// 		clear();
	// 		x = 0;
	// 	}
	// }
	// clear();
	// inbuff[0] = 256;
	// RTC_write(temp,(const char*)&inbuff,4);
	// x = 0;
	// for (i=0;i<256;i++)
	// {
	// 	x++;
	// 	RTC_read(temp,NULL,0);
	// 	if (x % (80*25) == 0)
	// 	{
	// 		clear();
	// 		x = 0;
	// 	}
	// }
	// clear();
	// inbuff[0] = 512;
	// RTC_write(temp,(const char*)&inbuff,4);
	// x = 0;
	// for (i=0;i<512;i++)
	// {
	// 	x++;
	// 	RTC_read(temp,NULL,0);
	// 	if (x % (80*25) == 0)
	// 	{
	// 		clear();
	// 		x = 0;
	// 	}
	// }
	// clear();
	// inbuff[0] = 1024;
	// RTC_write(temp,(const char*)&inbuff,4);
	// x = 0;
	// for (i=0;i<1024;i++)
	// {
	// 	x++;
	// 	RTC_read(temp,NULL,0);
	// 	if (x % (80*25) == 0)
	// 	{
	// 		clear();
	// 		x = 0;
	// 	}
	// }
	// clear();
	// printf("Close Return Value: %d\n", RTC_close(NULL));
	
	
	keyboard_install(1);
	

	/* Initialize devices, memory, filesystem, enable device interrupts on the
	 * PIC, any other initialization stuff... */
	
	init_kernel_pd();
	loadPageDirectory(kernel_page_directory);
	enablePaging();
	/* Enable interrupts */
	/* Do not enable the following until after you have set up your
	 * IDT correctly otherwise QEMU will triple fault and simple close
	 * without showing you any output */
	//printf("Enabling Interrupts\n");
	term_clear(0); // Comment out for RTC 
	
	sti();




	//Test Filesystem info
	
	// printf("\n");
	// clear();
	// printf("\n");
	// show_fs_info();
	

	//Test File read
	// init_filesystem();
	// printf("\n");
	// clear();
	// printf("\n");
	// char inbuff[1024] = {0};
	// printf("%d\n",test_read(".",&inbuff,80));
	// test_read("created.txt",&inbuff,1023);
	// printf("%s",inbuff);
	
	
	//Terminal Testing
	
	/* unsigned char inbuff[1024] = {0};
	int i; 
	while(true){
	 	terminal_read(NULL, inbuff, 391);
	 	if(strncmp(inbuff,"i",5))
	 		terminal_write(NULL,inbuff,strlen((int8_t*)inbuff));
	 	for(i=0;i<1024;i++){
	 		inbuff[i] == '\0';
	 	}
	} */
	/* Execute the first program (`shell') ... */

	/* Spin (nicely, so we don't chew up cycles) */

	execute((uint8_t *)"shell");
	asm volatile(".1: hlt; jmp .1;");


}

