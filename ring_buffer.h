#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

#ifdef __cplusplus
extern "C"{
#endif

#include "FreeRTOS.h"

typedef struct {
	int read;
	int write;
	int size;
	uint8_t* buffer;

} ring_buffer_t;

int ring_buffer_init(ring_buffer_t* ring_buffer, uint32_t size);
int ring_buffer_deinit(ring_buffer_t* ring_buffer);
int ring_buffer_clear(ring_buffer_t* ring_buffer, bool from_isr);
uint32_t ring_buffer_push(ring_buffer_t* ring_buffer, const uint8_t* buffer, uint32_t size, bool from_isr);
uint32_t ring_buffer_pop(ring_buffer_t* ring_buffer, uint8_t* buffer, uint32_t size, bool from_isr);
int ring_buffer_get_count(ring_buffer_t* ring_buffer);
int ring_buffer_get_free_count(ring_buffer_t* ring_buffer);

#ifdef __cplusplus
}
#endif
#endif
