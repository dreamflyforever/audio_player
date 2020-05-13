#include "audio_file.h"
#include "prompt_flash.h"
#include <string.h>

#define malloc(x)   pvPortMalloc(x)
#define free(x)     vPortFree(x)

#define AUDIO_FILE_MAX_WAIT_TIME        (30000/portTICK_RATE_MS)
#define AUDIO_FILE_WAIT_10K_TIMEOUT     (5000/portTICK_RATE_MS)
#define AUDIO_FILE_WAIT_HTTP_TIMEOUT    (5000/portTICK_RATE_MS)
#define AUDIO_FILE_HTTP_MAX_SEEK        (100*1024)

audio_file_t* audio_file_open(char* file_path, audio_file_type_t audio_type, bool range_enable)
{
    audio_file_t* audio_file = (audio_file_t*)malloc(sizeof(audio_file_t));

    if(NULL == audio_file)
        goto ERROR;

    memset(audio_file, 0, sizeof(audio_file_t));
    strncpy(audio_file->file_path, file_path, AUDIO_FILE_MAX_PATH_SIZE);

    audio_file->file_type = audio_type;
    audio_file->range_enable = range_enable;
    
    if(AUDIO_FILE_TYPE_WEB == audio_file->file_type)
    {
        common_buffer_init(&audio_file->http_buffer, COMMON_BUF_HTTP_NODE_SIZE, COMMON_BUF_HTTP_MAX_SIZE);
        http_download_init(&audio_file->http_proc);
        
        if(HTTP_DOWNLOAD_PROC_SUCCESS != http_download_start(&audio_file->http_proc, &audio_file->http_buffer, audio_file->file_path, audio_file->range_enable))
            goto ERROR;
    }
    else if(AUDIO_FILE_TYPE_FLASH == audio_file->file_type)
    {
        if(0 != prompt_addr_info_get(audio_file->file_path, &audio_file->prompt_offset, &audio_file->total_length))
            goto ERROR;

        if(audio_file->total_length <= 0)
            goto ERROR;
    }
    else if(AUDIO_FILE_TYPE_SD_CARD == audio_file->file_type)
    {
        if(FR_OK != f_open(&audio_file->file_handle, _T(audio_file->file_path), FA_OPEN_EXISTING |FA_READ))
            goto ERROR;

        audio_file->total_length = (uint32_t)f_size(&audio_file->file_handle);

        if(audio_file->total_length <= 0)
            goto ERROR;
    }

    audio_file->file_state = AUDIO_FILE_STA_OPEN;

    return audio_file;

ERROR:
    if(NULL != audio_file)
        free(audio_file);
    
    return NULL;
}

int audio_file_close(audio_file_t** pp_audio_file)
{
    int ret = 0;
    
    if(NULL == pp_audio_file || NULL == (*pp_audio_file))
        return -1;

    audio_file_t* audio_file = *pp_audio_file;

    if(AUDIO_FILE_TYPE_WEB == audio_file->file_type)
    {
        http_download_stop(&audio_file->http_proc);
        http_download_deinit(&audio_file->http_proc);
        common_buffer_deinit(&audio_file->http_buffer);
    }
    else if(AUDIO_FILE_TYPE_FLASH == audio_file->file_type)
    {
    }
    else if(AUDIO_FILE_TYPE_SD_CARD == audio_file->file_type)
    {
        if(AUDIO_FILE_STA_OPEN == audio_file->file_state && FR_OK != f_close(&audio_file->file_handle))
            ret = -1;
    }

#ifdef AUDIO_FILE_PREREAD_ENALBE
    if(NULL != audio_file->preread_buf) {
        free(audio_file->preread_buf);
        audio_file->preread_buf = NULL;
    }
#endif

    free(audio_file);
    *pp_audio_file = NULL;

    return ret;
}

int audio_file_pause(audio_file_t* audio_file)
{
    if(NULL == audio_file) {
        return -1;
    }

    if(AUDIO_FILE_STA_PAUSE == audio_file->file_state) {
        return 0;
    }

    if(AUDIO_FILE_TYPE_WEB == audio_file->file_type)
    {
        http_download_pause(&audio_file->http_proc);
    }
    else if(AUDIO_FILE_TYPE_FLASH == audio_file->file_type)
    {
    }
    else if(AUDIO_FILE_TYPE_SD_CARD == audio_file->file_type)
    {
        if(FR_OK != f_close(&audio_file->file_handle))
            return -1;
    }

    audio_file->file_state = AUDIO_FILE_STA_PAUSE;

    return 0;
}

int audio_file_resume(audio_file_t* audio_file)
{
    if(NULL == audio_file) {
        return -1;
    }

    if(AUDIO_FILE_STA_OPEN == audio_file->file_state) {
        return 0;
    }

    if(AUDIO_FILE_TYPE_WEB == audio_file->file_type)
    {
        http_download_resume(&audio_file->http_proc);
    }
    else if(AUDIO_FILE_TYPE_FLASH == audio_file->file_type)
    {
    }
    else if(AUDIO_FILE_TYPE_SD_CARD == audio_file->file_type)
    {
        if(FR_OK != f_open(&audio_file->file_handle, _T(audio_file->file_path), FA_OPEN_EXISTING |FA_READ))
            return -1;

        if(FR_OK != f_lseek(&audio_file->file_handle, (FSIZE_t)audio_file->read_pos)) {
            return -1;
        }
    }

    audio_file->file_state = AUDIO_FILE_STA_OPEN;

    return 0;
}

int audio_file_stop(audio_file_t* audio_file)
{
    if(NULL == audio_file) {
        return -1;
    }
    
    if(AUDIO_FILE_TYPE_WEB == audio_file->file_type)
    {
        http_download_stop(&audio_file->http_proc);
    }
    else if(AUDIO_FILE_TYPE_FLASH == audio_file->file_type)
    {
    }
    else if(AUDIO_FILE_TYPE_SD_CARD == audio_file->file_type)
    {
    }

    return 0;
}

static bool audio_file_wait_buffer(audio_file_t* audio_file, int size)
{
    int timeout = AUDIO_FILE_WAIT_HTTP_TIMEOUT;
    timeout += size *(AUDIO_FILE_WAIT_10K_TIMEOUT/100) /10240*100;

    if( true == http_download_is_stopped(&audio_file->http_proc) &&
        false == http_download_is_finish(&audio_file->http_proc))
    {
        int position = common_buffer_get_count(&audio_file->http_buffer) + audio_file->read_pos;
        
        http_download_stop(&audio_file->http_proc);
        http_download_start_seek(&audio_file->http_proc, &audio_file->http_buffer, audio_file->file_path, audio_file->range_enable, position);
    }
    
    if(HTTP_DOWNLOAD_PROC_SUCCESS == http_download_wait_buffer(&audio_file->http_proc, size, timeout)) {
        return true;
    }

    return false;
}

#ifdef AUDIO_FILE_PREREAD_ENALBE
static int audio_file_push_preread(audio_file_t* audio_file, uint8_t* buf, int read_len)
{
    uint8_t *tmp, *p;
    
    p = (uint8_t*)malloc(audio_file->preread_len + read_len);
    if(NULL == p) {
        return audio_file->preread_len;
    }
    
    if(NULL != audio_file->preread_buf) {
        memcpy(p, audio_file->preread_buf, audio_file->preread_len);
    }

    memcpy(&p[audio_file->preread_len], buf, read_len);
    audio_file->preread_len += read_len;

    tmp = audio_file->preread_buf;
    audio_file->preread_buf = p;

    if(NULL != tmp) {
        free(tmp);
    }

    return audio_file->preread_len;
}

static int audio_file_pop_preread(audio_file_t* audio_file, uint8_t* buf, int size)
{
    int read_len = (audio_file->preread_len < size) ?audio_file->preread_len :size;
    
    if(read_len > 0)
    {
        if(NULL != buf) {
            memcpy(buf, audio_file->preread_buf, read_len);
        }
        
        audio_file->preread_len -= read_len;
    }

    if(audio_file->preread_len > 0) {
        memmove(audio_file->preread_buf, &audio_file->preread_buf[read_len], audio_file->preread_len);
    }
    else {
        free(audio_file->preread_buf);
        audio_file->preread_buf = NULL;
    }

    return read_len;
}
#endif

int audio_file_read(audio_file_t* audio_file, uint8_t* buf, int size)
{
    uint32_t read_len, cur_len = 0;
    uint8_t* buffer = NULL;

    if(NULL == audio_file) {
        return -1;
    }

#ifdef AUDIO_FILE_PREREAD_ENALBE
    read_len = audio_file_pop_preread(audio_file, &buf[cur_len], size);
    cur_len += read_len;
    size -= read_len;
#endif

    if(size <= 0) {
        goto END;
    }

    if(AUDIO_FILE_TYPE_WEB == audio_file->file_type)
    {
        while(size > 0)
        {
            read_len = (size < AUDIO_FILE_HTTP_MAX_SEEK) ?size :AUDIO_FILE_HTTP_MAX_SEEK;
            
            if(true == http_download_is_finish(&audio_file->http_proc))
            {
                if(common_buffer_get_count(&audio_file->http_buffer) <= 0) {
                    break;
                }
            }
            else if(common_buffer_get_count(&audio_file->http_buffer) < read_len)
            {
                if(false == audio_file_wait_buffer(audio_file, read_len)) {
                    return -2;
                }
            }
            
            common_buffer_pop(&audio_file->http_buffer, &buf[cur_len], &read_len);
            cur_len += read_len;
            size -= read_len;
            audio_file->read_pos += read_len;
        }

        if(audio_file->total_length < http_download_get_total_length(&audio_file->http_proc))
            audio_file->total_length = http_download_get_total_length(&audio_file->http_proc);
    }
    else if(AUDIO_FILE_TYPE_FLASH == audio_file->file_type)
    {
        if(size + audio_file->read_pos > audio_file->total_length) {
            read_len = audio_file->total_length - audio_file->read_pos;
        }
        else {
            read_len = size;
        }
        
        if(read_len <= 0 || 0 != prompt_audio_data_get_by_offset(&buffer, audio_file->prompt_offset + audio_file->read_pos, read_len))
            goto END;

        memcpy(&buf[cur_len], buffer, read_len);
        cur_len += read_len;
        audio_file->read_pos += read_len;
    }
    else if(AUDIO_FILE_TYPE_SD_CARD == audio_file->file_type)
    {
        UINT bytes_read = 0;
        
        if(FR_OK != f_read(&audio_file->file_handle, &buf[cur_len], size, &bytes_read))
            goto END;

        cur_len += bytes_read;
        audio_file->read_pos += bytes_read;
    }

END:
    if(NULL != buffer)
        free(buffer);
    
    return cur_len;
}

int audio_file_seek(audio_file_t* audio_file, int position)
{
    if(position < 0 || NULL == audio_file) {
        return -1;
    }

    if(audio_file->total_length > 0 && position >= audio_file->total_length) {
        return -1;
    }

    int diff = position - audio_file->read_pos;

#ifdef AUDIO_FILE_PREREAD_ENALBE
    if(diff < 0 && (-diff) <= audio_file->preread_len) {
        audio_file_pop_preread(audio_file, NULL, audio_file->preread_len + diff);
        return 0;
    }
    else {
        audio_file_pop_preread(audio_file, NULL, audio_file->preread_len);
    }
#endif

    if(0 == diff) {
        return 0;
    }
    
    if(AUDIO_FILE_TYPE_WEB == audio_file->file_type)
    {
        if(diff < 0 || diff > AUDIO_FILE_HTTP_MAX_SEEK)
        {
            http_download_stop(&audio_file->http_proc);
            common_buffer_clear(&audio_file->http_buffer);
            http_download_start_seek(&audio_file->http_proc, &audio_file->http_buffer, audio_file->file_path, audio_file->range_enable, position);
        }
        else 
        {
            while(diff > 0)
            {
                uint32_t wait_size = diff;

                if(wait_size > AUDIO_FILE_HTTP_MAX_SEEK)
                    wait_size = AUDIO_FILE_HTTP_MAX_SEEK;

                if(false == audio_file_wait_buffer(audio_file, wait_size)) {
                    return -1;
                }

                common_buffer_pop(&audio_file->http_buffer, NULL, &wait_size);
                diff -= wait_size;
            }
        }
    }
    else if(AUDIO_FILE_TYPE_FLASH == audio_file->file_type)
    {
    }
    else if(AUDIO_FILE_TYPE_SD_CARD == audio_file->file_type)
    {
        if(FR_OK != f_lseek(&audio_file->file_handle, (FSIZE_t)position)) {
            return -1;
        }
    }

    audio_file->read_pos = position;

    return 0;
}

int audio_file_size(audio_file_t* audio_file)
{
    if(NULL == audio_file) {
        return 0;
    }
    
    if(AUDIO_FILE_TYPE_WEB == audio_file->file_type && audio_file->total_length <= 0) {
        audio_file_wait_buffer(audio_file, 1);
        audio_file->total_length = http_download_get_total_length(&audio_file->http_proc);
    }
    
    return audio_file->total_length;
}

int audio_file_tell(audio_file_t* audio_file)
{
    if(NULL == audio_file) {
        return 0;
    }

    return audio_file->read_pos;
}

#ifdef AUDIO_FILE_PREREAD_ENALBE
int audio_file_preread(audio_file_t* audio_file, uint8_t* buf, int size)
{
    int read_len = audio_file_read(audio_file, buf, size);
    return audio_file_push_preread(audio_file, buf, read_len);
}
#endif

