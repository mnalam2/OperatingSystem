#ifndef _SYSCALL_LINKAGE_H
#define _SYSCALL_LINKAGE_H

extern void systemcall_linkage();

asm("\n\
.globl systemcall_linkage;\n\
.globl bad;\n\
\n\
.text;\n\
.align 4;\n\
\n\
systemcall_linkage:\n\
\n\
\n\
	cmpl $1, %eax;\n\
	jl bad;\n\
	cmpl $10, %eax;\n\
	jg bad;\n\
\n\
	pushl %ebp ;\n\
	pushl %edi;\n\
	pushl %esi ;\n\
	pushl %edx;\n\
	pushl %ecx;\n\
	pushl %ebx ;\n\
\n\
	call *syscall_jumptable(,%eax,4);\n\
\n\
	popl %ebx;\n\
	popl %ecx;\n\
	popl %edx;\n\
	popl %esi;\n\
	popl %edi;\n\
	popl %ebp;\n\
\n\
	iret ;\n\
\n\
bad:\n\
	movl $-1, %eax;\n\
	 \n\
	iret ;\n\
\n\
\n\
\n\
syscall_jumptable:\n\
	.long 0x0, halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn");
#endif


