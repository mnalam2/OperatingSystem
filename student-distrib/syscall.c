#include "syscall.h"
#include "filesystem.h"
#include "paging.h"
#include "x86_desc.h"
#include "lib.h"
#include "RTC.h"
#include "scheduler.h"
#include "keyboard.h"
/*
INPUTS: command
OUTPUTS: returns -1 on failure, 256 for an excption, or 0->255 if a halt occurs

enters userspace, pages memory for process, context switch, etc
*/
asmlinkage int32_t execute (const uint8_t* command){
	cli();
	//Obtain PID
	if(command == NULL)
		return -1;
	int i,j;
	uint32_t pid = -1;
	for(i=0;i<MAX_USER_PROG;i++){    //set process id, if == -1, max processes
		if(proc_id_used[i] == false){
			pid = i;
			proc_id_used[i] = true;
			break;
		}
	}
	if(pid == -1){
		printf("Max processes used.\n");
		return -1;
	}
	PCB_t * parent;
	cur_pcb(parent);

	uint32_t terminal_id = 	parent->terminal_id;

	PCB_t * pcb = (PCB_t *)(KERNEL_TOP-KB8 * (pid+1)); //get pcb to point to kernel
	if(pid <= 2){
		parent = pcb;
		terminal_id = pid;
	}
	memset(pcb,0,sizeof(PCB_t));
	pcb->pid = pid;
	if(pcb->pid == 0)
		pcb->parent = pcb; //if pid = 0 is the first process
	//Parse args
	//printf("%s",command);
	for(i=0;i<=MAX_BUF_INDEX && command[i] != ' '&& command[i] != '\0';i++){
		pcb->name[i] = command[i];
	}
	pcb->name[i] = '\0';
	while(command[i] == ' ')i++;
	uint32_t offset = i;
	for(;i<=MAX_BUF_INDEX && command[i] != '\0';i++){
		pcb->args[i-offset] = command[i];
	}
	i--;
	while(pcb->args[i-offset] == ' ')pcb->args[(i--)-offset] = '\0';
	dentry_t d;
	if(read_dentry_by_name (pcb->name, &d) == -1){
		proc_id_used[pid] = false;
		return -1;
	}


	/* *** EDITS 5th DECEMBER BELOW -- Command History *** */
	/* Advance old cmds by one to make space for the new cmd */
	for(i=4; i>0; i--){
		for(j=0; j<LINE_BUF_SIZE; j++)
			terminals[terminal_id].cmd_hist[i][j] = terminals[terminal_id].cmd_hist[i-1][j];
	}
	/* Store each character of cmd name into first entry of terminal's cmd_hist */
	i=0;
	while(i<MAX_BUF_INDEX){
		terminals[terminal_id].cmd_hist[0][i] = command[i];
		i++;
	}
	/* *** EDITS 5th DECEMBER ABOVE -- Command History *** */


	uint8_t head[40];
	read_data (d.inode, 0, head, 40); //ELF check 
	if(head[0] != ELF_MAGIC_0 || head[1] != 'E' || head[2] != 'L' || head[3] != 'F'){
		proc_id_used[pid] = false;
		return -1;
	}
	//Convert EIP from little endian to big endian
	//Could use x86 bswap %reg
	uint32_t eip = (head[27] << 24) | (head[26] << 16) | (head[25] << 8) | head[24];


	//Change paging
	proc_page_directory[pid][0] =  (uint32_t)video_page_table | FLAG_WRITE_ENABLE | FLAG_PRESENT;
	proc_page_directory[pid][1] = MB4 | FLAG_4MB_PAGE | FLAG_WRITE_ENABLE | FLAG_PRESENT | FLAG_GLOBAL;
	proc_page_directory[pid][USER_MEM_LOCATION >> 22] = (KERNEL_TOP + MB4 * pid) | FLAG_4MB_PAGE | FLAG_WRITE_ENABLE | FLAG_PRESENT |FLAG_USER;

	loadPageDirectory(proc_page_directory[pid]);

	//Load program into program area
	read_data (d.inode, 0, (uint8_t *)(USER_PROG_LOCATION), MB4); //Copy up to 4MB of program data (stack will kill some of it)
	//Create PCB
	

	
	pcb->parent = parent;
	pcb->terminal_id = terminal_id;
	parent->esp0 = tss.esp0;
	active[pcb->parent->pid] = false;
	active[pid] = true;
	asm volatile("\
		movl	%%ss,%2		\n\
		movl	%%esp,%0 	\n\
		movl    %%ebp, %1"
		: "=r"(pcb->parent->esp), "=r"(pcb->parent->ebp), "=r"(pcb->parent->ss0)
		: 
		:"memory"); //change segment registers 
	uint32_t u_esp = USER_MEM_LOCATION + MB4 -4 ;
	//calculate user stack
	//init file ops

	//stdin / stdout
	pcb->fd[0].f_op = &stdin_ops;
	pcb->fd[0].flags =1;
	pcb->fd[1].f_op = &stdout_ops;
	pcb->fd[1].flags =1;
	//Change TSS
	tss.esp0 = KERNEL_TOP-KB8 * pid -4;//address of new kernel stack
	tss.ss0 = KERNEL_DS;
	//set up fake table thingy on stack context switch
	asm volatile("\
		movw	%2,%%ax 	\n\
		movw    %%ax, %%ds		\n\
		movw    %%ax, %%es		\n\
		movw    %%ax, %%fs		\n\
		movw    %%ax, %%gs		\n\
		movl	%%esp,%%eax		\n\
		pushl	%2		\n\
		pushl 	%1			\n\
		pushf					\n\
		popl	%%eax \n\
		orl $0x200,%%eax	# set the interrupt flag\n\
		pushl	%%eax	\n\
		pushl	%3		\n\
		pushl	%0				\n\
		iret	\n\
		.globl halt_ret_label	\n\
		halt_ret_label:"
		:
		: "r"(eip),"r"(u_esp),"i"(USER_DS),"i"(USER_CS)
		: "eax");
	uint32_t ret;
	asm volatile("movl %%eax, %0":"=r" (ret));
	return ret;
}


/*
INPUTS: status
OUTPUTS: returns -1 on fail, 0 on success
Closes pcb, restores process back to parent, 

*/
asmlinkage int32_t halt (uint8_t status){
	PCB_t * pcb;
	cur_pcb(pcb);
	//Change paging back

	//Security-shuld clean old memory space
	memset((void *)USER_MEM_LOCATION,0,MB4);

	loadPageDirectory(proc_page_directory[pcb->parent->pid]);
	//close any FDs that need it
	int i = 0;
	for (i=0;i<8;i++){
		close(i);
	}
	proc_id_used[pcb->pid] = false;
	active[pcb->pid] = false;
	active[pcb->parent->pid] = true;
	//Change TSS
	tss.esp0 = pcb->parent->esp0;
	tss.ss0 = pcb->parent->ss0;
	//change esp/ebp
	if(pcb->pid == pcb->parent->pid){
		execute((uint8_t *)"shell");
	}
	asm volatile("\
		movl 	%2,%%eax	\n\
		movl	%0,%%esp 	\n\
		movl    %1,%%ebp"	
		:
		: "r"(pcb->parent->esp),"r"(pcb->parent->ebp),"r"((uint32_t)status)
		: "memory" );
	asm volatile("jmp halt_ret_label");
	return 0;
}

/*
* int32_t read(fd, buf, nbytes);
*   Inputs: fd, buf, nbytes
*   Return Value: Return value of callee function
*	Function: Goes to specified read function
*/
asmlinkage int32_t read (int32_t fd, void* buf, int32_t nbytes){
	PCB_t * pcb;
	cur_pcb(pcb); // Retrieve updated PCB
	// Check if there are valid values
	if (pcb->fd[fd].flags == 0 || fd >= 8 || fd < 0 || is_kernel_ptr(buf))
	{
		return -1;
	}
	// Return specified read function return value
	if(pcb->fd[fd].f_op->read == NULL)
		return -1;
	int32_t val = (pcb->fd[fd].f_op->read(&(pcb->fd[fd]), buf, nbytes));
	return val;
}

/*
* int32_t write(fd, buf, nbytes);
*   Inputs: fd, buf, nbytes
*   Return Value: Return value of callee function
*	Function: Goes to specified write function
*/
asmlinkage int32_t write (int32_t fd, const void* buf, int32_t nbytes){
	PCB_t* pcb;
	cur_pcb(pcb); // Retrieve updated PCB
	// Check if there are valid values
	if (pcb->fd[fd].flags==0 || buf==NULL || fd > 7 || fd < 0)
		return -1;
	if(is_kernel_ptr(buf))
		return -1;
	// Return specified read function return value
	int32_t val=(pcb->fd[fd].f_op->write((&(pcb->fd[fd])), buf, nbytes));
	return val;
}

/*
INPUTS: filename
OUTPUTS: returns the proper file descriptor 
sets jump table, other member elements of file* struct
*/
asmlinkage int32_t open (const uint8_t* filename)
{
	int i;
	dentry_t d;
	uint32_t success; //flag
	PCB_t * current;	
	cur_pcb(current);	//macro grabs current process's pcb
	if(filename == NULL)
		return -1;
	//printf("%x %s\n",filename,filename);
	success = read_dentry_by_name(filename, &d);
	if (success == 0) //if read call worked 
	{
		for (i=2;i<8;i++) //iterate thru file descriptors that arent stdin/stdout
		{
			/*
			set flags == 1 if that file descriptor is in use 
			*/

			if ( current->fd[i].flags == 0)
			{
				
				//set the operations table, inode, flags?
				//inode_t * node = (inode_t *)(&fs_base[d->inode+1]);
				

				
				if (d.type == 0)
				{
					current->fd[i].f_inode = NULL; //set to null for rtc 
					current->fd[i].f_pos = 0;
					current->fd[i].flags = 1; //mark as in use 
					current->fd[i].f_op = &RTC_ops; //set jump table
				}
				
				else if (d.type == 1)
				{

					current->fd[i].f_inode = d.inode; //mark 
					current->fd[i].f_pos = 0;
					current->fd[i].flags = 1; //mark as in use 
					current->fd[i].f_op = &dir_ops; //jump table
					 //current->fd[i].f_op = fops_table[FILE_DAT_TYPE];
				}
				
				else if (d.type == 2)
				{
					current->fd[i].f_inode = d.inode; //set inode
					current->fd[i].f_pos = 0;
					current->fd[i].flags = 1; //mark as in use
					current->fd[i].f_op = &file_ops; //table
				}
				current->fd[i].fd_index = i; //store file descriptor
				if(current->fd[i].f_op->open((&(current->fd[i]))) == -1)
					return -1; //check if open works
				return i; //returns fd


			}
		}
	
	}
	return -1; //fail


	
}

/*
INPUTS: File descriptor
OUTPUTS: returns -1 on fail, else returns value from close
Closes file descriptor in PCB , sets members structs to null

*/
asmlinkage int32_t close (int32_t fd){
		
	PCB_t * current;
	cur_pcb(current); //grab current pcb

	if (fd > 7 || fd < 2) //sanity check 
	{
		return -1;
	}
	if(current->fd[fd].flags == 0){ //check for open descriptor 
		return -1;
	}
	int32_t ret = current->fd[fd].f_op->close(&(current->fd[fd])); //jump to correct function

	current->fd[fd].f_inode = NULL; //set all to null
	current->fd[fd].f_pos = NULL;
	current->fd[fd].f_op = NULL;
	current->fd[fd].flags = NULL;
	current->fd[fd].fd_index = NULL;

	//struct file* fp=(struct file*) (&(current->fd[fd]));


	return ret; 
}
asmlinkage int32_t getargs (uint8_t* buf, int32_t nbytes)
{
	PCB_t * current;
	cur_pcb(current);
	if(strlen((int8_t *)current->args) > nbytes)
		return -1;
	if(nbytes > LINE_BUF_SIZE)
		nbytes = LINE_BUF_SIZE;

	int i;
	for(i=0;i<nbytes;i++){
		buf[i] = current->args[i];
		if(current->args[i] == '\0')
			break;
	}
	return 0;
}
asmlinkage int32_t vidmap (uint8_t** screen_start)
{
	if (screen_start == NULL || is_kernel_ptr(screen_start))
		return -1;

	*screen_start = (uint8_t*) USER_VIDEO;

	PCB_t * current;
	cur_pcb(current);
	proc_page_directory[current->pid][USER_VIDEO >> 22] = (uint32_t)(&(term_video_tables[current->terminal_id][current->pid])) | FLAG_WRITE_ENABLE | FLAG_PRESENT | FLAG_USER;
	loadPageDirectory(proc_page_directory[current->pid]);
	return 0;
}
asmlinkage int32_t set_handler (int32_t signum, void* handler_address){
	return 0;
}
asmlinkage int32_t sigreturn (void){

	return 0;
}
