#include "samplerate_convert.h"
#include "common_player.h"
#include <math.h>

#define malloc(x)   pvPortMalloc(x)
#define free(x)     vPortFree(x)

samplerate_convert_t* samplerate_convert_alloc(ring_buffer_t* ring_buffer)
{
    /*int error;*/
    samplerate_convert_t* samplerate = malloc(sizeof(samplerate_convert_t));

    if(NULL == samplerate) {
        return NULL;
    }
    
    memset(samplerate, 0, sizeof(samplerate_convert_t));
    
    samplerate->ring_buffer           = ring_buffer;
    samplerate->src_data.data_in      = samplerate->input_buffer;
    samplerate->src_data.end_of_input = 0;
    samplerate->src_data.input_frames = 0;

    /*samplerate->src_state = src_new(SRC_LINEAR, COM_PLAYER_FIXED_CHANNELS, &error);
    if(NULL == samplerate->src_state) {
        goto ERR;
    }*/
    
    return samplerate;

/*ERR:
    if(NULL != samplerate) {
        free(samplerate);
    }

    return NULL;*/
}

void samplerate_convert_dealloc(samplerate_convert_t** pp_samplerate)
{
    if(NULL == pp_samplerate || NULL == (*pp_samplerate))
        return;

    samplerate_convert_t* samplerate = *pp_samplerate;

    if(NULL != samplerate->src_state) {
        src_delete(samplerate->src_state);
        samplerate->src_state = NULL;
    }

    free(samplerate);
    *pp_samplerate = NULL;
}

static int samplerate_convert_stereo_to_mono(uint8_t* src, int src_len, float* dst, int* dst_len, int channels)
{
    int i, j;
    int16_t tmp16;
    int frame_size = 2 * channels;
    int frame_count = src_len / frame_size;

    if(frame_count > (*dst_len))
        frame_count = *dst_len;

    for(i = 0; i < frame_count; i++)
    {
        for(dst[i] = 0, j = 0; j < channels; j++)
        {
            tmp16   = src[i*frame_size + j*2 + 1];
            tmp16 <<= 8;
            tmp16  |= src[i*frame_size + j*2];

            dst[i] += tmp16;
        }

        dst[i] /= channels;
    }

    *dst_len = frame_count;
    return frame_count * frame_size;
}

static int samplerate_convert_input_stereo(uint8_t* src, int src_len, float* dst, int* dst_len, int channels)
{
    int i, j, count;
    int16_t tmp16;
    int frame_size = 2 * channels;
    int frame_count = src_len / frame_size;

    if(frame_count > (*dst_len))
        frame_count = *dst_len;

    for(count = 0, i = 0; i < frame_count; i++)
    {
        for(j = 0; j < channels; j++)
        {
            tmp16   = src[i*frame_size + j*2 + 1];
            tmp16 <<= 8;
            tmp16  |= src[i*frame_size + j*2];

            if(1 == channels)
                dst[count++] = tmp16;
            
            dst[count++] = tmp16;
        }
    }

    *dst_len = frame_count;
    return frame_count * frame_size;
}

int samplerate_convert_process(samplerate_convert_t* samplerate, double src_ratio, int channels, uint8_t* buffer, int len)
{
    int i, j, error, count;
    int dst_channels = COM_PLAYER_FIXED_CHANNELS;
    int consumed = 0;
    int input_frames;
    int output_frames;
    int16_t tmp16;
    
    if(NULL == samplerate->src_state)
        samplerate->src_state = src_new(SRC_LINEAR, dst_channels, &error);

    if(NULL == samplerate->src_state)
        return 0;

    while(1)
    {
        input_frames = SAMPLERATE_CONVERT_INPUT_SIZE/dst_channels - samplerate->src_data.input_frames;
        output_frames = ring_buffer_get_free_count(samplerate->ring_buffer) /dst_channels/2;

        if(output_frames <= 0)
            break;

        if(1 == dst_channels) {
            consumed += samplerate_convert_stereo_to_mono(
                &buffer[consumed],
                len - consumed, 
                &samplerate->input_buffer[samplerate->src_data.input_frames], 
                &input_frames,
                channels);
        }
        else {
            consumed += samplerate_convert_input_stereo(
                &buffer[consumed],
                len - consumed, 
                &samplerate->input_buffer[samplerate->src_data.input_frames *dst_channels], 
                &input_frames,
                channels);
        }

        if(output_frames > SAMPLERATE_CONVERT_OUTPUT_SIZE/dst_channels)
            output_frames = SAMPLERATE_CONVERT_OUTPUT_SIZE/dst_channels;

        samplerate->src_data.input_frames += input_frames;
        samplerate->src_data.data_out      = samplerate->output_buffer;
        samplerate->src_data.src_ratio     = src_ratio;
        samplerate->src_data.output_frames = output_frames;

        src_process(samplerate->src_state, &samplerate->src_data);

        samplerate->src_data.input_frames -= samplerate->src_data.input_frames_used;

        if(samplerate->src_data.input_frames_used <= 0)
            break;
            
        memmove(samplerate->input_buffer, 
            &samplerate->input_buffer[samplerate->src_data.input_frames_used *dst_channels], 
            samplerate->src_data.input_frames *sizeof(float) *dst_channels);
        
        for(i = 0, count = 0; i < samplerate->src_data.output_frames_gen; i++)
        {
            for(j = 0; j < dst_channels; j++) {
                tmp16 = samplerate->output_buffer[i*dst_channels + j];
                samplerate->output_bytes[count++] = tmp16;
                samplerate->output_bytes[count++] = tmp16 >> 8;
            }
        }

        ring_buffer_push(samplerate->ring_buffer, samplerate->output_bytes, count, false);

        if(consumed >= len)
            break;
    }
    
    return consumed;
}

