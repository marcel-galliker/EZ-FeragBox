/*
 * terminal.h
 *
 *  Created on: Apr 12, 2024
 *      Author: marcel@galliker-engineering.ch
 */

#ifndef INC_TERM_H_
#define INC_TERM_H_

#include <stddef.h>

void term_init(void);

void term_handle_char(char ch);
void term_idle(void);

#endif /* INC_TERM_H_ */
