#include "audio_message_queue.h"
#include <string.h>

#define malloc(x)       pvPortMalloc(x)
#define free(x)         vPortFree(x)

int audio_msg_queue_init(audio_msg_queue_t* msg_queue, uint32_t length)
{
#if 0
	memset(msg_queue, 0, sizeof(audio_msg_queue_t));
        
    msg_queue->signal = xSemaphoreCreateBinary();
	if(NULL == msg_queue->signal) {
		return -1;
	}
	
    msg_queue->mutex = xSemaphoreCreateMutex();
	if(NULL == msg_queue->mutex) {
		return -2;
	}
	
	msg_queue->messages = (audio_msg_item_t*)malloc(length*sizeof(audio_msg_item_t));
	if(NULL == msg_queue->messages) {
		return -3;
	}
	
	msg_queue->length = length;

    memset(msg_queue->messages, 0, length*sizeof(audio_msg_item_t));

#else
    msg_queue->handle = xQueueCreate(length, sizeof(audio_msg_item_t));

#endif

	return 0;
}

int audio_msg_queue_deinit(audio_msg_queue_t* msg_queue)
{
#if 0
	if(NULL != msg_queue->messages) {
		free(msg_queue->messages);
		msg_queue->messages = NULL;
	}
	
	if(NULL != msg_queue->mutex) {
		vSemaphoreDelete(msg_queue->mutex);
		msg_queue->mutex = NULL;
	}
	
	if(NULL != msg_queue->signal) {
		vSemaphoreDelete(msg_queue->signal);
		msg_queue->signal = NULL;
	}

#else
    vQueueDelete(msg_queue->handle);

#endif
	
	return 0;
}

int audio_msg_queue_send(audio_msg_queue_t* msg_queue, audio_msg_item_t* p_msg, uint32_t timeout)
{
#if 0
	uint8_t i, j;
    
    if(NULL == p_msg) {
        return -1;
    }

    xSemaphoreTake(msg_queue->mutex, portMAX_DELAY);
    
    for(i = 0; i < msg_queue->length; i++) {
        audio_msg_item_t* tmp = &msg_queue->messages[i];
        
        if(p_msg->event == tmp->event)
        {
            tmp->event = 0;
            
            if(NULL != tmp->data) {
                free(tmp->data);
                tmp->data = NULL;
            }
        }

        if(0 == tmp->event)
        {
            for(j = i; j < msg_queue->length-1; j++) {
                memcpy(&msg_queue->messages[j], &msg_queue->messages[j+1], sizeof(audio_msg_item_t));
                memset(&msg_queue->messages[j+1], 0, sizeof(audio_msg_item_t));
            }
        }
    }

    for(i = 0; i < msg_queue->length; i++) {
        audio_msg_item_t* tmp = &msg_queue->messages[i];

        if(0 == tmp->event)
        {
            tmp->event = p_msg->event;
            tmp->data  = p_msg->data;
            break;
        }
    }

    xSemaphoreGive(msg_queue->mutex);

    if(i >= msg_queue->length)
    {
		LOG_E(common, "msg_queue(%p) is full", msg_queue);
        return -2;
    }
    else
    {
        xSemaphoreGive(msg_queue->signal);
    }

#else
    if(pdPASS != xQueueSend(msg_queue->handle, p_msg, timeout)) {
        return -1;
    }

#endif
	
	return 0;
}

int audio_msg_queue_recv(audio_msg_queue_t* msg_queue, audio_msg_item_t* p_msg, uint32_t timeout)
{
#if 0
	uint8_t i;
    int ret = -2;
    
    if(NULL == p_msg) {
        return -1;
    }

    do {
        xSemaphoreTake(msg_queue->mutex, portMAX_DELAY);

        for(i = 0; i < msg_queue->length; i++)
        {
            audio_msg_item_t* tmp = &msg_queue->messages[i];

            if(0 != tmp->event)
            {
                p_msg->event = tmp->event;
                p_msg->data  = tmp->data;
                
                tmp->event = 0;
                tmp->data  = NULL;

                ret = 0;
                break;
            }
        }

        xSemaphoreGive(msg_queue->mutex);
    }
    while(0 != ret && pdPASS == xSemaphoreTake(msg_queue->signal, timeout));

    return ret;

#else

    if(pdPASS != xQueueReceive(msg_queue->handle, p_msg, timeout)) {
        return -1;
    }

    return 0;

#endif
}
