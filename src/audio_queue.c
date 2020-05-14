#include "audio_queue.h"

#define malloc(x)       pvPortMalloc(x)
#define free(x)         vPortFree(x)

audio_queue_return_t audio_queue_init(audio_queue_t* queue)
{
    queue->mutex = xSemaphoreCreateMutex();
    queue->count = 0;
    queue->head  = NULL;
    queue->tail  = NULL;
    queue->curr  = NULL;
    
    return AUDIO_QUEUE_SUCCESS;
}

audio_queue_return_t audio_queue_deinit(audio_queue_t* queue)
{
    if(NULL != queue->mutex) {
		vSemaphoreDelete(queue->mutex);
		queue->mutex = NULL;
	}
    
    return AUDIO_QUEUE_SUCCESS;
}

static audio_queue_return_t audio_queue_lock(audio_queue_t* queue)
{
    xSemaphoreTake(queue->mutex, portMAX_DELAY);
    return AUDIO_QUEUE_SUCCESS;
}

static audio_queue_return_t audio_queue_unlock(audio_queue_t* queue)
{
    xSemaphoreGive(queue->mutex);
    return AUDIO_QUEUE_SUCCESS;
}

audio_queue_return_t audio_queue_clear(audio_queue_t* queue)
{
    audio_queue_return_t ret = AUDIO_QUEUE_SUCCESS;
    audio_queue_node_t* tmp;
    audio_queue_lock(queue);

    while(queue->head) {
        tmp = queue->head;
        queue->head = queue->head->next;

        if(NULL != tmp->player_info) {
            free(tmp->player_info);
        }
    
        free(tmp);
    }

    queue->count = 0;
    queue->head  = NULL;
    queue->tail  = NULL;
    queue->curr  = NULL;

    audio_queue_unlock(queue);
    return ret;
}

audio_queue_return_t audio_queue_push(audio_queue_t* queue, audio_player_info_t* player_info)
{
    audio_queue_return_t ret = AUDIO_QUEUE_SUCCESS;
    audio_queue_node_t* tmp;

    if(NULL == player_info) {
        return AUDIO_QUEUE_ERR_PARAM;
    }

    audio_queue_lock(queue);

    tmp = (audio_queue_node_t*)malloc(sizeof(audio_queue_node_t));

    if(NULL == tmp) {
        ret = AUDIO_QUEUE_ERR_MALLOC;
    }
    else
    {
        tmp->player_info = player_info;
        tmp->next = NULL;
        tmp->pre  = queue->tail;
            
        if(NULL == queue->head) {
            queue->head = queue->tail = tmp;
        }
        else {
            queue->tail->next = tmp;
            queue->tail = tmp;
        }

        queue->count++;
    }

    audio_queue_unlock(queue);
    return ret;
}

audio_queue_return_t audio_queue_pop(audio_queue_t* queue, audio_player_info_t** pp_player_info)
{
    audio_queue_return_t ret = AUDIO_QUEUE_SUCCESS;
	audio_queue_node_t* tmp;

    audio_queue_lock(queue);

    if(NULL == queue->head) {
        ret = AUDIO_QUEUE_ERR_EMPTY;
        goto END;
    }

    tmp = queue->head;

    queue->head = queue->head->next;

    if(NULL==queue->head) {
        queue->tail = NULL;
    }
    else {
        queue->head->pre = NULL;
    }

    queue->count--;

    if(NULL != pp_player_info) {
        *pp_player_info = tmp->player_info;
    }

    if(tmp == queue->curr) {
        queue->curr = NULL;
    }

    free(tmp);

END:
    audio_queue_unlock(queue);
    return ret;
}

audio_queue_return_t audio_queue_get_count(audio_queue_t* queue)
{
    return queue->count;
}

audio_queue_return_t audio_queue_get_current(audio_queue_t* queue, audio_player_info_t** pp_player_info)
{
    audio_queue_return_t ret = AUDIO_QUEUE_SUCCESS;

    if(NULL == pp_player_info) {
        return AUDIO_QUEUE_ERR_PARAM;
    }

    audio_queue_lock(queue);

    if(NULL == queue->curr) {
        ret = AUDIO_QUEUE_ERR_EMPTY;
        goto END;
      
    }
	else if(NULL != pp_player_info) {
        *pp_player_info = queue->curr->player_info;
    }

END:
    audio_queue_unlock(queue);
    return ret;
}

audio_queue_return_t audio_queue_move_prev(audio_queue_t* queue, audio_player_info_t** pp_player_info)
{
    audio_queue_return_t ret = AUDIO_QUEUE_SUCCESS;

    audio_queue_lock(queue);

    if(NULL != queue->curr) {
        queue->curr = queue->curr->pre;
    }
	else {
		queue->curr = queue->tail;
	}

    if(NULL == queue->curr) {
        ret = AUDIO_QUEUE_ERR_MOVE;
        goto END;
    }

    if(NULL != pp_player_info) {
        *pp_player_info = queue->curr->player_info;
    }

END:
    audio_queue_unlock(queue);
    return ret;
}

audio_queue_return_t audio_queue_move_next(audio_queue_t* queue, audio_player_info_t** pp_player_info)
{
    audio_queue_return_t ret = AUDIO_QUEUE_SUCCESS;

    audio_queue_lock(queue);

    if(NULL != queue->curr) {
        queue->curr = queue->curr->next;
    }
    else {
        queue->curr = queue->head;
    }

    if(NULL == queue->curr) {
        ret = AUDIO_QUEUE_ERR_MOVE;
        goto END;
    }

    if(NULL != pp_player_info) {
        *pp_player_info = queue->curr->player_info;
    }

END:
    audio_queue_unlock(queue);
    return ret;
}

