#include "FDC.h"
#include "utils.h"
#include "wires.h"
#include <string.h>

#define DEVICE_LOG_FILE "logs/fdc.log"
#define DEVICE_DATA_FILE "data/fdc.bin"
#define START_ADDR (uint32_t)0x3F0
#define END_ADDR   (uint32_t)0x3F7

typedef struct {
    uint8_t DOR;    // Data output register
    uint8_t MSR;
    uint8_t data_reg;
} device_regs_t;

device_regs_t regs;

DLL_PREFIX
uint32_t *module_get_address_range(void) {
    static uint32_t addresses[] = {START_ADDR, END_ADDR};
    return addresses;
}

void dummy_cb(wire_state_t new_state) {
    return;
}

DLL_PREFIX   // Keyboard interrupt
wire_t test_wire = WIRE_T(WIRE_OUTPUT_PP, &dummy_cb);

DLL_PREFIX
void module_reset(void) {
    memset(&regs, 0, sizeof(device_regs_t));
    regs.DOR = 0;
    regs.MSR = 0;
    regs.data_reg = 0;
}

DLL_PREFIX
void data_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(DEVICE_LOG_FILE, "FDC_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
}

DLL_PREFIX
uint16_t data_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    mylog(DEVICE_LOG_FILE, "FDC_READ addr = 0x%04X, width = %d bytes, data = 0x%04X\n", addr, width, ret_val);
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
int module_tick(void) {
    return 0;
}