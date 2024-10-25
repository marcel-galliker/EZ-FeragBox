/*
 * utils.c
 *
 *  Created on: Apr 12, 2024
 *      Author: marce_v533nrj
 */

#include <string.h>
#include <stdlib.h>
#include "ge_common.h"

//--- strstart ---------------------------------------
char *strstart(const char *str, const char *start)
{
	int len=strlen(start);
	if (!strncmp(str, start, len))
	{
		while (str[len]==' ') len++;
		return (char*)&str[len];
	}
	return 0;
}

//--- bin2hex ------------------------------
char *bin2hex(char *str, void *data, int len)
{
	UINT8 *src=(UINT8*)data;
	char *dst = str;
	dst+= sprintf(dst, "%03d ", len);
	for(int i=0; i<len; i++, src++)
	{
		dst+=sprintf(dst, "%02x ", *src);
	}
	return str;
}

//--- hex2bin ------------------------------
void *hex2bin(char *str, void *data, int len)
{
	UINT8 *dst=(UINT8*)data;
	int val;
	if ((int)strlen(str)<4+3*len) return NULL;
	if (atoi(str)!=len) return NULL;

	str += 4;
	for(int i=0; i<len; i++, str+=3)
	{
		sscanf(str, "%02x", &val);
		*dst++ = val;
	}
	return data;
}
