#ifndef _PROMPT_FLASH_H_
#define _PROMPT_FLASH_H_

#include <stdint.h>

#define PROMPT_DATA_FILE_NUM    (4)
#define FILE_ADDR_INFO_LEN      sizeof(file_addr_info_t)

typedef struct {
	unsigned int offset;   /* 提示音分区内偏移地址 */
	unsigned int len;      /* 提示音文件大小 */
} file_addr_info_t;

typedef struct {
	char *prompt_data_file_name;
	char **prompt_file_name;
} prompt_file_info_t;

typedef struct {
	char **prompt_file_array; /* 提示音文件列表 */
	unsigned int addr;        /* 提示音数据存储地址（整个CPU地址空间范围内） */
} prompt_data_info_t;


extern void test_print_prompt_addr_info();
extern bool prompt_file_check_exist(const char *file_name);
extern int32_t prompt_addr_info_get(const char *file_name, uint32_t *offset_addr, uint32_t *len);
extern int32_t prompt_audio_data_get_by_name(uint8_t **data_buf, uint32_t *data_len, const char *file_name);
extern int32_t prompt_audio_data_get_by_offset(uint8_t **data_buf, uint32_t offset_addr, uint32_t data_len);

#endif