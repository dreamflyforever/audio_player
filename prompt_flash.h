#ifndef _PROMPT_FLASH_H_
#define _PROMPT_FLASH_H_

#include <stdint.h>

#define PROMPT_DATA_FILE_NUM    (4)
#define FILE_ADDR_INFO_LEN      sizeof(file_addr_info_t)

typedef struct {
	unsigned int offset;   /* ��ʾ��������ƫ�Ƶ�ַ */
	unsigned int len;      /* ��ʾ���ļ���С */
} file_addr_info_t;

typedef struct {
	char *prompt_data_file_name;
	char **prompt_file_name;
} prompt_file_info_t;

typedef struct {
	char **prompt_file_array; /* ��ʾ���ļ��б� */
	unsigned int addr;        /* ��ʾ�����ݴ洢��ַ������CPU��ַ�ռ䷶Χ�ڣ� */
} prompt_data_info_t;


extern void test_print_prompt_addr_info();
extern bool prompt_file_check_exist(const char *file_name);
extern int32_t prompt_addr_info_get(const char *file_name, uint32_t *offset_addr, uint32_t *len);
extern int32_t prompt_audio_data_get_by_name(uint8_t **data_buf, uint32_t *data_len, const char *file_name);
extern int32_t prompt_audio_data_get_by_offset(uint8_t **data_buf, uint32_t offset_addr, uint32_t data_len);

#endif