#include "FDC.h"
#include "utils.h"
#include "wires.h"
#include <string.h>

// p166

#define DEVICE_LOG_FILE "logs/fdc.log"
#define DEVICE_DATA_FILE "data/fdc.bin"

typedef struct {
    uint8_t DOR;    // Data output register
    uint8_t MSR;
    uint8_t data_reg;
    uint8_t delayed_int;
    uint16_t delayed_int_ticks;
} device_regs_t;

device_regs_t regs;
uint8_t error;

DLL_PREFIX
void module_reset(void) {
    memset(&regs, 0, sizeof(device_regs_t));
    regs.DOR = 0;
    regs.MSR = 0x80;
    regs.data_reg = 0;
}

DLL_PREFIX
void data_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(0, DEVICE_LOG_FILE, "FDC_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    if(addr == 0x3F2) {
        if(((regs.DOR & 0x04) == 0) && ((value & 0x04) == 0x04)) {
            regs.delayed_int = 1;
            regs.delayed_int_ticks = 10;
        }
        regs.DOR = value;
    } else if(addr == 0x3F4) {
        regs.MSR = value;
    } else if(addr == 0x3F5) {
        regs.data_reg = value;
        if(value == 0x08) {
            regs.MSR |= 0x40;
        }
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
        regs.MSR &= ~0x40;
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

// void dummy_cb(wire_state_t new_state) {
//     return;
// }

DLL_PREFIX   // Disk Controller interrupt
pin_t int6_pin = CREATE_PIN(PIN_OUTPUT_PP, &dummy_cb);
// wire_t int6_wire = WIRE_T(WIRE_OUTPUT_PP, &dummy_cb);

DLL_PREFIX
int module_tick(void) {
    if(regs.delayed_int == 1) {
        if(regs.delayed_int_ticks > 0) {
            regs.delayed_int_ticks --;
        } else {
            if(int6_pin.pin_get_state() == 0) {
                mylog(0, DEVICE_LOG_FILE, "FDC Triggering Interrupt 6\n");
                int6_pin.pin_set_state(1);
                regs.delayed_int_ticks = 20;
            } else {
                int6_pin.pin_set_state(0);
                regs.delayed_int = 0;
            }
        }
    }
    return error;
}