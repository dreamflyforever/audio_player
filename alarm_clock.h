#ifndef __ALARM_CLOCK_H__
#define __ALARM_CLOCK_H__

#include "typedefs.h"

typedef struct{
	unsigned int date;	
    char object[32];
    char vid[256];
}alarm_clock_t;

#define alarm_clock_all_delete()
#define alarm_clock_set(x)	0

#endif