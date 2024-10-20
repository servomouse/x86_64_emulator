#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    void *data;
    uint32_t size;
} saved_data_t;


void mylog(const char *log_file, const char *format, ...);
void clear_console(void);
void sleep_ms(uint32_t ms);
char *get_time(void);
int store_data(void *data, size_t size, char *filename);
int restore_data(void *data, size_t size, char *filename);
uint64_t get_hash(uint8_t *data, size_t size);