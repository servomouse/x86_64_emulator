#include "8086_io.h"
#include "utils.h"

#define IO_LOG_FILE "logs/io_log.txt"

uint8_t *IO_SPACE = NULL;

void io_write(uint32_t addr, uint16_t value, uint8_t width) {
    printf("IO_WRITE addr = 0x%04X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    FILE *f;
    f = fopen(IO_LOG_FILE, "a");
    if(f == NULL) {
        printf("ERROR: Failed to open %s", IO_LOG_FILE);
        return;
    }
    fprintf(f, "Write to  addr: 0x%06X; value: 0x%04X; width: %d\n", addr, value, width);
    fclose(f);
}

uint8_t counter = 0;

uint16_t io_read(uint32_t addr, uint8_t width) {
    printf("IO_READ addr = 0x%04X, width = %d bytes\n", addr, width);
    uint16_t ret_val = 0;
    if(width == 1) {
        ret_val = 0xFF;
    } else if (width == 2) {
        ret_val = 0xFFFF;
    } else {
        printf("ERROR: Incorrect width while readiong from IO: %d", width);
        ret_val = 0xFFFF;
    }
    if(addr == 0x0041) {
        ret_val = counter++;
    }
    FILE *f;
    f = fopen(IO_LOG_FILE, "a");
    if(f == NULL) {
        printf("ERROR: Failed to open %s", IO_LOG_FILE);
        return ret_val;
    }
    fprintf(f, "Read from addr: 0x%06X; value: 0x%04X; width: %d\n", addr, ret_val, width);
    fclose(f);
    return ret_val;
}

int io_init(void) {
    IO_SPACE = (uint8_t*)calloc(sizeof(uint8_t), 0x100000);
    if(IO_SPACE == NULL) {
        return EXIT_FAILURE;
    }
    FILE *f;
    f = fopen(IO_LOG_FILE, "a");
    if(f == NULL) {
        printf("ERROR: Failed to open %s", IO_LOG_FILE);
        return EXIT_FAILURE;
    }
    fprintf(f, "%s Memory log start\n", get_time());
    fclose(f);
    return EXIT_SUCCESS;
}