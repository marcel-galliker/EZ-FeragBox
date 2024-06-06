/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <math.h>
#include "enc.h"
#include "box.h"
#include "term.h"

#include "AD5593.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define CDC_TRANSMIT_TIMEOUT_MS 5 // Timeout for CDC transmission retry before abort

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
// htim2: Encoder out clock
// htim3:
// htim5: Encoder counter
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim5;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
volatile uint8_t RxDataNUC;
volatile uint8_t RxDataFERAG;
#define NUC_FIFO_CNT 4
#define NUC_FIFO_SIZE	512
volatile uint8_t TxDataNuc[NUC_FIFO_CNT][NUC_FIFO_SIZE];
volatile uint8_t TxDataLenNuc[NUC_FIFO_CNT];

static int _NUC_InIdx=0;
static int _NUC_StartIdx=0;
static int _NUC_Busy=0;
static int _powerDisplay=12000;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_TIM3_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM5_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

#define WRITE_PROTOTYPE int _write(int file, char *ptr, int len)

// void powerCommand(const char* args);

static void _tick_10ms(int ticks);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART3_UART_Init();
  MX_TIM3_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_TIM5_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL);

  // Kick off asynchronous UART RCV
  HAL_UART_Receive_IT(&huart1, (uint8_t*)&RxDataFERAG, 1);
  HAL_UART_Receive_IT(&huart3, (uint8_t*)&RxDataNUC, 1);

  term_init();
  enc_init();
  box_init();

  power_nuc(TRUE);
//  power_display(TRUE);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	AD55936_init(&hi2c1, 0x10 << 1);
	int _ticks=0;
	while (1)
	{
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		int ticks= HAL_GetTick();

		while (ticks-_ticks>9)
		{
			_tick_10ms(ticks);
			_ticks=ticks;
			if (_powerDisplay && _ticks>_powerDisplay)
			{
				_powerDisplay=0;
				power_display(TRUE);
			}
		}
		box_idle();
		term_idle();
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_USART3
                              |RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x2000090E;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */
//	PWM for encoder output
  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 100000;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */
  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 7200;
  htim3.Init.CounterMode = TIM_COUNTERMODE_DOWN;
  htim3.Init.Period = 10000;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */
	if (HAL_TIM_Base_Start_IT(&htim3) != HAL_OK) {
		Error_Handler();
	}
	HAL_TIM_Base_Start(&htim3);
  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 0;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 4294967295;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 10;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 10;
  if (HAL_TIM_Encoder_Init(&htim5, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

//---------------------- UART1: Communication to FERAG --------------------------------

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 57600;
  huart1.Init.WordLength = UART_WORDLENGTH_9B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_ODD;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

//---------------------- UART3: Communication to NUC-PC --------------------------------

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(PRINT_GO_GPIO_Port, PRINT_GO_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, NUC_PWR_EN_Pin|DISPLAY_PWR_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ENCODER_A_GPIO_Port, ENCODER_A_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ENCODER_B_GPIO_Port, ENCODER_B_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : DIP_5_Pin DIP_4_Pin DIP_3_Pin DIP_2_Pin
                           DIP_1_Pin DIP_0_Pin */
  GPIO_InitStruct.Pin = DIP_5_Pin|DIP_4_Pin|DIP_3_Pin|DIP_2_Pin
                          |DIP_1_Pin|DIP_0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PRINT_GO_Pin */
  GPIO_InitStruct.Pin = PRINT_GO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(PRINT_GO_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PRINT_DONE_Pin */
  GPIO_InitStruct.Pin = PRINT_DONE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(PRINT_DONE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : NUC_PWR_EN_Pin DISPLAY_PWR_EN_Pin */
  GPIO_InitStruct.Pin = NUC_PWR_EN_Pin|DISPLAY_PWR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : ENCODER_A_Pin */
  GPIO_InitStruct.Pin = ENCODER_A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(ENCODER_A_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ENCODER_B_Pin */
  GPIO_InitStruct.Pin = ENCODER_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(ENCODER_B_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
//--- HAL_TIM_PeriodElapsedCallback -------------------------
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim==&htim3) 		enc_in_irq(htim);
	else if (htim==&htim2) 	enc_out_irq(htim);
}

//--- enc_get_pos -------------------------------------
int32_t	enc_get_pos(void)
{
	return __HAL_TIM_GET_COUNTER(&htim5);
}

// UART RX Interrupt function override
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1)
	{
		box_handle_ferag_char(RxDataFERAG);
		HAL_UART_Receive_IT(&huart1, (uint8_t*)&RxDataFERAG, 1);
	}
	else if (huart->Instance == USART3)
	{
		term_handle_char(RxDataNUC);
		HAL_UART_Receive_IT(&huart3, (uint8_t*)&RxDataNUC, 1);
	}
}

//--- ferag_send_char -----------------------
void ferag_send_char(char data)
{
	HAL_UART_Transmit(&huart1, (uint8_t*)&data, 1, HAL_MAX_DELAY);
}

//--- _tick_10ms ---------------------
static void _tick_10ms(int ticks)
{
	box_tick_10ms(ticks);
	enc_tick_10ms(ticks);
}

//--- adc_get_value --------------------------
float adc_get_value(int no, float factor)
{
	uint16_t val;
	AD5593R_ReadADC(&hi2c1, 0x10 << 1, no, &val);
	return ((float)val * factor) / 4095.0;
}

//--- adc_get_temp --------------------------------------
float adc_get_temp(void)
{
    const float ADC_25 = 819.0; // ADC value at 25 degrees Celsius
    const float SLOPE = 2.654; // ADC counts per degree Celsius
	uint16_t val;
	AD5593R_ReadADC(&hi2c1, 0x10 << 1, 8, &val);
    return 25.0 + ((float)val - ADC_25) / SLOPE;
}

//--- adc_get_revision ------------------------------------
uint8_t adc_get_revision(float val)
{
	const float baseVoltage = 0.075f; // Base voltage (0.1V - 0.025V)
    const float increment = 0.1f; // Voltage increment per revision
    const float maxValidVoltage = 26.0f; // Example: max expected voltage, adjust based on your last revision

    if (val < baseVoltage || val > maxValidVoltage) return 0; // Invalid voltage

    return (uint8_t)floor((val - baseVoltage) / increment) + 1;
}

//--- gpio_get_dipswitches -------------------------------
uint8_t gpio_get_dipswitches(void)
{
    uint8_t dipswitches = 0;

    if (HAL_GPIO_ReadPin(DIP_0_GPIO_Port, DIP_0_Pin) == GPIO_PIN_SET) dipswitches |= 1 << 0;
    if (HAL_GPIO_ReadPin(DIP_1_GPIO_Port, DIP_1_Pin) == GPIO_PIN_SET) dipswitches |= 1 << 1;
    if (HAL_GPIO_ReadPin(DIP_2_GPIO_Port, DIP_2_Pin) == GPIO_PIN_SET) dipswitches |= 1 << 2;
    if (HAL_GPIO_ReadPin(DIP_3_GPIO_Port, DIP_3_Pin) == GPIO_PIN_SET) dipswitches |= 1 << 3;
    if (HAL_GPIO_ReadPin(DIP_4_GPIO_Port, DIP_4_Pin) == GPIO_PIN_SET) dipswitches |= 1 << 4;
    if (HAL_GPIO_ReadPin(DIP_5_GPIO_Port, DIP_5_Pin) == GPIO_PIN_SET) dipswitches |= 1 << 5;

    return dipswitches;
}

//--- power_nuc -----------------------------
void    power_nuc(int on)
{
	if (on) HAL_GPIO_WritePin(NUC_PWR_EN_GPIO_Port, NUC_PWR_EN_Pin, GPIO_PIN_SET);
	else 	HAL_GPIO_WritePin(NUC_PWR_EN_GPIO_Port, NUC_PWR_EN_Pin, GPIO_PIN_RESET);
}

//--- power_display -------------------------
void    power_display(int on)
{
	if (on) HAL_GPIO_WritePin(DISPLAY_PWR_EN_GPIO_Port, DISPLAY_PWR_EN_Pin, GPIO_PIN_SET);
	else 	HAL_GPIO_WritePin(DISPLAY_PWR_EN_GPIO_Port, DISPLAY_PWR_EN_Pin, GPIO_PIN_RESET);
}

void _nuc_send_next()
{
	if (_NUC_InIdx!=_NUC_StartIdx && !_NUC_Busy)
	{
		int time=HAL_GetTick();
		_NUC_Busy = TRUE;
		HAL_UART_Transmit_IT(&huart3, TxDataNuc[_NUC_StartIdx], TxDataLenNuc[_NUC_StartIdx]); // NUC
		_NUC_StartIdx = (_NUC_StartIdx+1) % NUC_FIFO_CNT;
	    time=HAL_GetTick()-time;
	    if (time>1)
	    	printf("WARN: UART send time=%d\n", time);
	}
}

//--- HAL_UART_TxCpltCallback ---------------------------
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	_NUC_Busy = FALSE;
	_nuc_send_next();
}

//--- WRITE_PROTOTYPE ----------------------------------------------------
// Retarget stdout to UART and CDC
WRITE_PROTOTYPE {
	int idx = (_NUC_InIdx+1) % NUC_FIFO_CNT;
	__disable_irq();
	TxDataLenNuc[_NUC_InIdx] = len;
	memcpy(&TxDataNuc[_NUC_InIdx], ptr, len);
	_NUC_InIdx = idx;
	__enable_irq();

	_nuc_send_next();

	/*
	int time=HAL_GetTick();
    HAL_UART_Transmit_IT(&huart3, ptr, len); // NUC
  //  HAL_UART_Transmit_IT(&huart1, ptr, len); // debugging
    time=HAL_GetTick()-time;
    if (time>0)
    	printf("WARN: UART send time=%d\n", time);
    	*/
    return len; // Return the number of characters written
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
