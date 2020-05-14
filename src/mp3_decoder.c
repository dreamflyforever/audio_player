#include "mp3_decoder.h"
#include <string.h>

#define malloc(x)   pvPortMalloc(x)
#define free(x)     vPortFree(x)

log_create_module(mp3_decoder, PRINT_LEVEL_ERROR);

#define MP3_DECODER_TASK_STACK_SIZE (20*1024/sizeof(StackType_t))
#define MP3_DECODER_MAX_ERR_COUNT   (20)

static void mp3_decoder_dealloc_memory(mp3_decoder_memory_t** pp_mem)
{
    if(NULL == pp_mem || NULL == (*pp_mem))
        return;

    mp3_decoder_memory_t* tmp = *pp_mem;

    mad_synth_finish(&tmp->synth);
    mad_frame_finish(&tmp->frame);
    mad_stream_finish(&tmp->stream);

    direct_buffer_destroy(&tmp->output_buffer);

    free(tmp);
    *pp_mem = NULL;
}

static void mp3_decoder_alloc_memory(mp3_decoder_memory_t** pp_mem)
{
    if(NULL == pp_mem)
        return;

    mp3_decoder_dealloc_memory(pp_mem);

    mp3_decoder_memory_t* tmp = (mp3_decoder_memory_t*)malloc(sizeof(mp3_decoder_memory_t));
    if(NULL == tmp) {
        LOG_E(mp3_decoder, "alloc memory failed!");
        return;
    }

    memset(tmp, 0, sizeof(mp3_decoder_memory_t));
    
    if(NULL == (tmp->output_buffer=direct_buffer_create(512))) {
        LOG_E(mp3_decoder, "alloc memory failed!");
        free(tmp);
        return;
    }

    mad_stream_init(&tmp->stream);
    mad_frame_init(&tmp->frame);
    mad_synth_init(&tmp->synth);

    *pp_mem = tmp;
}

static void mp3_decoder_seek_handler(mp3_decoder_t* mp3_decoder, mp3_decoder_memory_t* mem)
{
    if(NULL == mem || NULL == mp3_decoder->seek_callback)
        return;

    direct_buffer_clear(mem->output_buffer);

    mp3_decoder->seek_callback(mp3_decoder->seek_param, mp3_decoder->seek_position);
}

static signed int scale(mad_fixed_t sample)
{
    /* round */
    sample += (1L << (MAD_F_FRACBITS - 16));

    /* clip */
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;

    /* quantize */
    return sample >> (MAD_F_FRACBITS + 1 - 16);
}

static bool mp3_decoder_output_handler(mp3_decoder_t* mp3_decoder, mp3_decoder_memory_t* mem)
{
    int count = direct_buffer_get_count(mem->output_buffer);
    uint8_t* buffer = direct_buffer_get_base(mem->output_buffer);
    
    if(count <= 0)
        return true;

    count = mp3_decoder->output_callback(mp3_decoder->output_param, &mem->decoder_info, buffer, count);
    direct_buffer_pop_only(mem->output_buffer, count);
    mem->output_total += count;

    if(direct_buffer_get_count(mem->output_buffer) > 0) {
        return false;
    }

    direct_buffer_clear(mem->output_buffer);

    return true;
}

static void mp3_decoder_error_handler(mp3_decoder_t* mp3_decoder, mp3_decoder_memory_t* mem)
{
    if(NULL == mem || NULL == mp3_decoder->error_callback)
        return;

    if(mem->decoder_error >= MP3_DECODER_MAX_ERR_COUNT) {
        mp3_decoder->error_callback(mp3_decoder->error_param, mem->stream.error);
    }
    else if(true == mp3_decoder->output_done && mem->output_total <= 0) {
        mp3_decoder->error_callback(mp3_decoder->error_param, mem->stream.error);
    }
}

static uint32_t mp3_decoder_start_action(void* param)
{
    mp3_decoder_t* mp3_decoder = (mp3_decoder_t*)param;

    mp3_decoder_alloc_memory(&mp3_decoder->mem);

    LOG_I(mp3_decoder, "--> MP3_DECODER_STA_RUN");
    return MP3_DECODER_STA_RUN;
}

static uint32_t mp3_decoder_stop_action(void* param)
{
    mp3_decoder_t* mp3_decoder = (mp3_decoder_t*)param;
    
    mp3_decoder_dealloc_memory(&mp3_decoder->mem);

    LOG_I(mp3_decoder, "--> MP3_DECODER_STA_IDLE");
    return MP3_DECODER_STA_IDLE;
}

static uint32_t mp3_decoder_pause_action(void* param)
{
    //mp3_decoder_t* mp3_decoder = (mp3_decoder_t*)param;
    
    //LOG_I(mp3_decoder, "--> MP3_DECODER_STA_PAUSE");
    return MP3_DECODER_STA_PAUSE;
}

static uint32_t mp3_decoder_resume_action(void* param)
{
    //mp3_decoder_t* mp3_decoder = (mp3_decoder_t*)param;

    //LOG_I(mp3_decoder, "--> MP3_DECODER_STA_RUN");
    return MP3_DECODER_STA_RUN;
}

static uint32_t mp3_decoder_seek_action(void* param)
{
    mp3_decoder_t* mp3_decoder = (mp3_decoder_t*)param;

    mp3_decoder_seek_handler(mp3_decoder, mp3_decoder->mem);
    
    return COM_FSM_HOLD_STATE;
}

static uint32_t mp3_decoder_run_poll(void* param)
{
    mp3_decoder_t* mp3_decoder = (mp3_decoder_t*)param;
    mp3_decoder_memory_t* mem = mp3_decoder->mem;
    int i, j, tmp;

    mp3_decoder_error_handler(mp3_decoder, mem);

    if(false == mp3_decoder_output_handler(mp3_decoder, mem)) {
        //LOG_I(mp3_decoder, "--> MP3_DECODER_STA_PAUSE");
        return MP3_DECODER_STA_PAUSE;
    }

    /* ---------------- [step 1] input mp3 ---------------- */
    if(NULL == mem->stream.buffer || MAD_ERROR_BUFLEN == mem->stream.error)
    {
        int input_size = MP3_DECODER_INPUT_SIZE;
        int remain_size = 0;

        if(true == mp3_decoder->input_done) {
            mp3_decoder->output_done = true;
            LOG_I(mp3_decoder, "mp3_decoder input done");
            return MP3_DECODER_STA_PAUSE;
        }

        if(NULL != mem->stream.next_frame)
        {
            remain_size = mem->stream.bufend - mem->stream.next_frame;
            input_size  = MP3_DECODER_INPUT_SIZE - remain_size;
            memmove(mem->input_buffer, mem->stream.next_frame, remain_size);
        }

        input_size = mp3_decoder->input_callback(mp3_decoder->input_param, &mem->input_buffer[remain_size], input_size);
            
        if(input_size > 0) {
            mad_stream_buffer(&mem->stream, mem->input_buffer, input_size + remain_size);
        }
        else {
            LOG_I(mp3_decoder, "mp3_decoder pause because no input");
            return MP3_DECODER_STA_PAUSE;
        }

        mem->stream.error = MAD_ERROR_NONE;
    }

    if(mem->decoder_info.tag_size <= 0) {
        mem->decoder_info.tag_size = id3_tag_query(mem->stream.this_frame, mem->stream.bufend - mem->stream.this_frame);
    }

    /* ---------------- [step 2] mp3 to pcm ---------------- */
    if(MAD_ERROR_NONE != mad_frame_decode(&mem->frame, &mem->stream))
    {
        if(MAD_ERROR_BUFLEN == mem->stream.error) {
            goto END;
        }
        else if(MAD_ERROR_LOSTSYNC == mem->stream.error) {
            int tagsize = id3_tag_query(mem->stream.this_frame, mem->stream.bufend - mem->stream.this_frame);
            if(tagsize > 0)
                mad_stream_skip(&mem->stream, tagsize);

            mem->decoder_error++;
        }
        else if(MAD_RECOVERABLE(mem->stream.error)) {
        }
        else {
            mem->decoder_error++;
            LOG_E(mp3_decoder, "mp3_decoder error 0x%04x (%s)", mem->stream.error, mad_stream_errorstr(&mem->stream));
            return MP3_DECODER_STA_PAUSE;
        }
        
        goto END;
    }
    else
    {
        mem->decoder_error = 0;
    }

    /* ---------------- [step 3] output pcm ---------------- */
#ifdef MP3_DECODER_HALF_SAMPLERATE
    if(mem->frame.header.samplerate >= 44100) {
        mem->frame.options |= MAD_OPTION_HALFSAMPLERATE;
    }
#endif
    mad_synth_frame(&mem->synth, &mem->frame);
    mem->decoder_info.sample_rate = mem->frame.header.samplerate;
    mem->decoder_info.bit_rate    = mem->frame.header.bitrate;
    mem->decoder_info.channels    = mem->synth.pcm.channels;

#ifdef MP3_DECODER_HALF_SAMPLERATE
    if(MAD_OPTION_HALFSAMPLERATE & mem->frame.options) {
        mem->decoder_info.sample_rate /= 2;
    }
#endif
    direct_buffer_resize(mem->output_buffer, mem->synth.pcm.length*mem->synth.pcm.channels*sizeof(uint16_t));
    direct_buffer_clear(mem->output_buffer);

    for(i = 0; i < mem->synth.pcm.length; i++)
    {
        for(j = 0; j < mem->synth.pcm.channels; j++) {
            tmp = scale(mem->synth.pcm.samples[j][i]);
            direct_buffer_push_one(mem->output_buffer, tmp);
            direct_buffer_push_one(mem->output_buffer, tmp >> 8);
        }
    }

    if(false == mp3_decoder_output_handler(mp3_decoder, mem)) {
        return MP3_DECODER_STA_PAUSE;
    }

END:
    return COM_FSM_HOLD_STATE;
}

mp3_decoder_return_t mp3_decoder_init(mp3_decoder_t* mp3_decoder)
{
    const com_fsm_lookup_t lookup[] = {
        { MP3_DECODER_EVENT_START,  MP3_DECODER_STA_IDLE,  mp3_decoder_start_action, },
        { MP3_DECODER_EVENT_STOP,   MP3_DECODER_STA_RUN,   mp3_decoder_stop_action, },
        { MP3_DECODER_EVENT_STOP,   MP3_DECODER_STA_PAUSE, mp3_decoder_stop_action, },
        { MP3_DECODER_EVENT_PAUSE,  MP3_DECODER_STA_RUN,   mp3_decoder_pause_action, },
        { MP3_DECODER_EVENT_SEEK,   MP3_DECODER_STA_RUN,   mp3_decoder_seek_action, },
        { MP3_DECODER_EVENT_SEEK,   MP3_DECODER_STA_PAUSE, mp3_decoder_seek_action, },
        { MP3_DECODER_EVENT_RESUME, MP3_DECODER_STA_PAUSE, mp3_decoder_resume_action, },
            
        { COM_FSM_EVENT_NONE, MP3_DECODER_STA_RUN, mp3_decoder_run_poll, 0, },
    };

    memset(mp3_decoder, 0, sizeof(mp3_decoder_t));
    
    mp3_decoder->input_done  = false;
    mp3_decoder->output_done = false;

    mp3_decoder->com_fsm = com_fsm_create(
        "mp3_decoder", 
        MP3_DECODER_TASK_STACK_SIZE, 
        TASK_PRIORITY_HIGH, 
        mp3_decoder,
        lookup,
        sizeof(lookup)/sizeof(com_fsm_lookup_t)
        );
    
    return MP3_DECODER_SUCCESS;
}

mp3_decoder_return_t mp3_decoder_deinit(mp3_decoder_t* mp3_decoder)
{
    com_fsm_delete(&mp3_decoder->com_fsm);
    return MP3_DECODER_SUCCESS;
}

mp3_decoder_return_t mp3_decoder_start(mp3_decoder_t* mp3_decoder)
{
    mp3_decoder->input_done = false;
    mp3_decoder->output_done = false;
    com_fsm_set_event(mp3_decoder->com_fsm, MP3_DECODER_EVENT_START, 0);

    return MP3_DECODER_SUCCESS;
}

mp3_decoder_return_t mp3_decoder_stop(mp3_decoder_t* mp3_decoder)
{
    com_fsm_set_event(mp3_decoder->com_fsm, MP3_DECODER_EVENT_STOP, COM_FSM_MAX_WAIT_TIME);
    
    return MP3_DECODER_SUCCESS;
}

mp3_decoder_return_t mp3_decoder_pause(mp3_decoder_t* mp3_decoder, bool from_isr)
{
    if(true == from_isr) {
        com_fsm_set_event_from_isr(mp3_decoder->com_fsm, MP3_DECODER_EVENT_PAUSE);
    }
    else {
        com_fsm_set_event(mp3_decoder->com_fsm, MP3_DECODER_EVENT_PAUSE, COM_FSM_MAX_WAIT_TIME);
    }

    return MP3_DECODER_SUCCESS;
}

mp3_decoder_return_t mp3_decoder_resume(mp3_decoder_t* mp3_decoder, bool from_isr)
{
    if(true == from_isr) {
        com_fsm_set_event_from_isr(mp3_decoder->com_fsm, MP3_DECODER_EVENT_RESUME);
    }
    else {
        com_fsm_set_event(mp3_decoder->com_fsm, MP3_DECODER_EVENT_RESUME, COM_FSM_MAX_WAIT_TIME);
    }

    return MP3_DECODER_SUCCESS;
}

mp3_decoder_return_t mp3_decoder_seek(mp3_decoder_t* mp3_decoder, double seek_position)
{
    mp3_decoder->seek_position = seek_position;
    com_fsm_set_event(mp3_decoder->com_fsm, MP3_DECODER_EVENT_SEEK, COM_FSM_MAX_WAIT_TIME);
    
    return MP3_DECODER_SUCCESS;
}

void mp3_decoder_set_input_done(mp3_decoder_t* mp3_decoder)
{
    mp3_decoder->input_done = true;
}

bool mp3_decoder_is_output_done(mp3_decoder_t* mp3_decoder)
{
    return mp3_decoder->output_done;
}

bool mp3_decoder_is_pause(mp3_decoder_t* mp3_decoder)
{
    return (MP3_DECODER_STA_PAUSE==mp3_decoder_get_state(mp3_decoder)) ?true :false;
}

