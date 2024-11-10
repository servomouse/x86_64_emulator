#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
// #include "utils.h"
#include "devices/8086_cpu.h"
// #include "8086_io.h"
// #include "devices/8259a_interrupt_controller.h"

// #include "log_server_iface/logs_win.h"

void test_mem_write(uint32_t addr, uint16_t value, uint8_t width) {
    printf("TEST_MEM_WRITE: addr = 0x%06X, value: 0x%04X, width: %d\n", addr, value, width);
}

uint16_t test_mem_read(uint32_t addr, uint8_t width) {
    printf("TEST_MEM_READ: addr = 0x%06X, width: %d\n", addr, width);
    return 0x1234;
}

int main(int argc, char *argv[]) {
    uint8_t data[6] = {0xFF, 0xA4, 0x45, 0xF0, 0xFF, 0xFF};
    module_reset();
    connect_address_space(0, &test_mem_write, &test_mem_read);
    connect_address_space(1, &test_mem_write, &test_mem_read);
    operands_t operands = decode_operands(data[0], &(data[1]), 0);
    printf("Instruction 0xFF: operands.src_val = 0x%04X\n", operands.src_val);
    printf("Instruction 0xFF: operands.src_addr = 0x%06X\n", operands.src.address);
    printf("Instruction 0xFF: operands.src_type = %d\n", operands.src_type);
    return 0;
}
