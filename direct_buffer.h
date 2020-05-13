#ifndef __DIRECT_BUFFER_H
#define __DIRECT_BUFFER_H

#ifdef __cplusplus
extern "C"{
#endif

#include "FreeRTOS.h"

typedef struct {
	int read;
	int write;
	int size;
	uint8_t* buffer;

} direct_buffer_t;

#define direct_buffer_push_one(x, y)    do { ((x)->buffer)[(x)->write++] = y; } while(0)
#define direct_buffer_pop_only(x, y)    do { (x)->read += y; } while(0)
#define direct_buffer_get_base(x)       (&(x)->buffer[(x)->read])
#define direct_buffer_clear(x)          do { (x)->read = (x)->write = 0; } while(0)
#define direct_buffer_get_count(x)      ((x)->write - (x)->read)
#define direct_buffer_get_free_count(x) ((x)->size - (x)->write)

direct_buffer_t* direct_buffer_create(int size);
void direct_buffer_destroy(direct_buffer_t** pp_direct_buffer);
int direct_buffer_resize(direct_buffer_t* direct_buffer, int size);
int direct_buffer_push(direct_buffer_t* direct_buffer, const uint8_t* buffer, int size);
int direct_buffer_pop(direct_buffer_t* direct_buffer, uint8_t** pp_buffer, int size);

#ifdef __cplusplus
}
#endif
#endif
