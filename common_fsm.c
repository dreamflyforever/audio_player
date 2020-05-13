#include "common_fsm.h"
#include <string.h>

static com_fsm_lookup_t* com_fsm_get_lookup(com_fsm_t* com_fsm, uint32_t state, uint32_t event)
{
    int i;
    com_fsm_lookup_t* lookup;
    
    for(i = 0; i < com_fsm->lookup_size; i++)
    {
        lookup = &com_fsm->lookup[i];
        
        if(state == lookup->state)
        {
            if(COM_FSM_EVENT_NONE == event) {
                if(event == lookup->event)
                    return lookup;
            }
            else if(lookup->event == (event & lookup->event)) {
                return lookup;
            }
        }
    }

    return NULL;
}

static void com_fsm_clear(com_fsm_t* com_fsm)
{
    if(NULL != com_fsm->event_handle) {
        vEventGroupDelete(com_fsm->event_handle);
        com_fsm->event_handle = NULL;
    }

    if(NULL != com_fsm->event_ack_handle) {
        vEventGroupDelete(com_fsm->event_ack_handle);
        com_fsm->event_ack_handle = NULL;
    }

    if(NULL != com_fsm->lookup) {
        free(com_fsm->lookup);
        com_fsm->lookup = NULL;
    }

    free(com_fsm);
}

static task_return_t com_fsm_task(void* param)
{
    com_fsm_t* com_fsm = (com_fsm_t*)param;
    com_fsm_action_t deinit_action = NULL;
    void* user_param = NULL;
    bool running = true;
    com_fsm_lookup_t* lookup;
    uint32_t state;
    uint32_t events;
    uint32_t timeout;

    if(com_fsm->init_action)
        com_fsm->cur_state = com_fsm->init_action(com_fsm->user_param);

    while(true == running)
    {
        lookup  = com_fsm_get_lookup(com_fsm, com_fsm->cur_state, COM_FSM_EVENT_NONE);
        timeout = lookup ?lookup->timeout :portMAX_DELAY;
        events  = xEventGroupWaitBits(com_fsm->event_handle, COM_FSM_EVENT_ALL, pdTRUE, pdFALSE, timeout);

        if(COM_FSM_EVENT_EXIT & events)
        {
            running = false;
            deinit_action = com_fsm->deinit_action;
            user_param = com_fsm->user_param;
        }
        else
        {
            lookup = com_fsm_get_lookup(com_fsm, com_fsm->cur_state, events);

            if(NULL == lookup && COM_FSM_EVENT_NONE == events) {
                lookup = com_fsm_get_lookup(com_fsm, com_fsm->cur_state, COM_FSM_EVENT_NONE);
            }

            if(NULL != lookup)
            {
                state = lookup->action(com_fsm->user_param);

                if(COM_FSM_HOLD_STATE != state && state != com_fsm->cur_state) {
                    com_fsm->cur_state = state;
                }
            }
        }

        if(COM_FSM_EVENT_NONE != events)
            xEventGroupSetBits(com_fsm->event_ack_handle, events);
    }

    com_fsm_clear(com_fsm);

    if(deinit_action)
        deinit_action(user_param);
        
    vTaskDelete(NULL);
}

com_fsm_t* com_fsm_create_inner(
    const char* name, 
    int stack_size, 
    int priority, 
    void* user_param, 
    const com_fsm_lookup_t* lookup, 
    int lookup_size,
    com_fsm_action_t init_action,
    com_fsm_action_t deinit_action)
{
    com_fsm_t* com_fsm = malloc(sizeof(com_fsm_t));

    if(NULL == com_fsm) {
        return NULL;
    }

    memset(com_fsm, 0, sizeof(com_fsm_t));

    com_fsm->lookup = malloc(lookup_size *sizeof(com_fsm_lookup_t));
    com_fsm->lookup_size = lookup_size;

    if(NULL == com_fsm->lookup) {
        free(com_fsm);
        return NULL;
    }

    memcpy(com_fsm->lookup, lookup, lookup_size *sizeof(com_fsm_lookup_t));

    com_fsm->event_handle     = xEventGroupCreate();
    com_fsm->event_ack_handle = xEventGroupCreate();
    com_fsm->user_param       = user_param;
    com_fsm->init_action      = init_action;
    com_fsm->deinit_action    = deinit_action;

    xTaskCreate(com_fsm_task, name, stack_size, com_fsm, priority, &com_fsm->task_handle);

    return com_fsm;
}

void com_fsm_delete(com_fsm_t** pp_com_fsm)
{
    if(NULL != pp_com_fsm && NULL != *pp_com_fsm)
    {
#ifdef DEF_LINUX_PLATFORM
        pthread_detach((*pp_com_fsm)->task_handle);
#endif
        com_fsm_set_event(*pp_com_fsm, COM_FSM_EVENT_EXIT, 0);
        *pp_com_fsm = NULL;
    }
}

void com_fsm_set_event(com_fsm_t* com_fsm, uint32_t event, uint32_t handle_timeout)
{
    if(NULL == com_fsm || NULL == com_fsm->event_handle)
        return;
    
    xEventGroupSetBits(com_fsm->event_handle, event);

    if(handle_timeout > 0)
        xEventGroupWaitBits(com_fsm->event_ack_handle, event, pdTRUE, pdFALSE, handle_timeout);
}

void com_fsm_set_event_from_isr(com_fsm_t* com_fsm, uint32_t event)
{
    if(NULL == com_fsm || NULL == com_fsm->event_handle)
        return;
    
    xEventGroupSetBitsFromISR(com_fsm->event_handle, event, NULL);
}

void com_fsm_clear_event(com_fsm_t* com_fsm, uint32_t event)
{
    if(NULL == com_fsm || NULL == com_fsm->event_handle)
        return;
    
    xEventGroupClearBits(com_fsm->event_handle, event);
    xEventGroupClearBits(com_fsm->event_ack_handle, event);
}

void com_fsm_set_poll(com_fsm_t* com_fsm, uint32_t state, uint32_t timeout)
{
    com_fsm_lookup_t* lookup = com_fsm_get_lookup(com_fsm, state, COM_FSM_EVENT_NONE);

    if(NULL != lookup) {
        lookup->timeout = timeout;
    }
}

uint32_t com_fsm_get_state(com_fsm_t* com_fsm)
{
    if(NULL == com_fsm) {
        return COM_FSM_HOLD_STATE;
    }
    
    return com_fsm->cur_state;
}

