#ifndef __AAC_DECODER_H
#define __AAC_DECODER_H

#include "mp3_decoder.h"
#include "neaacdec.h"
#include "mp4ff.h"

//#define DEF_AAC_DECODER_ENABLE

typedef enum {
    AAC_DECODER_SUCCESS = 0,
    AAC_DECODER_ERR_MALLOC,

} aac_decoder_return_t;

typedef enum {
    AAC_DECODER_EVENT_START     = 0x000001UL,
    AAC_DECODER_EVENT_RESUME    = 0x000002UL,
    AAC_DECODER_EVENT_PAUSE     = 0x000004UL,
    AAC_DECODER_EVENT_EXIT      = 0x000008UL,
    AAC_DECODER_EVENT_STOP      = 0x000010UL,
    AAC_DECODER_EVENT_SEEK      = 0x000020UL,

} aac_decoder_event_t;

typedef enum {
    AAC_DECODER_STA_IDLE = 0,
    AAC_DECODER_STA_RUN,
    AAC_DECODER_STA_PAUSE,

} aac_decoder_state_t;

#define AAC_DECODER_INPUT_SIZE      (FAAD_MIN_STREAMSIZE*6)

typedef struct {
	NeAACDecHandle dec_handle;
	audio_decoder_info_t decoder_info;
    mp4ff_callback_t mp4cb;
    mp4ff_t* infile;
    int track;
    long numSamples;
    long sampleId;

    uint8_t input_buffer[AAC_DECODER_INPUT_SIZE];
	int input_remain;

    uint8_t* output_buffer;
    int output_size;
    int output_total;

    int decoder_error;

} aac_decoder_memory_t;

typedef struct {
    com_fsm_t*              com_fsm;
	bool                    is_m4a;
    bool                    input_done;
    bool                    output_done;
    aac_decoder_memory_t*   mem;
    double                  seek_position;
    bool                    initialized;
    
    p_decoder_input_callback    input_callback;
    void*                       input_param;
    p_decoder_error_callback    error_callback;
    void*                       error_param;
    p_decoder_seek_callback     seek_callback;
    void*                       seek_param;
    p_decoder_output_callback   output_callback;
    void*                       output_param;
    
} aac_decoder_t;

#define aac_decoder_get_state(x)                       com_fsm_get_state((x)->com_fsm)
#define aac_decoder_register_input_callback(a, b, c)   do { (a)->input_callback  = b; (a)->input_param  = c; } while(0)
#define aac_decoder_register_error_callback(a, b, c)   do { (a)->error_callback  = b; (a)->error_param  = c; } while(0)
#define aac_decoder_register_seek_callback(a, b, c)    do { (a)->seek_callback   = b; (a)->seek_param   = c; } while(0)
#define aac_decoder_register_output_callback(a, b, c)  do { (a)->output_callback = b; (a)->output_param = c; } while(0)

aac_decoder_return_t aac_decoder_init(aac_decoder_t* aac_decoder);
aac_decoder_return_t aac_decoder_deinit(aac_decoder_t* aac_decoder);
aac_decoder_return_t aac_decoder_start(aac_decoder_t* aac_decoder, bool is_m4a);
aac_decoder_return_t aac_decoder_stop(aac_decoder_t* aac_decoder);
aac_decoder_return_t aac_decoder_pause(aac_decoder_t* aac_decoder, bool from_isr);
aac_decoder_return_t aac_decoder_resume(aac_decoder_t* aac_decoder, bool from_isr);
aac_decoder_return_t aac_decoder_seek(aac_decoder_t* aac_decoder, double seek_position);

void aac_decoder_set_input_done(aac_decoder_t* aac_decoder);
bool aac_decoder_is_output_done(aac_decoder_t* aac_decoder);
bool aac_decoder_is_pause(aac_decoder_t* aac_decoder);

#endif

