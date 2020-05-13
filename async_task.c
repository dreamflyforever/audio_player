#include "async_task.h"

typedef enum {
    ASYNC_TASK_STA_IDLE = 0,

} async_task_state_t;

typedef enum {
    ASYNC_TASK_EVENT_EXE = 0x000001UL,
    
} async_task_event_t;

static bool async_task_exe(async_task_t* async_task)
{
    if(common_buffer_get_count(&async_task->queue) >= sizeof(async_execute_t))
    {
        async_execute_t async_execute;
        uint32_t _size = sizeof(async_execute_t);
        
        common_buffer_pop(&async_task->queue, (uint8_t*)(&async_execute), &_size);
        assert(sizeof(async_execute_t) == _size);

        async_execute.callback(async_execute.user_data, async_execute.user_data2);
    }

    if(common_buffer_get_count(&async_task->queue) >= sizeof(async_execute_t))
        return true;

    return false;
}

static uint32_t async_task_exe_action(void* param)
{
    async_task_t* async_task = (async_task_t*)param;
    
    if(true == async_task_exe(async_task))
        com_fsm_set_event(async_task->com_fsm, ASYNC_TASK_EVENT_EXE, 0);
    
    return COM_FSM_HOLD_STATE;
}

async_task_t* async_task_create(void)
{
    async_task_t* async_task = NULL;

    const com_fsm_lookup_t lookup[] = {
        { ASYNC_TASK_EVENT_EXE, ASYNC_TASK_STA_IDLE, async_task_exe_action, },
    };

    if(NULL == (async_task = (async_task_t*)malloc(sizeof(async_task_t))))
        return NULL;

    common_buffer_init(&async_task->queue, 10240, 10240*64);

    async_task->com_fsm = com_fsm_create(
        "async_task", 
        1024, 
        TASK_PRIORITY_NORMAL, 
        async_task,
        lookup,
        sizeof(lookup)/sizeof(com_fsm_lookup_t)
        );

    return async_task;
}

void async_task_destroy(async_task_t** pp_async_task)
{
    async_task_t* async_task = (NULL != pp_async_task) ?(*pp_async_task) :NULL;

    if(NULL == async_task)
        return;

    com_fsm_delete(&async_task->com_fsm);
    common_buffer_deinit(&async_task->queue);
    free(async_task);

    *pp_async_task = NULL;
}

int async_task_add(async_task_t* async_task, void (*callback)(void*, void*), void* user_data, void* user_data2)
{
    async_execute_t async_execute;

    async_execute.user_data     = user_data;
    async_execute.user_data2    = user_data2;
    async_execute.callback      = callback;

    common_buffer_push(&async_task->queue, (uint8_t*)(&async_execute), sizeof(async_execute_t));
    com_fsm_set_event(async_task->com_fsm, ASYNC_TASK_EVENT_EXE, 0);
    
    return 0;
}

