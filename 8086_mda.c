#include "8086_mda.h"
#include "utils.h"

#define MDA_LOG_FILE "logs/mda.log"

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
} mda_regs_t;

mda_regs_t *mda_regs;

uint8_t status_register = 0x08;

uint8_t mda_init(void) {
    mda_regs = calloc(1, sizeof(mda_regs_t));
    return 0;
}

uint8_t mda_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(MDA_LOG_FILE, "MDA_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    return 0;
}

uint16_t mda_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    if(addr == 0x3DA) {
        ret_val = status_register;
    }
    mylog(MDA_LOG_FILE, "MDA_READ addr = 0x%04X, width = %d bytes, data = 0x%04X\n", addr, width, ret_val);
    return ret_val;
}
