#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "term.h"
#include "main.h"

#define CMD_FIFO_SIZE 8
static char _Input[128];
static char _Cmd[CMD_FIFO_SIZE][128];
static int  _CmdIn, _CmdOut;
static int	_InputLen=0;

static void _version(void);

//--- terminal_init --------------------------------
void term_init(void)
{
	_InputLen = 0;
	_CmdIn= _CmdOut = 0;
	memset(_Input, 0, sizeof(_Input));

	nuc_printf("term_init 123\n");
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
			term_idle();
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
    	nuc_printf("TERM: cmd[%d] >>%s<<\n", _CmdOut%CMD_FIFO_SIZE, cmd);
		_CmdOut++;
    	if      ((args=strstart(cmd, "version"))) 	_version();
    	/*

    	else if ((args=strstart(cmd, "start"))) 	box_start();
    	else if ((args=strstart(cmd, "stop"))) 		box_stop();
    	else if ((args=strstart(cmd, "pgDelay"))) 	box_set_pgDelay(atoi(args));
    	else if ((args=strstart(cmd, "prodLen"))) 	box_set_prodLen(atoi(args));
    	else if ((args=strstart(cmd, "pg"))) 		box_printGo();
    	else if ((args=strstart(cmd, "resetBX")))	box_reset_bx();
    	else if (strlen(cmd)) nuc_printf("WARN: Unknown command >>%s<<\n", cmd);
    	*/
    }
}

//--- _version ------------------------------
static void _version(void)
{
	nuc_printf("version 1.2.3.4\n");
}