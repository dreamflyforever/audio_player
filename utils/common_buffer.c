#include "common_buffer.h"
#include <string.h>

int common_buffer_init(common_buffer_t* com_buffer, uint32_t node_size, uint32_t max_size)
{
    com_buffer->read_count = 0;
	com_buffer->write_count = 0;
    com_buffer->node_size = node_size;
    com_buffer->max_size = max_size + 1;
    com_buffer->head = NULL;
    com_buffer->tail = NULL;
    
    return COMMON_BUF_SUCCESS;
}

int common_buffer_deinit(common_buffer_t* com_buffer)
{
	common_buffer_clear(com_buffer);
	return COMMON_BUF_SUCCESS;
}

static int common_buffer_create_node(common_node_t** pp_node, uint32_t size)
{
    int ret = COMMON_BUF_SUCCESS;
    common_node_t* tmp = (common_node_t*)malloc(sizeof(common_node_t));
    
    if(NULL == tmp) {
        ret = COMMON_BUF_ERR_MALLOC;
        goto END;
    }
	
	tmp->next = NULL;
	
    if(0 != ring_buffer_init(&tmp->buffer, size)) {
        free(tmp);
        ret = COMMON_BUF_ERR_MALLOC;
        goto END;
    }

    *pp_node = tmp;

END:
    return ret;
}

static int common_buffer_destroy_node(common_node_t* node)
{
	ring_buffer_deinit(&node->buffer);
    free(node);
    return COMMON_BUF_SUCCESS;
}

int common_buffer_clear(common_buffer_t* com_buffer)
{
    int ret = COMMON_BUF_SUCCESS;
	common_node_t* tmp;

    while(com_buffer->head) {
		tmp = com_buffer->head;
        com_buffer->head = com_buffer->head->next;

		common_buffer_destroy_node(tmp);
    }

    com_buffer->read_count = com_buffer->write_count;
    com_buffer->head = NULL;
    com_buffer->tail = NULL;

    return ret;
}

static int common_buffer_push_inner(common_buffer_t* com_buffer, const uint8_t* buffer, uint32_t size, bool from_isr)
{
    int ret = COMMON_BUF_SUCCESS;
    common_node_t* tmp;
	uint32_t len, pos = 0;
	int free_count = common_buffer_get_free_count(com_buffer);
	
	if(size > free_count) {
		ret = COMMON_BUF_ERR_SIZE;
		goto END;
	}
	
	while(pos < size)
	{
		if(NULL==com_buffer->tail || ring_buffer_get_free_count(&com_buffer->tail->buffer) <= 0)
		{
			ret = common_buffer_create_node(&tmp, com_buffer->node_size);
			
			if(COMMON_BUF_SUCCESS != ret)
				break;
			
			tmp->next = NULL;
				
			if(NULL == com_buffer->head) {
				com_buffer->head = com_buffer->tail = tmp;
			}
			else {
				com_buffer->tail->next = tmp;
				com_buffer->tail = tmp;
			}
		}
		
		len = ring_buffer_push(&com_buffer->tail->buffer, &buffer[pos], size-pos, from_isr);
		pos += len;
		com_buffer->write_count = (com_buffer->write_count + len) %com_buffer->max_size;
	}

END:
    return ret;
}

int common_buffer_push(common_buffer_t* com_buffer, const uint8_t* buffer, uint32_t size)
{
	return common_buffer_push_inner(com_buffer, buffer, size, false);
}

int common_buffer_push_from_isr(common_buffer_t* com_buffer, const uint8_t* buffer, uint32_t size)
{
	return common_buffer_push_inner(com_buffer, buffer, size, true);
}

static int common_buffer_pop_inner(common_buffer_t* com_buffer, uint8_t* buffer, uint32_t* p_size, bool from_isr)
{
    int ret = COMMON_BUF_SUCCESS;
	common_node_t* tmp;
	uint32_t len, pos = 0;
	uint32_t size = *p_size;
	int count = common_buffer_get_count(com_buffer);
	
    *p_size = 0;
	
	if(size > count)
		size = count;

	if(NULL == com_buffer->head) {
		ret = COMMON_BUF_ERR_EMPTY;
		goto END;
	}
	
	while(pos < size)
	{
		if(NULL == com_buffer->head) {
			ret = COMMON_BUF_ERR_EMPTY;
			break;
		}

        if(NULL != buffer)
		    len = ring_buffer_pop(&com_buffer->head->buffer, &buffer[pos], size-pos, from_isr);
        else
            len = ring_buffer_pop(&com_buffer->head->buffer, NULL, size-pos, from_isr);
        
		pos += len;
		com_buffer->read_count = (com_buffer->read_count + len) %com_buffer->max_size;
		
		if(ring_buffer_get_count(&com_buffer->head->buffer) <= 0 && com_buffer->head != com_buffer->tail)
		{
			tmp = com_buffer->head;
			com_buffer->head = com_buffer->head->next;

			if(NULL==com_buffer->head) {
				com_buffer->tail = NULL;
			}
			
			common_buffer_destroy_node(tmp);
		}
	}

END:
	*p_size = size;
    return ret;
}

int common_buffer_pop(common_buffer_t* com_buffer, uint8_t* buffer, uint32_t* p_size)
{
	return common_buffer_pop_inner(com_buffer, buffer, p_size, false);
}

int common_buffer_pop_from_isr(common_buffer_t* com_buffer, uint8_t* buffer, uint32_t* p_size)
{
	return common_buffer_pop_inner(com_buffer, buffer, p_size, true);
}

int common_buffer_get_count(common_buffer_t* com_buffer)
{
    int tmp = com_buffer->max_size + com_buffer->write_count - com_buffer->read_count;

    if(tmp >= com_buffer->max_size)
        tmp -= com_buffer->max_size;

    return tmp;
}

int common_buffer_get_free_count(common_buffer_t* com_buffer)
{
    int tmp = com_buffer->max_size + com_buffer->read_count - com_buffer->write_count;

    if(tmp > com_buffer->max_size)
        tmp -= com_buffer->max_size;

    return tmp - 1;
}

