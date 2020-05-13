#ifndef __AUDIO_MESSAGE_QUEUE_H
#define __AUDIO_MESSAGE_QUEUE_H

#include "FreeRTOS.h"
#include "queue.h"

typedef struct {
    uint32_t event;
    void* data;
    
} audio_msg_item_t;

typedef struct {
#if 0
    audio_msg_item_t* messages;
	uint32_t          length;
    SemaphoreHandle_t signal;
    SemaphoreHandle_t mutex;
    
#else
    QueueHandle_t handle;

#endif

} audio_msg_queue_t;

int audio_msg_queue_init(audio_msg_queue_t* msg_queue, uint32_t length);
int audio_msg_queue_deinit(audio_msg_queue_t* msg_queue);
int audio_msg_queue_send(audio_msg_queue_t* msg_queue, audio_msg_item_t* p_msg, uint32_t timeout);
int audio_msg_queue_recv(audio_msg_queue_t* msg_queue, audio_msg_item_t* p_msg, uint32_t timeout);

#endif
