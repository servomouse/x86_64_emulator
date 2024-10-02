#include "utils.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdarg.h>
#include <time.h>
#include "log_server_iface/logs_win.h"

#define BUFFER_SIZE 1024

void mylog(const char *log_file, const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[BUFFER_SIZE] = {0};
    vsnprintf(buffer, sizeof(buffer), format, args);
    // vprintf(format, args);
    // printf("Processing string %s\n", buffer);
    log_server_send(log_file, buffer);
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

char *get_time(void) {
    time_t t;
    struct tm *tmp;
    static char buffer[64] = {0};
    time(&t);
    tmp = localtime(&t);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tmp);
    return buffer;
}

