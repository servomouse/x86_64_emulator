#include "draft_device.h"
#include "utils.h"
#include "pins.h"
#include <string.h>

// p229 and p542

#define DEVICE_NAME         "DRAFT"
#define DEVICE_LOG_FILE     "logs/draft_device.log"
#define DEVICE_DATA_FILE    "data/draft_device.bin"

/*
Note: Divisor Latch Access Bit (DLAB) is the Most Significant Bit of the Line Control Register

DLAB      Address    Register
   0   0x3F8 (0x2F8) RX buffer (read) / TX buffer (write)
   1   0x3F8 (0x2F8) Divisor Latch LSB
   1   0x3F9 (0x2F9) Divisor Latch MSB
   0   0x3F9 (0x2F9) Interrupt Enable register
   x   0x3FA (0x2FA) Interrupt Identification register
   x   0x3FB (0x2FB) Line Control Register
   x   0x3FC (0x2FC) Modem Control Register
   x   0x3FD (0x2FD) Line Status Register
   x   0x3FE (0x2FE) Modem Status Register
   x   0x3FF (0x2FF) Scratch
*/

typedef struct {
    uint8_t rx_buf;
    uint8_t tx_buf;
    uint16_t divisor_latch;
    uint8_t int_enable;
    uint8_t int_id;
    uint8_t line_control;
    uint8_t modem_control;
    uint8_t line_status;
    uint8_t modem_status;
    uint8_t scratch;
} device_regs_t;

device_regs_t regs;

size_t ticks_num = 0;

CREATE_PIN(test_wire, PIN_OUTPUT_PP)

DLL_PREFIX
void module_reset(void) {
    memset(&regs, 0, sizeof(device_regs_t));
}

DLL_PREFIX
void data_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(0, DEVICE_LOG_FILE, "%lld, %s_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", ticks_num, DEVICE_NAME, addr, value, width);
    switch(addr) {
        case 0x2F8:
        case 0x3F8: {
            if((regs.line_control & 0x80) == 0x80) {
                regs.divisor_latch &= 0xFF00;
                regs.divisor_latch |= value;
            } else {
                regs.tx_buf = value;
            }
            }
            break;
        case 0x2F9:
        case 0x3F9: {
            if((regs.line_control & 0x80) == 0x80) {
                regs.divisor_latch &= 0x00FF;
                regs.divisor_latch |= value << 8;
            } else {
                regs.int_enable = value;
            }
            }
            break;
        case 0x2FA:
        case 0x3FA: {
            regs.int_id = value;
            }
            break;
        case 0x2FB:
        case 0x3FB: {
            regs.line_control = value;
            }
            break;
        case 0x2FC:
        case 0x3FC: {
            regs.modem_control = value;
            }
            break;
        case 0x2FD:
        case 0x3FD: {
            regs.line_status = value;
            }
            break;
        case 0x2FE:
        case 0x3FE: {
            regs.modem_status = value;
            }
            break;
        case 0x2FF:
        case 0x3FF: {
            regs.scratch = value;
            }
            break;
        default:
            printf("%lld, %s ERROR: attempt to write to incorrect port 0x%04x\n", ticks_num, DEVICE_NAME, addr);
    }
}

DLL_PREFIX
uint16_t data_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0xFF;
    switch(addr) {
        case 0x2F8:
        case 0x3F8: {
            if((regs.line_control & 0x80) == 0x80) {
                ret_val = regs.divisor_latch & 0xFF;
            } else {
                ret_val = regs.rx_buf;
            }
            }
            break;
        case 0x2F9:
        case 0x3F9: {
            if((regs.line_control & 0x80) == 0x80) {
                ret_val = (regs.divisor_latch & 0x00FF) >> 8;
            } else {
                ret_val = regs.int_enable;
            }
            }
            break;
        case 0x2FA:
        case 0x3FA: {
            ret_val = regs.int_id;
            }
            break;
        case 0x2FB:
        case 0x3FB: {
            ret_val = regs.line_control;
            }
            break;
        case 0x2FC:
        case 0x3FC: {
            ret_val = regs.modem_control;
            }
            break;
        case 0x2FD:
        case 0x3FD: {
            ret_val = regs.line_status;
            }
            break;
        case 0x2FE:
        case 0x3FE: {
            ret_val = regs.modem_status;
            }
            break;
        case 0x2FF:
        case 0x3FF: {
            ret_val = regs.scratch;
            }
            break;
        default:
            printf("%lld, %s ERROR: attempt to write to incorrect port 0x%04x\n", ticks_num, DEVICE_NAME, addr);
    }
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