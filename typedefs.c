#include "typedefs.h"
//#include "server_env.h"
#include <stdarg.h>

uint8_t g_volume_index = 5;
int g_child_lock = 0;

uint32_t xTaskGetTickCount(void)
{
    static uint32_t init_time = 0;
    static uint32_t cur_time;
    struct timeval tv;
    
	gettimeofday(&tv, NULL);

    cur_time = tv.tv_sec*1000+tv.tv_usec/1000;

    if(0 == init_time) {
        init_time = cur_time;
    }

    cur_time = cur_time - init_time;
	return cur_time;
}

pthread_mutex_t* xSemaphoreCreateMutex()
{
	pthread_mutex_t* tmp = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	
	if(NULL != tmp)
		*tmp = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	
	return tmp;
}

int xSemaphoreTake(SemaphoreHandle_t mutex, uint32_t timeout)
{
	if(portMAX_DELAY == timeout)
    {
        if(0 != pthread_mutex_lock(mutex)) {
            return pdFALSE;
        }
	
	    return pdTRUE;
	}
	
	struct timeval now;
	struct timespec outtime;

	gettimeofday(&now, NULL);

	now.tv_sec  += timeout/1000;
	now.tv_usec += (timeout%1000)*1000;

	now.tv_sec  += now.tv_usec/1000000;
	now.tv_usec %= 1000000;
        
	outtime.tv_sec  = now.tv_sec;
	outtime.tv_nsec = now.tv_usec*1000;

    if(0 != pthread_mutex_timedlock(mutex, &outtime)) {
        return pdFALSE;
    }
	
	return pdTRUE;
}

int xSemaphoreGive(SemaphoreHandle_t mutex)
{
    if(0 != pthread_mutex_unlock(mutex)) {
        return pdFALSE;
    }
	
	return pdTRUE;
}

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char* pcName, int usStackDepth, 
	void* pvParameters, int uxPriority, TaskHandle_t* pxCreatedTask)
{
	pthread_t tmp;
	
	if(NULL == pxCreatedTask)
		pxCreatedTask = &tmp;
	
    return pthread_create(pxCreatedTask, NULL, pxTaskCode, pvParameters) ?0 :1;
}

int f_open(FILE** file, char* path, int mode)
{
	FILE* ret = NULL;
	
	if(FA_READ & mode)
		ret = fopen(path, "rb");
	else
		ret = fopen(path, "wb");
	
	if(NULL == ret)
		return -1;
	
	*file = ret;
	return 0;
}

int f_close(FILE** file)
{
	if(*file == NULL)
		return -1;

    fflush(*file);
	
	fclose(*file);
    *file = NULL;
	return 0;
}

int f_read(FILE** file, uint8_t* buf, int size, UINT* bytes_read)
{
	int ret;
	ret = fread(buf, 1, size, *file);
	
	if(ret < 0)
		return ret;
	
	*bytes_read = ret;
	return 0;
}

int f_write(FILE** file, const uint8_t* buf, int size, UINT* bytes_read)
{
    int ret;
	ret = fwrite(buf, 1, size, *file);
	
	if(ret < 0)
		return ret;
	
	*bytes_read = ret;
	return 0;
}

int f_lseek(FILE** file, int offset)
{
	return fseek(*file, offset, SEEK_SET);
}

int f_size(FILE** file)
{
	int back = ftell(*file);
	int ret = 0;
	
	fseek(*file, 0, SEEK_END);
	ret = ftell(*file);
	fseek(*file, back, SEEK_SET);
	
	return ret;
}

void no_optimize(int p)
{
}

static const unsigned char device_mac_addr[] = {0x70,0x4F,0x08,0x00,0x00,0xB0};
//static const unsigned char device_mac_addr[] = {0x11,0x22,0x33,0x44,0x55,0x66};

int get_STA_MACAddr(char MACAddr[], uint8_t *size)
{
	char MACAddr_tmp[18];
	uint32_t MACAddr_size = sizeof(MACAddr_tmp);

	if((NULL == MACAddr) || (0 == *size))
		return -1;

    memset(MACAddr_tmp, 0, MACAddr_size);

    sprintf(MACAddr_tmp, "%02X:%02X:%02X:%02X:%02X:%02X",
        device_mac_addr[0],
        device_mac_addr[1],
        device_mac_addr[2],
        device_mac_addr[3],
        device_mac_addr[4],
        device_mac_addr[5]);
	
	memcpy(MACAddr, MACAddr_tmp, (*size < MACAddr_size ? *size : MACAddr_size));

	return (*size < MACAddr_size ? *size : MACAddr_size);
}

int32_t wifi_config_get_mac_address(uint8_t port, void *address)
{
    memcpy(address, device_mac_addr, sizeof(device_mac_addr));
    return 0;
}

unsigned char system_config_deviceid_num(void)
{
	return 4;
}

static char* find_value(char* str)
{
    int count = 0;
    const char* split = ": \r"; 
    char* p = strtok(str, split);
    
    while(NULL != p)
    {
        if(NULL == strchr(split, p[0]) && ++count == 3) {
            return p;
        }
            
        p = strtok(NULL, split); 
    }

    return "";
}

void cat_memory(void)
{
    FILE* reader;
    char str[512];

    sprintf(str, "/proc/%d/status", getpid());
    reader = fopen(str, "r");

    if(NULL == reader)
        return;

    while(!feof(reader))
    {
        fgets(str, sizeof(str), reader);

        if(NULL != strstr(str, "VmSize"))
        {
            LOG_I(common, ">>>>>>>>>>>>> VmSize: %s <<<<<<<<<<<<<<<", find_value(str));
        }
    }

    fclose(reader);
}

