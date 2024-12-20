#include "8086_cga.h"
#include "utils.h"
#include <string.h>

#define DEVICE_NAME         "CGA"
#define DEVICE_LOG_FILE     "logs/cga.log"
#define DEVICE_DATA_FILE    "data/cga.bin"

typedef struct {    // | Type                       | I/O | 40x25 | 80x25 | Graphic Modes
    uint8_t R0;     // | Horizontal total           | WO  | 0x38  | 0x71  | 0x38
    uint8_t R1;     // | Horizontal displayed       | WO  | 0x28  | 0x50  | 0x28
    uint8_t R2;     // | Horizontal sync position   | WO  | 0x2D  | 0x5A  | 0x2D
    uint8_t R3;     // | Horizontal sync width      | WO  | 0x0A  | 0x0A  | 0x0A
    uint8_t R4;     // | Vertical total             | WO  | 0x1F  | 0x1F  | 0x7F
    uint8_t R5;     // | Vertical total adjust      | WO  | 0x06  | 0x06  | 0x06
    uint8_t R6;     // | Vertical displayed         | WO  | 0x19  | 0x19  | 0x64
    uint8_t R7;     // | Vertical sync position     | WO  | 0x1C  | 0x1C  | 0x70
    uint8_t R8;     // | Interplace mode            | WO  | 0x02  | 0x02  | 0x02
    uint8_t R9;     // | Maximum scan line address  | WO  | 0x07  | 0x07  | 0x01
    uint8_t R10;    // | Cursor start               | WO  | 0x06  | 0x06  | 0x06
    uint8_t R11;    // | Cursor end                 | WO  | 0x07  | 0x07  | 0x07
    uint8_t R12;    // | Start Address H            | WO  | 0x00  | 0x00  | 0x00
    uint8_t R13;    // | Start Address L            | WO  | 0x00  | 0x00  | 0x00
    uint8_t R14;    // | Cursor Address H           | R/W | 0xXX  | 0xXX  | 0xXX
    uint8_t R15;    // | Cursor Address L           | R/W | 0xXX  | 0xXX  | 0xXX
    uint8_t R16;    // | Light Pen H                | RO  | 0xXX  | 0xXX  | 0xXX
    uint8_t R17;    // | Light Pen L                | RO  | 0xXX  | 0xXX  | 0xXX
    uint8_t status_register;
    uint8_t mode_control_register;
} device_regs_t;

device_regs_t regs;

size_t ticks_num = 0;

DLL_PREFIX
void module_reset(void) {
    memset(&regs, 0, sizeof(device_regs_t));
    regs.status_register = 0x09;
}

DLL_PREFIX
void data_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(0, DEVICE_LOG_FILE, "CGA_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    switch(addr) {
        case 0x3D8:
            regs.mode_control_register = value;
            break;
        case 0x3DA:
            regs.status_register = value;
            break;
    }
}

DLL_PREFIX
uint16_t data_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    switch(addr) {
        case 0x3D8:
            ret_val = regs.mode_control_register;
            break;
        case 0x3DA:
            ret_val = regs.status_register;
            break;
    }
    mylog(0, DEVICE_LOG_FILE, "CGA_READ addr = 0x%06X, width = %d bytes, data = 0x%04X\n", addr, width, ret_val);
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

size_t counter = 0;

DLL_PREFIX
int module_tick(uint32_t ticks) {
    if(counter++ == 20) {
        regs.status_register ^= 0x09;
        counter = 0;
    }
    return 0;
}
