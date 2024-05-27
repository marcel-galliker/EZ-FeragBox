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
static int 		    _TicksPgOff;
static SFeragMsg 	_FeragMsg;
static int			_FeragMsgIn, _FeragMsgOut;

#define TRACKING_CNT	8
static SProduct		_Tracking[TRACKING_CNT];
static int			_TrackInIdx, _TrackOutIdx;
static int			_PaceId;
static int			_PrintGoDelay=500;
static int			_EncoderPos;
static int			_PrinterReadyIn=-1;
static int			_PrintDoneDelay;

//--- prototypes ------------------

static void _check_system(void);
static void _handle_feragMsg(void);
static void _handle_encoder(void);
// static void _check_print_done(void);
static void _check_printer_ready(void);
static void _send_print_done(void);

//--- box_init -------------------------------
void box_init(void)
{
	memset(&_Status, 0, sizeof(_Status));
	_FeragMsgIn      = 0;
	_FeragMsgOut     = 0;
	_TrackInIdx  	 = 0;
	_TrackOutIdx 	 = 0;
	_PrintDoneDelay  = 0;
	_PrinterReadyIn  = -1;
	box_start();
	printf("LOG: box_init\n");
}

//--- box_set_pgDelay ------------------------------------
void box_set_pgDelay(int delay)
{
	_PrintGoDelay = delay;
	printf("set pgDelay=%d\n", delay);
}

//--- box_start -------------------------
void box_start(void)
{
	printf("start\n");
	_FeragMsgIn   = 0;
	_FeragMsgOut  = 0;
	_TrackInIdx  	  = 0;
	_TrackOutIdx 	  = 0;
	_Status.dtCnt = 0;
	_Status.pgCnt = 0;
	_Status.pdCnt = 0;
	_EncoderPos   = 0;
	_PrintDoneDelay = 0;
	_PrinterReadyIn  = -1;
	_PaceId		  = -1;
	box_send_status();
	_Running = TRUE;
	HAL_GPIO_WritePin(PRINT_GO_GPIO_Port, PRINT_GO_Pin, GPIO_PIN_RESET);
	enc_start();

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
	_Running = FALSE;
	enc_stop();
	HAL_GPIO_WritePin(PRINT_GO_GPIO_Port, PRINT_GO_Pin, GPIO_PIN_RESET);
}

//--- box_idle ----------------------------------------
void box_idle(void)
{
	_handle_feragMsg();
	_handle_encoder();
	_check_printer_ready();
//	if (!SIMULATION) _check_print_done();
}

//--- box_tick_10ms ------------------
void box_tick_10ms(int ticks)
{
	_Ticks = ticks;
	if (_TicksPgOff && _Ticks>_TicksPgOff)
	{
		HAL_GPIO_WritePin(PRINT_GO_GPIO_Port, PRINT_GO_Pin, GPIO_PIN_RESET);
	//	printf("PrintGo OFF %d\n",  _Ticks);
//		if (SIMULATION) _check_print_done();
		_TicksPgOff=0;
	}
	_check_system();
}

//--- _check_system -----------------
static void _check_system(void)
{
	_Status.voltages.voltage24V 			= adc_get_value(0, 30.0);
	_Status.voltages.voltage12V 			= adc_get_value(1, 15.0);
	_Status.voltages.voltage12VNuc 			= adc_get_value(2, 15.0);
	_Status.voltages.voltage12VDisplay 		= adc_get_value(3, 15.0);
	_Status.voltages.voltage5V 				= adc_get_value(4, 6.25);
	_Status.voltages.voltage3V3 			= adc_get_value(5, 4.125);
	_Status.voltages.voltagePcbRevision 	= adc_get_value(6, 4.125);
	_Status.pcbRevision 					= adc_get_revision(_Status.voltages.voltagePcbRevision);
	_Status.boardTemperature 				= adc_get_temp();

	// Update DIP Switch inputs
	_Status.dipSwitch						= gpio_get_dipswitches();

	// Update encoder generator parameters
	enc_get_status(&_Status.enc);

	// Update Power Status
	_Status.nucPower 		= HAL_GPIO_ReadPin(NUC_PWR_EN_GPIO_Port, NUC_PWR_EN_Pin);
	_Status.displayPower 	= HAL_GPIO_ReadPin(DISPLAY_PWR_EN_GPIO_Port, DISPLAY_PWR_EN_Pin);
}

//--- box_handle_ferag_char -----------------------------
void box_handle_ferag_char(char data)
{
	if (data & 0x80)
	{
		_FeragMsg.data[1]=data;
		_Status.paceId = _FeragMsg.paceId;
	//	printf("FERAG in 0x%02x 0x%02x, type=%d, info=%d, paceId=0x%02x\n", _FeragMsg.data[0], _FeragMsg.data[1], _FeragMsg.type, _FeragMsg.info, _FeragMsg.paceId);

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
		int idx;
		switch (_FeragMsg.type)
		{
		case 1:	idx=(_TrackInIdx+1)% TRACKING_CNT;
				if (idx==_TrackOutIdx)
					printf("ERROR: Tracking overflow\n");
				else
				{
					memcpy(&_Tracking[idx].prod, &_FeragMsg,  sizeof(_Tracking[idx].prod));
					_Tracking[idx].delay = _PrintGoDelay;
					_TrackInIdx=idx;
					if (_FeragMsg.info&1) printf("ProductDetect %d: PaceId=%d, \n", _Status.dtCnt, _FeragMsg.paceId);
					else printf("ProductDetect %d: PaceId=%d (EMPTY)\n", _Status.dtCnt, _FeragMsg.paceId);
					_Status.dtCnt++;
				}
				break;

		case 2:		printf("Alive\n"); break;
		default: 	printf("Unknown Message Type=%d\n", _FeragMsg.type); break;
		}
		_Status.feragMsgOutCnt++;
		_FeragMsgOut++;
	}
}

//--- _handle_encoder -------------------------------------
static void _handle_encoder(void)
{
	if (_EncoderPos!=_Status.enc.encOutPos)
	{
		_EncoderPos++;
	//	if (_EncoderPos%1000==0)
	//		printf("Enc=%d: DELAY=%d, %d, %d ,%d, %d, %d, %d, %d\n", _EncoderPos, _Tracking[0].delay, _Tracking[1].delay, _Tracking[2].delay, _Tracking[3].delay, _Tracking[4].delay, _Tracking[5].delay, _Tracking[6].delay, _Tracking[7].delay);
		for (int i=0; i<TRACKING_CNT; i++)
		{
			if (_Tracking[i].delay>0 && (--_Tracking[i].delay)==0)
			{
				_TrackOutIdx=i;
				printf("PrintGo PaceId=%d, ok=%d\n", _Tracking[i].prod.paceId, _Tracking[i].prod.info);
				box_printGo();
			}
		}
		if (_PrintDoneDelay>0)
		{
			if ((--_PrintDoneDelay)==0)
				_send_print_done();
		}
	}
}

//--- _check_print_done --------------------------------------------------
/*
static void _check_print_done(void)
{
	int pd=HAL_GPIO_ReadPin(PRINT_DONE_GPIO_Port, PRINT_DONE_Pin);
	if (_PrintDoneIn<0) _PrintDoneIn=pd;
	if (_PrintDoneIn!=pd)
		printf("FeragBox: PrintDone=%d, pgCnt=%d\n", pd, _Status.pgCnt);
	if (_PrintDoneIn>= 0 && pd && !_PrintDoneIn || SIMULATION)
	{
		printf("PRINT DONE: edge detected\n");
		_send_print_done();
	}
	_PrintDoneIn = pd;
}
*/

static void _check_printer_ready(void)
{
	int ready=HAL_GPIO_ReadPin(PRINT_DONE_GPIO_Port, PRINT_DONE_Pin);
	if (_PrinterReadyIn<0) _PrinterReadyIn=ready;
	if (_PrinterReadyIn!=ready)
		printf("LOG: FeragBox: PrinterReady=%d\n", ready);
	_PrinterReadyIn = ready;
}


//--- _send_print_done ----------------------------------------
static void _send_print_done(void)
{
	printf("PrintDone %d: PaceId[%d]=%d, ok=1\n", _Status.pdCnt, _TrackOutIdx, _Tracking[_TrackOutIdx].prod.paceId);
	_Status.pdCnt++;
	_PaceId = -1;
}

//--- box_printGo ----------------------
void box_printGo(void)
{
//	printf("PrintGo ON %d\n",  _Ticks);
	_Status.paceId = _Tracking[_TrackOutIdx].prod.paceId;
	if (enc_fixSpeed() || _Tracking[_TrackOutIdx].prod.info)
	{
		printf("PrintGo %d: PaceId[%d]=%d\n", _Status.pgCnt, _TrackOutIdx, _Tracking[_TrackOutIdx].prod.paceId);
		_Status.pgCnt++;
		_PaceId = _Tracking[_TrackOutIdx].prod.paceId;
		HAL_GPIO_WritePin(PRINT_GO_GPIO_Port, PRINT_GO_Pin, GPIO_PIN_SET);
		_TicksPgOff = _Ticks+2;
		if (_PrintDoneDelay) printf("ERROR: PringGo while still printing _PrintDoneDelay=%d\n",_PrintDoneDelay);
		_PrintDoneDelay = 2000;
	}
	else _PaceId=-1;
}

//--- box_send_status ----------------------------------------
void box_send_status(void)
{
//	printf("STATUS\n");
	/*
	printf("NUC Power:           %s\n", _Status.nucPower ? "ON" : "OFF");
	printf("Display Power:       %s\n", _Status.displayPower ? "ON" : "OFF");

	printf("DIP Switch: ");
	for (int i = 5; i >= 0; i--) printf("%d", (_Status.dipSwitch >> i) & 1);
	printf("\n");

	printf("Board Temperature:    %.2f °C\n", _Status.boardTemperature);

	printf("Voltage:          %.02fV (3.3V)\n", _Status.voltages.voltage3V3);
	printf("Voltage:          %.02fV (5V)\n",  _Status.voltages.voltage5V);
	printf("Voltage:          %.02fV (12V)\n", _Status.voltages.voltage12V);
	printf("NUC Voltage:      %.02fV (12V)\n", _Status.voltages.voltage12VNuc);
	printf("Display Voltage:  %.02fV (12V)\n", _Status.voltages.voltage12VDisplay);
	printf("Voltage:          %.02fV (24V)\n", _Status.voltages.voltage24V);
	printf("PCB Revision Voltage: %.02fV\n",   _Status.voltages.voltagePcbRevision);

	if (_Status.pcbRevision == 0) printf("PCB Revision: Invalid\n");
	else {
		char pcbRev = 'A' + (_Status.pcbRevision * 10 - 1) / 10; // 0.1V increments starting at 'A'
		printf("PCB Revision: %c\n", pcbRev);
	}

	// Encoder generator Settings
	printf("encIn Speed:  	%d Hz\n", (int)_Status.enc.encInSpeed);
	printf("encIn Pos:      %d\n",    (int)_Status.enc.encInPos);
	printf("encOut Speed:   %d Hz\n", (int)_Status.enc.encOutSpeed);
	printf("encOut Pos:     %d\n",    (int)_Status.enc.encOutPos);
	printf("PrintGo Cnt:    %d\n",    (int)_Status.pgCnt);
	printf("\n");
	 */
	char msg[512];
	_Status.test++;
	printf("STATUS %s\n", bin2hex(msg, &_Status, sizeof(_Status)));
	fflush(stdout);
}

