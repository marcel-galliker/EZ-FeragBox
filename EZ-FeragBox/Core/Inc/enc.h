/*
 * encoder_engine.h
 *
 *  Created on: Mar 25, 2024
 *      Author: WPATR
 */

#ifndef INC_ENCODER_ENGINE_H_
#define INC_ENCODER_ENGINE_H_


#include "stm32f3xx_hal.h"
#include "EzFeragBox_def.h"

extern int 	 	  EZ_EncoderOutPos;

void enc_command(const char *args);

// Initializes the encoder engine.
void enc_init(void);
void enc_tick_10ms(int ticks);
void enc_in_irq(TIM_HandleTypeDef *htim);
void enc_set_speed(int32_t speed);
int  enc_fixSpeed(void);
void enc_get_status(SEZFB_EncStatus *pstatus);

void enc_start(void);
void enc_stop(void);

#endif /* INC_ENCODER_ENGINE_H_ */
