#include "draft_device.h"
#include "utils.h"
#include "pins.h"
#include <string.h>

#define DEVICE_NAME         "PRINTER"
#define DEVICE_LOG_FILE     "logs/printer.log"
#define DEVICE_DATA_FILE    "data/printer.bin"

typedef struct {
    uint8_t reg1;
    uint8_t reg2;
    uint8_t reg3;
} device_regs_t;

device_regs_t regs;

size_t ticks_num = 0;

CREATE_PIN(test_wire, PIN_OUTPUT_PP)

DLL_PREFIX
void module_reset(void) {
    memset(&regs, 0, sizeof(device_regs_t));
    regs.reg1 = 0;
    regs.reg2 = 0;
    regs.reg3 = 0;
}

DLL_PREFIX
void data_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(0, DEVICE_LOG_FILE, "%lld, %s_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", ticks_num, DEVICE_NAME, addr, value, width);
}

DLL_PREFIX
uint16_t data_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0xFF;
    mylog(0, DEVICE_LOG_FILE, "%lld, %s_READ addr = 0x%04X, width = %d bytes, data = 0x%04X\n", ticks_num, DEVICE_NAME, addr, width, ret_val);
    return ret_val;
}

DLL_PREFIX
void module_save(void) {
    store_data(&regs, sizeof(device_regs_t), DEVICE_DATA_FILE);
}

DLL_PREFIX
void module_restore(void) {
    device_regs_t data;
    if(EXIT_SUCCESS == restore_data(&data, sizeof(device_regs_t), DEVICE_DATA_FILE)) {
        memcpy(&regs, &data, sizeof(device_regs_t));
    }
}

DLL_PREFIX
int module_tick(uint32_t ticks) {
    ticks_num = ticks;
    return 0;
}