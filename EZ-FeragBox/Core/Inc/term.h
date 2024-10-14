/*
 * terminal.h
 *
 *  Created on: Mar 25, 2024
 *      Author: WPATR
 */

#ifndef INC_TERM_H_
#define INC_TERM_H_

#include <stddef.h>

void term_init(void);

void term_handle_char(char ch);
void term_idle(void);
void term_printf (const char *format, ...);

#endif /* INC_TERM_H_ */
