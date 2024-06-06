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
static int _EncoderInPos;

static int   _Init=FALSE;
static int	 _Running=FALSE;
static float _EncIn_incPM  = 10240;	// impulese/ m
static float _EncOut_incPM = 128 * 1000.0 / 25.4; 	// impulese/ m (128 DPI)
static INT32 _EncInTime=0;
static int 	 _EncoderInPos;
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
	_EncoderInPos = 0;
}

//--- enc_irq ------------------------
void enc_in_irq(TIM_HandleTypeDef *htim)
{
	int time=HAL_GetTick();
	int pos = _EncoderInPos;
	_EncoderInPos = enc_get_pos();
	int dist=_EncoderInPos-pos;
	int t=time-_EncInTime;
	if (t==0) _EncStatus.encInSpeed=0;
	else _EncStatus.encInSpeed = (dist*1000)/t;

	_EncInTime=time;

	//--- set output speed ------
	enc_set_speed((int)(_EncStatus.encInSpeed*_EncOut_incPM/_EncIn_incPM));

//	if (!_Running)
//		enc_start();

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
  //  if (strstr(args, "start")) 		enc_start();
  //  else if (strstr(args, "stop"))	enc_stop();
  //  else
    if ((cnt=sscanf(args, "speed %d", &_FixedSpeed)))
    {
    	printf("LOG: enc_command speed=%d Hz\n", _FixedSpeed);
    	enc_set_speed(_FixedSpeed);
    }
    else
    {
        printf("Unknown command. Use 'encoder start', 'encoder stop', or 'encoder speed ...'\n");
    }
}

//--- enc_set_speed ---------------------------
void enc_set_speed(int32_t speed)
{
	if (_FixedSpeed) speed=_FixedSpeed;
	if (!_Init) _set_speed(speed);
	else if (speed!=_SpeedOutSet)
	{
		_SpeedOutChange = TRUE;
		_SpeedOutSet = speed;
		printf("Encoder Speedchange=%d\n", speed);
	}
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
		if (speed==0)
		{
			HAL_TIM_Base_Stop(&htim2);
			_Running = FALSE;
		}
		else
		{
			uint32_t period = ((_Timer_clock_frequency / (_Prescaler * speed)) / 2) - 1;

			TIM2->CNT = period;

			if (!_Running)
			{
				_Running = TRUE;
				HAL_TIM_Base_Start_IT(&htim2);
			}
		}
	}
}

//--- HAL_TIM_QUARTER_PulseFinishedCallback -------------------------------
void HAL_TIM_QUARTER_PulseFinishedCallback(TIM_HandleTypeDef *htim)
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
}

//--- enc_start ---------------------------
void enc_start(void)
{
	if (_FixedSpeed) printf("WARN: Encoder speed fixed to %d Hz\n", _FixedSpeed);

    // Apply the last configured settings and start PWM
	EZ_EncoderOutPos=0;
}

//--- enc_stop ---------------------------------
void enc_stop(void) {
    // Stop the PWM and ensure both outputs are low
	if (_Running)
	{
		_SpeedOutSet=0;
		_SpeedOutChange=TRUE;
		HAL_GPIO_WritePin(ENCODER_A_GPIO_Port, ENCODER_A_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(ENCODER_B_GPIO_Port, ENCODER_A_Pin, GPIO_PIN_SET);
	}
}
