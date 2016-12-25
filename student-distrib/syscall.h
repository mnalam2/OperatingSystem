#ifndef SYSCALL_H
#define SYSCALL_H

#define asmlinkage __attribute__((regparm(0)))
#define KERNEL_STACK_SIZE 0x2000 //8KB
#define MB4	0x00400000
#define KB8 0x2000
#define ELF_MAGIC_0 0x7f
#define KERNEL_TOP	0x00800000

#include "types.h"
#include "lib.h"
//struct file_operations fops_table[8];

asmlinkage int32_t halt (uint8_t status);
asmlinkage int32_t execute (const uint8_t* command);
asmlinkage int32_t read (int32_t fd, void* buf, int32_t nbytes);
asmlinkage int32_t write (int32_t fd, const void* buf, int32_t nbytes);
asmlinkage int32_t open (const uint8_t* filename);
asmlinkage int32_t close (int32_t fd);
asmlinkage int32_t getargs (uint8_t* buf, int32_t nbytes);
asmlinkage int32_t vidmap (uint8_t** screen_start);
asmlinkage int32_t set_handler (int32_t signum, void* handler_address);
asmlinkage int32_t sigreturn (void);




#endif
