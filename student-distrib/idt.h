#ifndef _IDT_H
#define _IDT_H

#include "x86_desc.h"
#include "lib.h"
#include "RTC.h"
#include "i8259.h"
#include "debug.h"
#include "multiboot.h"



extern void initialize_idt();

extern void de_handler();

extern void db_handler();

extern void nmi_handler();
extern void bp_handler(); 
extern void of_handler();
extern void br_handler();
extern void ud_handler();
extern void nm_handler();
extern void df_handler();
extern void co_segment_overrun_handler();
extern void ts_handler();
extern void np_handler();
extern void ss_handler();
extern void gp_handler();
extern void pf_handler();
extern void mf_handler();
extern void ac_handler();
extern void mc_handler();
extern void xf_handler();
extern void generic_handler();
extern void pit_irq_handler();
#endif
