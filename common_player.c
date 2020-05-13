#include "common_player.h"
#include <string.h>

#define malloc(x)   pvPortMalloc(x)
#define free(x)     vPortFree(x)

#define COM_PLAYER_OUTPUT_SIZE      (32*1024)
#define COM_PLAYER_START_TIMEOUT    (15000/portTICK_RATE_MS)

static int __decoder_output_callback(void* param, audio_decoder_info_t* decoder_info, uint8_t* buf, int size);
static int __pcm_trans_data_request_callback(void* param, uint8_t* buf, int size);

static com_player_t* g_pcm_trans_holder = NULL;

com_player_return_t com_player_init(com_player_t* com_player)
{
    memset(com_player, 0, sizeof(com_player_t));

    com_player->event_handle = xEventGroupCreate();
	
    ring_buffer_init(&com_player->output_buffer, COM_PLAYER_OUTPUT_SIZE);
    //mp3_decoder_init(&com_player->mp3_decoder);
    //aac_decoder_init(&com_player->aac_decoder);
    
    return COM_PLAYER_SUCCESS;
}

com_player_return_t com_player_deinit(com_player_t* com_player)
{
    if(NULL != com_player->event_handle) {
        vEventGroupDelete(com_player->event_handle);
        com_player->event_handle = NULL;
    }

    //aac_decoder_deinit(&com_player->aac_decoder);
	//mp3_decoder_deinit(&com_player->mp3_decoder);
    ring_buffer_deinit(&com_player->output_buffer);
    
    return COM_PLAYER_SUCCESS;
}

static void com_player_set_event(com_player_t* com_player, uint32_t events)
{
    if(NULL==com_player->event_handle)
        return;
    
    xEventGroupSetBits(com_player->event_handle, events);
}

static uint32_t com_player_wait_event(com_player_t* com_player, uint32_t events, uint32_t timeout)
{
    if(NULL==com_player->event_handle)
        return 0;
    
    return xEventGroupWaitBits(com_player->event_handle, events, pdTRUE, pdFALSE, timeout);
}

static void com_player_clear_event(com_player_t* com_player, uint32_t events)
{
    if(NULL==com_player->event_handle)
        return;
    
    xEventGroupClearBits(com_player->event_handle, events);
}

com_player_return_t com_player_start(
    com_player_t* com_player, 
    com_player_type_t audio_type,
    p_decoder_input_callback input_callback,
    p_decoder_error_callback error_callback,
    p_decoder_seek_callback seek_callback,
    void* callback_param)
{
    uint32_t events;
    
    com_player->input_done            = false;
    com_player->wait_decoder          = true;
    com_player->consumed              = 0;
    com_player->enable_pcm_or_decoder = true;
    com_player->converter             = NULL; 

    com_player->decoder_type = audio_type;

    memset(&com_player->decoder_info, 0, sizeof(audio_decoder_info_t));

    if(NULL != g_pcm_trans_holder)
        return COM_PLAYER_ERR_PCM_TRANS;
    
    if(NULL == (com_player->converter = com_player_convert_alloc(&com_player->output_buffer)))
        return COM_PLAYER_ERR_PCM_TRANS;

    if(COM_PLAYER_TYPE_MP3 == com_player->decoder_type) {
        mp3_decoder_init(&com_player->mp3_decoder);
        mp3_decoder_register_input_callback(&com_player->mp3_decoder, input_callback, callback_param);
        mp3_decoder_register_output_callback(&com_player->mp3_decoder, __decoder_output_callback, com_player);
        mp3_decoder_register_seek_callback(&com_player->mp3_decoder, seek_callback, callback_param);
        mp3_decoder_register_error_callback(&com_player->mp3_decoder, error_callback, callback_param);
        mp3_decoder_start(&com_player->mp3_decoder);
    }
#ifdef DEF_AAC_DECODER_ENABLE
    else if(COM_PLAYER_TYPE_AAC == com_player->decoder_type || COM_PLAYER_TYPE_M4A == com_player->decoder_type) {
        aac_decoder_init(&com_player->aac_decoder);
        aac_decoder_register_input_callback(&com_player->aac_decoder, input_callback, callback_param);
        aac_decoder_register_output_callback(&com_player->aac_decoder, __decoder_output_callback, com_player);
        aac_decoder_register_seek_callback(&com_player->aac_decoder, seek_callback, callback_param);
        aac_decoder_register_error_callback(&com_player->aac_decoder, error_callback, callback_param);
        aac_decoder_start(&com_player->aac_decoder, (COM_PLAYER_TYPE_M4A == com_player->decoder_type) ?true :false);
    }
#endif

    events = COM_PLAYER_EVENT_DECODER |COM_PLAYER_EVENT_EXIT;
    events = com_player_wait_event(com_player, events, COM_PLAYER_START_TIMEOUT);

    if(COM_PLAYER_EVENT_DECODER & events) {
        pcm_trans_register_data_request_callback(__pcm_trans_data_request_callback, com_player);
        
        if(PCM_TRANS_SUCCESS != pcm_trans_start_tx(com_player->decoder_info.sample_rate, com_player->decoder_info.channels, false))
            return COM_PLAYER_ERR_PCM_TRANS;
    }
    else if(COM_PLAYER_EVENT_EXIT & events) {
        return COM_PLAYER_ERR_EXIT;
    }
    else {
        return COM_PLAYER_ERR_TIMEOUT;
    }
    
    com_player->wait_decoder = false;
    g_pcm_trans_holder = com_player;
    
    return COM_PLAYER_SUCCESS;
}

com_player_return_t com_player_stop(com_player_t* com_player)
{
    com_player->enable_pcm_or_decoder = false;

    if(g_pcm_trans_holder == com_player) {
        pcm_trans_register_data_request_callback(NULL, NULL);
        pcm_trans_stop_tx();

        g_pcm_trans_holder = NULL;
    }
    
    if(COM_PLAYER_TYPE_MP3 == com_player->decoder_type) {
        mp3_decoder_stop(&com_player->mp3_decoder);
        mp3_decoder_register_input_callback(&com_player->mp3_decoder, NULL, NULL);
        mp3_decoder_register_output_callback(&com_player->mp3_decoder, NULL, NULL);
        mp3_decoder_register_seek_callback(&com_player->mp3_decoder, NULL, NULL);
        mp3_decoder_register_error_callback(&com_player->mp3_decoder, NULL, NULL);
        mp3_decoder_deinit(&com_player->mp3_decoder);
    }
#ifdef DEF_AAC_DECODER_ENABLE
    else if(COM_PLAYER_TYPE_AAC == com_player->decoder_type || COM_PLAYER_TYPE_M4A == com_player->decoder_type) {
        aac_decoder_stop(&com_player->aac_decoder);
        aac_decoder_register_input_callback(&com_player->aac_decoder, NULL, NULL);
        aac_decoder_register_output_callback(&com_player->aac_decoder, NULL, NULL);
        aac_decoder_register_error_callback(&com_player->aac_decoder, NULL, NULL);
        aac_decoder_register_seek_callback(&com_player->aac_decoder, NULL, NULL);
        aac_decoder_deinit(&com_player->aac_decoder);
    }
#endif

    ring_buffer_clear(&com_player->output_buffer, false);
    com_player_clear_event(com_player, COM_PLAYER_EVENT_ALL);
    com_player_convert_dealloc(&com_player->converter);
    
    return COM_PLAYER_SUCCESS;
}

com_player_return_t com_player_pause(com_player_t* com_player)
{
    com_player->enable_pcm_or_decoder = false;

    if(g_pcm_trans_holder == com_player) {
        pcm_trans_register_data_request_callback(NULL, NULL);
        pcm_trans_stop_tx();

        g_pcm_trans_holder = NULL;
    }

    if(COM_PLAYER_TYPE_MP3 == com_player->decoder_type) {
        mp3_decoder_pause(&com_player->mp3_decoder, false);
    }
#ifdef DEF_AAC_DECODER_ENABLE
    else if(COM_PLAYER_TYPE_AAC == com_player->decoder_type || COM_PLAYER_TYPE_M4A == com_player->decoder_type) {
        aac_decoder_pause(&com_player->aac_decoder, false);
    }
#endif
    
    return COM_PLAYER_SUCCESS;
}

com_player_return_t com_player_resume(com_player_t* com_player)
{
    if(NULL != g_pcm_trans_holder)
        return COM_PLAYER_ERR_PCM_TRANS;
    
    com_player->enable_pcm_or_decoder = true;

    pcm_trans_register_data_request_callback(__pcm_trans_data_request_callback, com_player);
    
    if(PCM_TRANS_SUCCESS != pcm_trans_start_tx(com_player->decoder_info.sample_rate, com_player->decoder_info.channels, false)) {
         return COM_PLAYER_ERR_PCM_TRANS;
    }

    g_pcm_trans_holder = com_player;
    
    return COM_PLAYER_SUCCESS;
}

com_player_return_t com_player_seek(com_player_t* com_player, int total_len, double seek_progress)
{
    int cur_pos;
    double tmp;
    
    if( total_len <= 0 ||
        com_player->decoder_info.bit_rate <= 0 || 
        com_player->decoder_info.sample_rate <= 0 || 
        com_player->decoder_info.channels <= 0) 
    {
        return COM_PLAYER_ERR_SEEK;
    }
        
    if(COM_PLAYER_TYPE_MP3 == com_player->decoder_type)
    {
        cur_pos = (total_len - com_player->decoder_info.tag_size) *seek_progress /100.0;

        tmp = COM_PLAYER_FIXED_CHANNELS *COM_PLAYER_FIXED_SAMPLERATE *2;
        tmp = tmp * cur_pos *8 /com_player->decoder_info.bit_rate;

        com_player->consumed = (int)tmp;

        cur_pos += com_player->decoder_info.tag_size;

        if(cur_pos >= total_len)
            cur_pos = total_len - 1;
        
        mp3_decoder_seek(&com_player->mp3_decoder, cur_pos);
        ring_buffer_clear(&com_player->output_buffer, false);
    }
#ifdef DEF_AAC_DECODER_ENABLE
    else if(COM_PLAYER_TYPE_M4A == com_player->decoder_type)
    {
        tmp = seek_progress *com_player->decoder_info.tag_size /com_player->decoder_info.sample_rate;
        tmp = tmp *COM_PLAYER_FIXED_CHANNELS *COM_PLAYER_FIXED_SAMPLERATE *2;

        com_player->consumed = (int)tmp;
        
        aac_decoder_seek(&com_player->aac_decoder, seek_progress);
        ring_buffer_clear(&com_player->output_buffer, false);
    }
#endif

    return COM_PLAYER_SUCCESS;
}

void com_player_set_done(com_player_t* com_player, bool error_occur)
{
    if(COM_PLAYER_TYPE_MP3 == com_player->decoder_type) {
        mp3_decoder_set_input_done(&com_player->mp3_decoder);
    }
#ifdef DEF_AAC_DECODER_ENABLE
    else if(COM_PLAYER_TYPE_AAC == com_player->decoder_type || COM_PLAYER_TYPE_M4A == com_player->decoder_type) {
        aac_decoder_set_input_done(&com_player->aac_decoder);
    }
#endif

    if(true == error_occur || true == com_player->wait_decoder) {
        com_player_set_event(com_player, COM_PLAYER_EVENT_EXIT);
    }
}

bool com_player_is_done(com_player_t* com_player)
{
    return pcm_trans_is_tx_done();
}

bool com_player_auto_resume(com_player_t* com_player)
{
    if(true == pcm_trans_is_tx_pause()) 
    {
        bool decoder_pause = false;

        if(COM_PLAYER_TYPE_MP3 == com_player->decoder_type) {
            decoder_pause = mp3_decoder_is_pause(&com_player->mp3_decoder);
        }
#ifdef DEF_AAC_DECODER_ENABLE
        else if(COM_PLAYER_TYPE_AAC == com_player->decoder_type || COM_PLAYER_TYPE_M4A == com_player->decoder_type) {
            decoder_pause = aac_decoder_is_pause(&com_player->aac_decoder);
        }
#endif
        
        if(true == decoder_pause) {
            pcm_trans_resume_tx();
            return true;
        }
    }

    return false;
}

bool com_player_get_progress(com_player_t* com_player, int cur_pos, int total_len, int* cur_time, int* all_time)
{
    audio_decoder_info_t* info = &com_player->decoder_info;

    if(COM_PLAYER_TYPE_MP3 == com_player->decoder_type && cur_pos < com_player->decoder_info.tag_size)
        return false;
    
    if( total_len <= 0 || 
        com_player->decoder_info.bit_rate <= 0 || 
        com_player->decoder_info.sample_rate <= 0 || 
        com_player->decoder_info.channels <= 0 ) 
    {
        return false;
    }

    *cur_time = com_player->consumed /COM_PLAYER_FIXED_CHANNELS /2 /COM_PLAYER_FIXED_SAMPLERATE;
    
    if(COM_PLAYER_TYPE_MP3 == com_player->decoder_type) {
        *all_time = (total_len - info->tag_size) *8 /info->bit_rate;
        //*cur_time = (cur_pos - info->tag_size) *8 /info->bit_rate;
    }
#ifdef DEF_AAC_DECODER_ENABLE
    else if(COM_PLAYER_TYPE_M4A == com_player->decoder_type) {
        *all_time = info->tag_size /info->sample_rate;
    }
    else if(COM_PLAYER_TYPE_AAC == com_player->decoder_type) {
        *all_time = total_len *8 /info->bit_rate;
    }
#endif
    
    return true;
}

com_player_type_t com_player_get_audio_type_by_name(char* path)
{
    com_player_type_t autio_type = COM_PLAYER_TYPE_MP3;

    if(NULL != strstr(path, ".mp3")) {
        autio_type = COM_PLAYER_TYPE_MP3;
    }
    else if(NULL != strstr(path, ".MP3")) {
        autio_type = COM_PLAYER_TYPE_MP3;
    }
#ifdef DEF_AAC_DECODER_ENABLE
    else if(NULL != strstr(path, ".aac")) {
        autio_type = COM_PLAYER_TYPE_AAC;
    }
    else if(NULL != strstr(path, ".AAC")) {
        autio_type = COM_PLAYER_TYPE_AAC;
    }
    else if(NULL != strstr(path, ".m4a")) {
        autio_type = COM_PLAYER_TYPE_M4A;
    }
    else if(NULL != strstr(path, ".M4A")) {
        autio_type = COM_PLAYER_TYPE_M4A;
    }
#endif
    
    return autio_type;
}

com_player_type_t com_player_get_audio_type_by_header(uint8_t* buf)
{
    com_player_type_t autio_type = COM_PLAYER_TYPE_MP3;

    if(0 == memcmp(buf, "ID3", 3)) {
        autio_type = COM_PLAYER_TYPE_MP3;
    }
#ifdef DEF_AAC_DECODER_ENABLE
    else if(0 == memcmp(&buf[4], "ftyp", 4)) {
        autio_type = COM_PLAYER_TYPE_M4A;
    }
    else if(0 == memcmp(buf, "ADIF", 4)) {
        autio_type = COM_PLAYER_TYPE_AAC;
    }
    else if(0xFF == buf[0] && 0xF0 == (0xF6 & buf[1])) {
        autio_type = COM_PLAYER_TYPE_AAC;
    }
#endif

    return autio_type;
}

static int __decoder_output_callback(void* param, audio_decoder_info_t* decoder_info, uint8_t* buf, int size)
{
    com_player_t* com_player = (com_player_t*)param;
    uint32_t consumed = 0;

    consumed = com_player_convert_process(com_player->converter, decoder_info->sample_rate, decoder_info->channels,
        COM_PLAYER_FIXED_SAMPLERATE, COM_PLAYER_FIXED_CHANNELS, buf, size);
    
    //consumed = ring_buffer_push(&com_player->output_buffer, buf, size, false);

    //com_player->consumed += consumed;

    if(true == com_player->wait_decoder) {
        memcpy(&com_player->decoder_info, decoder_info, sizeof(audio_decoder_info_t));
        com_player_set_event(com_player, COM_PLAYER_EVENT_DECODER);
    }
    else if(true == com_player->enable_pcm_or_decoder) {
        pcm_trans_resume_tx();
    }

    /* sometimes, the first decode bitrate is wrong */
    if(decoder_info->bit_rate != com_player->decoder_info.bit_rate) {
        com_player->decoder_info.bit_rate = decoder_info->bit_rate;
    }

    return consumed;
}

static int __pcm_trans_data_request_callback(void* param, uint8_t* buf, int size)
{
    com_player_t* com_player = (com_player_t*)param;
    uint32_t tmp, count = 0;
    uint32_t not_enough_th = (size/3/4)*4;
    bool output_done = false;
    int ring_count = ring_buffer_get_count(&com_player->output_buffer);
    int ring_free = ring_buffer_get_free_count(&com_player->output_buffer);

    if(COM_PLAYER_TYPE_MP3 == com_player->decoder_type) {
        output_done = mp3_decoder_is_output_done(&com_player->mp3_decoder);
    }
#ifdef DEF_AAC_DECODER_ENABLE
    else if(COM_PLAYER_TYPE_AAC == com_player->decoder_type || COM_PLAYER_TYPE_M4A == com_player->decoder_type) {
        output_done = aac_decoder_is_output_done(&com_player->aac_decoder);
    }
#endif

    if(true == output_done && ring_count <= 0) {
        pcm_trans_set_tx_no_data();
        return 0;
    }

    /* fill zero into codec for first pcm request when ring_buffer less than not_enough_th */
    if(com_player->consumed <= 0 && ring_count < not_enough_th) {
        memset(buf, 0, not_enough_th);
        count += not_enough_th;
    }

    tmp = ring_buffer_pop(&com_player->output_buffer, &buf[count], size-count, true);

    count += tmp;
    com_player->consumed += tmp;
    ring_count -= tmp;
    ring_free += tmp;

    if(false == output_done && true == com_player->enable_pcm_or_decoder && ring_count < ring_free)
    {
        if(COM_PLAYER_TYPE_MP3 == com_player->decoder_type) {
            mp3_decoder_resume(&com_player->mp3_decoder, true);
        }
#ifdef DEF_AAC_DECODER_ENABLE
        else if(COM_PLAYER_TYPE_AAC == com_player->decoder_type || COM_PLAYER_TYPE_M4A == com_player->decoder_type) {
            aac_decoder_resume(&com_player->aac_decoder, true);
        }
#endif
    }

    return count;
}

