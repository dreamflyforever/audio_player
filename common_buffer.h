#ifndef __COMMON_BUFFER_H
#define __COMMON_BUFFER_H

#include "ring_buffer.h"

#define COMMON_BUF_HTTP_MAX_SIZE      (640*1024)
#define COMMON_BUF_HTTP_NODE_SIZE     10240

typedef enum {
    COMMON_BUF_SUCCESS = 0,
    COMMON_BUF_ERR_EMPTY,
    COMMON_BUF_ERR_MALLOC,
	COMMON_BUF_ERR_SIZE,

} common_buffer_return_t;

typedef struct common_node_s {
    ring_buffer_t buffer;
    struct common_node_s* next;

} common_node_t;

typedef struct {
    int               read_count;
	int               write_count;
    int               node_size;
    int               max_size;
    common_node_t*    head;
    common_node_t*    tail;

} common_buffer_t;

int common_buffer_init(common_buffer_t* com_buffer, uint32_t node_size, uint32_t max_size);
int common_buffer_deinit(common_buffer_t* com_buffer);
int common_buffer_clear(common_buffer_t* com_buffer);
int common_buffer_push(common_buffer_t* com_buffer, const uint8_t* buffer, uint32_t size);
int common_buffer_push_from_isr(common_buffer_t* com_buffer, const uint8_t* buffer, uint32_t size);
int common_buffer_pop(common_buffer_t* com_buffer, uint8_t* buffer, uint32_t* p_size);
int common_buffer_pop_from_isr(common_buffer_t* com_buffer, uint8_t* buffer, uint32_t* p_size);
int common_buffer_get_count(common_buffer_t* com_buffer);
int common_buffer_get_free_count(common_buffer_t* com_buffer);


#endif

