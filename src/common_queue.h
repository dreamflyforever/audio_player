#ifndef __COMMON_QUEUE_H
#define __COMMON_QUEUE_H

#include "typedefs.h"

typedef struct {
    int msg_id;
    int item_size;
    
} common_queue_t;

common_queue_t* common_create_queue(int item_size);
void common_delete_queue(common_queue_t* common_queue);
int common_send_queue(common_queue_t* common_queue, void* data, uint32_t timeout);
int common_recv_queue(common_queue_t* common_queue, void* data, uint32_t timeout);

#endif
