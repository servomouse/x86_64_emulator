#include "8237a-5_dma.h"
#include "wires.h"
#include "utils.h"
#include <string.h>

#define DEVICE_LOG_FILE "logs/8237a-5_dma.log"
#define DEVICE_DATA_FILE "data/8237a-5_dma.bin"
#define START_ADDR (uint32_t)0x080
#define END_ADDR (uint32_t)0x083

typedef struct {
    uint8_t reg0;
    uint8_t reg1;
    uint8_t reg2;
    uint8_t reg3;
    uint8_t registers[16];
} device_regs_t;

device_regs_t regs;

__declspec(dllexport)
uint32_t *module_get_address_range(void) {
    static uint32_t addresses[] = {START_ADDR, END_ADDR};
    return addresses;
}

__declspec(dllexport)
void module_reset(void) {
    regs.reg0 = 0;
    regs.reg1 = 0;
    regs.reg2 = 0;
    regs.reg3 = 0;
}

__declspec(dllexport)
void data_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(DEVICE_LOG_FILE, "INT_CONTROLLER_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    switch(addr) {
        case 0x80:
            regs.reg0 = value;
            break;
        case 0x81:
            regs.reg1 = value;
            break;
        case 0x82:
            regs.reg2 = value;
            break;
        case 0x83:
            regs.reg3 = value;
            break;
        default:
            regs.registers[addr] = value;
            // printf("DMA ERROR: incorrect address: 0x%08X\n", addr);
    }
}

__declspec(dllexport)
uint16_t data_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    switch(addr) {
        case 0x80:
            ret_val = regs.reg0;
            break;
        case 0x81:
            ret_val = regs.reg1;
            break;
        case 0x82:
            ret_val = regs.reg2;
            break;
        case 0x83:
            ret_val = regs.reg3;
            break;
        case 0x08:
            ret_val = 1;
            break;
        default:
            // printf("DMA ERROR: incorrect address: 0x%08X\n", addr);
            ret_val = regs.registers[addr];
    }
    mylog(DEVICE_LOG_FILE, "INT_CONTROLLER_READ addr = 0x%08X, width = %d bytes, data = 0x%04X\n", addr, width, ret_val);
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
    return 0;
}

