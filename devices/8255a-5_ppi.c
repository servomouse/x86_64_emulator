#include "8255a-5_ppi.h"
// #include "8086.h"
#include "utils.h"
#include "wires.h"
#include <string.h>

#define DEVICE_LOG_FILE "logs/8255a-5_ppi.log"
#define DEVICE_DATA_FILE "data/8255a-5_ppi.bin"
#define START_ADDR (uint32_t)0x060
#define END_ADDR (uint32_t)0x063

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

__declspec(dllexport)
uint32_t *module_get_address_range(void) {
    static uint32_t addresses[] = {START_ADDR, END_ADDR};
    return addresses;
}

void dummy_cb(wire_state_t new_state) {
    return;
}

__declspec(dllexport)   // Keyboard interrupt
wire_t int1_wire = WIRE_T(WIRE_OUTPUT_PP, &dummy_cb);

void update_portc(void) {
    regs.portc_reg = 0;
    if((regs.portb_reg & 0x08) > 0) {
        regs.portc_reg |= SW1;
        regs.portc_reg |= SW2 << 1;
        regs.portc_reg |= SW3 << 2;
        regs.portc_reg |= SW4 << 3;
    } else {
        regs.portc_reg |= SW5;
        regs.portc_reg |= SW6 << 1;
        regs.portc_reg |= SW7 << 2;
        regs.portc_reg |= SW8 << 3;
    }
}

__declspec(dllexport)
void module_reset(void) {
    memset(&regs, 0, sizeof(device_regs_t));
    regs.porta_reg = 0xAA;
    regs.portb_reg = 0;
    regs.portc_reg = 0;
    update_portc();
}

__declspec(dllexport)
void data_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(DEVICE_LOG_FILE, "PPI_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    switch(addr) {
        case 0x60:
            // porta_reg = value;
            printf("BIOS STAGE: %d\n", value);
            break;
        case 0x61:
            if(((regs.portb_reg & 0x40) == 0) && ((value & 0x40) > 0)) {
                mylog(DEVICE_LOG_FILE, "PPI Setting Interrupt 2\n");
                module_reset();
                regs.delayed_int = 1;
                regs.delayed_int_ticks = 10;
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

__declspec(dllexport)
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
    mylog(DEVICE_LOG_FILE, "PPI_READ addr = 0x%04X, width = %d bytes, data = 0x%04X\n", addr, width, ret_val);
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
    if(regs.delayed_int == 1) {
        if(regs.delayed_int_ticks > 0) {
            regs.delayed_int_ticks --;
        } else {
            if(int1_wire.wire_get_state() == WIRE_LOW) {
                mylog(DEVICE_LOG_FILE, "PPI Triggering Interrupt 2\n");
                int1_wire.wire_set_state(WIRE_HIGH);
                regs.delayed_int_ticks = 20;
            } else {
                int1_wire.wire_set_state(WIRE_LOW);
                regs.delayed_int = 0;
            }
        }
    }
    return 0;
}