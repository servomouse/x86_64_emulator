#include "utils.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdarg.h>

#define LOG_LEVEL 3
#define LOG_FILE_NAME "console.log"
// #define PRINT_LOGS_TO_FILE

void mylog(uint8_t log_level, const char *format, ...) {
    if(log_level < LOG_LEVEL)
        return;
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    #ifdef PRINT_LOGS_TO_FILE
    FILE *file = fopen(LOG_FILE_NAME, "a");
    if(file) {
        vfprintf(file, format, args);
        fclose(file);
    } else {
        printf("ERROR: can't open log file %s", LOG_FILE_NAME);
    }
    #endif
    va_end(args);
}

void clear_console(void) {
    printf("\033[H\033[J");
}

void sleep_ms(uint32_t ms) {
    #ifdef _WIN32
    Sleep(ms);
    #else
    usleep(ms*1000);
    #endif
}

