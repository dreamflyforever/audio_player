#ifndef __COMMON_FSM_H
#define __COMMON_FSM_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "event_groups.h"
#include "task_def.h"

#define COM_FSM_MAX_WAIT_TIME   (30000/portTICK_RATE_MS)
#define COM_FSM_HOLD_STATE      0xFFFFFFUL
#define COM_FSM_EVENT_NONE      0x000000UL
#define COM_FSM_EVENT_ALL       0xFFFFFFUL
#define COM_FSM_EVENT_EXIT      0x800000UL

typedef uint32_t (*com_fsm_action_t)(void* user_param);

typedef struct {
    uint32_t            event;
    uint32_t            state;
    com_fsm_action_t    action;
    uint32_t            timeout;
    
} com_fsm_lookup_t;

typedef struct {
    TaskHandle_t            task_handle;
    EventGroupHandle_t      event_handle;
    EventGroupHandle_t      event_ack_handle;
    uint32_t                cur_state;
    void*                   user_param;
    com_fsm_lookup_t*       lookup;
    int                     lookup_size;
    com_fsm_action_t        init_action;
    com_fsm_action_t        deinit_action;
    
} com_fsm_t;

#define com_fsm_create(a,b,c,d,e,f)                 com_fsm_create_inner(a,b,c,d,e,f,NULL,NULL)
#define com_fsm_create_with_init(a,b,c,d,e,f,g,h)   com_fsm_create_inner(a,b,c,d,e,f,g,h)

com_fsm_t* com_fsm_create_inner(
    const char* name, 
    int stack_size, 
    int priority, 
    void* user_param, 
    const com_fsm_lookup_t* lookup, 
    int lookup_size,
    com_fsm_action_t init_action,
    com_fsm_action_t deinit_action);

void com_fsm_delete(com_fsm_t** pp_com_fsm);
void com_fsm_set_event(com_fsm_t* com_fsm, uint32_t event, uint32_t handle_timeout);
void com_fsm_set_event_from_isr(com_fsm_t* com_fsm, uint32_t event);
void com_fsm_clear_event(com_fsm_t* com_fsm, uint32_t event);
void com_fsm_set_poll(com_fsm_t* com_fsm, uint32_t state, uint32_t timeout);
uint32_t com_fsm_get_state(com_fsm_t* com_fsm);

#endif
