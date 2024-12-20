#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    void *data;
    uint32_t size;
} saved_data_t;

#define READ_FUNC_PTR(_func_name)  uint16_t(*_func_name)(uint32_t, uint8_t)
#define WRITE_FUNC_PTR(_func_name) void(*_func_name)(uint32_t, uint16_t, uint8_t)

#ifdef __unix__
    #define DLL_PREFIX 
#elif defined(_WIN32) || defined(WIN32)
    #define DLL_PREFIX __declspec(dllexport)
#endif

void mylog(uint8_t log_level, const char *log_file, const char *format, ...);
void clear_console(void);
void sleep_ms(uint32_t ms);
char *get_time(void);
int store_data(void *data, size_t size, char *filename);
int restore_data(void *data, size_t size, char *filename);
uint64_t get_hash(uint8_t *data, size_t size);

void set_log_level(uint8_t new_log_level);