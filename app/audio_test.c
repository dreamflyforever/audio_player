#include "audio_test.h"
#include "audio_manager.h"

typedef struct {
    com_player_t    com_player;
    FILE*           file_handle;
    uint32_t        total_length;
    uint32_t        read_pos;
    
} audio_player_t;

static int com_input_callback(void* param, uint8_t* buf, int size)
{
    audio_player_t* audio_player = (audio_player_t*)param;
    int bytes_read = 0;
    bool input_done = false;
	
	bytes_read = fread(buf, 1, size, audio_player->file_handle);
	if(bytes_read < 0) {
        LOG_E(common, "fail to read file!");
        com_player_set_done(&audio_player->com_player, true);
        return 0;
    }

    audio_player->read_pos += bytes_read;

    if(COM_PLAYER_TYPE_M4A == audio_player->com_player.decoder_type) {
        if(size == 0)
            input_done = true;
    }
    else if(audio_player->read_pos >= audio_player->total_length) {
        input_done = true;
    }
    
    if(true == input_done) {
        com_player_set_done(&audio_player->com_player, false);
        LOG_I(common, "input buffer done!");
    }
    
    return bytes_read;
}


static int com_seek_callback(void* param, int position)
{
    audio_player_t* audio_player = (audio_player_t*)param;
	
	//LOG_I(common, "position:%d", position);
	audio_player->read_pos = position;
    
    return fseek(audio_player->file_handle, position, SEEK_SET);
}

static int com_error_callback(void* param, int error)
{
    audio_player_t* audio_player = (audio_player_t*)param;

    LOG_E(common, "error:0x%X", error);
    com_player_set_done(&audio_player->com_player, true);
    
    return 0;
}

void com_player_test(void)
{
	static audio_player_t audio_player;
    int cur, all;
    
    memset(&audio_player, 0, sizeof(audio_player_t));
	
	//audio_player.file_handle = fopen("/home/look/files/www/music/518686034.mp3", "rb");
	audio_player.file_handle = fopen("/home/jim/test/player/test1.mp3", "rb");
	//audio_player.file_handle = fopen("/home/look/files/www/music/mp3_test/lr_test2.mp3", "rb");
	fseek(audio_player.file_handle, 0, SEEK_END);
	audio_player.total_length = ftell(audio_player.file_handle);
	fseek(audio_player.file_handle, 0, SEEK_SET);
    
	pcm_trans_init();
    com_player_init(&audio_player.com_player);
	
	if(COM_PLAYER_SUCCESS != com_player_start(
		&audio_player.com_player, 
		COM_PLAYER_TYPE_MP3,
		com_input_callback, 
		com_error_callback, 
		com_seek_callback, 
		&audio_player))
	{
		LOG_E(common, "com_player_start failed");
	}
	else
	{
		while(true)
		{
			if(true == com_player_get_progress(&audio_player.com_player, audio_player.read_pos, audio_player.total_length, &cur, &all))
				LOG_I(common, "progress: %02d:%02d/%02d:%02d", cur/60, cur%60, all/60, all%60);
			
			if(true == com_player_is_done(&audio_player.com_player)) {
				break;
			}
            else if(true == com_player_auto_resume(&audio_player.com_player)) {
                LOG_I(common, "auto resume com_player");
            }
			
			usleep(1000*1000);
		}
	}

    com_player_stop(&audio_player.com_player);
	com_player_deinit(&audio_player.com_player);
    fclose(audio_player.file_handle);
}

void http_download_test(void)
{
	common_buffer_t http_buffer;
	http_download_proc_t http_proc;

	uint32_t len;
	FILE* file_writer;
	/*XXX: python -m SimpleHTTPServer&*/
	char* file_name = "http://127.0.0.1:8000/test1.mp3";
	char buf[512];

	/*fetch the file name*/
	for(len=strlen(file_name); len>0; len--) {
		if(file_name[len-1]=='/')
			break;
	}
	/*XXX absolute address may be change to relative path*/
	sprintf(buf, "/home/jim/test/player/project/audio_player/%s", &file_name[len]);
	file_writer = fopen(buf, "wb");

	common_buffer_init(&http_buffer, COMMON_BUF_HTTP_NODE_SIZE, COMMON_BUF_HTTP_MAX_SIZE);
	http_download_init(&http_proc);
	http_download_start_seek(&http_proc, &http_buffer, file_name, true, 0);

	while(1) {
		len = common_buffer_get_count(&http_buffer);

		if(len > 0) {
			len = sizeof(buf);
			common_buffer_pop(&http_buffer, (uint8_t*)buf, &len);
			fwrite(buf, 1, len, file_writer);
		}
		else if(true==http_download_is_stopped(&http_proc)) {
			break;
		}
		else {
			vTaskDelay(200/portTICK_RATE_MS);
		}
	}		

	http_download_stop(&http_proc);
	http_download_deinit(&http_proc);
	common_buffer_deinit(&http_buffer);

	fclose(file_writer);
}

void audio_file_test(void)
{
	audio_file_t* web_file = audio_file_open("http://127.0.0.1:8000/test2.m4a", AUDIO_FILE_TYPE_WEB, true);
	audio_file_t* local_file = audio_file_open("/home/jim/test/player/test2.m4a", AUDIO_FILE_TYPE_SD_CARD, false);

	uint8_t buffer1[1024];
	uint8_t buffer2[1024];

	int i, len1, len2;
	int all1 = 0, all2 = 0;

	printf("[web_file] size:%d\n", audio_file_size(web_file));
	printf("[local_file] size:%d\n", audio_file_size(local_file));

    static uint8_t buf[1024*1024*5];
    printf("audio_file_preread:%d\n", audio_file_preread(web_file, buf, sizeof(buf)));

	for(i = 0; ; i++)
	{
		len1 = audio_file_read(web_file, buffer1, sizeof(buffer1));
		len2 = audio_file_read(local_file, buffer2, sizeof(buffer2));

		if(i < 100) {
			int seek = rand() % local_file->total_length;
			audio_file_seek(web_file, seek);
			audio_file_seek(local_file, seek);
		}

		if(len1 != len2) {
			printf("len1:%d <> len2:%d\n", len1, len2);
			break;
		}

		if(0 != memcmp(buffer1, buffer2, (len1>len2)?len1:len2)) {
			printf("buffer1:%d <> buffer2:%d\n", len1, len2);
			break;
		}

		all1 += len1;
		all2 += len2;

		if(len2 <= 0) {
			printf("all1:%d, all2:%d\n", all1, all2);
			printf("pos1:%d, pos2:%d\n", web_file->read_pos, local_file->read_pos);
			break;
		}
	}

	audio_file_close(&web_file);
	audio_file_close(&local_file);
}

void audio_player_test(void)
{
	static audio_player_proc_t resource_player;
	static audio_player_proc_t prompt_player;
	pcm_trans_init();
	audio_player_init(&resource_player);
	audio_player_init(&prompt_player);

	/*python -m SimpleHTTPServer& in the mp3 dirctory*/
	static audio_player_info_t resource_player_info[] = {
		{"http://0.0.0.0:8000/test1.mp3", AUDIO_PLAYER_SRC_WEB, AUDIO_PLAYER_TYPE_RESOURCE, 0},
		{"http://127.0.0.1:8000/test2.m4a", AUDIO_PLAYER_SRC_WEB, AUDIO_PLAYER_TYPE_RESOURCE, 0},
		{"../test3.aac", AUDIO_PLAYER_SRC_SD_CARD, AUDIO_PLAYER_TYPE_RESOURCE, 0},
		{"../test3.aac", AUDIO_PLAYER_SRC_SD_CARD, AUDIO_PLAYER_TYPE_PROMPT, 0},
		{"StartUp.mp3", AUDIO_PLAYER_SRC_FLASH, AUDIO_PLAYER_TYPE_PROMPT, 0},
		{"http://readbook.koo6.cn/soundU/64495cn.mp3?sign=d0f1c33058afb4fb1fef81ae198f13b4&t=5bf226bd",
		AUDIO_PLAYER_SRC_WEB, AUDIO_PLAYER_TYPE_RESOURCE, 0}
	};
	int i;
	for (i = 0; i < sizeof(resource_player_info)/sizeof(resource_player_info[0]); i++) {
		if (resource_player_info[i].type== AUDIO_PLAYER_TYPE_PROMPT)
			audio_player_start(&prompt_player, &resource_player_info[i], false);
		else
			audio_player_start(&resource_player, &resource_player_info[i], false);
		vTaskDelay(1000);
	}
	while(1) {
		if( AUDIO_PLAYER_STA_IDLE == audio_player_get_state(&resource_player) && 
				AUDIO_PLAYER_STA_IDLE == audio_player_get_state(&prompt_player)) {
			break;
		}

		vTaskDelay(1000);
	}

	audio_player_deinit(&resource_player);
	audio_player_deinit(&prompt_player);
	pcm_trans_deinit();
}

#if 0
void samplerate_convert_test(void)
{
	SRC_DATA src_data;
    int error, read_len, i;
    uint8_t read_buf[512];
    uint8_t write_buf[1024];
    int16_t tmp16;
    
    float input_buffer[1024];
    float output_buffer[2048];
    
    FILE* reader = fopen("/home/look/Files/www/music/pcm_test/sample_44100_chann_2.pcm", "rb");
    FILE* writer = fopen("/home/look/Files/www/music/pcm_test/samplerate_convert_test.pcm", "wb");    
    SRC_STATE* src_state = src_new(SRC_SINC_MEDIUM_QUALITY, 2, &error);

    src_data.src_ratio = 1.0 *32000 /44100;
    src_data.end_of_input = 0;
    src_data.data_in = input_buffer;
    src_data.input_frames = 0;

    while(!feof(reader))
    {
        read_len = fread(read_buf, 1, sizeof(read_buf), reader);

        for(i = 0; i < (read_len/2); i++)
        {
            tmp16  = read_buf[2*i+1];
            tmp16 <<= 8;
            tmp16 += read_buf[2*i];

            input_buffer[2*src_data.input_frames + i] = tmp16;

            //fprintf(writer, "%d\n", tmp16);
        }

        src_data.input_frames += read_len /2 /2;
        src_data.data_out = output_buffer;
        src_data.output_frames = 2048 /2;

        src_process(src_state, &src_data);

        src_data.input_frames -= src_data.input_frames_used;

        /*for(i = 0; i < src_data.input_frames; i++) {
            input_buffer[2*i] = input_buffer[2*src_data.input_frames_used + i];
            input_buffer[2*i+1] = input_buffer[2*src_data.input_frames_used + i+1];
        }*/
        
        memmove(input_buffer, input_buffer + sizeof(float)*src_data.input_frames_used*2, 
            sizeof(float)*src_data.input_frames*2);

        for(i = 0; i < src_data.output_frames_gen; i++)
        {
            tmp16 = output_buffer[2*i];
            write_buf[4*i + 0] = tmp16;
            write_buf[4*i + 1] = tmp16>>8;

            tmp16 = output_buffer[2*i+1];
            write_buf[4*i + 2] = tmp16;
            write_buf[4*i + 3] = tmp16>>8;
        }

        fwrite(write_buf, 1, 4*src_data.output_frames_gen, writer);

        /*for(i = 0; i < src_data.output_frames_gen; i++)
        {
            tmp16 = output_buffer[i];
            
            write_buf[2*i + 0] = tmp16;
            write_buf[2*i + 1] = tmp16>>8;
        }

        fwrite(write_buf, 1, 2*src_data.output_frames_gen, writer);*/
        
    }

    src_delete(src_state);
    fclose(reader);
    fclose(writer);
}

#else
void samplerate_convert_test(void)
{
    int read_len = 0;
    int write_len;
    int consumed;
    uint8_t read_buf[512];
    uint8_t write_buf[1024];
    
    FILE* reader = fopen("/home/look/files/www/music/pcm_test/lr_test2.pcm", "rb");
    FILE* writer = fopen("/home/look/files/www/music/pcm_test/samplerate_convert_test.pcm", "wb");    

    double src_ratio = 1.0 *32000 /44100;
    ring_buffer_t ring_buffer;
    samplerate_convert_t* samplerate;

    ring_buffer_init(&ring_buffer, 32000);
    samplerate = samplerate_convert_alloc(&ring_buffer);

    while(!feof(reader))
    {
        read_len += fread(&read_buf[read_len], 1, sizeof(read_buf) - read_len, reader);

        consumed = samplerate_convert_process(samplerate, src_ratio, 2, read_buf, read_len);

        read_len -= consumed;
        memmove(read_buf, &read_buf[consumed], read_len);

        do {
            write_len = ring_buffer_pop(&ring_buffer, write_buf, sizeof(write_buf), false);
            fwrite(write_buf, 1, write_len, writer);
        }
        while (write_len > 0);
    }

    samplerate_convert_dealloc(&samplerate);
    ring_buffer_deinit(&ring_buffer);
    fclose(reader);
    fclose(writer);
}

#endif

