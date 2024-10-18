#include "8259a_interrupt_controller.h"
#include "utils.h"

#define INT_CONTROLLER_LOG_FILE "logs/8259a_interrupt_controller.log"

uint8_t int_register;

uint8_t int_controller_reset(void) {
    int_register = 0;
    return 0;
}

void int_controller_write(uint32_t addr, uint16_t value, uint8_t width) {
    printf("INT_CONTROLLER_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    mylog(INT_CONTROLLER_LOG_FILE, "INT_CONTROLLER_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    int_register = value;
}

uint16_t int_controller_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    if(addr == 0xA0) {
        ret_val = int_register;
    }
    mylog(INT_CONTROLLER_LOG_FILE, "INT_CONTROLLER_READ addr = 0x%04X, width = %d bytes, data = 0x%04X\n", addr, width, ret_val);
    return ret_val;
}