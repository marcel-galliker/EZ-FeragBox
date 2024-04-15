/*
 * ring_buffer.h
 *
 *  Created on: Mar 25, 2024
 *      Author: WPATR
 */

#ifndef INC_RING_BUFFER_H_
#define INC_RING_BUFFER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define RING_BUFFER_SIZE 256

typedef struct {
    uint8_t buffer[RING_BUFFER_SIZE];
    volatile size_t head;
    volatile size_t tail;
} RingBuffer;

void RingBuffer_Init(RingBuffer *rb);
bool RingBuffer_IsFull(const RingBuffer *rb);
bool RingBuffer_IsEmpty(const RingBuffer *rb);
bool RingBuffer_PutChar(RingBuffer *rb, uint8_t c);
bool RingBuffer_GetChar(RingBuffer *rb, uint8_t *c);

#endif /* INC_RING_BUFFER_H_ */
