#include "utils.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdarg.h>
#include <time.h>
// #include "log_server_iface/logs_win.h"

#define BUFFER_SIZE 1024

typedef void(*log_func_t)(const char*, char*);
log_func_t print_log;

__declspec(dllexport)
void set_log_func(log_func_t python_log_func) {
    print_log = python_log_func;
}

void mylog(const char *log_file, const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[BUFFER_SIZE] = {0};
    vsnprintf(buffer, sizeof(buffer), format, args);
    // vprintf(format, args);
    // printf("Processing string %s\n", buffer);
    // log_server_send(log_file, buffer);
    print_log(log_file, buffer);
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

int store_data(void *data, size_t size, char *filename) {
    uint64_t hash = get_hash((uint8_t*)data, size);
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        printf("ERROR: Failed to open file %s\n", filename);
        return EXIT_FAILURE;
    }
    fwrite((uint8_t *)data, 1, size, file);
    fwrite(&hash, sizeof(hash), 1, file);
    fclose(file);
    return EXIT_SUCCESS;
}

int restore_data(void *data, size_t size, char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("ERROR: Failed to open file %s\n", filename);
        return EXIT_FAILURE;
    }
    size_t len_data = fread((uint8_t *)data, 1, size, file);
    uint64_t hash;
    fread(&hash, sizeof(hash), 1, file);
    if(hash != get_hash((uint8_t*)data, size)) {
        printf("ERROR: File %s corrupted (hash check failed)\n", filename);
        return EXIT_FAILURE;
    }
    fclose(file);
    if(len_data != size) {
        printf("ERROR: Failed reading file %s (%lld of %lld bytes were read)\n", filename, len_data, size);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

uint64_t get_hash(uint8_t *data, size_t size) {
    uint64_t hash = 5381;
    for(size_t i=0; i<size; i++) {
        hash = ((hash << 5) + hash) + data[i];
    }
    return hash;
}

