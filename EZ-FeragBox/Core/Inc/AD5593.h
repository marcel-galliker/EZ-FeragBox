/*
 * AD5593.h
 *
 *  Created on: Mar 15, 2024
 *      Author: WPATR
 */

#ifndef INC_AD5593_H_
#define INC_AD5593_H_

HAL_StatusTypeDef AD55936_init(I2C_HandleTypeDef *hi2c, uint16_t DevAddress);
HAL_StatusTypeDef AD5593R_ReadADC(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t channel, uint16_t *adcValue);

#endif /* INC_AD5593_H_ */
