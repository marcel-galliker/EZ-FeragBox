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
#include "enc.h"
#include "term.h"
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "feragbox.h"

#include "usbd_cdc_if.h"

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

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim5;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
volatile uint8_t RxData;

S_FeragBoxStatus boxStatus;

// USB CDC handle
extern USBD_HandleTypeDef hUsbDeviceFS;

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
float ADCValueToTemperature(uint16_t adcValue);
uint8_t readDipSwitches(void);
uint8_t convertVoltageToRevision(float revisionVoltage);
void echoCommand(const char* args);
void statusCommand(const char* args);
void powerCommand(const char* args);
void encoderCommand(const char* args);
void printGoCommand(const char* args);

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
	uint16_t adcValue = 0;
	memset(&boxStatus, 0, sizeof(S_FeragBoxStatus));
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
  MX_USB_DEVICE_Init();
  MX_TIM3_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_TIM5_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  // Kick off asynchronous UART RCV
  HAL_UART_Receive_IT(&huart3, &RxData, 1);

  term_init();

  enc_init();

  /*
  terminal_register_command("echo", "Echoes the input", echoCommand);
  terminal_register_command("status", "Print System Status", statusCommand);
  terminal_register_command("power", "Control NUC and Display Power", powerCommand);
  terminal_register_command("encoder", "Control Encoder Status, Speed and Direction", encoderCommand);
  terminal_register_command("printgo", "PrintGo on / off control", printGoCommand);
*/

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	AD55936_init(&hi2c1, 0x10 << 1);
	while (1)
	{
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		term_process_input();

		// Update ADC Values
		AD5593R_ReadADC(&hi2c1, 0x10 << 1, 0, &adcValue);
		boxStatus.voltages.voltage24V = ((float)adcValue * 30.0) / 4095.0;
		AD5593R_ReadADC(&hi2c1, 0x10 << 1, 1, &adcValue);
		boxStatus.voltages.voltage12V = ((float)adcValue * 15.0) / 4095.0;
		AD5593R_ReadADC(&hi2c1, 0x10 << 1, 2, &adcValue);
		boxStatus.voltages.voltage12VNuc = ((float)adcValue * 15.0) / 4095.0;
		AD5593R_ReadADC(&hi2c1, 0x10 << 1, 3, &adcValue);
		boxStatus.voltages.voltage12VDisplay = ((float)adcValue * 15.0) / 4095.0;
		AD5593R_ReadADC(&hi2c1, 0x10 << 1, 4, &adcValue);
		boxStatus.voltages.voltage5V = ((float)adcValue * 6.25) / 4095.0;
		AD5593R_ReadADC(&hi2c1, 0x10 << 1, 5, &adcValue);
		boxStatus.voltages.voltage3V3 = ((float)adcValue * 4.125) / 4095.0;
		AD5593R_ReadADC(&hi2c1, 0x10 << 1, 6, &adcValue);
		boxStatus.voltages.voltagePcbRevision = ((float)adcValue * 4.125) / 4095.0;
		boxStatus.pcbRevision = convertVoltageToRevision(boxStatus.voltages.voltagePcbRevision);

		// Read ADC Internal Temperature Sensor
		AD5593R_ReadADC(&hi2c1, 0x10 << 1, 8, &adcValue);
		boxStatus.boardTemperature = ADCValueToTemperature(adcValue);

		// Update DIP Switch inputs
		boxStatus.dipSwitchStatus = readDipSwitches();

		// Update PrintGo and PrintDone Status
		boxStatus.printGoStatus = HAL_GPIO_ReadPin(PRINT_GO_GPIO_Port, PRINT_GO_Pin);
		boxStatus.printDoneStatus = !HAL_GPIO_ReadPin(PRINT_DONE_GPIO_Port, PRINT_DONE_Pin); // Signal is inverted

		// Update encoder generator parameters
		boxStatus.encoderSpeed = enc_get_speed();

		// Update Power Status
		boxStatus.nucPower = HAL_GPIO_ReadPin(NUC_PWR_EN_GPIO_Port, NUC_PWR_EN_Pin);
		boxStatus.displayPower = HAL_GPIO_ReadPin(DISPLAY_PWR_EN_GPIO_Port, DISPLAY_PWR_EN_Pin);
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_USART1
                              |RCC_PERIPHCLK_USART3|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  PeriphClkInit.USBClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
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

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  htim3.Init.Prescaler = 4800;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
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
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI1;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
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

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 38400;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
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

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

// UART RX Interrupt function override
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART3)
	{
		term_addChar(RxData);
		// Ready to receive the next byte
		HAL_UART_Receive_IT(&huart3, &RxData, 1);
	}
}


float ADCValueToTemperature(uint16_t adcValue) {
    const float ADC_25 = 819.0; // ADC value at 25 degrees Celsius
    const float SLOPE = 2.654; // ADC counts per degree Celsius
    float temperatureCelsius = 25.0 + ((float)adcValue - ADC_25) / SLOPE;
    return temperatureCelsius;
}


uint8_t readDipSwitches(void) {
    uint8_t dipStates = 0;

    if (HAL_GPIO_ReadPin(DIP_0_GPIO_Port, DIP_0_Pin) == GPIO_PIN_SET) dipStates |= 1 << 0;
    if (HAL_GPIO_ReadPin(DIP_1_GPIO_Port, DIP_1_Pin) == GPIO_PIN_SET) dipStates |= 1 << 1;
    if (HAL_GPIO_ReadPin(DIP_2_GPIO_Port, DIP_2_Pin) == GPIO_PIN_SET) dipStates |= 1 << 2;
    if (HAL_GPIO_ReadPin(DIP_3_GPIO_Port, DIP_3_Pin) == GPIO_PIN_SET) dipStates |= 1 << 3;
    if (HAL_GPIO_ReadPin(DIP_4_GPIO_Port, DIP_4_Pin) == GPIO_PIN_SET) dipStates |= 1 << 4;
    if (HAL_GPIO_ReadPin(DIP_5_GPIO_Port, DIP_5_Pin) == GPIO_PIN_SET) dipStates |= 1 << 5;

    return dipStates;
}


uint8_t convertVoltageToRevision(float revisionVoltage) {
    const float baseVoltage = 0.075f; // Base voltage (0.1V - 0.025V)
    const float increment = 0.1f; // Voltage increment per revision
    const float maxValidVoltage = 26.0f; // Example: max expected voltage, adjust based on your last revision

    // Check if voltage is invalid (too low or too high)
    if (revisionVoltage < baseVoltage || revisionVoltage > maxValidVoltage) {
        return 0; // Invalid voltage
    }

    // Calculate revision number, rounding down to nearest whole number
    uint8_t revision = (uint8_t)floor((revisionVoltage - baseVoltage) / increment) + 1;

    return revision;
}


// Terminal Echo command
void echoCommand(const char* args) {
	printf("%s\n", args);
}

//--- main_status ----------------------------------------
void main_status(void)
{
	printf("NUC Power:           %s\n", boxStatus.nucPower ? "ON" : "OFF");
	printf("Display Power:       %s\n", boxStatus.displayPower ? "ON" : "OFF");
	printf("Print Go Status:     %s\n", boxStatus.printGoStatus ? "ON" : "OFF");
	printf("Print Done Status:   %s\n", boxStatus.printDoneStatus ? "ON" : "OFF");

	// Printing dipSwitchStatus bit by bit
	printf("DIP Switch Status: MSB --> ");
	for (int i = 5; i >= 0; i--) {
		printf("%d", (boxStatus.dipSwitchStatus >> i) & 1);

		if(i == 4)
			printf(" ");
	}
	printf(" <-- LSB \n");

	// Encoder generator Settings
	printf("Encoder Speed:        %d Hz\n", boxStatus.encoderSpeed);

	// Encoder input status
	printf("Encoder Position:     %u\n", (unsigned int)boxStatus.encoderPosition);

	// Print Temperature
	printf("Board Temperature:    %.2f °C\n", boxStatus.boardTemperature);

	// Printing voltages
	printf("3.3V Voltage:         %.2fV (3.3V)\n", boxStatus.voltages.voltage3V3);
	printf("5V Voltage:           %.2fV (5V)\n",   boxStatus.voltages.voltage5V);
	printf("12V Voltage:          %.2fV (12V)\n",  boxStatus.voltages.voltage12V);
	printf("12V NUC Voltage:      %.2fV (12V)\n",  boxStatus.voltages.voltage12VNuc);
	printf("12V Display Voltage:  %.2fV (12V)\n",  boxStatus.voltages.voltage12VDisplay);
	printf("24V Voltage:          %.2fV (24V)\n",  boxStatus.voltages.voltage24V);
	printf("PCB Revision Voltage: %.2fV\n",        boxStatus.voltages.voltagePcbRevision);

	// PCB Revision
	if (boxStatus.pcbRevision == 0) {
		printf("PCB Revision: Invalid\n");
	} else {
		char pcbRev = 'A' + (boxStatus.pcbRevision * 10 - 1) / 10; // 0.1V increments starting at 'A'
		printf("PCB Revision: %c\n", pcbRev);
	}
}

//--- main_power -----------------------------------
void main_power(const char* args)
{
    char device[10]; // Buffer for the device name ("nuc" or "display")
    char action[4];  // Buffer for the action ("on" or "off")

    // Parse the command arguments
    int parsedItems = sscanf(args, "%9s %3s", device, action);
    if (parsedItems != 2) {
        printf("Invalid command. Use 'power nuc on/off' or 'power display on/off'\n");
        return;
    }

    if (strcmp(device, "nuc") == 0) {
        if (strcmp(action, "on") == 0) {
            HAL_GPIO_WritePin(NUC_PWR_EN_GPIO_Port, NUC_PWR_EN_Pin, GPIO_PIN_SET);
            printf("NUC power on\n");
        } else if (strcmp(action, "off") == 0) {
            HAL_GPIO_WritePin(NUC_PWR_EN_GPIO_Port, NUC_PWR_EN_Pin, GPIO_PIN_RESET);
            printf("NUC power off\n");
        } else {
            printf("Invalid action for NUC. Use 'on' or 'off'.\n");
        }
    } else if (strcmp(device, "display") == 0) {
        if (strcmp(action, "on") == 0) {
            HAL_GPIO_WritePin(DISPLAY_PWR_EN_GPIO_Port, DISPLAY_PWR_EN_Pin, GPIO_PIN_SET);
            printf("Display power on\n");
        } else if (strcmp(action, "off") == 0) {
            HAL_GPIO_WritePin(DISPLAY_PWR_EN_GPIO_Port, DISPLAY_PWR_EN_Pin, GPIO_PIN_RESET);
            printf("Display power off\n");
        } else {
            printf("Invalid action for display. Use 'on' or 'off'.\n");
        }
    } else {
        printf("Invalid device. Use 'nuc' or 'display'.\n");
    }
}

//--- main_encoder ---------------------------------------
void main_encoder(const char* args)
{
	int speed;
    int cnt=0;

    // Parse the command arguments for action type
    if (strstr(args, "start")) 		enc_start();
    else if (strstr(args, "stop"))	enc_stop();
    else if ((cnt=sscanf(args, "speed %d", &speed))) enc_set_speed(speed);
    else
    {
        printf("Unknown command. Use 'encoder start', 'encoder stop', or 'encoder speed ...'\n");
    }
}

//--- printGoCommand -------------------
void printGoCommand(const char* args) {
    char action[4];  // Buffer for the action ("on" or "off")

    // Parse the command arguments
    int parsedItems = sscanf(args, "%3s", action);
    if (parsedItems != 1) {
        printf("Invalid command. Use 'printgo on/off'\n");
        return;
    }

	if (strcmp(action, "on") == 0) {
		HAL_GPIO_WritePin(PRINT_GO_GPIO_Port, PRINT_GO_Pin, GPIO_PIN_SET);
		printf("PrintGo on\n");
	} else if (strcmp(action, "off") == 0) {
		HAL_GPIO_WritePin(PRINT_GO_GPIO_Port, PRINT_GO_Pin, GPIO_PIN_RESET);
		printf("PrintGo off\n");
	} else {
		printf("Invalid action. Use 'on' or 'off'.\n");
	}
}


// Retarget stdout to UART and CDC
WRITE_PROTOTYPE {
    int DataIdx;
    for (DataIdx = 0; DataIdx < len; DataIdx++) {
        // Transmit over USART3
        HAL_UART_Transmit(&huart3, (uint8_t*)&ptr[DataIdx], 1, HAL_MAX_DELAY);
    }

    // Check if USB CDC is connected before trying to transmit
    if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) {
        uint32_t startTick = HAL_GetTick();
        for (DataIdx = 0; DataIdx < len; DataIdx++) {
            // Attempt to transmit over CDC, with timeout
            while (CDC_Transmit_FS((uint8_t*)&ptr[DataIdx], 1) == USBD_BUSY) {
                if (HAL_GetTick() - startTick > CDC_TRANSMIT_TIMEOUT_MS) {
                    break; // Exit if timeout is reached
                }
            }
        }
    }

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
