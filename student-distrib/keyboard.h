/* keyboard.h - Defines used in interactions with the keyboard
 * controller
 */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"
#include "filesystem.h"

/* IO ports for each keyboard controller */
#define KEYBOARD_ENCODER_IN_BUF	 0x60
#define KEYBOARD_ENCODER_CMD_REG 0x60
#define KEYBOARD_CTRL_STATUS_REG 0x64
#define KEYBOARD_CTRL_CMD_REG	 0x64

/* IRQ Constant */
#define KEYBOARD_IRQ 	1

/* Bit masks for R/W to the status register */
#define KEYBOARD_CTRL_STATUS_MASK_OUT_BUF	 0x1 	//0000 0001
#define KEYBOARD_CTRL_STATUS_MASK_IN_BUF	 0x2 	//0000 0010
#define KEYBOARD_CTRL_STATUS_MASK_SYSTEM	 0x4 	//0000 0100
#define KEYBOARD_CTRL_STATUS_MASK_CMD_DATA	 0x8 	//0000 1000
#define KEYBOARD_CTRL_STATUS_MASK_LOCKED	 0x10 	//0001 0000
#define KEYBOARD_CTRL_STATUS_MASK_AUX_BUF	 0x20 	//0010 0000
#define KEYBOARD_CTRL_STATUS_MASK_TIMEOUT	 0x40 	//0100 0000
#define KEYBOARD_CTRL_STATUS_MASK_PARITY	 0x80 	//1000 0000

#define KEY_STATE_MASK 	0x7F
#define KEY_STATUS_MASK 0x80
/* Keyboard encoder commands */
#define KEYBOARD_ENCODER_CMD_SET_LED 		 0xED

/*Make codes of modifier keys*/
#define KEYBOARD_LEFT_SHIFT  0x2a
#define KEYBOARD_RIGHT_SHIFT 0x36
#define KEYBOARD_CAPS_LOCK   0x3a
#define KEYBOARD_ALT         0x38
#define KEYBOARD_NUM_LOCK    0x45
#define KEYBOARD_SCROLL_LOCK 0x46
#define KEYBOARD_ENTER 		 0x1C
#define KEYBOARD_LINEFEED 	 0x0A
#define KEYBOARD_BACKSPACE   0x0E
#define KEYBOARD_LEFT_CONTROL 0x1D
#define F1CODE				 0x3B
#define F2CODE				 0x3C
#define F3CODE				 0x3D
#define UPCODE 				 0x48
#define DOWNCODE 			 0x50

 /*Helpful constants*/
 #define MAX_COL_INDEX 	79
 #define MAX_ROW_INDEX 	24
 #define MAX_BUF_INDEX 	127
 #define LINE_BUF_SIZE	128
 #define BYTE_PER_CHAR 	1
 #define MASK_BIT_2 	4
 #define MASK_BIT_1 	2
 #define MASK_BIT_0 	1

/* Stores the current state of certain keys */
//bool numlock, scrolllock, capslock, shift, alt, ctrl, typingLine;
typedef struct terminal{
	char cmd_hist[5][LINE_BUF_SIZE]; //arbitrarily store the 5 most recent commands
	char line_buffer[LINE_BUF_SIZE];
	uint32_t line_buffer_index;
	bool typingLine;
	uint32_t c_x;
	uint32_t c_y;
	uint8_t cmdCount;
	char* video_mem;
} terminal_t;
uint8_t cur_terminal;

volatile terminal_t terminals[3];

/* Prepares driver for use */
extern void keyboard_install(int irq);

/* Read status from keyboard controller */
uint8_t keyboard_ctrl_read_status ();

/* Send command byte to keyboard controller */
void keyboard_ctrl_send_cmd(uint8_t cmd);

/* Read keyboard encoder buffer */
uint8_t keyboard_encoder_read_buf ();

/* Send command byte to keyboard encoder */
void keyboard_encoder_send_cmd(uint8_t cmd);

/* Set LEDs */
void keyboard_set_leds(bool numlock, bool scrolllock, bool capslock);

/* Interrupt handler */
extern void key_irq_handler();

/* Opens the terminal driver */
int32_t terminal_open(struct file *);

/* Closes the terminal driver */
int32_t terminal_close(struct file *);

/* Reads input from keyboard (128-char line) */
int32_t terminal_read(struct file * f , char * buf, uint32_t nbytes);

/* Write input to screen (characters pressed) */
int32_t terminal_write(struct file * f, const char * buf, uint32_t nbytes);

/* Function to switch between multiple terminals */
void switch_terminals(uint8_t term);

static struct file_operations stdin_ops __attribute__((unused)) = {
	.read = terminal_read,
	.write = none,
	.open = terminal_open,
	.close = terminal_close
};
static struct file_operations stdout_ops __attribute__((unused)) = {
	.read = none,
	.write = terminal_write,
	.open = terminal_open,
	.close = terminal_close
};
#endif
