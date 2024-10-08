#include "8086_ppi.h"
#include "utils.h"

#define PPI_LOG_FILE "logs/ppi.log"

#define SW1 1
#define SW2 0
#define SW3 1
#define SW4 1
#define SW5 1
#define SW6 1
#define SW7 0
#define SW8 0

uint8_t porta_reg;
uint8_t portb_reg;
uint8_t portc_reg;
uint8_t cmd_reg;

void update_portc(void) {
    portc_reg = 0;
    if((portb_reg & 0x08) > 0) {
        portc_reg |= SW1;
        portc_reg |= SW2 << 1;
        portc_reg |= SW3 << 2;
        portc_reg |= SW4 << 3;
    } else {
        portc_reg |= SW5;
        portc_reg |= SW6 << 1;
        portc_reg |= SW7 << 2;
        portc_reg |= SW8 << 3;
    }
}

uint8_t ppi_init(void) {
    porta_reg = 0;
    portb_reg = 0;
    portc_reg = 0;
    update_portc();
    return 0;
}

uint8_t ppi_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(PPI_LOG_FILE, "PPI_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    switch(addr) {
        case 0x60:
            porta_reg = value;
            printf("BIOS STAGE: %d\n", value);
            break;
        case 0x61:
            portb_reg = value;
            update_portc();
            break;
        case 0x62:
            portc_reg = value;
            break;
        case 0x63:
            cmd_reg = value;
            break;
        default:
            printf("PPI ERROR: attempt to write to incorrect port 0x%04x\n", addr);
    }
    return 0;
}

uint16_t ppi_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    switch(addr) {
        case 0x60:
            ret_val = porta_reg;
            break;
        case 0x61:
            ret_val = portb_reg;
            break;
        case 0x62:
            ret_val = portc_reg;
            break;
        case 0x63:
            ret_val = cmd_reg;
            break;
        default:
            printf("PPI ERROR: attempt to read from incorrect port 0x%04x\n", addr);
    }
    mylog(PPI_LOG_FILE, "PPI_READ addr = 0x%04X, width = %d bytes, data = 0x%04X\n", addr, width, ret_val);
    return ret_val;
}