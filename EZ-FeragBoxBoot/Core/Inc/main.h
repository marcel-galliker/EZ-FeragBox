/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ge_common.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void 	nuc_printf (const char *format, ...);
void 	jump_to(UINT32 addr);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DIP_5_Pin GPIO_PIN_2
#define DIP_5_GPIO_Port GPIOA
#define DIP_4_Pin GPIO_PIN_3
#define DIP_4_GPIO_Port GPIOA
#define DIP_3_Pin GPIO_PIN_4
#define DIP_3_GPIO_Port GPIOA
#define DIP_2_Pin GPIO_PIN_5
#define DIP_2_GPIO_Port GPIOA
#define DIP_1_Pin GPIO_PIN_6
#define DIP_1_GPIO_Port GPIOA
#define PRINT_GO_Pin GPIO_PIN_8
#define PRINT_GO_GPIO_Port GPIOE
#define PRINT_DONE_Pin GPIO_PIN_9
#define PRINT_DONE_GPIO_Port GPIOE
#define DIP_0_Pin GPIO_PIN_8
#define DIP_0_GPIO_Port GPIOA
#define RESET_BX_Pin GPIO_PIN_6
#define RESET_BX_GPIO_Port GPIOF
#define DISPLAY_PWR_EN_Pin GPIO_PIN_7
#define DISPLAY_PWR_EN_GPIO_Port GPIOF
#define ENCODER_A_Pin GPIO_PIN_15
#define ENCODER_A_GPIO_Port GPIOA
#define ENCODER_B_Pin GPIO_PIN_3
#define ENCODER_B_GPIO_Port GPIOB
#define I2C1_SCL_Pin GPIO_PIN_6
#define I2C1_SCL_GPIO_Port GPIOB
#define I2C1_SDA_Pin GPIO_PIN_7
#define I2C1_SDA_GPIO_Port GPIOB
#define UART_NUC_TX_Pin GPIO_PIN_8
#define UART_NUC_TX_GPIO_Port GPIOB
#define UART_NUC_RX_Pin GPIO_PIN_9
#define UART_NUC_RX_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
