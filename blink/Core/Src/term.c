#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "box.h"
#include "enc.h"
#include "term.h"
#include "ge_common.h"
#include "main.h"

static char _Input[128];
static int	_InputLen=0;

//--- terminal_init --------------------------------
void term_init(void)
{
	memset(_Input, 0, sizeof(_Input));
}
//--- term_handle_char -------------------------
void term_handle_char(char ch)
{
	if (_InputLen<sizeof(_Input)) _Input[_InputLen++] = ch;
	else _InputLen=0;
//	putchar(ch);
}

//--- term_idle -------------------------------
void term_idle(void)
{
    if (_InputLen>1 && (_Input[_InputLen-1]=='\r' || _Input[_InputLen-1]=='\n'))
    {
    	char *args;
    	if (strstart(_Input, "status")) 		  		box_send_status();
    	else if ((args=strstart(_Input, "encoder"))) 	enc_command(args);
    	else if ((args=strstart(_Input, "pg"))) 		box_printGo();

    	memset(_Input, 0, sizeof(_Input));
    	_InputLen=0;
    }
}
