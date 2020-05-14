#include "pcm_preprocess.h"
#include "pcm_trans.h"

#define WAKEUP_ENV  "words=xiao bu;thresh=0.3;pauseTime=20;"

#if defined(VOICE_WAKEUP_ENABLE) || defined(VOICE_VAD_ENABLE)
static int __wakeup_callback(void *user_data, wakeup_status_t _status, char *json, int bytes)
{
    pcm_preproc_t* pcm_preproc = (pcm_preproc_t*)user_data;

    if(NULL != pcm_preproc && NULL != pcm_preproc->wakeup_callback) {
        pcm_preproc->wakeup_callback(pcm_preproc->wakeup_param, (WAKEUP_STATUS_WOKEN==_status) ?true :false, false);
    }
    
    return 0;
}

static int __vad_callback(void *user_data, int _status, int offset)
{
    pcm_preproc_t* pcm_preproc = (pcm_preproc_t*)user_data;

    if(pcm_preproc->vad_status != _status) {
        pcm_preproc->vad_status = _status;

        if(NULL != pcm_preproc && NULL != pcm_preproc->vad_callback) {
            pcm_preproc->vad_callback(pcm_preproc->wakeup_param, (0!=_status) ?true :false, false);
        }
    }
    
    return 0;
}
#endif

#ifdef EXT_WAKEUP_ENABLE
#include "hal.h"
static void ext_wakeup_eint_irq_handler(void* param)
{
    pcm_preproc_t* pcm_preproc = (pcm_preproc_t*)param;

#ifdef HAL_EINT_FEATURE_MASK
    hal_eint_mask(EXT_WAKEUP_INT_NUM);
#endif

    if(NULL != pcm_preproc && NULL != pcm_preproc->wakeup_callback) {
        pcm_preproc->wakeup_callback(pcm_preproc->wakeup_param, true, true);
    }

#ifdef HAL_EINT_FEATURE_MASK
    hal_eint_unmask(EXT_WAKEUP_INT_NUM);
#endif
}

static int pcm_preproc_ext_wakeup_init(pcm_preproc_t* pcm_preproc)
{
    hal_eint_config_t eint_config;
    
    hal_gpio_init(EXT_WAKEUP_GPIO_PIN);
    hal_pinmux_set_function(EXT_WAKEUP_GPIO_PIN, EXT_WAKEUP_GPIO_FUNC);
    hal_gpio_set_direction(EXT_WAKEUP_GPIO_PIN, HAL_GPIO_DIRECTION_INPUT);
    hal_gpio_pull_up(EXT_WAKEUP_GPIO_PIN);

    eint_config.trigger_mode = HAL_EINT_EDGE_FALLING;
    eint_config.debounce_time = 5;

#ifdef HAL_EINT_FEATURE_MASK
    hal_eint_mask(EXT_WAKEUP_INT_NUM);
#endif

    hal_eint_init(EXT_WAKEUP_INT_NUM, &eint_config);
    hal_eint_register_callback(EXT_WAKEUP_INT_NUM, ext_wakeup_eint_irq_handler, pcm_preproc);

#ifdef HAL_EINT_FEATURE_MASK
    hal_eint_unmask(EXT_WAKEUP_INT_NUM);
#endif

    return 0;
}

static int pcm_preproc_ext_wakeup_deinit(pcm_preproc_t* pcm_preproc)
{
    hal_eint_deinit(EXT_WAKEUP_INT_NUM);
    
    return 0;
}

#endif

pcm_preproc_t* pcm_preproc_create(void* user_param, int(*output_callback)(void*, uint8_t*, int))
{
    pcm_preproc_t* pcm_preproc = NULL;
    
    if(NULL == (pcm_preproc = malloc(sizeof(pcm_preproc_t)))) {
        goto ERR;
    }

    memset(pcm_preproc, 0, sizeof(pcm_preproc_t));

    pcm_preproc->wakeup_enable   = false;
    pcm_preproc->user_param      = user_param;
	pcm_preproc->output_callback = output_callback;

#ifdef VOICE_AEC_ENABLE
    if(NULL == (pcm_preproc->hAec = aec_api_new(NULL))) {
        goto ERR;
    }

    pcm_preproc->sBlockSZ = aec_api_memSize(pcm_preproc->hAec); // = 4
	pcm_preproc->sMicNum  = aec_api_micNum(pcm_preproc->hAec);	// = 1
	pcm_preproc->lFrameSZ = aec_api_frameSize(pcm_preproc->hAec); // = 512

	pcm_preproc->aec_work_size = pcm_preproc->lFrameSZ *sizeof(short) *2;
	pcm_preproc->aec_out_size  = pcm_preproc->lFrameSZ *sizeof(short) *pcm_preproc->sMicNum;

    pcm_preproc->aec_feed_size   = pcm_preproc->aec_work_size;
	pcm_preproc->aec_feed_length = 0;

	if(NULL == (pcm_preproc->aec_work_buffer = malloc(pcm_preproc->aec_work_size))) {
        goto ERR;
    }

    if(NULL == (pcm_preproc->aec_out_buffer = malloc(pcm_preproc->aec_out_size))) {
        goto ERR;
    }

    pcm_preproc->aec_feed_buffer = (char*)pcm_preproc->aec_work_buffer;

#endif
    
    return pcm_preproc;

ERR:
    pcm_preproc_destroy(&pcm_preproc);
    return NULL;
}

void pcm_preproc_destroy(pcm_preproc_t** pp_pcm_preproc)
{
    pcm_preproc_t* pcm_preproc = pp_pcm_preproc ?(*pp_pcm_preproc) :NULL;
    
    if(NULL == pcm_preproc)
        return;

    pcm_preproc_wakeup_disable(pcm_preproc);

#ifdef VOICE_AEC_ENABLE
    if(NULL != pcm_preproc->aec_work_buffer) {
        free(pcm_preproc->aec_work_buffer);
        pcm_preproc->aec_work_buffer = NULL;
    }

    if(NULL != pcm_preproc->aec_out_buffer) {
        free(pcm_preproc->aec_out_buffer);
        pcm_preproc->aec_out_buffer = NULL;
    }

    if(NULL != pcm_preproc->hAec) {
        aec_api_delete(pcm_preproc->hAec);
        pcm_preproc->hAec = NULL;
    }
#endif

    free(*pp_pcm_preproc);
    *pp_pcm_preproc = NULL;
}

static int pcm_preproc_output(pcm_preproc_t* pcm_preproc, uint8_t* buf, int size)
{
#if defined(VOICE_WAKEUP_ENABLE) || defined(VOICE_VAD_ENABLE)
    if(true == pcm_preproc->wakeup_enable) {
        wakeup_feed(pcm_preproc->hWakeup, (char*)buf, size);
    }
#endif

    if(NULL != pcm_preproc->output_callback) {
        pcm_preproc->output_callback(pcm_preproc->user_param, buf, size);
    }

    return size;
}

int pcm_preproc_feed(pcm_preproc_t* pcm_preproc, uint8_t* buf, int size)
{
    bool aec_handler = false;

#ifdef VOICE_AEC_ENABLE
    int pos = 0, tmp, i, j;
    static bool last_aec_handler = false;

    if(false == pcm_trans_is_tx_idle())
        aec_handler = true;

    if(last_aec_handler != aec_handler) {
        aec_api_reset(pcm_preproc->hAec);
        last_aec_handler = aec_handler;
    }

    while(true == aec_handler && pos < size)
    {
        tmp = size - pos;

        if(tmp > (pcm_preproc->aec_feed_size - pcm_preproc->aec_feed_length))
            tmp = pcm_preproc->aec_feed_size - pcm_preproc->aec_feed_length;

        memcpy(&pcm_preproc->aec_feed_buffer[pcm_preproc->aec_feed_length], &buf[pos], tmp);
        pcm_preproc->aec_feed_length += tmp;
        pos += tmp;

        if(pcm_preproc->aec_feed_length >= pcm_preproc->aec_feed_size)
        {
            //memset(pcm_preproc->aec_out_buffer, 0, pcm_preproc->aec_out_size);

            aec_api_feed(pcm_preproc->hAec, (uint8_t*)pcm_preproc->aec_feed_buffer, pcm_preproc->lFrameSZ);

            for(i = 0; i < pcm_preproc->sMicNum; i++)
        	{
        		aec_api_pop(pcm_preproc->hAec, pcm_preproc->aec_work_buffer, i);
                
        		for(j = 0; j < pcm_preproc->lFrameSZ; j++) {
        			*(pcm_preproc->aec_out_buffer + pcm_preproc->sMicNum * j + i) = *(pcm_preproc->aec_work_buffer + j);
        		}
        	}

            pcm_preproc_output(pcm_preproc, (uint8_t*)pcm_preproc->aec_out_buffer, pcm_preproc->aec_out_size);
            
            pcm_preproc->aec_feed_length = 0;
        }
    }
#endif

    if(false == aec_handler)
    {
#if defined(ES8388_ENABLE) && !defined(DEF_LINUX_PLATFORM)
        size /= 2;
        for(int i = 0; i < size/2; i++) {
            buf[i*2+0] = buf[i*4+0];
            buf[i*2+1] = buf[i*4+1];
        }
#endif
        pcm_preproc_output(pcm_preproc, buf, size);
    }

    return size;
}

int pcm_preproc_wakeup_disable(pcm_preproc_t* pcm_preproc)
{
    if(false == pcm_preproc->wakeup_enable)
        return 0;

    pcm_preproc->wakeup_enable = false;

#if defined(VOICE_WAKEUP_ENABLE) || defined(VOICE_VAD_ENABLE)
    if(NULL != pcm_preproc->hWakeup) {
        wakeup_end(pcm_preproc->hWakeup);
        wakeup_delete(pcm_preproc->hWakeup);
        pcm_preproc->hWakeup = NULL;
    }
#endif

#ifdef EXT_WAKEUP_ENABLE
    pcm_preproc_ext_wakeup_deinit(pcm_preproc);
#endif

    return 0;
}

int pcm_preproc_wakeup_enable(pcm_preproc_t* pcm_preproc)
{
    if(true == pcm_preproc->wakeup_enable)
        return 0;
        
#ifdef VOICE_AEC_ENABLE
    pcm_preproc->aec_feed_length = 0;
    aec_api_reset(pcm_preproc->hAec);
#endif

#ifdef EXT_WAKEUP_ENABLE
    pcm_preproc_ext_wakeup_init(pcm_preproc);
#endif

#if defined(VOICE_WAKEUP_ENABLE) || defined(VOICE_VAD_ENABLE)
    if(NULL == (pcm_preproc->hWakeup = wakeup_new(NULL, NULL))) {
        return -1;
    }    
        
#ifndef EXT_WAKEUP_ENABLE
    wakeup_register_handler(pcm_preproc->hWakeup, pcm_preproc, __wakeup_callback);
#endif
    vad_register_handler(pcm_preproc->hWakeup, pcm_preproc, __vad_callback);
    wakeup_start(pcm_preproc->hWakeup, WAKEUP_ENV, strlen(WAKEUP_ENV));
    
#endif

    pcm_preproc->vad_status = 0;
    pcm_preproc->wakeup_enable = true;

    return 0;
}

