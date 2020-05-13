#ifndef __HTTP_DOWNLOAD_PROCESS_H
#define __HTTP_DOWNLOAD_PROCESS_H

#include "common_fsm.h"
#include "common_buffer.h"
#include "httpclient.h"

typedef enum {
	HTTP_DOWNLOAD_STA_IDLE = 0,
	HTTP_DOWNLOAD_STA_PAUSE,
	HTTP_DOWNLOAD_STA_CONN,
	HTTP_DOWNLOAD_STA_RECV,
	HTTP_DOWNLOAD_STA_PUSH_DATA,
	HTTP_DOWNLOAD_STA_WAIT_FREE,
	HTTP_DOWNLOAD_STA_WAIT_RANGE,
	
} http_download_status_t;

typedef enum {
	HTTP_DOWNLOAD_PROC_SUCCESS = 0,
    HTTP_DOWNLOAD_PROC_ALL_END,
    HTTP_DOWNLOAD_PROC_RANGE_END,
    HTTP_DOWNLOAD_PROC_REDIRECT,
    HTTP_DOWNLOAD_PROC_ERR_PARAM,
    HTTP_DOWNLOAD_PROC_ERR_MALLOC,
    HTTP_DOWNLOAD_PROC_ERR_CONN,
    HTTP_DOWNLOAD_PROC_ERR_TRY_CONN,
    HTTP_DOWNLOAD_PROC_ERR_SEND,
    HTTP_DOWNLOAD_PROC_ERR_RECV,
    HTTP_DOWNLOAD_PROC_ERR_RECV_SKIP,
    HTTP_DOWNLOAD_PROC_ERR_RESPONSE,
    HTTP_DOWNLOAD_PROC_ERR_BUF_TOO_SMALL,
    HTTP_DOWNLOAD_PROC_ERR_TRY_RECV,
    HTTP_DOWNLOAD_PROC_ERR_TIMEOUT,
    HTTP_DOWNLOAD_PROC_ERR_DOWNLOAD_FAILED,
    HTTP_DOWNLOAD_PROC_ERR_DOWNLOAD_PAUSE,
	
} http_download_proc_return_t;

typedef enum {
    HTTP_DOWNLOAD_EVENT_START       = 0x000001UL,
    HTTP_DOWNLOAD_EVENT_STOP        = 0x000002UL,
    HTTP_DOWNLOAD_EVENT_PAUSE       = 0x000004UL,
    HTTP_DOWNLOAD_EVENT_RESUME      = 0x000008UL,

} http_download_event_t;

typedef uint32_t http_download_handle_t;

typedef struct {
    com_fsm_t*                  com_fsm;
    http_download_handle_t      download_handle;
    char*                       url;
    common_buffer_t*            http_buffer;
    httpclient_t                client;
    httpclient_data_t           client_data;
    httpclient_data_ext_t*      client_data_ext;
    bool                        http_opened;
    uint16_t                    err_conn_count;
    uint16_t                    err_recv_count;
    char*                       recv_buf;
    int                         recv_len;
    int                         read_pos;
    int                         pre_download_pos;
    int                         cur_download_pos;
    bool                        range_enable;
    bool                        range_forecast;
    int                         range_end;
    bool                        close_if_rang_end;
    bool                        redirect;
    bool                        is_chunked;
    bool                        length_received;
    int                         total_length;
    int                         http_ret;
    http_download_proc_return_t last_error;
    uint32_t                    last_monitor_tick;
    int                         last_monitor_pos;
	
} http_download_proc_t;

#define http_download_get_state(x) com_fsm_get_state((x)->com_fsm)

http_download_proc_return_t http_download_init(http_download_proc_t* http_proc);
http_download_proc_return_t http_download_deinit(http_download_proc_t* http_proc);
http_download_proc_return_t http_download_start(http_download_proc_t* http_proc, common_buffer_t* http_buffer, char* url, bool range_enable);
http_download_proc_return_t http_download_start_seek(http_download_proc_t* http_proc, common_buffer_t* http_buffer, char* url, bool range_enable, int seek_pos);
http_download_proc_return_t http_download_stop(http_download_proc_t* http_proc);
http_download_proc_return_t http_download_pause(http_download_proc_t* http_proc);
http_download_proc_return_t http_download_resume(http_download_proc_t* http_proc);
http_download_proc_return_t http_download_wait_buffer(http_download_proc_t* http_proc, uint32_t size, uint32_t timeout);
bool http_download_is_finish(http_download_proc_t* http_proc);
bool http_download_is_stopped(http_download_proc_t* http_proc);
http_download_proc_return_t http_download_get_last_error(http_download_proc_t* http_proc);
int http_download_get_total_length(http_download_proc_t* http_proc);

#endif

