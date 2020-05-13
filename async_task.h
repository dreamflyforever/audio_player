#ifndef __ASYNC_TASK_H
#define __ASYNC_TASK_H

#include "common_fsm.h"
#include "common_buffer.h"

typedef struct {
    void* user_data;
    void* user_data2;
    void (*callback)(void*, void*);

} async_execute_t;

typedef struct {
    com_fsm_t*      com_fsm;
    common_buffer_t queue;

} async_task_t;

async_task_t* async_task_create(void);
void async_task_destroy(async_task_t** pp_async_task);
int async_task_add(async_task_t* async_task, void (*callback)(void*, void*), void* user_data, void* user_data2);

#endif

