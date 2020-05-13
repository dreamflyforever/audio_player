#ifndef __MP3_DECODER_H
#define __MP3_DECODER_H

#include "common_fsm.h"
#include "mad.h"
#include "id3tag.h"
#include "direct_buffer.h"

#define MP3_DECODER_HALF_SAMPLERATE

typedef enum {
    MP3_DECODER_SUCCESS = 0,
    MP3_DECODER_ERR_MALLOC,

} mp3_decoder_return_t;

typedef enum {
    MP3_DECODER_EVENT_START     = 0x000001UL,
    MP3_DECODER_EVENT_RESUME    = 0x000002UL,
    MP3_DECODER_EVENT_PAUSE     = 0x000004UL,
    MP3_DECODER_EVENT_EXIT      = 0x000008UL,
    MP3_DECODER_EVENT_STOP      = 0x000010UL,
    MP3_DECODER_EVENT_SEEK      = 0x000020UL,
    
} mp3_decoder_event_t;

typedef enum {
    MP3_DECODER_STA_IDLE = 0,
    MP3_DECODER_STA_RUN,
    MP3_DECODER_STA_PAUSE,

} mp3_decoder_state_t;

typedef struct {
    uint32_t tag_size;
    uint32_t sample_rate;
    uint32_t bit_rate;
    uint8_t  channels;

} audio_decoder_info_t;

typedef int(*p_decoder_input_callback)(void* param, uint8_t* buf, int size);
typedef int(*p_decoder_error_callback)(void* param, int error);
typedef int(*p_decoder_seek_callback)(void* param, int position);
typedef int(*p_decoder_output_callback)(void* param, audio_decoder_info_t* decoder_info, uint8_t* buf, int size);

#define MP3_DECODER_INPUT_SIZE      (10*1024)
#define MP3_DECODER_OUTPUT_SIZE     (10*1024)

typedef struct {
    struct mad_stream stream;
    struct mad_frame frame;
    struct mad_synth synth;
    audio_decoder_info_t decoder_info;

    uint8_t input_buffer[MP3_DECODER_INPUT_SIZE];

    direct_buffer_t* output_buffer;
    int output_total;

    int decoder_error;

} mp3_decoder_memory_t;

typedef struct {
    com_fsm_t*              com_fsm;
    bool                    input_done;
    bool                    output_done;
    double                  seek_position;
    mp3_decoder_memory_t*   mem;
    
    p_decoder_input_callback    input_callback;
    void*                       input_param;
    p_decoder_error_callback    error_callback;
    void*                       error_param;
    p_decoder_seek_callback     seek_callback;
    void*                       seek_param;
    p_decoder_output_callback   output_callback;
    void*                       output_param;

} mp3_decoder_t;

#define mp3_decoder_get_state(x)                       com_fsm_get_state((x)->com_fsm)
#define mp3_decoder_register_input_callback(a, b, c)   do { (a)->input_callback  = b; (a)->input_param  = c; } while(0)
#define mp3_decoder_register_error_callback(a, b, c)   do { (a)->error_callback  = b; (a)->error_param  = c; } while(0)
#define mp3_decoder_register_seek_callback(a, b, c)    do { (a)->seek_callback   = b; (a)->seek_param   = c; } while(0)
#define mp3_decoder_register_output_callback(a, b, c)  do { (a)->output_callback = b; (a)->output_param = c; } while(0)

mp3_decoder_return_t mp3_decoder_init(mp3_decoder_t* mp3_decoder);
mp3_decoder_return_t mp3_decoder_deinit(mp3_decoder_t* mp3_decoder);
mp3_decoder_return_t mp3_decoder_start(mp3_decoder_t* mp3_decoder);
mp3_decoder_return_t mp3_decoder_stop(mp3_decoder_t* mp3_decoder);
mp3_decoder_return_t mp3_decoder_pause(mp3_decoder_t* mp3_decoder, bool from_isr);
mp3_decoder_return_t mp3_decoder_resume(mp3_decoder_t* mp3_decoder, bool from_isr);
mp3_decoder_return_t mp3_decoder_seek(mp3_decoder_t* mp3_decoder, double seek_position);

void mp3_decoder_set_input_done(mp3_decoder_t* mp3_decoder);
bool mp3_decoder_is_output_done(mp3_decoder_t* mp3_decoder);
bool mp3_decoder_is_pause(mp3_decoder_t* mp3_decoder);

#endif

