#include "common_queue.h"
#include <sys/msg.h>
#include <sys/ipc.h>

common_queue_t* common_create_queue(int item_size)
{
    int msg_id;
    common_queue_t* common_queue = NULL;
	
	if(NULL == (common_queue = (common_queue_t*)malloc(sizeof(common_queue_t))))
        return NULL;

    if(-1 == (msg_id = msgget((int)(int64_t)common_queue, IPC_CREAT | 0666)))
        return NULL;

    common_queue->item_size = item_size;
    common_queue->msg_id = msg_id;
	
	return common_queue;
}

void common_delete_queue(common_queue_t* common_queue)
{
    if(NULL == common_queue)
        return;

    msgctl(common_queue->msg_id, IPC_RMID, 0);
	
	free(common_queue);
}

int common_send_queue(common_queue_t* common_queue, void* data, uint32_t timeout)
{
    int flag = (timeout > 0) ?0 :IPC_NOWAIT;
    
    if(0 != msgsnd(common_queue->msg_id, data, common_queue->item_size, flag)) {
        return pdFALSE;
    }

    return pdTRUE;
}

int common_recv_queue(common_queue_t* common_queue, void* data, uint32_t timeout)
{
    int flag = (timeout > 0) ?0 :IPC_NOWAIT;
    
    if(-1 == msgrcv(common_queue->msg_id, data, common_queue->item_size, 0, flag)) {
        return pdFALSE;
    }

    return pdTRUE;
}
