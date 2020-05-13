#include "ring_buffer.h"
#include <string.h>
#include <stdlib.h>

int ring_buffer_init(ring_buffer_t* ring_buffer, uint32_t size)
{
    ring_buffer->read = 0;
    ring_buffer->write = 0;
    ring_buffer->size = size + 1;
    ring_buffer->buffer = (uint8_t*)malloc(ring_buffer->size);

    if(NULL == ring_buffer->buffer) {
        return -1;
    }

    return 0;
}

int ring_buffer_deinit(ring_buffer_t* ring_buffer)
{
    if(NULL != ring_buffer->buffer) {
        free(ring_buffer->buffer);
        ring_buffer->buffer = NULL;
    }

	return 0;
}

uint32_t ring_buffer_push(ring_buffer_t* ring_buffer, const uint8_t* buffer, uint32_t size, bool from_isr)
{
	int tmp = ring_buffer->size - ring_buffer->write;
	int count = ring_buffer_get_free_count(ring_buffer);
	
	if(count > size)
		count = size;
	
	if(tmp >= count) {
		memcpy(&ring_buffer->buffer[ring_buffer->write], &buffer[0], count);
	}
	else {
		memcpy(&ring_buffer->buffer[ring_buffer->write], &buffer[0], tmp);
		memcpy(&ring_buffer->buffer[0], &buffer[tmp], count - tmp);
	}
	
	ring_buffer->write = (ring_buffer->write + count) %ring_buffer->size;
	
    return count;
}

uint32_t ring_buffer_pop(ring_buffer_t* ring_buffer, uint8_t* buffer, uint32_t size, bool from_isr)
{
	int tmp = ring_buffer->size - ring_buffer->read;
	int count = ring_buffer_get_count(ring_buffer);
	
	if(count > size)
		count = size;
	
    if(NULL != buffer)
    {
    	if(tmp >= count) {
	    	memcpy(&buffer[0], &ring_buffer->buffer[ring_buffer->read], count);
	    }
	    else {
		    memcpy(&buffer[0], &ring_buffer->buffer[ring_buffer->read], tmp);
		    memcpy(&buffer[tmp], &ring_buffer->buffer[0], count - tmp);
	    }
    }
	
	ring_buffer->read = (ring_buffer->read + count) %ring_buffer->size;
	
    return count;
}

int ring_buffer_clear(ring_buffer_t* ring_buffer, bool from_isr)
{
    ring_buffer->read = ring_buffer->write;
    return 0;
}

int ring_buffer_get_count(ring_buffer_t* ring_buffer)
{
    int tmp = ring_buffer->size + ring_buffer->write - ring_buffer->read;

    if(tmp >= ring_buffer->size)
        tmp -= ring_buffer->size;

    return tmp;
}

int ring_buffer_get_free_count(ring_buffer_t* ring_buffer)
{
    int tmp = ring_buffer->size + ring_buffer->read - ring_buffer->write;

    if(tmp > ring_buffer->size)
        tmp -= ring_buffer->size;

    return tmp - 1;
}
