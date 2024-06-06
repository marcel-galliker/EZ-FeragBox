// *************************************************************************************************
//																				
//	EzFeragBox_def.h: 
//																				
// *************************************************************************************************
//
//  This is a program that receives commands over tcp/ip and drives a eZ-Inkjet
//
//
//
// *************************************************************************************************
//
//    Copyright 2024 Galliker-Engineering GmbH. All rights reserved.		
//    Written by marcel@galliker-engineering.ch											
//																				
//
// *************************************************************************************************

#pragma once

#include "ge_common.h"

#pragma pack(1)

typedef struct
{
	INT32 	encInSpeed;
	INT32 	encOutSpeed;
} SEZFB_EncStatus;

typedef struct
{
	UINT8 voltage3V3;
	UINT8 voltage5V;
	UINT8 voltage12V;
	UINT8 voltage12VNuc;
	UINT8 voltage12VDisplay;
	UINT8 voltage24V;
	UINT8 voltagePcbRevision;
} SEZFB_Voltage;

typedef struct
{
	UINT8 		flags;
	#define FLAG_nucPower		0x01
	#define FLAG_displayPower	0x02
	#define FLAG_tcp_connected	0x04

	UINT8			tcp_status;

	UINT8 			dipSwitch;
	UINT8 			boardTemperature;
	SEZFB_Voltage 	voltages;
	UINT8 			pcbRevision;

	SEZFB_EncStatus	enc;

	INT32			test;
	INT32			feragMsgInCnt;
	INT32			feragMsgOutCnt;
	INT32			paceId;
	INT32			dtCnt;
	INT32			aliveCnt;
	INT32			emptyGoCnt;
	INT32			emptyDoneCnt;
	INT32			pgCnt;
	INT32			pdCnt;
} SEZFB_Status;

#pragma pack()