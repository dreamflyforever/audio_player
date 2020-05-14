#include "aac_decoder.h"
#include <string.h>

#ifdef DEF_AAC_DECODER_ENABLE

#define malloc(x)   pvPortMalloc(x)
#define free(x)     vPortFree(x)

log_create_module(aac_decoder, PRINT_LEVEL_INFO);

#define AAC_DECODER_TASK_STACK_SIZE (50*1024/sizeof(StackType_t))
#define AAC_DECODER_MAX_ERR_COUNT   (20)

static uint32_t read_callback(void *user_data, void *buffer, uint32_t length)
{
    aac_decoder_t* aac_decoder = (aac_decoder_t*)user_data;

    return aac_decoder->input_callback(aac_decoder->input_param, buffer, length);
}

static uint32_t seek_callback(void *user_data, uint64_t position)
{
    aac_decoder_t* aac_decoder = (aac_decoder_t*)user_data;
    
    return aac_decoder->seek_callback(aac_decoder->seek_param, (int)position);
}

static int GetAACTrack(mp4ff_t *infile)
{
    /* find AAC track */
    int i, rc;
    int numTracks = mp4ff_total_tracks(infile);

    for (i = 0; i < numTracks; i++)
    {
        unsigned char *buff = NULL;
        unsigned int buff_size = 0;
        mp4AudioSpecificConfig mp4ASC;

        mp4ff_get_decoder_config(infile, i, &buff, &buff_size);

        if (buff)
        {
            rc = NeAACDecAudioSpecificConfig(buff, buff_size, &mp4ASC);
            free(buff);

            if (rc < 0)
                continue;
            return i;
        }
    }

    /* can't decode this */
    return -1;
}



static void aac_decoder_dealloc_memory(aac_decoder_memory_t** pp_mem)
{
    if(NULL == pp_mem || NULL == (*pp_mem))
        return;

    aac_decoder_memory_t* tmp = *pp_mem;
	
	NeAACDecClose(tmp->dec_handle);

    if(NULL != tmp->output_buffer) {
        free(tmp->output_buffer);
        tmp->output_buffer = NULL;
    }

    if(NULL != tmp->infile) {
        mp4ff_close(tmp->infile);
        tmp->infile = NULL;
    }

    free(tmp);
    *pp_mem = NULL;
}

static void aac_decoder_alloc_memory(aac_decoder_memory_t** pp_mem)
{
    if(NULL == pp_mem)
        return;

    aac_decoder_dealloc_memory(pp_mem);

    aac_decoder_memory_t* tmp = (aac_decoder_memory_t*)malloc(sizeof(aac_decoder_memory_t));
    if(NULL == tmp) {
        LOG_E(aac_decoder, "alloc memory failed!");
        return;
    }

    memset(tmp, 0, sizeof(aac_decoder_memory_t));
	tmp->input_remain = 0;

	tmp->dec_handle = NeAACDecOpen();
	
	NeAACDecConfigurationPtr config = NeAACDecGetCurrentConfiguration(tmp->dec_handle);
	
	config->outputFormat = FAAD_FMT_16BIT;
    NeAACDecSetConfiguration(tmp->dec_handle, config);

    *pp_mem = tmp;
}

static bool aac_decoder_init_handler(aac_decoder_t* aac_decoder, aac_decoder_memory_t* mem)
{
	int ret = 0;
	unsigned long sample_rate;
	unsigned char channels;
    unsigned char* buffer;
    unsigned int buffer_size;
	
	if(true == aac_decoder->is_m4a)
	{
        mem->mp4cb.read = read_callback;
        mem->mp4cb.seek = seek_callback;
        mem->mp4cb.user_data = aac_decoder;
        mem->infile = mp4ff_open_read(&mem->mp4cb);

        if(NULL == mem->infile) {
            LOG_E(aac_decoder, "mp4ff_open_read error");
            return false;
        }

        mem->track = GetAACTrack(mem->infile);
        if(mem->track < 0) {
            LOG_E(aac_decoder, "Unable to find correct AAC sound track in the MP4 file.");
            return false;
        }

        mp4ff_get_decoder_config(mem->infile, mem->track, &buffer, &buffer_size);
    
		ret = NeAACDecInit2(mem->dec_handle, buffer, buffer_size, &sample_rate, &channels);

        if(NULL != buffer) {
            free(buffer);
        }

        if(ret >= 0) {
            mem->numSamples = mp4ff_num_samples(mem->infile, mem->track);
            mem->decoder_info.bit_rate = mp4ff_get_avg_bitrate(mem->infile, mem->track);
            mem->decoder_info.tag_size = mp4ff_get_track_duration(mem->infile, mem->track);
        }
	}
	else
	{
		ret = NeAACDecInit(mem->dec_handle, mem->input_buffer, mem->input_remain, &sample_rate, &channels);
		
		if(memcmp(mem->input_buffer, "ADIF", 4) == 0)
		{
			int skip_size = (mem->input_buffer[4] & 0x80) ? 9 : 0;
			int bitrate =
				((unsigned int)(mem->input_buffer[4 + skip_size] & 0x0F)<<19) |
				((unsigned int)mem->input_buffer[5 + skip_size]<<11) |
				((unsigned int)mem->input_buffer[6 + skip_size]<<3) |
				((unsigned int)(mem->input_buffer[7 + skip_size] & 0xE0)>>5);

			mem->decoder_info.bit_rate = bitrate;
		}
		else if(mem->input_buffer[0] == 0xFF && (mem->input_buffer[1] & 0xF6) == 0xF0) // ADTS
        {
            int frame_len =
				((unsigned int)(mem->input_buffer[3] & 0x03)<<11) |
				((unsigned int)mem->input_buffer[4]<<3) |
				((unsigned int)(mem->input_buffer[5] & 0xE0)>>5);

            mem->decoder_info.bit_rate = frame_len*8*sample_rate/1024;
		}
	}
	
	if(ret < 0) {
		LOG_E(aac_decoder, "NeAACDecInit(2) error");
		return false;
	}
	
	mem->decoder_info.sample_rate = sample_rate;
	mem->decoder_info.channels = channels;

    return true;
}

static bool aac_decoder_output_handler(aac_decoder_t* aac_decoder, aac_decoder_memory_t* mem)
{
    if(NULL == mem->output_buffer)
        return true;

    int size = aac_decoder->output_callback(aac_decoder->output_param, &mem->decoder_info, mem->output_buffer, mem->output_size);
    mem->output_total += size;

    if(size < mem->output_size)
    {
        mem->output_size -= size;
        memmove(mem->output_buffer, &mem->output_buffer[size], mem->output_size);
        return false;
    }

    free(mem->output_buffer);
    mem->output_buffer = NULL;
    mem->output_size = 0;

    return true;
}

static void aac_decoder_error_handler(aac_decoder_t* aac_decoder, aac_decoder_memory_t* mem)
{
    if(NULL == mem || NULL == aac_decoder->error_callback)
        return;

    if(mem->decoder_error >= AAC_DECODER_MAX_ERR_COUNT) {
        aac_decoder->error_callback(aac_decoder->error_param, mem->decoder_error);
    }
    else if(true == aac_decoder->output_done && mem->output_total <= 0) {
        aac_decoder->error_callback(aac_decoder->error_param, mem->decoder_error);
    }
}

static void aac_decoder_seek_handler(aac_decoder_t* aac_decoder, aac_decoder_memory_t* mem)
{
    if(NULL == mem || NULL == aac_decoder->seek_callback)
        return;

    if(NULL != mem->output_buffer) {
        free(mem->output_buffer);
        mem->output_buffer = NULL;
        mem->output_size = 0;
    }

    if(true == aac_decoder->is_m4a)
    {
        if(mem->numSamples > 0) {
            mem->sampleId = aac_decoder->seek_position *mem->numSamples;
        }
    }
}

static uint32_t aac_decoder_start_action(void* param)
{
    aac_decoder_t* aac_decoder = (aac_decoder_t*)param;

    aac_decoder_alloc_memory(&aac_decoder->mem);
    aac_decoder->initialized = false;

    LOG_I(aac_decoder, "--> AAC_DECODER_STA_RUN");
    return AAC_DECODER_STA_RUN;
}

static uint32_t aac_decoder_stop_action(void* param)
{
    aac_decoder_t* aac_decoder = (aac_decoder_t*)param;
    
    aac_decoder_dealloc_memory(&aac_decoder->mem);

    LOG_I(aac_decoder, "--> AAC_DECODER_STA_IDLE");
    return AAC_DECODER_STA_IDLE;
}

static uint32_t aac_decoder_pause_action(void* param)
{
    //aac_decoder_t* aac_decoder = (aac_decoder_t*)param;
    
    //LOG_I(aac_decoder, "--> AAC_DECODER_STA_PAUSE");
    return AAC_DECODER_STA_PAUSE;
}

static uint32_t aac_decoder_resume_action(void* param)
{
    //aac_decoder_t* aac_decoder = (aac_decoder_t*)param;

    //LOG_I(aac_decoder, "--> AAC_DECODER_STA_RUN");
    return AAC_DECODER_STA_RUN;
}

static uint32_t aac_decoder_seek_action(void* param)
{
    aac_decoder_t* aac_decoder = (aac_decoder_t*)param;

    aac_decoder_seek_handler(aac_decoder, aac_decoder->mem);
    
    return COM_FSM_HOLD_STATE;
}

static uint32_t aac_decoder_run_poll(void* param)
{
    aac_decoder_t* aac_decoder = (aac_decoder_t*)param;
    aac_decoder_memory_t* mem = aac_decoder->mem;
    NeAACDecFrameInfo frame_info;
	void *sample_buffer;
    
    aac_decoder_error_handler(aac_decoder, mem);

    if(false == aac_decoder_output_handler(aac_decoder, mem)) {
        return AAC_DECODER_STA_PAUSE;
    }

    /* ---------------- [step 1] aac decoder ---------------- */
    if(true == aac_decoder->input_done) {
        aac_decoder->output_done = true;
        LOG_I(aac_decoder, "aac_decoder input done");
        return AAC_DECODER_STA_PAUSE;
    }
        
    if(false == aac_decoder->is_m4a)
    {
        int input_size = aac_decoder->input_callback(aac_decoder->input_param, 
            &mem->input_buffer[mem->input_remain], AAC_DECODER_INPUT_SIZE - mem->input_remain);

        if(input_size <= 0) {
            LOG_I(aac_decoder, "aac_decoder pause because no input");
            return AAC_DECODER_STA_PAUSE;
        }

        mem->input_remain += input_size;
    }

    if(false == aac_decoder->initialized)
		aac_decoder->initialized = aac_decoder_init_handler(aac_decoder, mem);

    if(false == aac_decoder->initialized) {
        mem->decoder_error = AAC_DECODER_MAX_ERR_COUNT;
        goto END;
    }

    if(true == aac_decoder->is_m4a)
    {
        if(mem->sampleId >= mem->numSamples) {
            aac_decoder->input_callback(aac_decoder->input_param, &mem->input_buffer[mem->input_remain], 0);
        }
        else {
            unsigned char* buffer;
            unsigned int buffer_size;
                
            /*long dur = */mp4ff_get_sample_duration(mem->infile, mem->track, mem->sampleId);
            /*int rc = */mp4ff_read_sample(mem->infile, mem->track, mem->sampleId, &buffer,  &buffer_size);

            sample_buffer = NeAACDecDecode(mem->dec_handle, &frame_info, buffer, buffer_size);

            if(NULL != buffer) {
                free(buffer);
                buffer = NULL;
            }
                
            mem->sampleId++;

            //LOG_I(aac_decoder, "NeAACDecDecode: %d/%d", mem->sampleId, mem->numSamples);
        }
    }
    else
    {
        sample_buffer = NeAACDecDecode(mem->dec_handle, &frame_info, mem->input_buffer, mem->input_remain);
        mem->input_remain -= frame_info.bytesconsumed;

        if(mem->input_remain > 0) {
    		memmove(mem->input_buffer, &mem->input_buffer[frame_info.bytesconsumed], mem->input_remain);
    	}
	}
		
	if(frame_info.error != 0) {
		LOG_E(aac_decoder, "NeAACDecDecode error: %s", NeAACDecGetErrorMessage(frame_info.error));
        mem->input_remain = 0;
		mem->decoder_error = AAC_DECODER_MAX_ERR_COUNT;
	}
	else if(frame_info.samples > 0) {
		mem->output_size = frame_info.samples*sizeof(uint16_t);
		mem->output_buffer = (uint8_t*)malloc(mem->output_size);
					
		for(int j = 0; j < frame_info.samples; j++) {
			mem->output_buffer[2*j+0] = ((uint16_t*)sample_buffer)[j];
			mem->output_buffer[2*j+1] = ((uint16_t*)sample_buffer)[j] >> 8;
		}

        mem->decoder_error = 0;
					
		if(false == aac_decoder_output_handler(aac_decoder, mem)) {
			return AAC_DECODER_STA_PAUSE;
		}
	}

END:
    return COM_FSM_HOLD_STATE;
}

aac_decoder_return_t aac_decoder_init(aac_decoder_t* aac_decoder)
{
    const com_fsm_lookup_t lookup[] = {
        { AAC_DECODER_EVENT_START,  AAC_DECODER_STA_IDLE,  aac_decoder_start_action, },
        { AAC_DECODER_EVENT_STOP,   AAC_DECODER_STA_RUN,   aac_decoder_stop_action, },
        { AAC_DECODER_EVENT_STOP,   AAC_DECODER_STA_PAUSE, aac_decoder_stop_action, },
        { AAC_DECODER_EVENT_PAUSE,  AAC_DECODER_STA_RUN,   aac_decoder_pause_action, },
        { AAC_DECODER_EVENT_RESUME, AAC_DECODER_STA_PAUSE, aac_decoder_resume_action, },
        { AAC_DECODER_EVENT_SEEK,   AAC_DECODER_STA_RUN,   aac_decoder_seek_action, },
        { AAC_DECODER_EVENT_SEEK,   AAC_DECODER_STA_PAUSE, aac_decoder_seek_action, },
        
        { COM_FSM_EVENT_NONE, AAC_DECODER_STA_RUN, aac_decoder_run_poll, 0, },
    };

    memset(aac_decoder, 0, sizeof(aac_decoder_t));
    aac_decoder->input_done      = false;
    aac_decoder->output_done     = false;
	aac_decoder->is_m4a          = false;
	
    aac_decoder->com_fsm = com_fsm_create(
        "aac_decoder", 
        AAC_DECODER_TASK_STACK_SIZE, 
        TASK_PRIORITY_HIGH, 
        aac_decoder,
        lookup,
        sizeof(lookup)/sizeof(com_fsm_lookup_t)
        );
    
    return AAC_DECODER_SUCCESS;
}

aac_decoder_return_t aac_decoder_deinit(aac_decoder_t* aac_decoder)
{
    com_fsm_delete(&aac_decoder->com_fsm);
    return AAC_DECODER_SUCCESS;
}

aac_decoder_return_t aac_decoder_start(aac_decoder_t* aac_decoder, bool is_m4a)
{
    aac_decoder->is_m4a      = is_m4a;
    aac_decoder->input_done  = false;
    aac_decoder->output_done = false;
    com_fsm_set_event(aac_decoder->com_fsm, AAC_DECODER_EVENT_START, 0);

    return AAC_DECODER_SUCCESS;
}

aac_decoder_return_t aac_decoder_stop(aac_decoder_t* aac_decoder)
{
    com_fsm_set_event(aac_decoder->com_fsm, AAC_DECODER_EVENT_STOP, COM_FSM_MAX_WAIT_TIME);
    
    return AAC_DECODER_SUCCESS;
}

aac_decoder_return_t aac_decoder_pause(aac_decoder_t* aac_decoder, bool from_isr)
{
    if(true == from_isr) {
        com_fsm_set_event_from_isr(aac_decoder->com_fsm, AAC_DECODER_EVENT_PAUSE);
    }
    else {
        com_fsm_set_event(aac_decoder->com_fsm, AAC_DECODER_EVENT_PAUSE, COM_FSM_MAX_WAIT_TIME);
    }

    return AAC_DECODER_SUCCESS;
}

aac_decoder_return_t aac_decoder_resume(aac_decoder_t* aac_decoder, bool from_isr)
{
    if(true == from_isr) {
        com_fsm_set_event_from_isr(aac_decoder->com_fsm, AAC_DECODER_EVENT_RESUME);
    }
    else {
        com_fsm_set_event(aac_decoder->com_fsm, AAC_DECODER_EVENT_RESUME, COM_FSM_MAX_WAIT_TIME);
    }

    return AAC_DECODER_SUCCESS;
}

aac_decoder_return_t aac_decoder_seek(aac_decoder_t* aac_decoder, double seek_position)
{
    aac_decoder->seek_position = seek_position;
    com_fsm_set_event(aac_decoder->com_fsm, MP3_DECODER_EVENT_SEEK, COM_FSM_MAX_WAIT_TIME);

    return AAC_DECODER_SUCCESS;
}

void aac_decoder_set_input_done(aac_decoder_t* aac_decoder)
{
    aac_decoder->input_done = true;
}

bool aac_decoder_is_output_done(aac_decoder_t* aac_decoder)
{
    return aac_decoder->output_done;
}

bool aac_decoder_is_pause(aac_decoder_t* aac_decoder)
{
    return (AAC_DECODER_STA_PAUSE==aac_decoder_get_state(aac_decoder)) ?true :false;
}

#endif