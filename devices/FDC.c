#include "FDC.h"
#include "pins.h"
#include <string.h>

// p166

#define DEVICE_NAME         "FDC"
#define DEVICE_LOG_FILE     "logs/fdc.log"
#define DEVICE_DATA_FILE    "data/fdc.bin"

typedef enum {
    READ_TRACK             = 0x02,  // 0  MF SK 0 0 0 1 0
    SPECIFY                = 0x03,  // 0  0  0  0 0 0 1 1
    SENSE_DRIVE_STATUS     = 0x04,  // 0  0  0  0 0 1 0 0
    WRITE_DATA             = 0x05,  // MT MF 0  0 0 1 0 1
    READ_DATA              = 0x06,  // MT MF SK 0 0 1 1 0
    RECALIBRATE            = 0x07,  // 0  0  0  0 0 1 1 1
    SENSE_INTERRUPT_STATUS = 0x08,  // 0  0  0  0 1 0 0 0
    WRITE_DELETED_DATA     = 0x09,  // MT MF 0  0 1 0 0 1
    READ_ID                = 0x0A,  // 0  MF 0  0 1 0 1 0
    READ_DELETED_DATA      = 0x0C,  // MT MF SK 0 1 1 0 0
    FORMAT_TRACK           = 0x0D,  // 0  MF 0  0 1 1 0 1
    SEEK                   = 0x0F,  // 0  0  0  0 1 1 1 1
    SCAN_EQUAL             = 0x11,  // MT MF SK 1 0 0 0 1
    SCAN_LOW_OR_EQUAL      = 0x19,  // MT MF SK 1 1 0 0 1
    SCAN_HIGH_OR_EQUAL     = 0x1D,  // MT MF SK 1 1 1 0 1
} commands_t;

typedef struct {
    uint8_t DOR;    // Data output register
    uint8_t MSR;
    uint8_t data_reg;
    uint8_t delayed_int;
    uint16_t delayed_int_ticks;
    uint8_t ST0;
    uint8_t ST1;
    uint8_t ST2;
    uint8_t PCN;
    commands_t command;
    uint8_t step;
    uint8_t wait_write;
    uint8_t mode;   // 1 for Non-DMA, 0 for DMA mode
} device_regs_t;

device_regs_t regs;
uint8_t error;
size_t ticks_num = 0;

void state_machine(uint8_t cmd, uint8_t is_write) {
    // static commands_t command;
    // static uint8_t step = 0;
    if(is_write && ((regs.wait_write == 0) || (regs.command == 0))) {
        regs.command = cmd;
        regs.step = 0;
    }
    switch(regs.command) {
        case SENSE_INTERRUPT_STATUS:
            mylog(0, DEVICE_LOG_FILE, "%lld, FDC INFO: SENSE_INTERRUPT_STATUS command\n", ticks_num);
            if(is_write) {
                regs.wait_write = 0;
                regs.step = 1;
                regs.MSR = 0xC0;    // Ready, Direction: output
                regs.data_reg = 0;
                regs.ST0 = 0xC0;
            } else if(regs.step == 1) {
                regs.step = 2;
                regs.data_reg = regs.ST0;
                regs.MSR &= ~0x40;
            } else if(regs.step == 2) {
                regs.command = 0;
                regs.step = 0;
                regs.data_reg = regs.PCN;
            }
            break;
        case SPECIFY:
            mylog(0, DEVICE_LOG_FILE, "%lld, FDC INFO: SPECIFY command\n", ticks_num);
            if(regs.step == 0) {
                regs.step = 1;
                regs.wait_write = 1;
                regs.MSR = 0x80;            // Ready, Direction: input
            } else if(regs.step == 1) {     // Set SRT (Step Rate Time) and HUT (Head Unload Time), ignore for emulation
                regs.step = 2;
                regs.wait_write = 1;
                regs.MSR = 0x80;            // Ready, Direction: input
            } else if(regs.step == 2) {     // Set HLT (Head Load Time) and ND (Non-DMA mode)
                if((cmd & 0x01) > 0) {
                    regs.mode = 1;          // Non-DMA mode
                } else {
                    regs.mode = 0;          // DMA mode
                }
                regs.step = 0;
                regs.wait_write = 0;
                regs.command = 0;
                regs.MSR = 0xC0;            // Ready, Direction: output
                mylog(0, DEVICE_LOG_FILE, "%lld, FDC INFO: SPECIFY command second byte: 0x%02X\n", ticks_num, cmd);
            }
            break;
        default:
            mylog(0, DEVICE_LOG_FILE, "%lld, FDC ERROR: Unknown command: 0x%02X\n", ticks_num, regs.command);
            printf("%lld, FDC ERROR: Unknown command: 0x%02X\n", ticks_num, regs.command);
    }
}

DLL_PREFIX
void module_reset(void) {
    memset(&regs, 0, sizeof(device_regs_t));
    regs.DOR = 0;
    regs.MSR = 0x80;
    regs.data_reg = 0;
}

DLL_PREFIX
void data_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(0, DEVICE_LOG_FILE, "%lld, FDC_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", ticks_num, addr, value, width);
    if(addr == 0x3F2) {
        if(((regs.DOR & 0x04) == 0) && ((value & 0x04) == 0x04)) {
            regs.delayed_int = 1;
            regs.delayed_int_ticks = 10;
        }
        regs.DOR = value;
    } else if(addr == 0x3F4) {
        printf("FDC ERROR: Attempt to write the Main Status Register which can only be read!\n");
    } else if(addr == 0x3F5) {
        state_machine(value, 1);
    } else {
        printf("FDC ERROR: Incorrect address: 0x%04X\n", addr);
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
        state_machine(0, 0);
        ret_val = regs.data_reg;
    } else {
        printf("FDC ERROR: Incorrect address: 0x%04X", addr);
        error = 1;
    }
    mylog(0, DEVICE_LOG_FILE, "%lld, FDC_READ addr = 0x%04X, width = %d bytes, data = 0x%04X\n", ticks_num, addr, width, ret_val);
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

CREATE_PIN(int6_pin, PIN_OUTPUT_PP)   // Disk Controller interrupt

DLL_PREFIX
int module_tick(uint32_t ticks) {
    ticks_num = ticks;
    if(regs.delayed_int == 1) {
        if(regs.delayed_int_ticks > 0) {
            regs.delayed_int_ticks --;
        } else {
            if(int6_pin.get_state() == 0) {
                mylog(0, DEVICE_LOG_FILE, "FDC Triggering Interrupt 6\n");
                int6_pin.set_state(1);
                regs.delayed_int_ticks = 20;
            } else {
                int6_pin.set_state(0);
                regs.delayed_int = 0;
            }
        }
    }
    return error;
}