#ifndef __FLASH_ACCESS_H
#define __FLASH_ACCESS_H

#include "typedefs.h"

#define ROM_BASE 0
#define PROMPT_DATA_1_BASE	0x00000000
#define PROMPT_DATA_2_BASE	0x40000000
#define PROMPT_DATA_3_BASE	0x80000000
#define PROMPT_DATA_4_BASE	0xC0000000

#define xPortGetFreeHeapSize() 0

int flash_read(unsigned int addr, uint8_t* buffer, int len);

#endif
