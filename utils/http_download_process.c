#include "http_download_process.h"
#include "task_def.h"
#include <string.h>

#define malloc(x)   pvPortMalloc(x)
#define free(x)     vPortFree(x)

#define HTTP_DOWNLOAD_MAX_ERR_CONN_COUNT        5
#define HTTP_DOWNLOAD_MAX_ERR_RECV_COUNT        15
#define HTTP_DOWNLOAD_RECV_BUF_SIZE             (2048 * 1 + 1)
#define HTTP_DOWNLOAD_TASK_STACK_SIZE           (10240/sizeof(StackType_t))
#define HTTP_DOWNLOAD_MONITOR_INTERVAL          (2000/portTICK_RATE_MS)

log_create_module(http_download_proc, PRINT_LEVEL_ERROR);

static http_download_handle_t g_last_alloc_handle = 0;

static http_download_proc_return_t http_download_proc_url_preprocess(http_download_proc_t* http_proc)
{
    // https --> http
    const char* str_cmp = "https";
    int i;
    
    if(0 == strncasecmp(http_proc->url, str_cmp, strlen(str_cmp))) {
        for(i=strlen(str_cmp); i<2048; i++) {
            http_proc->url[i-1] = http_proc->url[i];

            if(http_proc->url[i]=='\0')
                break;
        }
    }
        
    return HTTP_DOWNLOAD_PROC_SUCCESS;
}

static http_download_proc_return_t http_download_proc_open_client(http_download_proc_t* http_proc)
{
    int ret;
    
    if(true == http_proc->http_opened) {
        return HTTP_DOWNLOAD_PROC_SUCCESS;
    }
    
    memset(&http_proc->client, 0, sizeof(http_proc->client));
    
    http_download_proc_url_preprocess(http_proc);
    
    ret = httpclient_connect(&http_proc->client, http_proc->url);
    
    if(HTTPCLIENT_OK != ret)
    {
        LOG_E(http_download_proc, "[%d] httpclient_connect failed, ret: %d, url: %s", http_proc->download_handle, ret, http_proc->url);
        
        if(++http_proc->err_conn_count >= HTTP_DOWNLOAD_MAX_ERR_CONN_COUNT) {
            return HTTP_DOWNLOAD_PROC_ERR_TRY_CONN;
        }
        else {
            return HTTP_DOWNLOAD_PROC_ERR_CONN;
        }
    }

    http_proc->http_opened    = true;
    http_proc->err_conn_count = 0;
    
    return HTTP_DOWNLOAD_PROC_SUCCESS;
}

static http_download_proc_return_t http_download_proc_close_client(http_download_proc_t* http_proc)
{
    if(true == http_proc->http_opened) {
        httpclient_close(&http_proc->client);
        http_proc->http_opened = false;
    }
    
    return HTTP_DOWNLOAD_PROC_SUCCESS;
}

static http_download_proc_return_t http_download_proc_start(http_download_proc_t* http_proc)
{
    http_proc->recv_buf = (char*)malloc(HTTP_DOWNLOAD_RECV_BUF_SIZE);
    if(NULL == http_proc->recv_buf) {
        return HTTP_DOWNLOAD_PROC_ERR_MALLOC;
    }

    http_proc->client_data_ext = (httpclient_data_ext_t*)malloc(sizeof(httpclient_data_ext_t));
    if(NULL == http_proc->client_data_ext) {
        return HTTP_DOWNLOAD_PROC_ERR_MALLOC;
    }

    memset(http_proc->recv_buf, 0, HTTP_DOWNLOAD_RECV_BUF_SIZE);
    memset(http_proc->client_data_ext, 0, sizeof(httpclient_data_ext_t));

    if(http_proc->pre_download_pos <= 0) {
        http_proc->total_length    = -1;
        http_proc->length_received = false;
    }

    http_proc->err_conn_count               = 0;
    http_proc->err_recv_count               = 0;
    http_proc->client_data_ext->is_range    = false;
    http_proc->redirect                     = false;
    http_proc->is_chunked                   = false;
    http_proc->http_opened                  = false;
    http_proc->http_ret                     = 0;
    http_proc->last_monitor_tick            = xTaskGetTickCount();
    http_proc->last_monitor_pos             = 0;
    http_proc->range_forecast               = http_proc->range_enable;
    http_proc->close_if_rang_end            = false;
    
    return HTTP_DOWNLOAD_PROC_SUCCESS;
}

static http_download_proc_return_t http_download_proc_stop(http_download_proc_t* http_proc)
{
    http_download_proc_close_client(http_proc);

    if(NULL != http_proc->client_data_ext) {
        free(http_proc->client_data_ext);
        http_proc->client_data_ext = NULL;
    }
    
    if(NULL != http_proc->recv_buf) {
        free(http_proc->recv_buf);
        http_proc->recv_buf = NULL;
    }

    LOG_I(http_download_proc, "[%d] last_error:%d", http_proc->download_handle, http_proc->last_error);
    
    return HTTP_DOWNLOAD_PROC_SUCCESS;
}

static http_download_proc_return_t http_download_proc_conn(http_download_proc_t* http_proc)
{
    http_download_proc_return_t ret;

    //http_download_proc_close_client(http_proc);

    if(NULL == http_proc->url || NULL == http_proc->http_buffer) {
        LOG_E(http_download_proc, "url or http_buffer is null!");
        return HTTP_DOWNLOAD_PROC_ERR_PARAM;
    }
    
    ret = http_download_proc_open_client(http_proc);
    if(HTTP_DOWNLOAD_PROC_SUCCESS != ret) {
        return ret;
    }

    memset(&http_proc->client_data, 0, sizeof(httpclient_data_t));
    memset(http_proc->client_data_ext->remain_data_buf, 0, HTTPCLIENT_CHUNK_SIZE);

    http_proc->client_data_ext->remain_data_len = 0;
    http_proc->cur_download_pos                 = 0;
    http_proc->client_data.response_buf         = http_proc->recv_buf;
    http_proc->client_data.response_buf_len     = HTTP_DOWNLOAD_RECV_BUF_SIZE;
    http_proc->client_data.ext                  = http_proc->client_data_ext;

    if(true==http_proc->range_enable)
    {
        int range_count;
        char* if_range = NULL;

        if(true==http_proc->length_received)
        {
            range_count = common_buffer_get_free_count(http_proc->http_buffer);

            if(range_count > (http_proc->total_length - http_proc->pre_download_pos)) {
                range_count = http_proc->total_length - http_proc->pre_download_pos;
            }
        }
        else if(true==http_proc->range_forecast)
        {
            range_count = common_buffer_get_free_count(http_proc->http_buffer);
        }
        else
        {
            range_count = 1024;
        }

        if(true==http_proc->client_data_ext->is_range && strlen(http_proc->client_data_ext->if_range) > 0) {
            if_range = http_proc->client_data_ext->if_range;
        }
        else {
            if_range = NULL;
        }

        http_proc->range_end = http_proc->pre_download_pos + range_count;
    
        http_proc->http_ret = httpclient_send_request_with_range(
            &http_proc->client, 
            http_proc->url, 
            HTTPCLIENT_GET, 
            &http_proc->client_data,
            http_proc->pre_download_pos, 
            http_proc->range_end - 1,
            if_range);
    }
    else
    {
        http_proc->http_ret = httpclient_send_request(
            &http_proc->client, 
            http_proc->url, 
            HTTPCLIENT_GET, 
            &http_proc->client_data);
    }

    if(http_proc->http_ret < 0) {
        return HTTP_DOWNLOAD_PROC_ERR_SEND;
    }
    
    return HTTP_DOWNLOAD_PROC_SUCCESS;
}

static http_download_proc_return_t http_download_proc_recv(http_download_proc_t* http_proc)
{
    httpclient_set_response_timeout(&http_proc->client, 3000);

    http_proc->http_ret = httpclient_recv_response(&http_proc->client, &http_proc->client_data);
    if(http_proc->http_ret < 0)
    {
        LOG_E(http_download_proc, "[%d] recv_error: %d, err_count: %d, received: %d", http_proc->download_handle, http_proc->http_ret, http_proc->err_recv_count, http_proc->pre_download_pos);

        if(HTTPCLIENT_ERROR_CONN != http_proc->http_ret)
            http_proc->err_recv_count++;
        
        if(http_proc->err_recv_count >= HTTP_DOWNLOAD_MAX_ERR_RECV_COUNT) {
            return HTTP_DOWNLOAD_PROC_ERR_TRY_RECV;
        }
        else {
            return HTTP_DOWNLOAD_PROC_ERR_RECV;
        }
    }

    //http_proc->err_recv_count = 0;

    httpclient_set_response_timeout(&http_proc->client, 0x7FFFFFFF);

    if(false==http_proc->redirect && NULL != http_proc->client_data.ext)
    {
        int url_len = strlen(http_proc->client_data.ext->location);
        
        if(url_len > 0)
        {
            LOG_E(http_download_proc, "[%d] Redirected url is %s", http_proc->download_handle, http_proc->client_data.ext->location);
            
            if(NULL != http_proc->url) {
                free(http_proc->url);
                http_proc->url = NULL;
            }

            http_proc->url = (char*)malloc(url_len+1);
            if(NULL == http_proc->url) {
                return HTTP_DOWNLOAD_PROC_ERR_MALLOC;
            }

    		strncpy(http_proc->url, http_proc->client_data.ext->location, url_len);

            http_proc->redirect = true;
            return HTTP_DOWNLOAD_PROC_REDIRECT;
        }
    }

    /* client_data.response_content_len is -1 when response code is 503. */
    if(http_proc->client_data.response_content_len <= 0) {
        LOG_E(http_download_proc, "[%d] reponse error %d", http_proc->download_handle, http_proc->http_ret);

        http_proc->close_if_rang_end = true;
        
        if(++http_proc->err_recv_count >= HTTP_DOWNLOAD_MAX_ERR_RECV_COUNT) {
            return HTTP_DOWNLOAD_PROC_ERR_RESPONSE;
        }
        else {
            return HTTP_DOWNLOAD_PROC_ERR_RECV;
        }
    }

    if(false==http_proc->range_enable)
    {
        http_proc->client_data_ext->is_range = false;
    }
    else if(true==http_proc->range_forecast)
    {
        if(false==http_proc->client_data_ext->is_range) {
            http_proc->range_forecast = false;
            return HTTP_DOWNLOAD_PROC_ERR_RECV;
        }
    }

    if(true == http_proc->client_data.is_chunked)
    {
        http_proc->is_chunked = true;

        if(http_proc->total_length != http_proc->client_data.response_content_len) {
            http_proc->total_length = http_proc->client_data.response_content_len;
            LOG_I(http_download_proc, "[%d] chunked: true, total_length: %d\n", http_proc->download_handle, http_proc->total_length);
        }
    }
    else if(false==http_proc->length_received)
    {
        if(true == http_proc->client_data.ext->is_range && http_proc->client_data.ext->range_len > 0) {
            http_proc->total_length = http_proc->client_data.ext->range_len;
        }
        else {
            http_proc->total_length = http_proc->client_data.response_content_len;
        }
        
        http_proc->length_received = true;
        LOG_I(http_download_proc, "[%d] chunked: false, total_length: %d, is_range: %d\n", http_proc->download_handle, http_proc->total_length, http_proc->client_data.ext->is_range);
    }

    if(HTTPCLIENT_RETRIEVE_MORE_DATA == http_proc->http_ret) {
        http_proc->recv_len = HTTP_DOWNLOAD_RECV_BUF_SIZE-1;
    }
    else {
        http_proc->recv_len = http_proc->client_data.response_content_len % (HTTP_DOWNLOAD_RECV_BUF_SIZE-1);
    }

    http_proc->read_pos = 0;

    if(true == http_proc->client_data_ext->is_range) {
        if(http_proc->cur_download_pos < http_proc->pre_download_pos)
            http_proc->cur_download_pos = http_proc->pre_download_pos;
    }
    else {
        int count = http_proc->pre_download_pos - http_proc->cur_download_pos;

        if(count > http_proc->recv_len)
            count = http_proc->recv_len;

        http_proc->cur_download_pos += count;
        http_proc->read_pos         += count;
    }

    if(http_proc->read_pos >= http_proc->recv_len) {
        return HTTP_DOWNLOAD_PROC_ERR_RECV_SKIP;
    }
        
    return HTTP_DOWNLOAD_PROC_SUCCESS;
}

static http_download_proc_return_t http_download_proc_push_data(http_download_proc_t* http_proc)
{
    int len = http_proc->recv_len - http_proc->read_pos;
    double speed, progress;
    
    if(common_buffer_get_free_count(http_proc->http_buffer) < len)
    {
        return HTTP_DOWNLOAD_PROC_ERR_BUF_TOO_SMALL;
    }
    else
    {
        common_buffer_push(http_proc->http_buffer, (uint8_t*)(&http_proc->recv_buf[http_proc->read_pos]), len);
        http_proc->pre_download_pos += len;
        http_proc->cur_download_pos += len;
        http_proc->read_pos         += len;

        if((xTaskGetTickCount() - http_proc->last_monitor_tick) >= HTTP_DOWNLOAD_MONITOR_INTERVAL) {
            speed  = http_proc->pre_download_pos - http_proc->last_monitor_pos;
            speed /= (xTaskGetTickCount()-http_proc->last_monitor_tick);

            progress = 100.0*http_proc->pre_download_pos/http_proc->total_length;

            LOG_I(http_download_proc, "[%d] download: %.2f%%(%d/%d) %.2fkB/s", http_proc->download_handle, progress, http_proc->pre_download_pos, http_proc->total_length, speed);

            http_proc->last_monitor_pos  = http_proc->pre_download_pos;
            http_proc->last_monitor_tick = xTaskGetTickCount();
        }

        if(false==http_proc->client_data_ext->is_range && HTTPCLIENT_RETRIEVE_MORE_DATA!=http_proc->http_ret) {
            LOG_I(http_download_proc, "[%d] all unrange end, download_pos:%d", http_proc->download_handle, http_proc->pre_download_pos);
                
            return HTTP_DOWNLOAD_PROC_ALL_END;
        }

        if(false==http_proc->is_chunked && http_proc->pre_download_pos >= http_proc->total_length) {
            LOG_I(http_download_proc, "[%d] all range end, download_pos:%d", http_proc->download_handle, http_proc->pre_download_pos);
                
            return HTTP_DOWNLOAD_PROC_ALL_END;
        }

        if(true==http_proc->client_data_ext->is_range && http_proc->pre_download_pos >= http_proc->range_end) {
            LOG_I(http_download_proc, "[%d] range end, download_pos:%d", http_proc->download_handle, http_proc->pre_download_pos);
        
            return HTTP_DOWNLOAD_PROC_RANGE_END;
        }
    }

    return HTTP_DOWNLOAD_PROC_SUCCESS;
}

static http_download_proc_return_t http_download_wait_free(http_download_proc_t* http_proc)
{
    int len = http_proc->recv_len - http_proc->read_pos;
    
    if(common_buffer_get_free_count(http_proc->http_buffer) < len) {
        return HTTP_DOWNLOAD_PROC_ERR_BUF_TOO_SMALL;
    }

    return HTTP_DOWNLOAD_PROC_SUCCESS;
}

static http_download_proc_return_t http_download_wait_range(http_download_proc_t* http_proc)
{
    if(common_buffer_get_free_count(http_proc->http_buffer) <= common_buffer_get_count(http_proc->http_buffer)) {
        return HTTP_DOWNLOAD_PROC_ERR_BUF_TOO_SMALL;
    }
    
    return HTTP_DOWNLOAD_PROC_SUCCESS;
}


static uint32_t http_download_proc_conn_action(void* param)
{
    http_download_proc_t* http_proc = (http_download_proc_t*)param;

    http_proc->last_error = http_download_proc_conn(http_proc);
            
    if(HTTP_DOWNLOAD_PROC_SUCCESS == http_proc->last_error) {
        LOG_I(http_download_proc, "[%d] --> HTTP_DOWNLOAD_STA_RECV", http_proc->download_handle);
        return HTTP_DOWNLOAD_STA_RECV;
    }
    else if(HTTP_DOWNLOAD_PROC_ERR_TRY_CONN == http_proc->last_error || HTTP_DOWNLOAD_PROC_ERR_PARAM == http_proc->last_error) {
        http_download_proc_stop(http_proc);
        LOG_I(http_download_proc, "[%d] --> HTTP_DOWNLOAD_STA_IDLE", http_proc->download_handle);
        return HTTP_DOWNLOAD_STA_IDLE;
    }
    
    http_download_proc_close_client(http_proc);
    return COM_FSM_HOLD_STATE;
}

static uint32_t http_download_proc_push_action(void* param)
{
    http_download_proc_t* http_proc = (http_download_proc_t*)param;

    http_proc->last_error = http_download_proc_push_data(http_proc);

    if(HTTP_DOWNLOAD_PROC_SUCCESS == http_proc->last_error) {
        //LOG_I(http_download_proc, "[%d] --> HTTP_DOWNLOAD_STA_RECV", http_proc->download_handle);
        return HTTP_DOWNLOAD_STA_RECV;
    }
    else if( HTTP_DOWNLOAD_PROC_ERR_BUF_TOO_SMALL== http_proc->last_error ) {
        //LOG_I(http_download_proc, "[%d] --> HTTP_DOWNLOAD_STA_WAIT_FREE", http_proc->download_handle);
        return HTTP_DOWNLOAD_STA_WAIT_FREE;
    }
    else if(HTTP_DOWNLOAD_PROC_RANGE_END == http_proc->last_error) {
        if(true == http_proc->close_if_rang_end) {
            http_download_proc_close_client(http_proc);
        }

        LOG_I(http_download_proc, "[%d] --> HTTP_DOWNLOAD_STA_WAIT_RANGE", http_proc->download_handle);
        return HTTP_DOWNLOAD_STA_WAIT_RANGE;
    }
    
    http_download_proc_stop(http_proc);
    
    LOG_I(http_download_proc, "[%d] --> HTTP_DOWNLOAD_STA_IDLE", http_proc->download_handle);
    return HTTP_DOWNLOAD_STA_IDLE;
}

static uint32_t http_download_proc_start_action(void* param)
{
    http_download_proc_t* http_proc = (http_download_proc_t*)param;

    http_proc->last_error = http_download_proc_start(http_proc);

    if(HTTP_DOWNLOAD_PROC_SUCCESS == http_proc->last_error) {
        return http_download_proc_conn_action(http_proc);
    }

    http_download_proc_stop(http_proc);
    return COM_FSM_HOLD_STATE;
}

static uint32_t http_download_proc_stop_action(void* param)
{
    http_download_proc_t* http_proc = (http_download_proc_t*)param;
    
    http_proc->last_error = HTTP_DOWNLOAD_PROC_SUCCESS;     
	http_download_proc_stop(http_proc);

    LOG_I(http_download_proc, "[%d] --> HTTP_DOWNLOAD_STA_IDLE", http_proc->download_handle);
    return HTTP_DOWNLOAD_STA_IDLE;
}

static uint32_t http_download_proc_pause_action(void* param)
{
    http_download_proc_t* http_proc = (http_download_proc_t*)param;

    http_download_proc_close_client(http_proc);

    LOG_I(http_download_proc, "[%d] --> HTTP_DOWNLOAD_STA_PAUSE", http_proc->download_handle);
    return HTTP_DOWNLOAD_STA_PAUSE;
}

static uint32_t http_download_proc_recv_poll(void* param)
{
    http_download_proc_t* http_proc = (http_download_proc_t*)param;

    http_proc->last_error = http_download_proc_recv(http_proc);

    if(HTTP_DOWNLOAD_PROC_SUCCESS== http_proc->last_error) {
        return http_download_proc_push_action(http_proc);
    }
    else if(HTTP_DOWNLOAD_PROC_ERR_RECV_SKIP== http_proc->last_error) {
        return COM_FSM_HOLD_STATE;
    }
    else if(HTTP_DOWNLOAD_PROC_ERR_RECV == http_proc->last_error || HTTP_DOWNLOAD_PROC_REDIRECT == http_proc->last_error) {
        http_download_proc_close_client(http_proc);
        vTaskDelay(500/portTICK_RATE_MS);
        
        return http_download_proc_conn_action(http_proc);
    }
    
    http_download_proc_stop(http_proc);

    LOG_I(http_download_proc, "[%d] --> HTTP_DOWNLOAD_STA_IDLE", http_proc->download_handle);
    return HTTP_DOWNLOAD_STA_IDLE;
}

static uint32_t http_download_proc_wait_free_poll(void* param)
{
    http_download_proc_t* http_proc = (http_download_proc_t*)param;

    http_proc->last_error = http_download_wait_free(http_proc);

    if(HTTP_DOWNLOAD_PROC_SUCCESS == http_proc->last_error) {
        return http_download_proc_push_action(http_proc);
    }
    
    return COM_FSM_HOLD_STATE;
}

static uint32_t http_download_proc_wait_range_poll(void* param)
{
    http_download_proc_t* http_proc = (http_download_proc_t*)param;

    http_proc->last_error = http_download_wait_range(http_proc);

    if(HTTP_DOWNLOAD_PROC_SUCCESS == http_proc->last_error) {
        return http_download_proc_conn_action(http_proc);
    }

    return COM_FSM_HOLD_STATE;
}

http_download_proc_return_t http_download_init(http_download_proc_t* http_proc)
{
    const com_fsm_lookup_t lookup[] = {
        { HTTP_DOWNLOAD_EVENT_START,   HTTP_DOWNLOAD_STA_IDLE,          http_download_proc_start_action, },
        { HTTP_DOWNLOAD_EVENT_STOP,    HTTP_DOWNLOAD_STA_PAUSE,         http_download_proc_stop_action, },
        { HTTP_DOWNLOAD_EVENT_STOP,    HTTP_DOWNLOAD_STA_CONN,          http_download_proc_stop_action, },
        { HTTP_DOWNLOAD_EVENT_STOP,    HTTP_DOWNLOAD_STA_RECV,          http_download_proc_stop_action, },
        { HTTP_DOWNLOAD_EVENT_STOP,    HTTP_DOWNLOAD_STA_WAIT_FREE,     http_download_proc_stop_action, },
        { HTTP_DOWNLOAD_EVENT_STOP,    HTTP_DOWNLOAD_STA_WAIT_RANGE,    http_download_proc_stop_action, },
        { HTTP_DOWNLOAD_EVENT_PAUSE,   HTTP_DOWNLOAD_STA_CONN,          http_download_proc_pause_action, },
        { HTTP_DOWNLOAD_EVENT_PAUSE,   HTTP_DOWNLOAD_STA_RECV,          http_download_proc_pause_action, },
        { HTTP_DOWNLOAD_EVENT_PAUSE,   HTTP_DOWNLOAD_STA_WAIT_FREE,     http_download_proc_pause_action, },
        { HTTP_DOWNLOAD_EVENT_PAUSE,   HTTP_DOWNLOAD_STA_WAIT_RANGE,    http_download_proc_pause_action, },
        { HTTP_DOWNLOAD_EVENT_RESUME,  HTTP_DOWNLOAD_STA_PAUSE,         http_download_proc_conn_action, },

        { COM_FSM_EVENT_NONE, HTTP_DOWNLOAD_STA_CONN,       http_download_proc_conn_action,     500/portTICK_RATE_MS, },
        { COM_FSM_EVENT_NONE, HTTP_DOWNLOAD_STA_RECV,       http_download_proc_recv_poll,       0/portTICK_RATE_MS, },
        { COM_FSM_EVENT_NONE, HTTP_DOWNLOAD_STA_WAIT_FREE,  http_download_proc_wait_free_poll,  100/portTICK_RATE_MS, },
        { COM_FSM_EVENT_NONE, HTTP_DOWNLOAD_STA_WAIT_RANGE, http_download_proc_wait_range_poll, 100/portTICK_RATE_MS, },
    };
    
    memset(http_proc, 0, sizeof(http_download_proc_t));
    
    http_proc->range_enable = true;
    http_proc->download_handle = 0;

    http_proc->com_fsm = com_fsm_create(
        "http_proc", 
        HTTP_DOWNLOAD_TASK_STACK_SIZE, 
        TASK_PRIORITY_NORMAL, 
        http_proc,
        lookup,
        sizeof(lookup)/sizeof(com_fsm_lookup_t)
        );
    
	return HTTP_DOWNLOAD_PROC_SUCCESS;
}

http_download_proc_return_t http_download_deinit(http_download_proc_t* http_proc)
{
    com_fsm_delete(&http_proc->com_fsm);
    return HTTP_DOWNLOAD_PROC_SUCCESS;
}

static http_download_proc_return_t http_download_start_inner(
    http_download_proc_t* http_proc, 
    common_buffer_t* http_buffer, 
    char* url, 
    bool range_enable,
    int seek_pos)
{
    int url_len;
    
    if(NULL==url || NULL==http_buffer) {
        return HTTP_DOWNLOAD_PROC_ERR_PARAM;
    }

    http_download_stop(http_proc);

    url_len = strlen(url);
    
    http_proc->url = (char*)malloc(url_len+1);
    if(NULL==http_proc->url) {
        return HTTP_DOWNLOAD_PROC_ERR_MALLOC;
    }

    memcpy(http_proc->url, url, url_len);
    http_proc->url[url_len] = '\0';

    http_proc->http_buffer      = http_buffer;
    http_proc->last_error       = HTTP_DOWNLOAD_PROC_SUCCESS;
    http_proc->range_enable     = range_enable;
    http_proc->download_handle  = ++g_last_alloc_handle;
    http_proc->pre_download_pos = seek_pos;
    
    com_fsm_set_event(http_proc->com_fsm, HTTP_DOWNLOAD_EVENT_START, COM_FSM_MAX_WAIT_TIME);
    
    return HTTP_DOWNLOAD_PROC_SUCCESS;
}

http_download_proc_return_t http_download_start(http_download_proc_t* http_proc, common_buffer_t* http_buffer, char* url, bool range_enable)
{
    return http_download_start_inner(http_proc, http_buffer, url, range_enable, 0);
}

http_download_proc_return_t http_download_start_seek(http_download_proc_t* http_proc, common_buffer_t* http_buffer, char* url, bool range_enable, int seek_pos)
{
    return http_download_start_inner(http_proc, http_buffer, url, range_enable, seek_pos);
}

http_download_proc_return_t http_download_stop(http_download_proc_t* http_proc)
{
    com_fsm_set_event(http_proc->com_fsm, HTTP_DOWNLOAD_EVENT_STOP, COM_FSM_MAX_WAIT_TIME);
    com_fsm_clear_event(http_proc->com_fsm, COM_FSM_EVENT_ALL);

    if(NULL != http_proc->url) {
        free(http_proc->url);
        http_proc->url = NULL;
    }

    if(NULL != http_proc->http_buffer) {
        http_proc->http_buffer = NULL;
    }

    return HTTP_DOWNLOAD_PROC_SUCCESS;
}

http_download_proc_return_t http_download_pause(http_download_proc_t* http_proc)
{
    com_fsm_set_event(http_proc->com_fsm, HTTP_DOWNLOAD_EVENT_PAUSE, 0);
    return HTTP_DOWNLOAD_PROC_SUCCESS;
}

http_download_proc_return_t http_download_resume(http_download_proc_t* http_proc)
{
    com_fsm_set_event(http_proc->com_fsm, HTTP_DOWNLOAD_EVENT_RESUME, 0);
    return HTTP_DOWNLOAD_PROC_SUCCESS;
}

http_download_proc_return_t http_download_wait_buffer(http_download_proc_t* http_proc, uint32_t size, uint32_t timeout)
{
    uint32_t begTick = xTaskGetTickCount();

    while(1)
    {
        if(NULL != http_proc->http_buffer && common_buffer_get_count(http_proc->http_buffer) >= size) {
            return HTTP_DOWNLOAD_PROC_SUCCESS;
        }
        
        if((xTaskGetTickCount()-begTick) > timeout) {
            LOG_E(http_download_proc, "[%d] Wait download timeout!", http_proc->download_handle);
                
            return HTTP_DOWNLOAD_PROC_ERR_TIMEOUT;
        }

        if(HTTP_DOWNLOAD_STA_IDLE == http_download_get_state(http_proc))
        {
            if( HTTP_DOWNLOAD_PROC_SUCCESS == http_proc->last_error || 
                HTTP_DOWNLOAD_PROC_ALL_END == http_proc->last_error )
            {
                return HTTP_DOWNLOAD_PROC_SUCCESS;
            }
            else
            {
                LOG_E(http_download_proc, "[%d] Wait download failed!", http_proc->download_handle);
                return HTTP_DOWNLOAD_PROC_ERR_DOWNLOAD_FAILED;
            }
        }
        else if(HTTP_DOWNLOAD_STA_PAUSE == http_download_get_state(http_proc)) {
            //LOG_E(http_download_proc, "[%d] Wait download error (pause)!", http_proc->download_handle);
            //return HTTP_DOWNLOAD_PROC_ERR_DOWNLOAD_PAUSE;

            uint32_t tmp = xTaskGetTickCount();
            
            while(HTTP_DOWNLOAD_STA_PAUSE == http_download_get_state(http_proc))
                vTaskDelay(100/portTICK_RATE_MS);

            begTick += xTaskGetTickCount() - tmp;
        }
        
        vTaskDelay(50/portTICK_RATE_MS);
    }
}

bool http_download_is_finish(http_download_proc_t* http_proc)
{
    if(HTTP_DOWNLOAD_STA_IDLE==http_download_get_state(http_proc)) {
        if( HTTP_DOWNLOAD_PROC_SUCCESS == http_proc->last_error || 
            HTTP_DOWNLOAD_PROC_ALL_END == http_proc->last_error )
        {
            return true;
        }
    }

    return false;
}

bool http_download_is_stopped(http_download_proc_t* http_proc)
{
    return (HTTP_DOWNLOAD_STA_IDLE==http_download_get_state(http_proc)) ?true :false;
}

http_download_proc_return_t http_download_get_last_error(http_download_proc_t* http_proc)
{
    return http_proc->last_error;
}

int http_download_get_total_length(http_download_proc_t* http_proc)
{
    return http_proc->total_length;
}

