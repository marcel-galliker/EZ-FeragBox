#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "ge_common.h"
#include "term.h"
#include "main.h"

static UINT64 _Data[128/8];

//--- prototypes ---------------------------------------------
static void _bl_version(char *args);
static void _bl_read(char *args);
static void _bl_write(char *args);
static void _bl_erase(char *args);

//--- bl_handle_cmd --------------------------------------
int bl_handle_cmd(char *cmd)
{
	char *args;

	if      ((args=strstart(cmd, "BL_VERSION")))  		_bl_version(args);
	else if ((args=strstart(cmd, "BL_RD"))) 		 	_bl_read(args);
	else if ((args=strstart(cmd, "BL_WR"))) 		 	_bl_write(args);
	else if ((args=strstart(cmd, "BL_ERASE"))) 		 	_bl_erase(args);
	else if ((args=strstart(cmd, "BL_START FeragBox"))) jump_to(0x8008000);
	else if (strlen(cmd)) return FALSE;
	return TRUE;
}

//--- _bl_version ------------------------------
static void _bl_version(char *args)
{
	unsigned int addr;
	sscanf(args, "0x%08x", &addr);
	nuc_printf("BL_VERSION %s\n", bin2hex((char*)_Data, (void*)addr, 8));
}

//--- _bl_read ----------------------------------------------------
static void _bl_read(char *args)
{
	unsigned int addr;
	int len;
	UINT32 data[2];
	sscanf(args, "0x%08x %d", &addr, &len);
	memcpy(data, (void*)addr, 8);
	nuc_printf("BL_RD 0x%08x 0x%08x%08x\n", addr, data[0], data[1]);
}

//--- _bl_write ----------------------------------------------------
static void _bl_write(char *args)
{
	UINT64 *pdata;
	unsigned int addr, len;
	INT32 error=-1;

	sscanf(args, "0x%08x %d", &addr, &len);
	hex2bin(&args[11], _Data, len);
	HAL_FLASH_Unlock();
	pdata = _Data;
	for(int i=0; i<len; i+=8)
	{
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr+i, pdata[i/8]) != HAL_OK)
		{
			error = HAL_FLASH_GetError();
			break;
		}
	}
	HAL_FLASH_Lock();
	if (error>=0) nuc_printf("ERROR: BL_WR at 0x%08x\n", error);
	else   		  nuc_printf("BL_WR 0x%08x\n", addr, len);
}

//--- _bl_erase --------------------------------------------
static void _bl_erase(char *args)
{
	unsigned int addr;
	unsigned int len;
	sscanf(args, "0x%08x %d", &addr, &len);
	UINT32 StartPage = (addr-FLASH_BASE)/FLASH_PAGE_SIZE;
	UINT32 EndPage   = ((addr+len+FLASH_PAGE_SIZE-1)-FLASH_BASE)/FLASH_PAGE_SIZE;

	INT32 error=0;
	static FLASH_EraseInitTypeDef _erasePar;
	_erasePar.TypeErase = FLASH_TYPEERASE_PAGES;
	_erasePar.PageAddress = addr;
	_erasePar.NbPages     = EndPage-StartPage;
	HAL_FLASH_Unlock();
	if (HAL_FLASHEx_Erase(&_erasePar, (UINT32*)&error) != HAL_OK)
	{

		/*Error occurred while page erase.*/

		error = HAL_FLASH_GetError();
	}
	HAL_FLASH_Lock();
	if (error>=0) nuc_printf("ERROR: BL_ERASE at 0x%08x\n", error);
	else		  nuc_printf("BL_ERASE 0x%08x %d\n", addr, len);
}
