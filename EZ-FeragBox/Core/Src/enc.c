/*
 * encoder_engine.c
 *
 *  Created on: Mar 25, 2024
 *      Author: marcel@galliker-engineering.ch
 */
// Encoder Output: TIM2
//
// Encoder Input: TIM5
// https://www.steppeschool.com/pages/blog/stm32-timer-encoder-mode

#include "ge_common.h"
#include "EzFeragBox_def.h"
#include "main.h"
#include "enc.h"

static uint32_t _Timer_clock_frequency;
static uint32_t _Prescaler;

static SEZFB_EncStatus _EncStatus;

static int   _Init=FALSE;
static int	 _Running=FALSE;
static float _EncIn_incPM  = 10240;	// impulese/ m
static float _EncOut_incPM = 128 * 1000.0 / 25.4; 	// impulese/ m (128 DPI)
static INT32 _EncInTime=0;
static INT32 _SpeedOutSet;
static INT32 _SpeedOutChange;
static INT32 _EncOutTime;
static INT32 _EncOutSpeedCnt=0;
static int 	 _FixedSpeed=0;

static void _init_pwm(int32_t speed);

//--- enc_init ----------------------------------
void enc_init(void)
{
	_Timer_clock_frequency = HAL_RCC_GetPCLK1Freq(); // Adjust based on your clock tree settings
	_Prescaler=1;
}

//--- enc_irq ------------------------
void enc_in_irq(TIM_HandleTypeDef *htim)
{
	int time=HAL_GetTick();
	int pos = _EncStatus.encInPos;
	_EncStatus.encInPos = enc_get_pos();
	int dist=_EncStatus.encInPos-pos;
	int t=time-_EncInTime;
	if (t==0) _EncStatus.encInSpeed=0;
	else _EncStatus.encInSpeed = (dist*1000)/t;

	_EncInTime=time;

	//--- set output speed ------
	enc_set_speed((int)(_EncStatus.encInSpeed*_EncOut_incPM/_EncIn_incPM));

	if (!_Running)
		enc_start();

//	printf("TRACE: Encoder In: pos=%d, speed=%d, time=%d\n", (int)_EncStatus.encInPos, (int)_EncStatus.encInSpeed, t);
}

//--- enc_tick_10ms ---------------------------
void enc_tick_10ms(int ticks)
{
	if (ticks-_EncOutTime>1000)
	{
		float t=(float)(ticks-_EncOutTime);
		_EncStatus.encOutSpeed = (int32_t) (1000.0*_EncOutSpeedCnt/t/2);
		_EncOutTime=ticks;
		_EncOutSpeedCnt=0;
	}
}

//--- enc_get_status -------------------------------
void enc_get_status(SEZFB_EncStatus *pstatus)
{
	memcpy(pstatus, &_EncStatus, sizeof(_EncStatus));
}

//--- enc_command ----------------------------------
void enc_command(const char *args)
{
    int cnt=0;

    // Parse the command arguments for action type
    if (strstr(args, "start")) 		enc_start();
    else if (strstr(args, "stop"))	enc_stop();
    else if ((cnt=sscanf(args, "speed %d", &_FixedSpeed))) enc_set_speed(_FixedSpeed);
    else
    {
        printf("Unknown command. Use 'encoder start', 'encoder stop', or 'encoder speed ...'\n");
    }
}

//--- enc_set_speed ---------------------------
void enc_set_speed(int32_t speed)
{
	if (_FixedSpeed) speed=_FixedSpeed;
	if (!_Init) _init_pwm(speed);
	else if (speed!=_SpeedOutSet)
	{
		_SpeedOutChange = speed;
		printf("Encoder Speedchange=%d\n", speed);
	}
}

//--- enc_fixSpeed ----------------------------
int  enc_fixSpeed(void)
{
	return _FixedSpeed;
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
		  _EncStatus.encOutPos++;
		  _EncOutSpeedCnt++;
		  if (_SpeedOutChange)// && !(_EncOutSpeedCnt&1)) // only the even counts to be sure we at at the end of a complete sequence
		  {
			  HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
			  HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
			  _Init=FALSE;
			  _init_pwm(_SpeedOutChange);
			  _SpeedOutSet = _SpeedOutChange;
			  _SpeedOutChange = 0;
		  }
	  }
  }
}

//--- enc_start ---------------------------
void enc_start(void)
{
	if (_FixedSpeed) printf("WARN: Encoder speed fixed to %d Hz\n", _FixedSpeed);

    // Apply the last configured settings and start PWM
	_EncStatus.encOutPos=0;
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    _Running=TRUE;
}

//--- enc_stop ---------------------------------
void enc_stop(void) {
    // Stop the PWM and ensure both outputs are low
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
    _Running=FALSE;
}
