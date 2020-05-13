#include "direct_buffer.h"
#include <string.h>
#include <stdlib.h>

direct_buffer_t* direct_buffer_create(int size)
{
	direct_buffer_t* direct_buffer;
	
	if(NULL == (direct_buffer=(direct_buffer_t*)malloc(sizeof(direct_buffer_t)))) {
		return NULL;
	}
	
	memset(direct_buffer, 0, sizeof(direct_buffer_t));
	direct_buffer->size = size;
	
	if(NULL == (direct_buffer->buffer=(uint8_t*)malloc(direct_buffer->size))) {
		free(direct_buffer);
		return NULL;
	}
	
    return direct_buffer;
}

void direct_buffer_destroy(direct_buffer_t** pp_direct_buffer)
{
	direct_buffer_t* direct_buffer = (NULL != pp_direct_buffer) ?(*pp_direct_buffer) :NULL;
	
	if(NULL == direct_buffer)
		return;
	
    if(NULL != direct_buffer->buffer) {
        free(direct_buffer->buffer);
        direct_buffer->buffer = NULL;
    }
	
	free(direct_buffer);
	*pp_direct_buffer = NULL;
}

int direct_buffer_resize(direct_buffer_t* direct_buffer, int size)
{
	if(direct_buffer->size < size)
	{
		uint8_t* buffer = (uint8_t*)malloc(size);
		int count = direct_buffer->write - direct_buffer->read;
		
		if(NULL == buffer)
			return 0;
		
		if(count > 0)
			memcpy(&buffer[direct_buffer->read], &direct_buffer->buffer[direct_buffer->read], count);
		
		free(direct_buffer->buffer);
		direct_buffer->buffer = buffer;
		direct_buffer->size = size;
	}
	
	return size;
}

int direct_buffer_push(direct_buffer_t* direct_buffer, const uint8_t* buffer, int size)
{
	int count = direct_buffer->size - direct_buffer->write;
	
	if(count <= 0)
		return 0;
	
	if(count > size)
		count = size;
	
	memcpy(&direct_buffer->buffer[direct_buffer->write], buffer, count);
	direct_buffer->write += count;
	
    return count;
}

int direct_buffer_pop(direct_buffer_t* direct_buffer, uint8_t** pp_buffer, int size)
{
	int count = direct_buffer->write - direct_buffer->read;
	
	if(count <= 0) {
		if(NULL != pp_buffer)
			*pp_buffer = NULL;
		
		return 0;
	}
	
	if(count > size)
		count = size;
	
	if(NULL != pp_buffer)
		*pp_buffer = &direct_buffer->buffer[direct_buffer->read];
	
	direct_buffer->read += count;
	
    return count;
}

