/*
 * encoder_engine.c
 *
 *  Created on: Mar 25, 2024
 *      Author: WPATR
 */

#include "utils.h"
#include "main.h"
#include "enc.h"

static uint32_t _Timer_clock_frequency;
static uint32_t _Prescaler;

static int     _Init=FALSE;
static int32_t _SpeedSet;
static int32_t _SpeedChange;
static int32_t _Speed = 0;
static int32_t _Counter=0;
static int32_t _time;
static int32_t _cnt=0;

static void _init_pwm(int32_t speed);

//--- enc_init ----------------------------------
void enc_init(void)
{
	_Timer_clock_frequency = HAL_RCC_GetPCLK1Freq(); // Adjust based on your clock tree settings
	_Prescaler=1;
}

//--- enc_tick_10ms ---------------------------
void enc_tick_10ms(int ticks)
{
	if (ticks-_time>1000)
	{
		float cnt=1000*(float)(_Counter-_cnt);
		float t=(float)(ticks-_time);
		_Speed = (int32_t) (cnt/t);
		_cnt = _Counter;
		_time=ticks;
	}
}

//--- enc_command ----------------------------------
void enc_command(const char *args)
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


//--- enc_set_speed ---------------------------
void enc_set_speed(int32_t speed)
{
	if (!_Init) _init_pwm(speed);
	else if (speed!=_SpeedSet) _SpeedChange = speed;
}

//--- _init_pwm ------------------------------
static void _init_pwm(int32_t speed)
{
	if (_Prescaler * speed == 0) return;

	int reverse;
	if (speed>0)
	{
		reverse=0;
	}
	else
	{
		reverse=1;
		speed=-speed;
	}

	uint32_t period = (_Timer_clock_frequency / (_Prescaler * speed)) - 1;

	// Update Timer configuration
	htim2.Instance 				 = TIM2;
	htim2.Init.Prescaler 		 = _Prescaler - 1; // Prescaler is 0-based, for prescaler = 1, this will be 0
	htim2.Init.CounterMode 		 = TIM_COUNTERMODE_UP;
	htim2.Init.Period 			 = period;
	htim2.Init.ClockDivision 	 = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.RepetitionCounter = 0;
	HAL_TIM_PWM_Init(&htim2);

	TIM_OC_InitTypeDef sConfigOC = {0};
	sConfigOC.OCMode 		= TIM_OCMODE_TOGGLE;
	sConfigOC.OCIdleState 	= TIM_OUTPUTSTATE_ENABLE;
	sConfigOC.OCPolarity 	= TIM_OCPOLARITY_HIGH;

	// Apply direction
	if (reverse)
	{ // Reverse
		sConfigOC.Pulse = (period * 3) / 4; // CH1 at 25%
		HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);
		sConfigOC.Pulse = (period * 1) / 4; // CH2 at 75%, ie half cycle later
		HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2);
	}
	else
	{ // Forward
		sConfigOC.Pulse = (period * 1) / 4; // CH1 at 25%
		HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);
		sConfigOC.Pulse = (period * 3) / 4; // CH2 at 75%, ie half cycle later
		HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2);
	}

	if(HAL_TIM_PWM_Start_IT(&htim2, TIM_CHANNEL_1)!=HAL_OK)
	{
	   Error_Handler();
	}
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
	_Init = TRUE;
}

//--- HAL_TIM_PWM_PulseFinishedCallback -------------------------------
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
  if(htim -> Instance == TIM2)
  {
	  if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
	  {
		  _Counter++;
		  if (_SpeedChange)
		  {
			  HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
			  HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
			  _Init=FALSE;
			  _init_pwm(_SpeedChange);
			  _SpeedChange = 0;
		  }
	  }
  }
}

//--- enc_get_counter ----------------------------------
int32_t enc_get_counter(void)
{
	return _Counter;
}

//--- enc_start ---------------------------
void enc_start(void)
{
    // Apply the last configured settings and start PWM
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

//--- enc_stop ---------------------------------
void enc_stop(void) {
    // Stop the PWM and ensure both outputs are low
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
}

//--- enc_get_speed ------------------------
int32_t enc_get_speed(void)
{
	return _Speed;
}

