#include "audio_player_process.h"
#include "prompt_flash.h"
#include <string.h>

#define malloc(x)   pvPortMalloc(x)
#define free(x)     vPortFree(x)

log_create_module(audio_player_proc, PRINT_LEVEL_ERROR);

#define AUDIO_PLAYER_TASK_STACK_SIZE            (10240/sizeof(StackType_t))
#define AUDIO_PLAYER_MONITOR_INTERVAL           (300/portTICK_RATE_MS)
#define AUDIO_PLAYER_PROGRESS_INTERVAL          (1000/portTICK_RATE_MS)

static audio_player_handle_t  g_last_alloc_handle = 0;

static void audio_player_update_last_error(audio_player_proc_t* audio_player, audio_player_return_t ret)
{
    if(AUDIO_PLAYER_PROC_SUCCESS == audio_player->last_error && AUDIO_PLAYER_PROC_SUCCESS != ret) {
        audio_player->last_error = ret;
    }
}

static int __player_input_callback(void* param, uint8_t* buf, int size)
{
    audio_player_proc_t* audio_player = (audio_player_proc_t*)param;
    bool input_done = false;
    
    int read_len = audio_file_read(audio_player->audio_file, buf, size);

    if(COM_PLAYER_TYPE_M4A == audio_player->com_player.decoder_type) {
        if(size == 0)
            input_done = true;
    }
    else if(read_len == 0) {
        input_done = true;
    }

    if(true == input_done) {
        com_player_set_done(&audio_player->com_player, false);
        LOG_I(audio_player_proc, "[%d] input buffer done!", audio_player->player_handle);
    }

    if(read_len < 0) {
        audio_player_update_last_error(audio_player, AUDIO_PLAYER_PROC_ERR_WEB_TIMEOUT);
        com_player_set_done(&audio_player->com_player, true);

        LOG_E(audio_player_proc, "[%d] audio_file_read failed(%d)", audio_player->player_handle, read_len);
    }
    
    return (read_len < 0) ?0 :read_len;
}

static int __player_seek_callback(void* param, int position)
{
    audio_player_proc_t* audio_player = (audio_player_proc_t*)param;
    return audio_file_seek(audio_player->audio_file, position);
}

static int __player_error_callback(void* param, int error)
{
    audio_player_proc_t* audio_player = (audio_player_proc_t*)param;

    audio_player_update_last_error(audio_player, AUDIO_PLAYER_PROC_ERR_AUDIO_TYPE);
    com_player_set_done(&audio_player->com_player, true);
    
    return 0;
}

static audio_player_return_t audio_player_proc_start(audio_player_proc_t* audio_player)
{
    com_player_type_t audio_type;
    uint8_t header[COM_PLAYER_AUDIO_TYPE_HEADER_SIZE];

    audio_player->progress_monitor_tick = 0;

    LOG_I(common, "[%d] player_path: %s", audio_player->player_handle, audio_player->player_info.path);

    bool range_enable = (AUDIO_PLAYER_TYPE_PROMPT==audio_player->player_info.type) ?false :true;
    audio_player->audio_file = audio_file_open(audio_player->player_info.path, audio_player->player_info.source, range_enable);

    if(NULL == audio_player->audio_file) {
        LOG_E(audio_player_proc, "[%d] fail to open %s!", audio_player->player_handle, audio_player->player_info.path);
        return AUDIO_PLAYER_PROC_ERR_OPEN_FILE;
    }

    //audio_type = com_player_get_audio_type_by_name(audio_player->player_info.path);

    audio_file_preread(audio_player->audio_file, header, COM_PLAYER_AUDIO_TYPE_HEADER_SIZE);
    audio_type = com_player_get_audio_type_by_header(header);

    if(COM_PLAYER_SUCCESS != com_player_start(
        &audio_player->com_player,
        audio_type,
        __player_input_callback,
        __player_error_callback,
        __player_seek_callback,
        audio_player))
    {
        LOG_E(audio_player_proc, "[%d] fail to call play!", audio_player->player_handle);
        return AUDIO_PLAYER_PROC_ERR_PLAYER_START;
    }
	
	if(NULL != audio_player->audio_player_callback) {
        audio_player->audio_player_callback(audio_player, AUDIO_PLAYER_EVENT_START);
    }
    
    return AUDIO_PLAYER_PROC_SUCCESS;
}

static audio_player_return_t audio_player_proc_stop(audio_player_proc_t* audio_player)
{
    audio_file_stop(audio_player->audio_file);
    com_player_stop(&audio_player->com_player);

    if(0 != audio_file_close(&audio_player->audio_file)) {
        LOG_E(audio_player_proc, "[%d] fail to close %s!", audio_player->player_handle, audio_player->player_info.path);
    }

    if(NULL != audio_player->audio_player_callback) {
        audio_player->audio_player_callback(audio_player, AUDIO_PLAYER_EVENT_STOP);
    }
    
    return AUDIO_PLAYER_PROC_SUCCESS;
}

static audio_player_return_t audio_player_proc_pause(audio_player_proc_t* audio_player, bool is_break)
{
    com_player_pause(&audio_player->com_player);
    audio_file_pause(audio_player->audio_file);

    if(NULL != audio_player->audio_player_callback) {
        audio_player->audio_player_callback(audio_player, (true==is_break) ?AUDIO_PLAYER_EVENT_BREAK :AUDIO_PLAYER_EVENT_PAUSE);
    }
    
    return AUDIO_PLAYER_PROC_SUCCESS;
}

static audio_player_return_t audio_player_proc_seek(audio_player_proc_t* audio_player)
{
    int total_len = audio_file_size(audio_player->audio_file);
    
    com_player_pause(&audio_player->com_player);
    com_player_seek(&audio_player->com_player, total_len, audio_player->seek_progress);
    com_player_resume(&audio_player->com_player);
    
    return AUDIO_PLAYER_PROC_SUCCESS;
}

static audio_player_return_t audio_player_proc_resume(audio_player_proc_t* audio_player)
{
    if(0 != audio_file_resume(audio_player->audio_file)) {
        LOG_E(audio_player_proc, "[%d] audio_file_resume failed!", audio_player->player_handle);
        return AUDIO_PLAYER_PROC_ERR_RESUME;
    }

    if(COM_PLAYER_SUCCESS != com_player_resume(&audio_player->com_player)) {
        LOG_E(audio_player_proc, "[%d] com_player_resume failed!", audio_player->player_handle);
        return AUDIO_PLAYER_PROC_ERR_RESUME;
    }

    if(NULL != audio_player->audio_player_callback) {
        audio_player->audio_player_callback(audio_player, AUDIO_PLAYER_EVENT_RESUME);
    }
    
    return AUDIO_PLAYER_PROC_SUCCESS;
}

static audio_player_return_t audio_player_proc_monitor(audio_player_proc_t* audio_player)
{
    if((xTaskGetTickCount() - audio_player->progress_monitor_tick) > AUDIO_PLAYER_PROGRESS_INTERVAL)
    {
        int cur, all;
        int cur_pos = audio_file_tell(audio_player->audio_file);
        int total_len = audio_file_size(audio_player->audio_file);
        
        if(true == com_player_get_progress(&audio_player->com_player, cur_pos, total_len, &cur, &all))
        {
            if(NULL != audio_player->audio_player_callback) {
                audio_player->audio_player_callback(audio_player, AUDIO_PLAYER_EVENT_PROGRESS);
            }
            
            LOG_I(common, "[%d] progress: %02d:%02d/%02d:%02d", audio_player->player_handle, cur/60, cur%60, all/60, all%60);
        }
        else
        {
            LOG_E(audio_player_proc, "[%d] com_player_get_progress failed", audio_player->player_handle);
        }

        audio_player->progress_monitor_tick = xTaskGetTickCount();
    }

    if(true == com_player_is_done(&audio_player->com_player)) {
        LOG_I(common, "[%d] all end!", audio_player->player_handle);
        return AUDIO_PLAYER_PROC_ALL_END;
    }
    else if(true == com_player_auto_resume(&audio_player->com_player)) {
        LOG_I(audio_player_proc, "[%d] auto resume com_player", audio_player->player_handle);
    }
    
    return AUDIO_PLAYER_PROC_SUCCESS;
}

static void audio_player_lock(audio_player_proc_t* audio_player)
{
    xSemaphoreTake(audio_player->mutex_handle, portMAX_DELAY);
}

static void audio_player_unlock(audio_player_proc_t* audio_player)
{
    xSemaphoreGive(audio_player->mutex_handle);
}

static uint32_t audio_player_proc_start_action(void* param)
{
    audio_player_proc_t* audio_player = (audio_player_proc_t*)param;

    audio_player_update_last_error(audio_player, audio_player_proc_start(audio_player));

    if(AUDIO_PLAYER_PROC_SUCCESS == audio_player->last_error) {
        LOG_I(audio_player_proc, "--> AUDIO_PLAYER_STA_PLAY");
        return AUDIO_PLAYER_STA_PLAY;
    }
    else {
        audio_player_proc_stop(audio_player);
    }
    
    return COM_FSM_HOLD_STATE;
}

static uint32_t audio_player_proc_stop_action(void* param)
{
    audio_player_proc_t* audio_player = (audio_player_proc_t*)param;

    audio_player_proc_stop(audio_player);

    LOG_I(audio_player_proc, "--> AUDIO_PLAYER_STA_IDLE");
    return AUDIO_PLAYER_STA_IDLE;
}

static uint32_t audio_player_proc_pause_action(void* param)
{
    audio_player_proc_t* audio_player = (audio_player_proc_t*)param;

    if(AUDIO_PLAYER_STA_BREAK == audio_player_get_state(audio_player)) {
        if(NULL != audio_player->audio_player_callback)
            audio_player->audio_player_callback(audio_player, AUDIO_PLAYER_EVENT_PAUSE);

        LOG_I(audio_player_proc, "--> AUDIO_PLAYER_STA_PAUSE");
        return AUDIO_PLAYER_STA_PAUSE;
    }
    else
    {
        audio_player_update_last_error(audio_player, audio_player_proc_pause(audio_player, false));

        if(AUDIO_PLAYER_PROC_SUCCESS != audio_player->last_error) {
            audio_player_proc_stop(audio_player);

            LOG_I(audio_player_proc, "--> AUDIO_PLAYER_STA_IDLE");
            return AUDIO_PLAYER_STA_IDLE;
        }
    }

    LOG_I(audio_player_proc, "--> AUDIO_PLAYER_STA_PAUSE");
    return AUDIO_PLAYER_STA_PAUSE;
}

static uint32_t audio_player_proc_break_action(void* param)
{
    audio_player_proc_t* audio_player = (audio_player_proc_t*)param;

    audio_player_update_last_error(audio_player, audio_player_proc_pause(audio_player, true));

    if(AUDIO_PLAYER_PROC_SUCCESS != audio_player->last_error) {
        audio_player_proc_stop(audio_player);

        LOG_I(audio_player_proc, "--> AUDIO_PLAYER_STA_IDLE");
        return AUDIO_PLAYER_STA_IDLE;
    }

    LOG_I(audio_player_proc, "--> AUDIO_PLAYER_STA_BREAK");
    return AUDIO_PLAYER_STA_BREAK;
}

static uint32_t audio_player_proc_resume_action(void* param)
{
    audio_player_proc_t* audio_player = (audio_player_proc_t*)param;

    audio_player_update_last_error(audio_player, audio_player_proc_resume(audio_player));

    if(AUDIO_PLAYER_PROC_SUCCESS != audio_player->last_error) {
        audio_player_proc_stop(audio_player);

        LOG_I(audio_player_proc, "--> AUDIO_PLAYER_STA_IDLE");
        return AUDIO_PLAYER_STA_IDLE;
    }

    LOG_I(audio_player_proc, "--> AUDIO_PLAYER_STA_PLAY");
    return AUDIO_PLAYER_STA_PLAY;
}

static uint32_t audio_player_proc_seek_action(void* param)
{
    audio_player_proc_t* audio_player = (audio_player_proc_t*)param;

    audio_player_update_last_error(audio_player, audio_player_proc_seek(audio_player));

    if(AUDIO_PLAYER_PROC_SUCCESS != audio_player->last_error) {
        audio_player_proc_stop(audio_player);

        LOG_I(audio_player_proc, "--> AUDIO_PLAYER_STA_IDLE");
        return AUDIO_PLAYER_STA_IDLE;
    }
    
    return COM_FSM_HOLD_STATE;
}

static uint32_t audio_player_proc_play_poll(void* param)
{
    audio_player_proc_t* audio_player = (audio_player_proc_t*)param;

    audio_player_update_last_error(audio_player, audio_player_proc_monitor(audio_player));

    if(AUDIO_PLAYER_PROC_SUCCESS != audio_player->last_error) {
        audio_player_proc_stop(audio_player);

        LOG_I(audio_player_proc, "--> AUDIO_PLAYER_STA_IDLE");
        return AUDIO_PLAYER_STA_IDLE;
    }
    
    return COM_FSM_HOLD_STATE;
}

audio_player_return_t audio_player_init(audio_player_proc_t* audio_player)
{
    const com_fsm_lookup_t lookup[] = {
        { AUDIO_PLAYER_EVENT_START, AUDIO_PLAYER_STA_IDLE,  audio_player_proc_start_action, },
        { AUDIO_PLAYER_EVENT_STOP,  AUDIO_PLAYER_STA_PLAY,  audio_player_proc_stop_action, },
        { AUDIO_PLAYER_EVENT_STOP,  AUDIO_PLAYER_STA_BREAK, audio_player_proc_stop_action, },
        { AUDIO_PLAYER_EVENT_STOP,  AUDIO_PLAYER_STA_PAUSE, audio_player_proc_stop_action, },
        { AUDIO_PLAYER_EVENT_PAUSE, AUDIO_PLAYER_STA_PLAY,  audio_player_proc_pause_action, },
        { AUDIO_PLAYER_EVENT_PAUSE, AUDIO_PLAYER_STA_BREAK, audio_player_proc_pause_action, },
        { AUDIO_PLAYER_EVENT_BREAK, AUDIO_PLAYER_STA_PLAY,  audio_player_proc_break_action, },
        { AUDIO_PLAYER_EVENT_RESUME,AUDIO_PLAYER_STA_PAUSE, audio_player_proc_resume_action, },
        { AUDIO_PLAYER_EVENT_RESUME,AUDIO_PLAYER_STA_BREAK, audio_player_proc_resume_action, },
        { AUDIO_PLAYER_EVENT_SEEK,  AUDIO_PLAYER_STA_PLAY,  audio_player_proc_seek_action, },
            
        { COM_FSM_EVENT_NONE, AUDIO_PLAYER_STA_PLAY, audio_player_proc_play_poll, AUDIO_PLAYER_MONITOR_INTERVAL, },
    };
    
    memset(audio_player, 0, sizeof(audio_player_proc_t));
    audio_player->mutex_handle = xSemaphoreCreateMutex();

    com_player_init(&audio_player->com_player);

    audio_player->com_fsm = com_fsm_create(
        "audio_player", 
        AUDIO_PLAYER_TASK_STACK_SIZE, 
        TASK_PRIORITY_HIGH, 
        audio_player,
        lookup,
        sizeof(lookup)/sizeof(com_fsm_lookup_t)
        );
    
    return AUDIO_PLAYER_PROC_SUCCESS;
}

audio_player_return_t audio_player_deinit(audio_player_proc_t* audio_player)
{
    if(NULL != audio_player->mutex_handle) {
        vSemaphoreDelete(audio_player->mutex_handle);
        audio_player->mutex_handle = NULL;
    }
    
    com_player_deinit(&audio_player->com_player);
    com_fsm_delete(&audio_player->com_fsm);
    
    return AUDIO_PLAYER_PROC_SUCCESS;
}

audio_player_return_t audio_player_start(audio_player_proc_t* audio_player, audio_player_info_t* player_info, bool wait_finish)
{
    audio_player_handle_t player_handle;
    uint32_t begTick = xTaskGetTickCount();
    
    if(strlen(player_info->path) <= 0) {
        return AUDIO_PLAYER_PROC_ERR_PARAM;
    }

    audio_player_stop(audio_player);
    audio_player_lock(audio_player);

    memcpy(&audio_player->player_info, player_info, sizeof(audio_player_info_t));

    player_handle = ++g_last_alloc_handle;
    
    audio_player->last_error = AUDIO_PLAYER_PROC_SUCCESS;
    audio_player->player_handle = player_handle;
    
    com_fsm_set_event(audio_player->com_fsm, AUDIO_PLAYER_EVENT_START, COM_FSM_MAX_WAIT_TIME);

    audio_player_unlock(audio_player);

    LOG_I(common, "audio_player_start time: %d ms", (xTaskGetTickCount()-begTick));

    while(true == wait_finish)
    {
        if(player_handle != audio_player->player_handle) {
            return AUDIO_PLAYER_PROC_ERR_BREAK;
        }
        else if(AUDIO_PLAYER_STA_IDLE == audio_player_get_state(audio_player)) {
            break;
        }

        vTaskDelay(300/portTICK_RATE_MS);
    }
    
    return AUDIO_PLAYER_PROC_SUCCESS;
}

audio_player_return_t audio_player_stop(audio_player_proc_t* audio_player)
{
    audio_player_lock(audio_player);
    
    com_fsm_set_event(audio_player->com_fsm, AUDIO_PLAYER_EVENT_STOP, COM_FSM_MAX_WAIT_TIME);
    com_fsm_clear_event(audio_player->com_fsm, COM_FSM_EVENT_ALL);

    audio_player_unlock(audio_player);
    return AUDIO_PLAYER_PROC_SUCCESS;
}

audio_player_return_t audio_player_pause(audio_player_proc_t* audio_player)
{
    audio_player_return_t ret = AUDIO_PLAYER_PROC_SUCCESS;
    
    audio_player_lock(audio_player);
    
    if(AUDIO_PLAYER_STA_PLAY == audio_player_get_state(audio_player) || AUDIO_PLAYER_STA_BREAK == audio_player_get_state(audio_player))
    {
        com_fsm_set_event(audio_player->com_fsm, AUDIO_PLAYER_EVENT_PAUSE, COM_FSM_MAX_WAIT_TIME);
    }
    else
    {
        ret = AUDIO_PLAYER_PROC_ERR_NO_PLAY;
    }

    audio_player_unlock(audio_player);
    return ret;
}

audio_player_return_t audio_player_resume(audio_player_proc_t* audio_player)
{
    audio_player_return_t ret = AUDIO_PLAYER_PROC_SUCCESS;
    
    audio_player_lock(audio_player);
    
    if(AUDIO_PLAYER_STA_PAUSE == audio_player_get_state(audio_player) || AUDIO_PLAYER_STA_BREAK == audio_player_get_state(audio_player))
    {
        com_fsm_set_event(audio_player->com_fsm, AUDIO_PLAYER_EVENT_RESUME, COM_FSM_MAX_WAIT_TIME);
    }
    else
    {
        ret = AUDIO_PLAYER_PROC_ERR_NO_PAUSE;
    }

    audio_player_unlock(audio_player);
    return ret;
}

audio_player_return_t audio_player_break(audio_player_proc_t* audio_player)
{
    audio_player_return_t ret = AUDIO_PLAYER_PROC_SUCCESS;
    
    audio_player_lock(audio_player);
    
    if(AUDIO_PLAYER_STA_PLAY == audio_player_get_state(audio_player))
    {
        com_fsm_set_event(audio_player->com_fsm, AUDIO_PLAYER_EVENT_BREAK, COM_FSM_MAX_WAIT_TIME);
    }
    else
    {
        ret = AUDIO_PLAYER_PROC_ERR_NO_PLAY;
    }

    audio_player_unlock(audio_player);
    return ret;
}

audio_player_return_t audio_player_seek(audio_player_proc_t* audio_player, double seek_progress)
{
    audio_player_return_t ret = AUDIO_PLAYER_PROC_SUCCESS;
    
    audio_player_lock(audio_player);
    
    if(AUDIO_PLAYER_STA_PLAY == audio_player_get_state(audio_player))
    {
        audio_player->seek_progress = seek_progress;
        com_fsm_set_event(audio_player->com_fsm, AUDIO_PLAYER_EVENT_SEEK, COM_FSM_MAX_WAIT_TIME);
    }
    else
    {
        ret = AUDIO_PLAYER_PROC_ERR_NO_PLAY;
    }

    audio_player_unlock(audio_player);
    return ret;
}

audio_player_return_t audio_player_register_callback(audio_player_proc_t* audio_player, p_audio_player_callback callback)
{
    audio_player->audio_player_callback = callback;
    return AUDIO_PLAYER_PROC_SUCCESS;
}

bool audio_player_get_progress(audio_player_proc_t* audio_player, double* progress)
{
    int cur, all;
    int cur_pos, total_len;

    if(NULL == audio_player || NULL == audio_player->audio_file)
        return false;

    cur_pos = audio_file_tell(audio_player->audio_file);
    total_len = audio_file_size(audio_player->audio_file);
    
    if(false == com_player_get_progress(&audio_player->com_player, cur_pos, total_len, &cur, &all))
        return false;

    *progress = 100.0*cur/all;
    
    return true;
}

