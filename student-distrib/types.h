/* types.h - Defines to use the familiar explicitly-sized types in this
 * OS (uint32_t, int8_t, etc.).  This is necessary because we don't want
 * to include <stdint.h> when building this OS
 * vim:ts=4 noexpandtab
 */

#ifndef _TYPES_H
#define _TYPES_H

#define NULL 0

#ifndef ASM

/* Types defined here just like in <stdint.h> */
typedef int int32_t;
typedef unsigned int uint32_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef char int8_t;
typedef unsigned char uint8_t;

typedef short int bool;
#define true 1
#define false 0



#define LINE_BUF_SIZE	128
#define PCB_MASK 0xFFFFE000

struct file;

typedef struct file_operations{
	int32_t (*read) (struct file *, char *, uint32_t);
	int32_t (*write) (struct file *, const char *, uint32_t);
	int32_t (*open) (struct file *);
	int32_t (*close) (struct file *);
} file_operations_t;

typedef struct file{
	struct file_operations * f_op;
	uint32_t f_inode;
	uint32_t f_pos;
	uint32_t flags;
	uint32_t fd_index;
} file_t;

typedef struct PCB{
	uint8_t name[LINE_BUF_SIZE];
	uint8_t args[LINE_BUF_SIZE];
	uint32_t pid;
	uint32_t esp0;
	uint32_t ss0;
	uint32_t esp;
	uint32_t ebp;
	uint32_t terminal_id; //ranges from 1-3 depending on what terminal this program is executing in
 	struct file fd[8];	
	struct PCB * parent;
	//Register table
} PCB_t;

#define cur_pcb(addr)                   \
do {                                    \
	asm volatile("movl %%esp,%0      \n\
			andl %1,%0"			\
			: "=r" (addr)               \
			: "i"(PCB_MASK)                          \
			: "memory");   				\
} while(0)


#endif /* ASM */

#endif /* _TYPES_H */
