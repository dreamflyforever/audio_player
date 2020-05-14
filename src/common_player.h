#ifndef __COMMMON_PLAYER_H
#define __COMMMON_PLAYER_H

#include "mp3_decoder.h"
#include "aac_decoder.h"
#include "ring_buffer.h"
#include "pcm_trans.h"
#include "samplerate_convert.h"
#include "blisrc_convert.h"

#define COM_PLAYER_FIXED_SAMPLERATE         32000
#if defined(ES8388_ENABLE) && defined(STEREO_ENABLE)
#define COM_PLAYER_FIXED_CHANNELS           2
#else
#define COM_PLAYER_FIXED_CHANNELS           1
#endif
#define COM_PLAYER_AUDIO_TYPE_HEADER_SIZE   8

typedef enum {
    COM_PLAYER_SUCCESS = 0,
    COM_PLAYER_ERR_PARAM,
    COM_PLAYER_ERR_PCM_TRANS,
    COM_PLAYER_ERR_EXIT,
    COM_PLAYER_ERR_TIMEOUT,
    COM_PLAYER_ERR_SEEK,
    
} com_player_return_t;

typedef enum {
    COM_PLAYER_EVENT_NONE     = 0x000000UL,
    COM_PLAYER_EVENT_ALL      = 0xFFFFFFUL,
    COM_PLAYER_EVENT_DECODER  = 0x000001UL,
    COM_PLAYER_EVENT_EXIT     = 0x000002UL,
    
} com_player_event_t;

typedef enum {
    COM_PLAYER_TYPE_MP3 = 0,
    COM_PLAYER_TYPE_AAC,
    COM_PLAYER_TYPE_M4A,

} com_player_type_t;

#ifdef DEF_LINUX_PLATFORM
#define com_player_convert_t            samplerate_convert_t
#define com_player_convert_alloc        samplerate_convert_alloc
#define com_player_convert_dealloc      samplerate_convert_dealloc

#define com_player_convert_process(converter, in_sample, in_chann, out_sample, out_chann, buffer, len) \
    samplerate_convert_process(converter, 1.0 *out_sample /in_sample, in_chann, buffer, len)

#else
#define com_player_convert_t            blisrc_convert_t
#define com_player_convert_alloc        blisrc_convert_alloc
#define com_player_convert_dealloc      blisrc_convert_dealloc
#define com_player_convert_process      blisrc_convert_process

#endif

typedef struct {
    EventGroupHandle_t    event_handle;
    com_player_type_t     decoder_type;
    mp3_decoder_t         mp3_decoder;
    aac_decoder_t         aac_decoder;
    ring_buffer_t         output_buffer;
    audio_decoder_info_t  decoder_info;
    bool                  wait_decoder;
    bool                  input_done;
    uint32_t              consumed;
    bool                  enable_pcm_or_decoder;
    com_player_convert_t* converter;
    
} com_player_t;

com_player_return_t com_player_init(com_player_t* com_player);
com_player_return_t com_player_deinit(com_player_t* com_player);
com_player_return_t com_player_start(
    com_player_t* com_player,
    com_player_type_t audio_type,
    p_decoder_input_callback input_callback,
    p_decoder_error_callback error_callback,
    p_decoder_seek_callback seek_callback,
    void* callback_param);

com_player_return_t com_player_stop(com_player_t* com_player);
com_player_return_t com_player_pause(com_player_t* com_player);
com_player_return_t com_player_resume(com_player_t* com_player);
com_player_return_t com_player_seek(com_player_t* com_player, int total_len, double seek_progress);

void com_player_set_done(com_player_t* com_player, bool error_occur);
bool com_player_is_done(com_player_t* com_player);
bool com_player_auto_resume(com_player_t* com_player);
bool com_player_get_progress(com_player_t* com_player, int cur_pos, int total_len, int* cur_time, int* all_time);
com_player_type_t com_player_get_audio_type_by_name(char* path);
com_player_type_t com_player_get_audio_type_by_header(uint8_t* buf);


#endif

