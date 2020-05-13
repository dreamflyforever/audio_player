#include "flash_access.h"

extern int _binary_prompt_data_1_dat_start;
extern int _binary_prompt_data_2_dat_start;
extern int _binary_prompt_data_3_dat_start;
extern int _binary_prompt_data_4_dat_start;

int flash_read(unsigned int addr, uint8_t* buffer, int len)
{
	uint8_t* start_addr = NULL;
	
	switch(0xC0000000 & addr)
	{
	case PROMPT_DATA_1_BASE:	start_addr = (uint8_t*)&_binary_prompt_data_1_dat_start;	break;
	case PROMPT_DATA_2_BASE:	start_addr = (uint8_t*)&_binary_prompt_data_2_dat_start;	break;
	case PROMPT_DATA_3_BASE:	start_addr = (uint8_t*)&_binary_prompt_data_3_dat_start;	break;
	case PROMPT_DATA_4_BASE:	start_addr = (uint8_t*)&_binary_prompt_data_4_dat_start;	break;
	}
	
	start_addr += 0x0FFFFFFF & addr;
	
	if(NULL == start_addr)
		return -1;
	
	memcpy(buffer, start_addr, len);
	
	return 0;
}
