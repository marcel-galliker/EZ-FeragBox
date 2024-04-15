/*
 * terminal.h
 *
 *  Created on: Mar 25, 2024
 *      Author: WPATR
 */

#ifndef INC_TERM_H_
#define INC_TERM_H_

#include <stddef.h>

#include "ring_buffer.h"

void term_addChar(char ch);
void term_init(void);
void term_process_input(void);
void term_register_command(const char *name, const char *description, void (*func)(const char*));

#endif /* INC_TERM_H_ */
