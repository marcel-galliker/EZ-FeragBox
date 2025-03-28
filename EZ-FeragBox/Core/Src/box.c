/*
 * box.c
 *
 *  Created on: Apr 15, 2024
 *      Author: marcel@galliker-engineering.ch
 */

#include <stdio.h>
#include <string.h>
#include "ge_common.h"
#include "EzFeragBox_def.h"
#include "main.h"
#include "term.h"
#include "enc.h"
#include "box.h"

#define SIMULATION FALSE

#pragma pack(1)
typedef struct SFeragMsg
{
	union
	{
		struct
		{
			//--- BYTE 1 ----------------------
			UINT8	info:4;
			UINT8 	type:3;
			UINT8	som:1; // start of message

			//--- BYTE 2 ----------------------
			UINT8	paceId:7;
			UINT8	eom:1;
		};
		UINT8 data[2];
	};
}SFeragMsg;

typedef struct SProduct
{
	int	 delay;
	SFeragMsg prod;
} SProduct;

#pragma pack()

//--- static variables --------------------------
static int			_Running;
static SEZFB_Status _Status;
static int 		    _Ticks;
static int			_TicksSysCheck;
static int			_TicksResetBX;
static SFeragMsg 	_FeragMsg;
static int			_FeragMsgIn, _FeragMsgOut;

#define TRACKING_CNT	16
static SProduct		_Tracking[TRACKING_CNT];
static int			_TrackIdx;
static int			_PaceCheck;
static int			_FeragCheck;
static int			_PrintGoDelay=500;
static int 		    _ProdLen=100;
static int 		    _PrintGoOffDelay=0;
static int			_EncoderPos;
static int			_LastPDPos;
static int			_LastSpeed;
static int			_PrinterDoneIn=0;
static int			_AwaitPrintDone;
static int			_PrintGoPos;
static int			_PrintDoneDelay;
static int			_PrintDonePeakFilter;
static int			_PrintDoneError;
static int 			_ErrorFlag;

//--- prototypes ------------------

static void _check_system(void);
static void _handle_feragMsg(void);
static void _check_print_done(void);
static void _send_print_done(void);

//--- box_init -------------------------------
void box_init(void)
{
	memset(&_Status, 0, sizeof(_Status));
	_Running 		 = FALSE;
	_FeragMsgIn      = 0;
	_FeragMsgOut     = 0;
	_TrackIdx 	 	 = 0;
	_PrintGoPos		 = 0;
	_PrintDoneDelay  = 0;
	_AwaitPrintDone  = FALSE;
	_PrinterDoneIn   = 0;
	_PaceCheck		 = -1;
	_FeragCheck		 = -1;
	box_start();
	nuc_printf("LOG: box_init\n");
}

//--- box_set_pgDelay ------------------------------------
void box_set_pgDelay(int delay)
{
	_PrintGoDelay = delay;
	_Status.pgDelay=delay;
}

//--- box_set_prodLen ------------------------------------
void box_set_prodLen(int len)
{
	_ProdLen = len;
	_Status.prodLen = len;
}

//--- box_start -------------------------
void box_start(void)
{
	nuc_printf("start\n");
	memset(_Tracking, 0, sizeof(_Tracking));
	_FeragMsgIn   	= 0;
	_FeragMsgOut  	= 0;
	_FeragCheck		= -1;
	_TrackIdx 		= 0;
	_Status.dtCnt 	= 0;
	_Status.pgCnt 	= 0;
	_Status.pdCnt 	= 0;
	_Status.emptyGoCnt = 0;
	_Status.emptyDoneCnt = 0;
	_Status.paceId   = 0;
	_EncoderPos   	 = 0;
	_LastPDPos		 = 0;
	_ErrorFlag		 = 0;
	_PrintDoneDelay  = 0;
	_AwaitPrintDone  = FALSE;
//	box_send_status();
	_Running = TRUE;
	HAL_GPIO_WritePin(PRINT_GO_GPIO_Port, PRINT_GO_Pin, GPIO_PIN_RESET);
	enc_start();
	_PrinterDoneIn = HAL_GPIO_ReadPin(PRINT_DONE_GPIO_Port, PRINT_DONE_Pin);
	nuc_printf("print-done[start]=%d\n", _PrinterDoneIn);

//	_Status.flags |= FLAG_encoder_running;
	//--- Simulation -----------------
	if (SIMULATION)
	{
		enc_set_speed(100);
		//--- PaceId 10 ----
		box_handle_ferag_char(0x11);
		box_handle_ferag_char(0x8a);
	}
}

//--- box_stop ----------------------------
void box_stop(void)
{
	if (_Running)
	{
		nuc_printf("stop\n");
		_Running = FALSE;
	//	_Status.flags &= ~FLAG_encoder_running;
	}
//	HAL_GPIO_WritePin(PRINT_GO_GPIO_Port, PRINT_GO_Pin, GPIO_PIN_RESET);
	enc_stop();
	_PrintDoneDelay  = 0;
	_AwaitPrintDone = FALSE;
	memset(_Tracking, 0, sizeof(_Tracking));
}

//--- box_idle ----------------------------------------
void box_idle(void)
{
	_handle_feragMsg();
}

//--- box_tick_10ms ------------------
void box_tick_10ms(int ticks)
{
	_Ticks = ticks;

//	_check_print_done();
	if (_Ticks > _TicksSysCheck)
	{
		_TicksSysCheck = _Ticks+500;
		_check_system();
		box_send_status();
	}
	if (_TicksResetBX && _Ticks > _TicksResetBX)
	{
		HAL_GPIO_WritePin(RESET_BX_GPIO_Port, RESET_BX_Pin, GPIO_PIN_RESET);
		_TicksResetBX=0;
	}
}

//--- box_reset_bx ----------------------------------------
void box_reset_bx(void)
{
	HAL_GPIO_WritePin(RESET_BX_GPIO_Port, RESET_BX_Pin, GPIO_PIN_SET);
	_TicksResetBX = _Ticks+200;
}

//--- _check_system -----------------
static void _check_system(void)
{
	_Status.voltages.voltage24V 			= (INT8)(10*adc_get_value(0, 30.0));
	_Status.voltages.voltage12V 			= (INT8)(10*adc_get_value(1, 15.0));
	_Status.voltages.voltage12VNuc 			= (INT8)(10*adc_get_value(2, 15.0));
	_Status.voltages.voltage12VDisplay 		= (INT8)(10*adc_get_value(3, 15.0));
	_Status.voltages.voltage5V 				= (INT8)(10*adc_get_value(4, 6.25));
	_Status.voltages.voltage3V3 			= (INT8)(10*adc_get_value(5, 4.125));
	_Status.voltages.voltagePcbRevision 	= (INT8)(10*adc_get_value(6, 4.125));
	_Status.pcbRevision 					= adc_get_revision(_Status.voltages.voltagePcbRevision);
	_Status.boardTemperature 				= (INT8)(10*adc_get_temp());

	// Update DIP Switch inputs
	_Status.dipSwitch						= gpio_get_dipswitches();

	// Update encoder generator parameters
	enc_get_status(&_Status.enc);

	// Update Power Status
//	_Status.flags=0;
//	if (HAL_GPIO_ReadPin(DISPLAY_PWR_EN_GPIO_Port, DISPLAY_PWR_EN_Pin)) _Status.flags |= FLAG_displayPower;
}

//--- box_handle_ferag_char -----------------------------
void box_handle_ferag_char(char data)
{
	if (data & 0x80)
	{
		_FeragMsg.data[1]=data;
		_Status.paceId = _FeragMsg.paceId;
	//	nuc_printf("FERAG in 0x%02x 0x%02x, type=%d, info=%d, paceId=0x%02x\n", _FeragMsg.data[0], _FeragMsg.data[1], _FeragMsg.type, _FeragMsg.info, _FeragMsg.paceId);

		_Status.feragMsgInCnt++;
		_FeragMsgIn++;
		ferag_send_char(0x80);
	}
	else
	{
		_FeragMsg.data[0]=data;
	}
}

//--- _handle_feragMsg ---------------------
static void _handle_feragMsg(void)
{
	if (_FeragMsgOut!=_FeragMsgIn)
	{
		switch (_FeragMsg.type)
		{
		case 1:	if (!_Running)
				{
					if (!_ErrorFlag&1)
						nuc_printf("ProductDetect pace=%d, ok=%d while encoder off\n", _FeragMsg.paceId, _FeragMsg.info);
					_ErrorFlag |= 1;
				}
				else if (_Status.dtCnt-_Status.pdCnt-_Status.emptyDoneCnt>=TRACKING_CNT)
				{
					if (!(_ErrorFlag&2))
					{
						nuc_printf("Tracking overflow dtCnt=%d, pdCnt=%d, eChnt=%d\n", _Status.dtCnt, _Status.pdCnt, _Status.emptyDoneCnt);
						if (EZ_EncoderInPos<100)
							nuc_printf("ERROR: Encoder input missing!\n");
						else
							nuc_printf("ERROR: Tracking overflow encIn=%d, encOut=%d, inSpeed=%d, outSpeed=%d, period=%d, cnt=%d\n", EZ_EncoderInPos, EZ_EncoderOutPos, _Status.enc.encInSpeed, _Status.enc.encOutSpeed);
					}
					_ErrorFlag |= 2;
				}
				else
				{
					if (_FeragCheck<0) _FeragCheck=_FeragMsg.paceId & 0x7F;
					else if (_FeragMsg.paceId!=_FeragCheck) nuc_printf("WARN: Ferag Pace[%03d] expected=%d\n", _FeragMsg.paceId, _FeragCheck);
					_FeragCheck = (_FeragMsg.paceId+1) & 0x7F;

					int idx  = _FeragMsg.paceId % TRACKING_CNT;
					memcpy(&_Tracking[idx].prod, &_FeragMsg,  sizeof(_Tracking[idx].prod));
					//--- speed compensation ----------------
					int corr=0;
					if (_Status.enc.encOutSpeed<5600)
					{
						// between 1'400 HZ and 4'200 Hz: Delay 100 Incs
						corr=110*(5600-_Status.enc.encOutSpeed)/5600;
					}
					int dist = _EncoderPos - _LastPDPos;
					int acc  = _Status.enc.encOutSpeed-_LastSpeed;
					_LastSpeed = _Status.enc.encOutSpeed;
					_LastPDPos = _EncoderPos;
					_Tracking[idx].delay = _PrintGoDelay+corr;
					nuc_printf("Speed=%d, corr=%d\n", _Status.enc.encOutSpeed, corr);
					nuc_printf("DT:%03d,%d dist=%d, EncIn=%d, EncOut=%d, inSpeed=%d, outSpeed=%d, acc=%d, corr=%d\n", _FeragMsg.paceId, _Tracking[idx].prod.info&1, dist, EZ_EncoderInPos, EZ_EncoderOutPos, _Status.enc.encInSpeed, _Status.enc.encOutSpeed, acc, corr);
					if (_Status.dtCnt && dist<100) nuc_printf("ERROR: Encoder input missing!\n");
					_Status.dtCnt++;
				}
				break;

		case 2:		_Status.aliveCnt++; break;
		default: 	nuc_printf("Unknown Message Type=%d\n", _FeragMsg.type);
					break;
		}
		_Status.feragMsgOutCnt++;
		_FeragMsgOut++;
	}
}

//--- _handle_encoder -------------------------------------
void box_handle_encoder(void)
{
	_EncoderPos++;
	_check_print_done();
//	if (_EncoderPos%1000==0)
//		nuc_printf("Enc=%d: DELAY=%d, %d, %d ,%d, %d, %d, %d, %d\n", _EncoderPos, _Tracking[0].delay, _Tracking[1].delay, _Tracking[2].delay, _Tracking[3].delay, _Tracking[4].delay, _Tracking[5].delay, _Tracking[6].delay, _Tracking[7].delay);
	for (int i=0; i<TRACKING_CNT; i++)
	{
		if (_Tracking[i].delay>0 && (--_Tracking[i].delay)==0)
		{
			_TrackIdx=i;
			nuc_printf("PrintGo Pace[%03d], ok=%d\n", _Tracking[i].prod.paceId, _Tracking[i].prod.info);
			box_printGo();
		}
	}
	if ((_PrintGoOffDelay>0) && (--_PrintGoOffDelay==0))
	{
		HAL_GPIO_WritePin(PRINT_GO_GPIO_Port, PRINT_GO_Pin, GPIO_PIN_RESET);
	}
	if ((_PrintDoneDelay>0) && (--_PrintDoneDelay==0))
	{
		_send_print_done();
	}
}

//--- _check_print_done --------------------------------------------------
static void _check_print_done(void)
{
	int printDone=HAL_GPIO_ReadPin(PRINT_DONE_GPIO_Port, PRINT_DONE_Pin);
	int dist= _EncoderPos-_PrintGoPos;

	if (printDone!=_PrinterDoneIn) nuc_printf("print-done[%d]=%d at %d, dist=%d\n", _Status.pgCnt, printDone, _EncoderPos, dist);
	if (!printDone) _PrintDonePeakFilter=0;
	if (_PrintDoneDelay)
	{
		if (dist<10)
		{
			if (!printDone)
			{
				if (!_PrintDoneError) nuc_printf("ERR: PRINT-DONE high expected\n");
				_PrintDoneError=TRUE;
			}
		}
		else if (dist>=_ProdLen-100)
		{
			_send_print_done();
		}
		else if (dist>35)
		{
			if (printDone) _PrintDonePeakFilter++;
			if (_PrintDonePeakFilter==5) nuc_printf("ERR: PRINT-DONE low expected\n");
		}
	}
	_PrinterDoneIn = printDone;
}

//--- _send_print_done ----------------------------------------
static void _send_print_done(void)
{
	_PrintDoneDelay=0;
	_PrintDonePeakFilter = 0;
	_PrintDoneError = 0;
	if (enc_fixSpeed() || _Tracking[_TrackIdx].prod.info&0x01)
	{
		nuc_printf("PD%d: PaceId[%d]=%d\n", _Status.pdCnt+_Status.emptyDoneCnt, _TrackIdx, _Tracking[_TrackIdx].prod.paceId);
		_Status.pdCnt++;
	}
	else
	{
		nuc_printf("ED%d: PaceId[%d]=%d\n", _Status.pdCnt+_Status.emptyDoneCnt, _TrackIdx, _Tracking[_TrackIdx].prod.paceId);
		_Status.emptyDoneCnt++;
	}
}

//--- box_printGo ----------------------
void box_printGo(void)
{
	static int lastpos=0;
	_Status.paceId = _Tracking[_TrackIdx].prod.paceId;
	int dist = _EncoderPos - lastpos;
	lastpos=_EncoderPos;
	if (enc_fixSpeed() || _Tracking[_TrackIdx].prod.info&0x01)
	{
		if (!_PrinterDoneIn) nuc_printf("ERR: PRINT-GO while printing\n");

		nuc_printf("PG%d: PaceId[%d]=%d, dist=%d, pos=%d, print-done=%d, _PrintDoneDelay=%d\n", _Status.pgCnt+_Status.emptyGoCnt, _TrackIdx, _Tracking[_TrackIdx].prod.paceId, dist, _EncoderPos, _PrinterDoneIn, _PrintDoneDelay);

		HAL_GPIO_WritePin(PRINT_GO_GPIO_Port, PRINT_GO_Pin, GPIO_PIN_SET);
		_PrintGoPos 	 = _EncoderPos;
		_PrintGoOffDelay = 10;
		_PrintDoneError  = FALSE;
		_PrintDoneDelay  = _ProdLen;
		_Status.pgCnt++;
	}
	else
	{
		nuc_printf("EG%d: PaceId[%d]=%d, dist=%d\n", _Status.pgCnt+_Status.emptyGoCnt, _TrackIdx, _Tracking[_TrackIdx].prod.paceId, dist);
		_PrintDoneDelay = 10;
		_Status.emptyGoCnt++;
	}
}

//--- box_send_status ----------------------------------------
void box_send_status(void)
{
	char msg[512];
	_Status.test++;
	nuc_printf("STATUS %s\n", bin2hex(msg, &_Status, sizeof(_Status)));
}

