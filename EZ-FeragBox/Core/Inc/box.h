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
void box_handle_ferag_char(char data);
void box_handle_encoder(void);

void box_set_pgDelay(int delay);
void box_set_prodLen(int len);
void box_start(void);
void box_stop(void);

void box_printGo(void);

void box_reset_bx(void);

#endif /* INC_BOX_H_ */
