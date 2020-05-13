#include "log_print.h"
#include "typedefs.h"

log_create_module(common, PRINT_LEVEL_INFO);

void log_print(log_control_block_t* log, print_level_t level, const char* func, int line, const char* fmt, ...)
{
    char str[5*1024];
    const char* level_name = "";
    
    if(level < log->print_level)
        return;

    switch(level)
    {
    case PRINT_LEVEL_DEBUG:     level_name = "debug";   break;
    case PRINT_LEVEL_INFO:      level_name = "info";    break;
    case PRINT_LEVEL_WARNING:   level_name = "warn";    break;
    case PRINT_LEVEL_ERROR:     level_name = "error";   break;
    }

    va_list args;
	va_start(args, fmt);
	vsprintf(str, fmt, args);
	va_end(args);

    printf("[T:%u M:%s F:%s L:%d C:%s] %s\n", xTaskGetTickCount(), log->module_name, func, line, level_name, str);
}

