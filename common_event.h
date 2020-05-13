#ifndef __COMMON_EVENT_H
#define __COMMON_EVENT_H

#include "typedefs.h"

typedef struct {
    pthread_mutex_t  mutex;
    pthread_cond_t   cond;
    uint32_t         event;
    
} common_event_t;

common_event_t* common_create_event(void);
void common_set_event(common_event_t* common_event, uint32_t events);
uint32_t common_wait_event(common_event_t* common_event, uint32_t events, uint32_t timeout);
void common_clear_event(common_event_t* common_event, uint32_t events);
void common_delete_event(common_event_t* common_event);

#endif
