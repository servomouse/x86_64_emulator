#include "FDC.h"
#include "utils.h"
#include "wires.h"
#include <string.h>

#define DEVICE_LOG_FILE "logs/fdc.log"
#define DEVICE_DATA_FILE "data/fdc.bin"

typedef struct {
    uint8_t DOR;    // Data output register
    uint8_t MSR;
    uint8_t data_reg;
} device_regs_t;

device_regs_t regs;
uint8_t error;

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
    mylog(0, DEVICE_LOG_FILE, "FDC_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    if(addr == 0x3F2) {
        regs.DOR = value;
    } else if(addr == 0x3F4) {
        regs.MSR = value;
    } else if(addr == 0x3F5) {
        regs.data_reg = value;
    } else {
        printf("FDC ERROR: Incorrect address: 0x%04X", addr);
        error = 1;
    }
}

DLL_PREFIX
uint16_t data_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    if(addr == 0x3F2) {
        ret_val = regs.DOR;
    } else if(addr == 0x3F4) {
        ret_val = regs.MSR;
    } else if(addr == 0x3F5) {
        ret_val = regs.data_reg;
    } else {
        printf("FDC ERROR: Incorrect address: 0x%04X", addr);
        error = 1;
    }
    mylog(0, DEVICE_LOG_FILE, "FDC_READ addr = 0x%04X, width = %d bytes, data = 0x%04X\n", addr, width, ret_val);
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
    return error;
}