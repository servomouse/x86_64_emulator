#include "8259a_interrupt_controller.h"
#include "wires.h"
#include "utils.h"

#define DEVICE_LOG_FILE "logs/8259a_interrupt_controller.log"
#define DEVICE_DATA_FILE "data/8259a_interrupt_controller.bin"
#define START_ADDR (uint32_t)0x0A0
#define END_ADDR (uint32_t)0x0AF

typedef struct {
    uint8_t int_register;
    uint8_t triggerded_int;
    uint8_t reg20;
    uint8_t reg21;
} device_regs_t;

device_regs_t regs;

__declspec(dllexport)
uint32_t *module_get_address_range(void) {
    static uint32_t addresses[] = {START_ADDR, END_ADDR};
    return addresses;
}

__declspec(dllexport)
void module_reset(void) {
    regs.int_register = 0;
}

__declspec(dllexport)
void data_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(DEVICE_LOG_FILE, "INT_CONTROLLER_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    regs.int_register = value;
    if(addr == 0xA0) {
        regs.triggerded_int = value;
    } else if(addr == 0x20) {
        regs.reg20 = value;
    } else if(addr == 0x21) {
        regs.reg21 = value;
    }
}

__declspec(dllexport)
uint16_t data_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    if(addr == 0xA0) {
        ret_val = regs.triggerded_int;
    } else if(addr == 0x20) {
        ret_val = regs.reg20;
    } else if(addr == 0x21) {
        ret_val = regs.reg21;
    }
    mylog(DEVICE_LOG_FILE, "INT_CONTROLLER_READ addr = 0x%04X, width = %d bytes, data = 0x%04X\n", addr, width, ret_val);
    return ret_val;
}

__declspec(dllexport)
void module_save(void) {
    device_regs_t data = {
        .int_register = regs.int_register,
    };
    store_data(&data, sizeof(device_regs_t), DEVICE_DATA_FILE);
}

__declspec(dllexport)
void module_restore(void) {
    device_regs_t data;
    if(EXIT_SUCCESS == restore_data(&data, sizeof(device_regs_t), DEVICE_DATA_FILE)) {
        regs.int_register = data.int_register;
    }
}

__declspec(dllexport)
int module_tick(void) {
    return 0;
}

void dummy_cb(wire_state_t new_state) {
    return;
}

__declspec(dllexport)
wire_t nmi_wire = WIRE_T(WIRE_OUTPUT_PP, &dummy_cb);

__declspec(dllexport)
wire_t int_wire = WIRE_T(WIRE_OUTPUT_PP, &dummy_cb);

void int0_cb(wire_state_t new_state) {
    if(new_state == WIRE_LOW) {
        regs.triggerded_int = 0xFF;
        int_wire.wire_set_state(WIRE_LOW);
    } else {
        regs.triggerded_int = 0x00;
        int_wire.wire_set_state(WIRE_HIGH);
    }
}

void int1_cb(wire_state_t new_state) {
    if(new_state == WIRE_LOW) {
        regs.triggerded_int = 0xFF;
        int_wire.wire_set_state(WIRE_LOW);
    } else {
        regs.triggerded_int = 1;
        int_wire.wire_set_state(WIRE_HIGH);
    }
}

__declspec(dllexport)   // Timer interrupt
wire_t int0_wire = WIRE_T(WIRE_INPUT, &int0_cb);

__declspec(dllexport)   // Keyboard interrupt
wire_t int1_wire = WIRE_T(WIRE_INPUT, &int1_cb);
