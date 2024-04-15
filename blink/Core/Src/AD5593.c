/*
 * AD5593.c
 *
 *  Created on: Mar 15, 2024
 *      Author: WPATR
 */

#include <stm32f3xx_hal.h>

//Definitions
#define _ADAC_NULL           0b00000000
#define _ADAC_ADC_SEQUENCE   0b00000010 // ADC sequence register - Selects ADCs for conversion
#define _ADAC_GP_CONTROL     0b00000011 // General-purpose control register - DAC and ADC control register
#define _ADAC_ADC_CONFIG     0b00000100 // ADC pin configuration - Selects which pins are ADC inputs
#define _ADAC_DAC_CONFIG     0b00000101 // DAC pin configuration - Selects which pins are DAC outputs
#define _ADAC_PULL_DOWN      0b00000110 // Pull-down configuration - Selects which pins have an 85 kO pull-down resistor to GND
#define _ADAC_LDAC_MODE      0b00000111 // LDAC mode - Selects the operation of the load DAC
#define _ADAC_GPIO_WR_CONFIG 0b00001000 // GPIO write configuration - Selects which pins are general-purpose outputs
#define _ADAC_GPIO_WR_DATA   0b00001001 // GPIO write data - Writes data to general-purpose outputs
#define _ADAC_GPIO_RD_CONFIG 0b00001010 // GPIO read configuration - Selects which pins are general-purpose inputs
#define _ADAC_POWER_REF_CTRL 0b00001011 // Power-down/reference control - Powers down the DACs and enables/disables the reference
#define _ADAC_OPEN_DRAIN_CFG 0b00001100 // Open-drain configuration - Selects open-drain or push-pull for general-purpose outputs
#define _ADAC_THREE_STATE    0b00001101 // Three-state pins - Selects which pins are three-stated
#define _ADAC_RESERVED       0b00001110 // Reserved
#define _ADAC_SOFT_RESET     0b00001111 // Software reset - Resets the AD5593R

/**
 * @name     ADAC Configuration Data Bytes
 ******************************************************************************/
 ///@{
 //write into MSB after _ADAC_POWER_REF_CTRL command to enable VREF
#define _ADAC_VREF_ON     0b00000010
#define _ADAC_SEQUENCE_ON 0b00000010



/**
 * @name   ADAC Write / Read Pointer Bytes
******************************************************************************/
///@{
#define _ADAC_DAC_WRITE       0b00010000
#define _ADAC_ADC_READ        0b01000000
#define _ADAC_DAC_READ        0b01010000
#define _ADAC_GPIO_READ       0b01100000
#define _ADAC_REG_READ        0b01110000

HAL_StatusTypeDef AD55936_init(I2C_HandleTypeDef *hi2c, uint16_t DevAddress) {
	HAL_StatusTypeDef status;
	uint8_t data[3];

	data[0] = _ADAC_GP_CONTROL;
	data[1] = 0x01;
	data[2] = 0x00;

	status = HAL_I2C_Master_Transmit(hi2c, DevAddress, data, 3, HAL_MAX_DELAY);

	if(status != HAL_OK)
		return status;

	data[0] = _ADAC_POWER_REF_CTRL;
	data[1] = 0x02;
	data[2] = 0x00;

	status = HAL_I2C_Master_Transmit(hi2c, DevAddress, data, 3, HAL_MAX_DELAY);

	if(status != HAL_OK)
		return status;

	data[0] = _ADAC_ADC_CONFIG;
	data[1] = 0;
	data[2] = 0xFF;

	status = HAL_I2C_Master_Transmit(hi2c, DevAddress, data, 3, HAL_MAX_DELAY);

	return status;
}

HAL_StatusTypeDef AD5593R_ReadADC(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t channel, uint16_t *adcValue) {
    uint16_t channelBit = 1 << channel;

	if (channel > 8) {
        // Invalid channel, return error (0-7 = ADC Inputs, 8 = Internal Temperature Sensor)
        return HAL_ERROR;
    }

    HAL_StatusTypeDef status;
    uint8_t command[3];
    uint8_t data[2];

    // Configure the ADC channel
    command[0] = _ADAC_ADC_SEQUENCE;

    // Select Channel
    command[1] = (uint8_t)(channelBit >> 8);
    command[2] = (uint8_t)(channelBit & 0xFF);

    status = HAL_I2C_Master_Transmit(hi2c, DevAddress, command, sizeof(command), HAL_MAX_DELAY);

    if (status != HAL_OK) {
        // Handle error here
        return status;
    }

	// Initiate an ADC read
	command[0] = _ADAC_ADC_READ;

    // Begin transmission to set the ADC read pointer
    status = HAL_I2C_Master_Transmit(hi2c, DevAddress, command, 1, HAL_MAX_DELAY);

    if (status != HAL_OK) {
        // Handle error here
        return status;
    }

    // Read the ADC value (2 bytes)
    status = HAL_I2C_Master_Receive(hi2c, DevAddress, data, 2, HAL_MAX_DELAY);

    if (status != HAL_OK) {
        // Handle error here
        return status;
    }

    // Convert the two received bytes into a single 16-bit value
    *adcValue = (uint16_t)((data[0] & 0x0F) << 8) + data[1];

    return status;
}
