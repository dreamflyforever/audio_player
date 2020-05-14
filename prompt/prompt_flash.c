#include <stdio.h>
#include <string.h>
#include "flash_access.h"
#include "prompt_flash.h"
#include "prompt_def.h"

log_create_module(prompt_flash, PRINT_LEVEL_INFO);

char *prompt_file_name_1[] = { PROMPT_FILE_NAME_1 };
char *prompt_file_name_2[] = { PROMPT_FILE_NAME_2 };
char *prompt_file_name_3[] = { PROMPT_FILE_NAME_3 };
char *prompt_file_name_4[] = { PROMPT_FILE_NAME_4 };

static prompt_data_info_t data_info[PROMPT_DATA_FILE_NUM] = {{prompt_file_name_1, PROMPT_DATA_1_BASE},
	                                                         {prompt_file_name_2, PROMPT_DATA_2_BASE},
	                                                         {prompt_file_name_3, PROMPT_DATA_3_BASE},
	                                                         {prompt_file_name_4, PROMPT_DATA_4_BASE}};

/* Get prompt file index according to prompt file name */
static int32_t prompt_file_index_get(const char *file_name, int *data_index, int *file_index)
{
	int count = 0;
	int len = strlen(file_name);
	int index = 0;
	char **prompt_file_list = NULL;
	
	if (NULL == file_name) {
		LOG_E(prompt_flash, "Input parameter is NULL!\n");
		return -1;
	}

	for (index = 0; index < PROMPT_DATA_FILE_NUM; index++) {
		count = 0;
		prompt_file_list = data_info[index].prompt_file_array;

		while (NULL != prompt_file_list[count]) {
			if (0 == strncmp(file_name, prompt_file_list[count], len)) {
				*data_index = index;
				*file_index = count;
				break;
			}

			count++;
		}

		if (NULL == prompt_file_list[count]) {
			continue;
		} else {
			break;
		}
	}

	if (PROMPT_DATA_FILE_NUM == index) {
		LOG_E(prompt_flash, "Cannot find prompt %s!\n", file_name);
		return -1;
	}

	return 0;
}

/*function:check the prompt file is exist or not
 *true:prompt exist
 *false:prompt not exist
 */
bool prompt_file_check_exist(const char *file_name)
{
	int32_t ret = -1;
	int data_index = -1, file_index = -1;
	ret = prompt_file_index_get(file_name, &data_index, &file_index);
	if(-1 == ret){
		return false;
	}
	else{
		return true;
	}
}

/* Get prompt data offset addr in flash and data len */
int32_t prompt_addr_info_get(const char *file_name, uint32_t *offset_addr, uint32_t *len)
{
	int data_index = 0;
	int file_index = 0;
	int32_t ret = 0;
	uint32_t start_address = 0;
	file_addr_info_t file_addr;
	
	if ((NULL == file_name) || (NULL == offset_addr) || (NULL == len)) {
		LOG_E(prompt_flash, "Input parameter is NULL!\n");
		return -1;
	}

	ret = prompt_file_index_get(file_name, &data_index, &file_index);
	if (0 != ret) {
		LOG_E(prompt_flash, "Get prompt %s index failed!\n", file_name);
		return -1;
	}

	start_address = data_info[data_index].addr + file_index * FILE_ADDR_INFO_LEN;
	
	ret = flash_read(start_address, (uint8_t *)&file_addr, FILE_ADDR_INFO_LEN);
	if (0 != ret) {
		LOG_E(prompt_flash, "Call flash read failed! start_address = 0x%x\n", start_address);
		return -1;
	}

	*offset_addr = file_addr.offset + data_info[data_index].addr - ROM_BASE;
	*len         = file_addr.len;

	return 0;
}

/* 
 * 说明：
 *     该函数内部分配内存，调用者在使用完毕后，需自行释放所使用内存。
 */
int32_t prompt_audio_data_get_by_offset(uint8_t **data_buf, uint32_t offset_addr, uint32_t data_len)
{
	int32_t ret = 0;
	
	if (0 == data_buf) {
		LOG_E(prompt_flash, "Input parameter is NULL!\n");
		return -1;
	}

	*data_buf = (uint8_t *)pvPortCalloc(data_len, sizeof(uint8_t));
	if (0 == *data_buf) {
		LOG_E(prompt_flash, "Memory calloc failed! data_len = %d, free_heap: %d\n", data_len, xPortGetFreeHeapSize());
		return -1;
	}

	ret = flash_read(ROM_BASE + offset_addr, *data_buf, data_len);
	if (0 != ret) {
		LOG_E(prompt_flash, "Call flash read failed! start_address = 0x%x\n", ROM_BASE + offset_addr);
		return -1;
	}

	return 0;
}

/* 
 * 说明：
 *     该函数内部分配内存，调用者在使用完毕后，需自行释放所使用内存。
 * 功能：
 *     根据提示音文件名，从Flash中读取音频数据以及数据量大小。
 */
int32_t prompt_audio_data_get_by_name(uint8_t **data_buf, uint32_t *data_len, const char *file_name)
{
	uint32_t offset_addr = 0;
	uint32_t len = 0;
	int32_t ret = 0;
	
	if (0 == file_name) {
		LOG_E(prompt_flash, "Input parameter is NULL!\n");
		return -1;
	}

	ret = prompt_addr_info_get(file_name, &offset_addr, &len);
	if (0 != ret) {
		LOG_E(prompt_flash, "Call prompt_addr_info_get failed!\n");
		return -1;
	}

	*data_len = len;
	*data_buf = (uint8_t *)pvPortCalloc(len, sizeof(uint8_t));
	if (0 == *data_buf) {
		LOG_E(prompt_flash, "Memory calloc failed! len: %d, free_heap: %d\n", len, xPortGetFreeHeapSize());
		return -1;
	}

	ret = flash_read(ROM_BASE + offset_addr, *data_buf, len);
	if (0 != ret) {
		LOG_E(prompt_flash, "Call flash read failed! start_address = 0x%x\n", ROM_BASE + offset_addr);
		return -1;
	}

	return 0;
}

void test_print_prompt_addr_info(int data_index)
{
	int count = 0;
	int32_t ret = 0;
	uint32_t offset_addr;
	uint32_t len;
	char **prompt_file_list = NULL;
	uint32_t prompt_data_base;

	prompt_file_list = data_info[data_index].prompt_file_array;
	prompt_data_base = data_info[data_index].addr;
	
	while (NULL != prompt_file_list[count]) {
		offset_addr = 0;
		len = 0;
	
		ret = prompt_addr_info_get(prompt_file_list[count], &offset_addr, &len);
		if (0 == ret) {
			printf("[File]: %s, [addr]: 0x%x, [len]: %u\n", prompt_file_list[count], 
				offset_addr - (prompt_data_base - ROM_BASE),
				len);
		} else {
			printf("Call prompt_addr_info_get failed!, file_name: %s, count: %d\n", prompt_file_list[count], count);
		}

		count++;
	}
}
