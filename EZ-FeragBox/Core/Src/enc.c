/*
 * encoder_engine.c
 *
 *  Created on: Mar 25, 2024
 *      Author: marcel@galliker-engineering.ch
 */
// Encoder Output: TIM2
//	https://community.st.com/t5/stm32-mcus-products/quadrature-output-stm32f407/td-p/504824
//
// Encoder Input: TIM5
// 	https://www.steppeschool.com/pages/blog/stm32-timer-encoder-mode

#include "ge_common.h"
#include "EzFeragBox_def.h"
#include "main.h"
#include "box.h"
#include "enc.h"

static uint32_t _Timer_clock_frequency;
static uint32_t _Prescaler;

static SEZFB_EncStatus _EncStatus;

static int   _Init=FALSE;
static int	 _Running=FALSE;
static int	 _TimerRunning=FALSE;
static float _EncIn_incPM  = 10240;	// impulese/ m
static float _EncOut_incPM = 128 * 1000.0 / 25.4; 	// impulese/ m (128 DPI)
static INT32 _EncInTime=0;
int 	 	  EZ_EncoderInPos;
int 	 	  EZ_EncoderOutPos;
static INT32 _SpeedOutSet;
static INT32 _SpeedOutChange;
static INT32 _EncOutTime;
static INT32 _EncOutSpeedCnt=0;
static int 	 _FixedSpeed=0;

static void _set_speed(int32_t speed);

//--- enc_init ----------------------------------
void enc_init(void)
{
	_Timer_clock_frequency = HAL_RCC_GetPCLK1Freq(); // Adjust based on your clock tree settings
	_Prescaler=1;
	EZ_EncoderInPos = 0;
}

//--- enc_irq ------------------------
void enc_in_irq(TIM_HandleTypeDef *htim)
{
	int time=HAL_GetTick();
	int pos = EZ_EncoderInPos;
	EZ_EncoderInPos = enc_get_pos();
	int dist=EZ_EncoderInPos-pos;
	int t=time-_EncInTime;
	if (t==0) _EncStatus.encInSpeed=0;
	else
		_EncStatus.encInSpeed = (dist*1000)/t;

	_EncInTime=time;

	//--- set output speed ------
	int speed = (int)(_EncStatus.encInSpeed*_EncOut_incPM/_EncIn_incPM/2);
//	nuc_printf("encoderIn: speed=%d, FixedSpeed=%d, TimerRunning=%d\n", speed, _FixedSpeed, _TimerRunning);
	if (!_FixedSpeed)
		enc_set_speed(speed);
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
		if (_FixedSpeed==777 && _SpeedOutSet<20000)
		{
			_SpeedOutSet    += 1000;
			_SpeedOutChange = TRUE;
			nuc_printf("LOG: SetSpeed=%d Hz\n", _SpeedOutSet);
		}
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
    else if ((cnt=sscanf(args, "speed %d", &_FixedSpeed)))
    {
    //	nuc_printf("LOG: enc_command speed=%d Hz\n", _FixedSpeed);
    	enc_set_speed(_FixedSpeed);
    }
    else
    {
    	nuc_printf("Unknown command. Use 'encoder start', 'encoder stop', or 'encoder speed ...'\n");
    }
}

//--- enc_set_speed ---------------------------
void enc_set_speed(int32_t speed)
{
	if (_FixedSpeed) speed=_FixedSpeed;
	if (!_Init) _set_speed(speed);
	else if (speed!=_SpeedOutSet)
	{
		_SpeedOutSet = speed;
		_SpeedOutChange = TRUE;
	//	nuc_printf("Encoder Speedchange=%d\n", speed);
		if (!_TimerRunning) _set_speed(speed);
	}
	_Init=TRUE;
}

//--- enc_fixSpeed ----------------------------
int  enc_fixSpeed(void)
{
	return _FixedSpeed;
}

//--- _set_speed ------------------------------
static void _set_speed(int32_t speed)
{
	if (htim2.Instance)
	{
	//	nuc_printf("_set_speed(%d)\n", speed);
		if (speed<10)
		{
		//	nuc_printf("LOG: encoder HAL_TIM_Base_Stop\n");
			HAL_TIM_Base_Stop(&htim2);
			_TimerRunning = FALSE;
		}
		else
		{
			uint32_t period = (((_Timer_clock_frequency*2) / (_Prescaler * speed)) / 1) - 1;

			TIM2->ARR = period;

			if (!_TimerRunning)
			{
				_TimerRunning = TRUE;
			//	nuc_printf("LOG: encoder HAL_TIM_Base_Start period=%d\n", period);
				if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK) {
					Error_Handler();
				}
				HAL_TIM_Base_Start(&htim2);
			}
		}
	}
}

int enc_aar(void)
{
	return TIM2->ARR;
}

int enc_cnt(void)
{
	return TIM2->CNT;
}

//--- enc_out_irq -------------------------------
void enc_out_irq(TIM_HandleTypeDef *htim)
{
  EZ_EncoderOutPos++;
  _EncOutSpeedCnt++;
  switch(EZ_EncoderOutPos&0x03)
  {
  case 0: ENCODER_A_GPIO_Port->BRR  = ENCODER_A_Pin; break;
  case 1: ENCODER_B_GPIO_Port->BRR  = ENCODER_B_Pin; break;
  case 2: ENCODER_A_GPIO_Port->BSRR = ENCODER_A_Pin; break;
  case 3: ENCODER_B_GPIO_Port->BSRR = ENCODER_B_Pin; break;
  }
  if (_SpeedOutChange)
	  _set_speed(_SpeedOutSet);
  _SpeedOutChange = FALSE;
  box_handle_encoder();
}

//--- enc_start ---------------------------
void enc_start(void)
{
	_Running = TRUE;
	EZ_EncoderOutPos=0;
	ENCODER_A_GPIO_Port->BSRR  = ENCODER_A_Pin;
	ENCODER_B_GPIO_Port->BSRR  = ENCODER_B_Pin;
//	box_init();
}

//--- enc_stop ---------------------------------
void enc_stop(void) {
	if (_Running)
	{
		_SpeedOutSet=0;
		_SpeedOutChange=TRUE;
		ENCODER_A_GPIO_Port->BSRR  = ENCODER_A_Pin;
		ENCODER_B_GPIO_Port->BSRR  = ENCODER_B_Pin;
	}
	_Running = FALSE;
}
