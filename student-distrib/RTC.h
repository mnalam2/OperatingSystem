#ifndef _RTC_H
#define _RTC_H

#include "i8259.h"
#include "lib.h"
#include "syscall.h"

// Define MACROS
#define REGISTER_A	0x8A
#define REGISTER_B 	0x8B
#define REGISTER_C	0x8C

#define NMI			0x70
#define CMOS		0x71

#define MASKBIT6	0x40
#define RATEMASK	0xF0
#define IRQ_NUM		8
#define BYTE4		4
#define RTC_DEFAULT_RATE	0x0F
#define RATE1		0x06
#define RATE2		0x07	
#define RATE3		0x08	
#define RATE4		0x09	
#define RATE5		0x0A
#define RATE6		0x0B	
#define RATE7		0x0C	
#define RATE8		0x0D
#define RATE9		0x0E
#define FREQ1		1024
#define FREQ2		512	
#define FREQ3		256	
#define FREQ4		128	
#define FREQ5		64	
#define FREQ6		32
#define FREQ7		16
#define FREQ8		8
#define FREQ9		4
#define RTC_DEFAULT_FREQ	2										

// Define RTC functions
extern void RTC_init();
extern void rtc_irq_handler();
extern int32_t RTC_read(struct file *, char *, uint32_t);
extern int32_t RTC_open(struct file *);
extern int32_t RTC_write(struct file *, const char *, uint32_t);
extern int32_t RTC_close(struct file *);

static struct file_operations RTC_ops __attribute__((unused)) = {
	.read = RTC_read,
	.write = RTC_write,
	.open = RTC_open,
	.close = RTC_close
};

#endif
