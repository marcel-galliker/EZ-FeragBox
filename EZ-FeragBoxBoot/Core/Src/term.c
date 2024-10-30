#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "ge_common.h"
#include "term.h"
#include "main.h"

#define CMD_FIFO_SIZE 8
static char _Input[512];
static char _Cmd[CMD_FIFO_SIZE][512];
static int  _CmdIn, _CmdOut;
static int	_InputLen=0;

//--- prototypes ---------------------------------------------
static void _version(void);
static void _flash_read(char *args);
static void _flash_write(char *args);
static void _flash_erase(char *args);

//--- terminal_init --------------------------------
void term_init(void)
{
	_InputLen = 0;
	_CmdIn= _CmdOut = 0;
	memset(_Input, 0, sizeof(_Input));
}

//--- term_handle_char -------------------------
void term_handle_char(char ch)
{
	if (_InputLen<sizeof(_Input))
	{
		_Input[_InputLen++] = ch;
		if (ch=='\n'||ch=='\r')
		{
			memcpy(_Cmd[_CmdIn%CMD_FIFO_SIZE], _Input, _InputLen);
			_InputLen=0;
			_CmdIn++;
		}
	}
	else
	{
		printf("ERR: TERM fifo overflow\n");
		_InputLen=0;
	}
}

//--- term_idle -------------------------------
void term_idle(void)
{
 //   if (_InputLen>1 && (_Input[_InputLen-1]=='\r' || _Input[_InputLen-1]=='\n'))
	if (_CmdOut!=_CmdIn)
    {
    	char *cmd = _Cmd[_CmdOut%CMD_FIFO_SIZE];
		char *args;
 //   	nuc_printf("TERM: cmd[%d] >>%s<<\n", _CmdOut%CMD_FIFO_SIZE, cmd);
		_CmdOut++;
    	if      ((args=strstart(cmd, "FLASH_Version")))  _version();
    	else if ((args=strstart(cmd, "FLASH_RD"))) 		 _flash_read(args);
    	else if ((args=strstart(cmd, "FLASH_WR"))) 		 _flash_write(args);
    	else if ((args=strstart(cmd, "FLASH_ERASE"))) 		 _flash_erase(args);
    	else if ((args=strstart(cmd, "FLASH_START FeragBox"))) jump_to(0x8008000);
    	else if (strlen(cmd))
    		nuc_printf("WARN: Unknown command >>%s<<\n", cmd);
    }
}

//--- _version ------------------------------
static void _version(void)
{
	nuc_printf("version 1.2.3.4\n");
}

//--- _flash_read ----------------------------------------------------
static void _flash_read(char *args)
{
	UINT32 addr;
	int len;
	UINT32 data[2];
	sscanf(args, "0x%08x %d", &addr, &len);
	memcpy(data, addr, 8);
	nuc_printf("FLASH_RD 0x%08x 0x%08x%08x\n", addr, data[0], data[1]);
}

//--- _flash_write ----------------------------------------------------
static void _flash_write(char *args)
{
	static UINT64 data[128/8];
	UINT64 *pdata;
	UINT32 addr, vaddr, len;
	INT32 error=-1;
	sscanf(args, "0x%08x %d", &addr, &len);
	if (len!=128)
		printf("test\n");
	hex2bin(&args[11], data, len);
	HAL_FLASH_Unlock();
	pdata = data;
	for(int i=0; i<len; i+=8)
	{
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr+i, pdata[i/8]) != HAL_OK)
		{
			error = HAL_FLASH_GetError();
			break;
		}
	}
	HAL_FLASH_Lock();
	if (error>=0) nuc_printf("ERROR: FLASH_WR at 0x%08x\n", error);
	else   		  nuc_printf("FLASH_WR 0x%08x\n", addr, len);
}

//--- _flash_erase --------------------------------------------
static void _flash_erase(char *args)
{
	UINT32 addr;
	int len;
	sscanf(args, "0x%08x %d", &addr, &len);
	UINT32 StartPage = (addr-FLASH_BASE)/FLASH_PAGE_SIZE;
	UINT32 EndPage   = ((addr+len+FLASH_PAGE_SIZE-1)-FLASH_BASE)/FLASH_PAGE_SIZE;

	INT32 error=0;
	static FLASH_EraseInitTypeDef _erasePar;
	_erasePar.TypeErase = FLASH_TYPEERASE_PAGES;
	_erasePar.PageAddress = addr;
	_erasePar.NbPages     = EndPage-StartPage;
	HAL_FLASH_Unlock();
	if (HAL_FLASHEx_Erase(&_erasePar, &error) != HAL_OK)
	{

		/*Error occurred while page erase.*/

		error = HAL_FLASH_GetError();
	}
	HAL_FLASH_Lock();
	if (error>=0) nuc_printf("ERROR: FLASH_ERASE at 0x%08x\n", error);
	else		  nuc_printf("FLASH_ERASE 0x%08x %d\n", addr, len);
}
