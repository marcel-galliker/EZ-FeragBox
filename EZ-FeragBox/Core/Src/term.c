#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include "box.h"
#include "enc.h"
#include "term.h"
#include "ge_common.h"
#include "main.h"

#define CMD_FIFO_SIZE 8
static char _Input[128];
static char _Cmd[CMD_FIFO_SIZE][128];
static int  _CmdIn, _CmdOut;
static int	_InputLen=0;

//--- terminal_init --------------------------------
void term_init(void)
{
	_InputLen = 0;
	_CmdIn= _CmdOut = 0;
	memset(_Input, 0, sizeof(_Input));
}

//--- term_printf --------------------------------------------
void term_printf (const char *format, ...)
{
	int idx;
	char *buf;
	nuc_get_buffer(&idx, &buf);
	va_list args;
	va_start(args, format);
	vsprintf(buf, format, args);
	va_end(args);
	nuc_send_buffer(idx);
}

//--- term_handle_char -------------------------
void term_handle_char(char ch)
{
	if (_InputLen<sizeof(_Input))
	{
		_Input[_InputLen++] = ch;
		if (ch=='\n')
		{
			memcpy(_Cmd[_CmdIn%CMD_FIFO_SIZE], _Input, _InputLen);
			_InputLen=0;
			_CmdIn++;
			term_idle();
		}
	}
	else
	{
		term_printf("ERR: TERM fifo overflow\n");
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
    //	printf("TERM: cmd[%d] >>%s<<\n", _CmdOut%CMD_FIFO_SIZE, cmd);
		_CmdOut++;
    	if      ((args=strstart(cmd, "encoder"))) 	enc_command(args);
    	else if ((args=strstart(cmd, "start"))) 	box_start();
    	else if ((args=strstart(cmd, "stop"))) 		box_stop();
    	else if ((args=strstart(cmd, "pgDelay"))) 	box_set_pgDelay(atoi(args));
    	else if ((args=strstart(cmd, "prodLen"))) 	box_set_prodLen(atoi(args));
    	else if ((args=strstart(cmd, "pg"))) 		box_printGo();
    	else if ((args=strstart(cmd, "resetBX")))	box_reset_bx();
    	else if (strlen(cmd)) term_printf("WARN: Unknown command >>%s<<\n", cmd);
    }
}
