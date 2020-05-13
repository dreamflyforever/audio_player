#ifndef __TYPEDEFS_H
#define __TYPEDEFS_H

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>

#include "task_def.h"
#include "log_print.h"

#if 0
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long  uint32_t;
#endif

typedef unsigned   char    u8_t;
typedef signed     char    s8_t;
typedef unsigned   short   u16_t;
typedef signed     short   s16_t;
typedef unsigned   long    u32_t;
typedef signed     long    s32_t;

#define COMPILE_ASSERT(expression) switch (0) {case 0: case expression:;}

typedef void* task_return_t;
typedef pthread_t TaskHandle_t;
typedef int UINT;
typedef uint32_t StackType_t;
typedef long BaseType_t;
typedef void* (*TaskFunction_t)(void*);
typedef uint32_t portSTACK_TYPE;


#define portTICK_PERIOD_MS                  1
#define pdTASK_CODE                         TaskFunction_t
#define SemaphoreHandle_t					pthread_mutex_t*
#define xSemaphoreCreateBinary()			xSemaphoreCreateMutex()
#define vSemaphoreDelete(x)                 do { /*pthread_mutex_destroy(x);*/ free(x); } while(0)
#define xSemaphoreTakeFromISR(x, NULL)		xSemaphoreTake(x, portMAX_DELAY)
#define xSemaphoreGiveFromISR(x, NULL)		xSemaphoreGive(x)

#define pvPortMalloc(x)						malloc((int)(x))
#define pvPortCalloc(x,y)					calloc((int)(x),(int)(y))
#define vPortFree(x)						free(x)
#define vTaskDelete(x)						no_optimize((int)(uint64_t)(x)); return NULL
#define portMAX_DELAY                       0xFFFFFFFFUL
#define pdPASS			                    (pdTRUE)
#define pdFAIL			                    (pdFALSE)
#define portTICK_RATE_MS					1
#define pdTRUE                              1
#define pdFALSE                             0
#define TickType_t							uint32_t

#define taskENTER_CRITICAL()
#define taskEXIT_CRITICAL()
#define vTaskDelay(x)						usleep(1000*x)
#define xQueueGetMutexHolder(x)				1
#define xTaskGetCurrentTaskHandle()			0

#define WIFI_PORT_STA                       0
#define WIFI_CONNECTED						true
#define WIFI_UNCONNECTED					false
#define ALARM_REMIND_AUDIO					"AlmReminder.mp3"
#define device_state_get(x)                 0
#define device_state_judge(x)				0
#define display_event_handler(x)
#define speech_multwheel_handler()			false
#define speech_status_get()					false
#define tf_card_audio_file_num_get()		0
#define tf_card_file_path_get(x,y)			0
#define set_alarm_clock_ring(x)
#define low_power_timer_stop()
#define battery_capacity_get()				100
#define wifi_connection_disconnect_ap()
#define wifi_config_reload_setting()
#define low_power_mode_flag_get()           false
#define alarm_clock_ring_check_and_stop()
#define network_mgr_restart()
#define g_rtc_update_flag                   true
#define pub_update_alarm_list_msg()         0
#define startup_interactive_wait_network_check(x)
#define net_proc_is_connected(x)            true
#define network_mgr_is_connected()			true
#define network_mgr_check_connection()		true
#define set_child_lock(x)                   0
#define aucodec_spk_gain_set(x)
#define set_volume(x)
#define volume_max_level_get()              10

#define DEF_VOICE_FILE_TRACE

typedef FILE* FIL;
typedef int FSIZE_t;
#define _T(x)								x
#define FR_OK								0
#define FA_OPEN_EXISTING					0x01
#define FA_WRITE							0x02
#define FA_READ								0x04
#define	FA_CREATE_NEW		                0x08
#define	FA_CREATE_ALWAYS	                0x10
#define	FA_OPEN_ALWAYS		                0x20
#define	FA_OPEN_APPEND		                0x40

#define EventGroupHandle_t 					common_event_t*
#define xEventGroupCreate()					common_create_event()
#define vEventGroupDelete(x)				common_delete_event(x)
#define xEventGroupSetBits(x,y)				common_set_event(x,y)
#define xEventGroupSetBitsFromISR(x,y,t)	common_set_event(x,y)
#define xEventGroupClearBits(x,y)			common_clear_event(x,y)
#define xEventGroupClearBitsFromISR(x,y)	common_clear_event(x,y)
#define xEventGroupWaitBits(a,b,c,d,e)		common_wait_event(a,b,e)

#define QueueHandle_t						common_queue_t*
#define xQueueCreate(x,y)					common_create_queue(y)
#define vQueueDelete(x)						common_delete_queue(x)
#define xQueueSend(x, y, z)					common_send_queue(x,y,z)
#define xQueueReceive(x, y, z)				common_recv_queue(x,y,z)
#define xQueueReset(x)						do {int tmp=(x)->item_size; common_delete_queue(x); (x)=common_create_queue(tmp);} while(0)

#define vSemaphoreCreateBinary(x) x = xSemaphoreCreateMutex()

uint32_t xTaskGetTickCount(void);
SemaphoreHandle_t xSemaphoreCreateMutex();
int xSemaphoreTake(SemaphoreHandle_t mutex, uint32_t timeout);
int xSemaphoreGive(SemaphoreHandle_t mutex);

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char* pcName, int usStackDepth, 
	void* pvParameters, int uxPriority, TaskHandle_t* pxCreatedTask);

int f_open(FILE** file, char* path, int mode);
int f_close(FILE** file);
int f_read(FILE** file, uint8_t* buf, int size, UINT* bytes_read);
int f_write(FILE** file, const uint8_t* buf, int size, UINT* bytes_read);
int f_lseek(FILE** file, int offset);
int f_size(FILE** file);

void no_optimize(int p);

extern uint8_t g_volume_index;
extern int g_child_lock;

int get_STA_MACAddr(char MACAddr[], uint8_t *size);
int32_t wifi_config_get_mac_address(uint8_t port, void *address);
unsigned char system_config_deviceid_num(void);
void cat_memory(void);


#endif
