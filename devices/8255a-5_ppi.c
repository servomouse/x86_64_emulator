#include "8255a-5_ppi.h"
#include "utils.h"
#include "pins.h"
#include <string.h>

#define DEVICE_NAME         "PPI"
#define DEVICE_LOG_FILE     "logs/8255a-5_ppi.log"
#define DEVICE_DATA_FILE    "data/8255a-5_ppi.bin"

#define SW1 1
#define SW2 0
#define SW3 1
#define SW4 1
#define SW5 1
#define SW6 1
#define SW7 0
#define SW8 0

typedef struct {
    uint8_t porta_reg;
    uint8_t portb_reg;
    uint8_t portc_reg;
    uint8_t cmd_reg;
    uint8_t delayed_int;
    uint8_t delayed_int_ticks;
} device_regs_t;

device_regs_t regs;

size_t ticks_num = 0;

CREATE_PIN(int1_pin, PIN_OUTPUT_PP)   // Keyboard interrupt
CREATE_PIN(beep_pin, PIN_OUTPUT_PP)   // Keyboard interrupt

void update_portc(void) {
    regs.portc_reg = 0;
    if((regs.portb_reg & 0x08) == 0) {  // 4 LSBits
        regs.portc_reg |= SW1;
        regs.portc_reg |= SW2 << 1;
        regs.portc_reg |= SW3 << 2;
        regs.portc_reg |= SW4 << 3;
    } else {
        regs.portc_reg |= SW5;          // 4 MSBits
        regs.portc_reg |= SW6 << 1;
        regs.portc_reg |= SW7 << 2;
        regs.portc_reg |= SW8 << 3;
    }
}

DLL_PREFIX
void module_reset(void) {
    memset(&regs, 0, sizeof(device_regs_t));
    regs.porta_reg = 0xAA;
    regs.portb_reg = 0;
    regs.portc_reg = 0;
    update_portc();
}

DLL_PREFIX
void data_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(0, DEVICE_LOG_FILE, "PPI_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    switch(addr) {
        case 0x60:
            // porta_reg = value;
            printf("BIOS STAGE: %d\n", value);
            break;
        case 0x61:
            if(((regs.portb_reg & 0x40) == 0) && ((value & 0x40) > 0)) {
                mylog(0, DEVICE_LOG_FILE, "PPI Setting Interrupt 2\n");
                module_reset();
                regs.delayed_int = 1;
                regs.delayed_int_ticks = 10;
            }
            if((value & 0x03) == 0x03) {    // Turn beep signal on
                if(beep_pin.get_state() == 0)
                    beep_pin.set_state(1);
            } else  {                       // Turn beep signal off
                if(beep_pin.get_state() == 1)
                    beep_pin.set_state(0);
            }
            regs.portb_reg = value;
            update_portc();
            break;
        case 0x62:
            regs.portc_reg = value;
            break;
        case 0x63:
            regs.cmd_reg = value;
            break;
        default:
            printf("PPI ERROR: attempt to write to incorrect port 0x%04x\n", addr);
    }
}

DLL_PREFIX
uint16_t data_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    switch(addr) {
        case 0x60:
            ret_val = regs.porta_reg;
            regs.porta_reg = 0;
            break;
        case 0x61:
            ret_val = regs.portb_reg;
            break;
        case 0x62:
            ret_val = regs.portc_reg;
            break;
        case 0x63:
            ret_val = regs.cmd_reg;
            break;
        default:
            printf("PPI ERROR: attempt to read from incorrect port 0x%04x\n", addr);
    }
    mylog(0, DEVICE_LOG_FILE, "PPI_READ addr = 0x%04X, width = %d bytes, data = 0x%04X\n", addr, width, ret_val);
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
    if(regs.delayed_int == 1) {
        if(regs.delayed_int_ticks > 0) {
            regs.delayed_int_ticks --;
        } else {
            if(int1_pin.get_state() == 0) {
                mylog(0, DEVICE_LOG_FILE, "PPI Triggering Interrupt 2\n");
                int1_pin.set_state(1);
                regs.delayed_int_ticks = 20;
            } else {
                int1_pin.set_state(0);
                regs.delayed_int = 0;
            }
        }
    }
    return 0;
}