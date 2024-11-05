#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "ge_common.h"
#include "boot_loader.h"
#include "main.h"
#include "term.h"

#define CMD_FIFO_SIZE 8
static char _Input[512];
static char _Cmd[CMD_FIFO_SIZE][512];
static int  _CmdIn, _CmdOut;
static int	_InputLen=0;

//--- terminal_init --------------------------------
void term_init(void)
{
	_InputLen = 0;
	_CmdIn= _CmdOut = 0;
	memset(_Cmd, 0, sizeof(_Cmd));
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
 //   	nuc_printf("TERM: cmd[%d] >>%s<<\n", _CmdOut%CMD_FIFO_SIZE, cmd);
		_CmdOut++;
		int ret = bl_handle_cmd(cmd);
		if (!ret && strlen(cmd))
    		nuc_printf("WARN: Unknown command >>%s<<\n", cmd);
    }
}
