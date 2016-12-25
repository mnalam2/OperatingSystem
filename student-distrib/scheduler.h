#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "paging.h"
#include "types.h"
uint8_t active[MAX_USER_PROG];

void init_PIT();
void sched();
uint32_t get_next_proc(uint32_t curr);
void show_status();
#endif
