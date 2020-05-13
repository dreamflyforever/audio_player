#ifndef __LOG_PRINT_H
#define __LOG_PRINT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef enum {
    PRINT_LEVEL_DEBUG,
    PRINT_LEVEL_INFO,
    PRINT_LEVEL_WARNING,
    PRINT_LEVEL_ERROR,
} print_level_t;

typedef struct {
    const char*     module_name;
    print_level_t   print_level;
    
} log_control_block_t;

#define LOG_CONTROL_BLOCK_DECLARE(_module) extern log_control_block_t log_control_block_##_module
#define log_create_module(_module, _level) log_control_block_t log_control_block_##_module = { #_module, (_level), }

#define LOG_I(x,...)            log_print(&(log_control_block_##x), PRINT_LEVEL_INFO, __func__, __LINE__, __VA_ARGS__)
#define LOG_W(x,...)            log_print(&(log_control_block_##x), PRINT_LEVEL_WARNING, __func__, __LINE__, __VA_ARGS__)
#define LOG_E(x,...)            log_print(&(log_control_block_##x), PRINT_LEVEL_ERROR, __func__, __LINE__, __VA_ARGS__)

LOG_CONTROL_BLOCK_DECLARE(common);

void log_print(log_control_block_t* log, print_level_t level, const char* func, int line, const char* fmt, ...);

#endif
