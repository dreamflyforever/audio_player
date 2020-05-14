#ifndef __AUDIO_FILE_H
#define __AUDIO_FILE_H

#include "ff.h"
#include "http_download_process.h"

#define AUDIO_FILE_MAX_PATH_SIZE  2048
#define AUDIO_FILE_PREREAD_ENALBE

typedef enum {
    AUDIO_FILE_TYPE_WEB = 0,
    AUDIO_FILE_TYPE_FLASH,
    AUDIO_FILE_TYPE_SD_CARD,

} audio_file_type_t;

typedef enum {
    AUDIO_FILE_STA_CLOSE = 0,
    AUDIO_FILE_STA_OPEN,
    AUDIO_FILE_STA_PAUSE,

} audio_file_state_t;

typedef struct {
	char                  file_path[AUDIO_FILE_MAX_PATH_SIZE];
    audio_file_state_t    file_state;
	audio_file_type_t     file_type;
    FIL                   file_handle;
    uint32_t              total_length;
    uint32_t              prompt_offset;
    uint32_t              read_pos;
    common_buffer_t       http_buffer;
    http_download_proc_t  http_proc;
    bool                  range_enable;

#ifdef AUDIO_FILE_PREREAD_ENALBE
    uint8_t*              preread_buf;
    int                   preread_len;
#endif
    
} audio_file_t;

audio_file_t* audio_file_open(char* file_path, audio_file_type_t audio_type, bool range_enable);
int audio_file_close(audio_file_t** pp_audio_file);
int audio_file_pause(audio_file_t* audio_file);
int audio_file_resume(audio_file_t* audio_file);
int audio_file_stop(audio_file_t* audio_file);
int audio_file_read(audio_file_t* audio_file, uint8_t* buf, int size);
int audio_file_seek(audio_file_t* audio_file, int position);
int audio_file_size(audio_file_t* audio_file);
int audio_file_tell(audio_file_t* audio_file);
int audio_file_preread(audio_file_t* audio_file, uint8_t* buf, int size);

#endif

