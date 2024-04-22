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
	UINT32 	encInPos;
	INT32 	encOutSpeed;
	UINT32	encOutPos;
} SEZFB_EncStatus;

typedef struct
{
	float voltage3V3;
	float voltage5V;
	float voltage12V;
	float voltage12VNuc;
	float voltage12VDisplay;
	float voltage24V;
	float voltagePcbRevision;
} SEZFB_Voltage;

typedef struct
{
	UINT8 			nucPower;
	UINT8 			displayPower;
	UINT8 			dipSwitch;
	float 			boardTemperature;
	SEZFB_Voltage 	voltages;
	UINT8 			pcbRevision;

	SEZFB_EncStatus	enc;

	INT32			test;
	INT32			paceId;
	INT32			dtCnt;
	INT32			pgCnt;
	INT32			pdCnt;
} SEZFB_Status;

#pragma pack()
