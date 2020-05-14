#ifndef __AUDIO_QUEUE_H
#define __AUDIO_QUEUE_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "audio_player_process.h"

typedef enum {
    AUDIO_QUEUE_SUCCESS = 0,
    AUDIO_QUEUE_ERR_EMPTY,
    AUDIO_QUEUE_ERR_PARAM,
    AUDIO_QUEUE_ERR_MALLOC,
    AUDIO_QUEUE_ERR_MOVE,

} audio_queue_return_t;

typedef struct audio_queue_node_s{
    audio_player_info_t* player_info;

    struct audio_queue_node_s* pre;
    struct audio_queue_node_s* next;

} audio_queue_node_t;

typedef struct {
    SemaphoreHandle_t mutex;
    int count;
    audio_queue_node_t* head;
    audio_queue_node_t* tail;
    audio_queue_node_t* curr;

} audio_queue_t;

audio_queue_return_t audio_queue_init(audio_queue_t* queue);
audio_queue_return_t audio_queue_deinit(audio_queue_t* queue);
audio_queue_return_t audio_queue_clear(audio_queue_t* queue);
audio_queue_return_t audio_queue_push(audio_queue_t* queue, audio_player_info_t* player_info);
audio_queue_return_t audio_queue_pop(audio_queue_t* queue, audio_player_info_t** pp_player_info);
audio_queue_return_t audio_queue_get_count(audio_queue_t* queue);
audio_queue_return_t audio_queue_get_current(audio_queue_t* queue, audio_player_info_t** pp_player_info);
audio_queue_return_t audio_queue_move_prev(audio_queue_t* queue, audio_player_info_t** pp_player_info);
audio_queue_return_t audio_queue_move_next(audio_queue_t* queue, audio_player_info_t** pp_player_info);

#endif
