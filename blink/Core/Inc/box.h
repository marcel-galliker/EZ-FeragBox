/*
 * box.h
 *
 *  Created on: Apr 15, 2024
 *      Author: marcel@galliker-engineering.ch
 */

#ifndef INC_BOX_H_
#define INC_BOX_H_

void box_init(void);
void box_idle(void);
void box_tick_10ms(int ticks);
void box_send_status(void);
void box_printGo(void);

void box_handle_char(char data);

#endif /* INC_BOX_H_ */
