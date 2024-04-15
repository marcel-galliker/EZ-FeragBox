/*
 * utils.c
 *
 *  Created on: Apr 12, 2024
 *      Author: marce_v533nrj
 */

#include <string.h>

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
