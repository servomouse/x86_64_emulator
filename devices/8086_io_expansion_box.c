#include "8086_io_expansion_box.h"
#include "utils.h"
#include <string.h>

#define DEVICE_NAME "IO_Expansion_box"
#define DEVICE_LOG_FILE "logs/io_expansion_box.log"
#define DEVICE_DATA_FILE "data/io_expansion_box.bin"

__declspec(dllexport)
void module_reset(void) {
    return;
}

__declspec(dllexport)
void data_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(DEVICE_LOG_FILE, "%s_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", DEVICE_NAME, addr, value, width);
}

__declspec(dllexport)
uint16_t data_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0xFFFF;
    mylog(DEVICE_LOG_FILE, "%s_READ addr = 0x%06X, width = %d bytes, data = 0x%04X\n", DEVICE_NAME, addr, width, ret_val);
    return ret_val;
}

__declspec(dllexport)
void module_save(void) {
    return;
}

__declspec(dllexport)
void module_restore(void) {
    return;
}

__declspec(dllexport)
int module_tick(void) {
    return 0;
}
