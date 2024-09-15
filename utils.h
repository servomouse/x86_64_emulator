#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


void mylog(uint8_t log_level, const char *format, ...);
void clear_console(void);
void sleep_ms(uint32_t ms);
char *get_time(void);