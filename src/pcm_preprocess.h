#ifndef __PCM_PREPROCESS_H
#define __PCM_PREPROCESS_H

#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"

#if defined(DEF_LINUX_PLATFORM)

#elif defined(ES8388_ENABLE)
#define VOICE_AEC_ENABLE
#define VOICE_WAKEUP_ENABLE
#define VOICE_VAD_ENABLE
//#define EXT_WAKEUP_ENABLE

#else
#define VOICE_VAD_ENABLE

#endif

#ifdef EXT_WAKEUP_ENABLE
#define EXT_WAKEUP_GPIO_PIN     HAL_GPIO_12
#define EXT_WAKEUP_GPIO_FUNC    HAL_GPIO_12_EINT12
#define EXT_WAKEUP_INT_NUM      HAL_EINT_NUMBER_12
#endif

typedef struct
{
#ifdef VOICE_AEC_ENABLE
    void*     hAec;
    int       sBlockSZ;
    int       sMicNum;
    int       lFrameSZ;

    short*    aec_work_buffer;
    int       aec_work_size;
    short*    aec_out_buffer;
    int       aec_out_size;

    char*     aec_feed_buffer;
    int       aec_feed_size;
    int       aec_feed_length;
#endif

#if defined(VOICE_WAKEUP_ENABLE) || defined(VOICE_VAD_ENABLE)
    void*     wakeup_param;
    wakeup_t* hWakeup;
    
    int       (*wakeup_callback)(void* user_param, bool wakeup, bool from_isr);
    int       (*vad_callback)(void* user_param, bool valid, bool from_isr);
#endif

    bool      wakeup_enable;
    int       vad_status;
    void*     user_param;
    int       (*output_callback)(void* user_param, uint8_t* buf, int size);
    
} pcm_preproc_t;

#if defined(VOICE_WAKEUP_ENABLE) || defined(VOICE_VAD_ENABLE)
#define pcm_preproc_register_wakeup_callback(a,b,c,d) do { (a)->wakeup_param=b; (a)->wakeup_callback=c; (a)->vad_callback=d; } while(0)
#else
#define pcm_preproc_register_wakeup_callback(a,b,c,d) do { assert((void*)b!=(void*)d); assert((void*)b!=(void*)c); } while(0)
#endif

pcm_preproc_t* pcm_preproc_create(void* user_param, int(*output_callback)(void*, uint8_t*, int));
void pcm_preproc_destroy(pcm_preproc_t** pp_pcm_preproc);
int pcm_preproc_feed(pcm_preproc_t* pcm_preproc, uint8_t* buf, int size);
int pcm_preproc_wakeup_disable(pcm_preproc_t* pcm_preproc);
int pcm_preproc_wakeup_enable(pcm_preproc_t* pcm_preproc);

#endif


