/*
 * utils.h
 *
 *  Created on: Apr 12, 2024
 *      Author: marcel@galliker-engineering.ch
 */

#ifndef INC_GE_COMMON_H_
#define INC_GE_COMMON_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FALSE 0
#define TRUE 1

#define INT8	int8_t
#define UINT8	uint8_t
#define INT16	int16_t
#define INT32	int32_t
#define UINT32	uint32_t

char *strstart(const char *str, const char *start);
char *bin2hex(char *str, void *data, int len);	// converts to hex-string
void *hex2bin(char *str, void *data, int len);	// converts from hex-string

#endif /* INC_GE_COMMON_H_ */
