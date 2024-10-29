#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "ge_common.h"
#include "term.h"
#include "main.h"

#define CMD_FIFO_SIZE 8
static char _Input[128];
static char _Cmd[CMD_FIFO_SIZE][128];
static int  _CmdIn, _CmdOut;
static int	_InputLen=0;

//--- prototypes ---------------------------------------------
static void _version(void);
static void _flash_read(char *args);

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
	UINT32 *pdata;
	UINT32 *pdata1;
	sscanf(args, "0x%08x %d", &addr, &len);
	pdata = (UINT32*)addr;
	pdata1 = (UINT32*)(addr+4);
	nuc_printf("FLASH_RD 0x%08x 0x%08x%08x\n", addr, *pdata, *pdata1);
}
