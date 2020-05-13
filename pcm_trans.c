#include "pcm_trans.h"
#include <alsa/asoundlib.h>
#include "common_player.h"

log_create_module(pcm_trans, PRINT_LEVEL_ERROR);

#define I2S_TX_BUFFER_SIZE      4096
#define I2S_RX_BUFFER_SIZE      1024
#define I2S_DATA_REQUEST_SIZE   (I2S_TX_BUFFER_SIZE*sizeof(uint32_t))
#define I2S_DATA_NOTIFY_SIZE    (I2S_RX_BUFFER_SIZE*sizeof(uint32_t))

typedef enum {
    PCM_TRANS_EVENT_START   = 0x000001UL,
    PCM_TRANS_EVENT_RESUME  = 0x000002UL,
    PCM_TRANS_EVENT_PAUSE   = 0x000004UL,
    PCM_TRANS_EVENT_STOP    = 0x000008UL,

} pcm_trans_event_t;

typedef struct {
    com_fsm_t*  com_fsm;
    snd_pcm_t*  pcm_handle;
    uint32_t    sample_rate;
    uint8_t     channels;
    bool        no_data;
    uint8_t*    pcm_buffer;
    int         pcm_size;

    int(*pcm_callback)(void* param, uint8_t* buf, int size);
    void* user_param;
    
} pcm_trans_t;

static pcm_trans_t pcm_trans_tx;
static pcm_trans_t pcm_trans_rx;

#define pcm_trans_get_tx_instance() (&pcm_trans_tx)
#define pcm_trans_get_rx_instance() (&pcm_trans_rx)

static uint32_t pcm_trans_tx_start_action(void* param)
{
    pcm_trans_t* pcm_trans = (pcm_trans_t*)param;
    int err, channels = COM_PLAYER_FIXED_CHANNELS;

    if((err = snd_pcm_open(&pcm_trans->pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        LOG_E(pcm_trans, "Playback open error: %s\n", snd_strerror(err));
    }
    
    if((err = snd_pcm_set_params(pcm_trans->pcm_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 
        channels, COM_PLAYER_FIXED_SAMPLERATE, 1, 500000)) < 0)
    {
        LOG_E(pcm_trans, "Playback open error: %s\n", snd_strerror(err));
    }

    pcm_trans->pcm_size = I2S_DATA_REQUEST_SIZE;
    pcm_trans->pcm_buffer = malloc(pcm_trans->pcm_size);

    return PCM_TRANS_STA_TX_RUN;
}

static uint32_t pcm_trans_tx_stop_action(void* param)
{
    pcm_trans_t* pcm_trans = (pcm_trans_t*)param;
    
    free(pcm_trans->pcm_buffer);
    snd_pcm_drop(pcm_trans->pcm_handle);
    snd_pcm_close(pcm_trans->pcm_handle);

    return PCM_TRANS_STA_IDLE;
}

static uint32_t pcm_trans_tx_resume_action(void* param)
{
    return PCM_TRANS_STA_TX_RUN;
}

static uint32_t pcm_trans_tx_run_poll(void* param)
{
    pcm_trans_t* pcm_trans = (pcm_trans_t*)param;
    
    snd_pcm_sframes_t frames;
    int input_size;
    int frame_size = COM_PLAYER_FIXED_CHANNELS*2;

    if(NULL == pcm_trans->pcm_callback) {
        return PCM_TRANS_STA_TX_PAUSE;
    }

    input_size = pcm_trans->pcm_callback(pcm_trans->user_param, pcm_trans->pcm_buffer, pcm_trans->pcm_size);
    
    if(input_size > 0)
    {
        frames = snd_pcm_writei(pcm_trans->pcm_handle, pcm_trans->pcm_buffer, input_size/frame_size);

        if(frames < 0) {
            LOG_E(pcm_trans, "snd_pcm_writei failed: %s\n", snd_strerror(frames));
            
            frames = snd_pcm_recover(pcm_trans->pcm_handle, frames, 0);
        }
    }
    else if(true == pcm_trans->no_data) {//LOG_I(xxxxxx, "xxxxxxxxxxxxxxx");
        //snd_pcm_drain(pcm_trans->pcm_handle);
        //vTaskDelay(300);
    
        LOG_I(pcm_trans, "--> PCM_TRANS_STA_TX_DONE");
        return PCM_TRANS_STA_TX_DONE;
    }
    else {
        return PCM_TRANS_STA_TX_PAUSE;
    }

    return COM_FSM_HOLD_STATE;
}

static uint32_t pcm_trans_rx_start_action(void* param)
{
    pcm_trans_t* pcm_trans = (pcm_trans_t*)param;

    snd_pcm_open(&pcm_trans->pcm_handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    
    snd_pcm_set_params(pcm_trans->pcm_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 
        1, pcm_trans->sample_rate, 1, 100000);

    pcm_trans->pcm_size = I2S_DATA_NOTIFY_SIZE;
    pcm_trans->pcm_buffer = malloc(pcm_trans->pcm_size);

    return PCM_TRANS_STA_RX_RUN;
}

static uint32_t pcm_trans_rx_stop_action(void* param)
{
    pcm_trans_t* pcm_trans = (pcm_trans_t*)param;
    
    free(pcm_trans->pcm_buffer);
    snd_pcm_drop(pcm_trans->pcm_handle);
    snd_pcm_close(pcm_trans->pcm_handle);

    return PCM_TRANS_STA_IDLE;
}

static uint32_t pcm_trans_rx_run_poll(void* param)
{
    pcm_trans_t* pcm_trans = (pcm_trans_t*)param;
    
    snd_pcm_sframes_t frames;
    int frame_size = 2;

    frames = snd_pcm_readi(pcm_trans->pcm_handle, pcm_trans->pcm_buffer, pcm_trans->pcm_size/frame_size);

    if(-EPIPE == frames) {
        snd_pcm_prepare(pcm_trans->pcm_handle);
    }
    else if(frames < 0) {
        LOG_E(pcm_trans, "error from read: %s\n", snd_strerror(frames));
    }
        
    /*for(int i = 0; i < frames; i++) {
        int16_t tmp = pcm_trans->pcm_buffer[2*i+1];
        tmp <<= 8;
        tmp |= pcm_trans->pcm_buffer[2*i];

        tmp *= 10;

        pcm_trans->pcm_buffer[2*i] = tmp;
        pcm_trans->pcm_buffer[2*i+1] = tmp >> 8;
    }*/

    if(NULL != pcm_trans->pcm_callback) {
        pcm_trans->pcm_callback(pcm_trans->user_param, pcm_trans->pcm_buffer, frames*frame_size);
    }

    return COM_FSM_HOLD_STATE;
}

int pcm_trans_init(void)
{
    memset(&pcm_trans_tx, 0, sizeof(pcm_trans_t));
    memset(&pcm_trans_rx, 0, sizeof(pcm_trans_t));

    const com_fsm_lookup_t lookup_tx[] = {
        { PCM_TRANS_EVENT_START,     PCM_TRANS_STA_IDLE,     pcm_trans_tx_start_action, },
        { PCM_TRANS_EVENT_STOP,      PCM_TRANS_STA_TX_RUN,   pcm_trans_tx_stop_action, },
        { PCM_TRANS_EVENT_STOP,      PCM_TRANS_STA_TX_PAUSE, pcm_trans_tx_stop_action, },
        { PCM_TRANS_EVENT_STOP,      PCM_TRANS_STA_TX_DONE,  pcm_trans_tx_stop_action, },
        { PCM_TRANS_EVENT_RESUME,    PCM_TRANS_STA_TX_PAUSE, pcm_trans_tx_resume_action, },
            
        { COM_FSM_EVENT_NONE, PCM_TRANS_STA_TX_RUN, pcm_trans_tx_run_poll, 0, },
    };

    const com_fsm_lookup_t lookup_rx[] = {
        { PCM_TRANS_EVENT_START,     PCM_TRANS_STA_IDLE,     pcm_trans_rx_start_action, },
        { PCM_TRANS_EVENT_STOP,      PCM_TRANS_STA_RX_RUN,   pcm_trans_rx_stop_action, },
        { PCM_TRANS_EVENT_STOP,      PCM_TRANS_STA_RX_PAUSE, pcm_trans_rx_stop_action, },
            
        { COM_FSM_EVENT_NONE, PCM_TRANS_STA_RX_RUN, pcm_trans_rx_run_poll, 0, },
    };

    pcm_trans_tx.com_fsm = com_fsm_create(
        "pcm_trans_tx", 
        (1024*5/sizeof(StackType_t)), 
        TASK_PRIORITY_NORMAL, 
        &pcm_trans_tx,
        lookup_tx,
        sizeof(lookup_tx)/sizeof(com_fsm_lookup_t)
        );

    pcm_trans_rx.com_fsm = com_fsm_create(
        "pcm_trans_rx", 
        (1024*5/sizeof(StackType_t)), 
        TASK_PRIORITY_NORMAL, 
        &pcm_trans_rx,
        lookup_rx,
        sizeof(lookup_rx)/sizeof(com_fsm_lookup_t)
        );
    
    return PCM_TRANS_SUCCESS;
}

int pcm_trans_deinit(void)
{
    com_fsm_delete(&pcm_trans_tx.com_fsm);
    com_fsm_delete(&pcm_trans_rx.com_fsm);
    
    snd_config_update_free_global();
    
    return PCM_TRANS_SUCCESS;
}

int pcm_trans_start_tx(uint32_t sample_rate, uint8_t channels, bool only_init)
{
    pcm_trans_t* pcm_trans = pcm_trans_get_tx_instance();

    pcm_trans->sample_rate = sample_rate;
    pcm_trans->channels = channels;
    pcm_trans->no_data = false;
    
    com_fsm_set_event(pcm_trans->com_fsm, PCM_TRANS_EVENT_START, COM_FSM_MAX_WAIT_TIME);
    LOG_I(pcm_trans, "pcm_trans_start_tx sample_rate:%d, channels:%d", sample_rate, channels);
    
    return PCM_TRANS_SUCCESS;
}

int pcm_trans_stop_tx(void)
{
    pcm_trans_t* pcm_trans = pcm_trans_get_tx_instance();

    com_fsm_set_event(pcm_trans->com_fsm, PCM_TRANS_EVENT_STOP, COM_FSM_MAX_WAIT_TIME);
    com_fsm_clear_event(pcm_trans->com_fsm, COM_FSM_EVENT_ALL);
    LOG_I(pcm_trans, "pcm_trans_stop_tx");
    
    return PCM_TRANS_SUCCESS;
}

int pcm_trans_pause_tx(void)
{
    return PCM_TRANS_SUCCESS;
}

int pcm_trans_resume_tx(void)
{
    pcm_trans_t* pcm_trans = pcm_trans_get_tx_instance();
    com_fsm_set_event(pcm_trans->com_fsm, PCM_TRANS_EVENT_RESUME, COM_FSM_MAX_WAIT_TIME);
    
    return PCM_TRANS_SUCCESS;
}

int pcm_trans_start_rx(uint32_t sample_rate, uint8_t channels, bool only_init)
{
    pcm_trans_t* pcm_trans = pcm_trans_get_rx_instance();

    pcm_trans->sample_rate = sample_rate;
    pcm_trans->channels = channels;
    
    com_fsm_set_event(pcm_trans->com_fsm, PCM_TRANS_EVENT_START, COM_FSM_MAX_WAIT_TIME);
    
    LOG_I(pcm_trans, "pcm_trans_start_rx sample_rate:%d, channels:%d", sample_rate, channels);
    
    return PCM_TRANS_SUCCESS;
}

int pcm_trans_stop_rx(void)
{
    pcm_trans_t* pcm_trans = pcm_trans_get_rx_instance();

    com_fsm_set_event(pcm_trans->com_fsm, PCM_TRANS_EVENT_STOP, COM_FSM_MAX_WAIT_TIME);
    com_fsm_clear_event(pcm_trans->com_fsm, COM_FSM_EVENT_ALL);
    LOG_I(pcm_trans, "pcm_trans_stop_rx");
    
    return PCM_TRANS_SUCCESS;
}

int pcm_trans_register_data_request_callback(p_pcm_trans_data_request_callback callback, void* param)
{
    pcm_trans_t* pcm_trans = pcm_trans_get_tx_instance();
    
    pcm_trans->pcm_callback = callback;
    pcm_trans->user_param = param;
    
    return PCM_TRANS_SUCCESS;
}

int pcm_trans_register_data_notify_callback(p_pcm_trans_data_notify_callback callback, void* param)
{
    pcm_trans_t* pcm_trans = pcm_trans_get_rx_instance();
    
    pcm_trans->pcm_callback = callback;
    pcm_trans->user_param = param;
    
    return PCM_TRANS_SUCCESS;
}

void pcm_trans_set_tx_no_data(void)
{
    pcm_trans_t* pcm_trans = pcm_trans_get_tx_instance();
    pcm_trans->no_data = true;
}

int pcm_trans_get_cur_state(void)
{
    pcm_trans_t* pcm_trans = pcm_trans_get_tx_instance();
    return pcm_trans->com_fsm->cur_state;
}

