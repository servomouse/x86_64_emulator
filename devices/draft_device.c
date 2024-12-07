#include "draft_device.h"
#include "utils.h"
#include "wires.h"
#include <string.h>

#define DEVICE_LOG_FILE "logs/draft_device.log"
#define DEVICE_DATA_FILE "data/draft_device.bin"

typedef struct {
    uint8_t reg1;
    uint8_t reg2;
    uint8_t reg3;
} device_regs_t;

device_regs_t regs;

void dummy_cb(wire_state_t new_state) {
    return;
}

__declspec(dllexport)   // Keyboard interrupt
wire_t test_wire = WIRE_T(WIRE_OUTPUT_PP, &dummy_cb);

__declspec(dllexport)
void module_reset(void) {
    memset(&regs, 0, sizeof(device_regs_t));
    regs.reg1 = 0;
    regs.reg2 = 0;
    regs.reg3 = 0;
}

__declspec(dllexport)
void data_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(0, DEVICE_LOG_FILE, "DRAFT_DEVICE_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
}

__declspec(dllexport)
uint16_t data_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    mylog(0, DEVICE_LOG_FILE, "DRAFT_DEVICE_READ addr = 0x%04X, width = %d bytes, data = 0x%04X\n", addr, width, ret_val);
    return ret_val;
}

__declspec(dllexport)
void module_save(void) {
    store_data(&regs, sizeof(device_regs_t), DEVICE_DATA_FILE);
}

__declspec(dllexport)
void module_restore(void) {
    device_regs_t data;
    if(EXIT_SUCCESS == restore_data(&data, sizeof(device_regs_t), DEVICE_DATA_FILE)) {
        memcpy(&regs, &data, sizeof(device_regs_t));
    }
}

__declspec(dllexport)
int module_tick(void) {
    return 0;
}