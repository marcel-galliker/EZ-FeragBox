/*
 * encoder_engine.h
 *
 *  Created on: Mar 25, 2024
 *      Author: WPATR
 */

#ifndef INC_ENCODER_ENGINE_H_
#define INC_ENCODER_ENGINE_H_


#include "stm32f3xx_hal.h"

// Initializes the encoder engine.
void enc_init(void);

// Starts generating the encoder signal with the previously configured parameters.
void enc_start(void);

// Stops the encoder signal, setting both outputs to low.
void enc_stop(void);

// Configures the encoder simulation parameters.
// speed: Frequency of the PWM signal.
// direction: 0 for forward, non-zero for reverse.
void enc_set_speed(int32_t speed);

// Get current speed
int32_t enc_get_speed(void);

#endif /* INC_ENCODER_ENGINE_H_ */
