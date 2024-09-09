#include "8086_io.h"

uint8_t *IO_SPACE = NULL;

void io_write(uint32_t addr, uint16_t value, uint8_t width) {
    printf("IO_WRITE addr = 0x%04X, value = 0x%04X, width = %d bytes\n", addr, value, width);
}

uint16_t io_read(uint32_t addr, uint8_t width) {
    printf("IO_READ addr = 0x%04X, width = %d bytes\n", addr, width);
    return 0;
}

int io_init(void) {
    IO_SPACE = (uint8_t*)calloc(sizeof(uint8_t), 0x100000);
    if(IO_SPACE == NULL) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}