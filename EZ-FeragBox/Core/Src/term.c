#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
    //	printf("TERM: >>%s<<\n", _Input);
    	if (strstart(_Input, "status")) 		  		box_send_status();
    	else if ((args=strstart(_Input, "encoder"))) 	enc_command(args);
    	else if ((args=strstart(_Input, "start"))) 		box_start();
    	else if ((args=strstart(_Input, "stop"))) 		box_stop();
    	else if ((args=strstart(_Input, "pgDelay"))) 	box_set_pgDelay(atoi(args));
    	else if ((args=strstart(_Input, "pg"))) 		box_printGo();
    	else if ((args=strstart(_Input, "h"))) 			box_test(TRUE);
    	else if ((args=strstart(_Input, "l"))) 			box_test(FALSE);

    	memset(_Input, 0, sizeof(_Input));
    	_InputLen=0;
    }
}
