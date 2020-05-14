#include "common_event.h"

common_event_t* common_create_event(void)
{
	common_event_t* common_event = (common_event_t*)malloc(sizeof(common_event_t));
	
	if(NULL != common_event) {
		common_event->event = 0;
		common_event->cond  = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
		common_event->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	}
	
	return common_event;
}

void common_set_event(common_event_t* common_event, uint32_t events)
{
	pthread_mutex_lock(&common_event->mutex);
    
    common_event->event |= events;
    pthread_cond_signal(&common_event->cond);

    pthread_mutex_unlock(&common_event->mutex);
}

uint32_t common_wait_event(common_event_t* common_event, uint32_t events, uint32_t timeout)
{
	uint32_t ret = 0;

    pthread_mutex_lock(&common_event->mutex);

    ret = common_event->event & events;

    if(0 == ret && timeout > 0)
    {
        struct timeval now;
        struct timespec outtime;

        gettimeofday(&now, NULL);

        now.tv_sec  += timeout/1000;
        now.tv_usec += (timeout%1000)*1000;

        now.tv_sec  += now.tv_usec/1000000;
        now.tv_usec %= 1000000;
        
        outtime.tv_sec  = now.tv_sec;
        outtime.tv_nsec = now.tv_usec*1000;

        do {
            if(ETIMEDOUT == pthread_cond_timedwait(&common_event->cond, &common_event->mutex, &outtime))
                break;
            
            ret = common_event->event & events;
            
        } while(0 == ret);
        
    }

    if(0 != ret)
    {
        common_event->event &= ~ret;

        if(0 != common_event->event) {
            pthread_cond_signal(&common_event->cond);
        }
    }
    
    pthread_mutex_unlock(&common_event->mutex);

    return ret;
}

void common_clear_event(common_event_t* common_event, uint32_t events)
{
	pthread_mutex_lock(&common_event->mutex);

    common_event->event &= ~events;

    pthread_mutex_unlock(&common_event->mutex);
}

void common_delete_event(common_event_t* common_event)
{
    pthread_cond_destroy(&common_event->cond);
    pthread_mutex_destroy(&common_event->mutex);
	
	free(common_event);
}
