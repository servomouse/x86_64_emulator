#include "8259a_interrupt_controller.h"
#include "wires.h"
#include "utils.h"
#include <string.h>

#define DEVICE_LOG_FILE "logs/8259a_interrupt_controller.log"
#define DEVICE_DATA_FILE "data/8259a_interrupt_controller.bin"

// check 0xFF2E

typedef struct {
    uint8_t ISR;    // In-Service Register
    uint8_t IRR;    // Interrupt Request Register
    uint8_t IMR;    // Interrupt Mask Register
    uint8_t ICW1;   // Initialization Command Word 1
    uint8_t ICW2;   // Initialization Command Word 2
    uint8_t ICW3;   // Initialization Command Word 3
    uint8_t ICW4;   // Initialization Command Word 4
    uint8_t OCW1;   // Operation Command Word 1
    uint8_t OCW2;   // Operation Command Word 1
    uint8_t OCW3;   // Operation Command Word 1
    uint8_t int_register;
    uint8_t triggerded_int;
    uint8_t reg20;
    uint8_t enabled_ints;
} device_regs_t;

device_regs_t regs;

__declspec(dllexport)
void module_reset(void) {
    memset(&regs, 0, sizeof(device_regs_t));
}

void dummy_cb(wire_state_t new_state) {
    return;
}

__declspec(dllexport)
wire_t nmi_wire = WIRE_T(WIRE_OUTPUT_PP, &dummy_cb);

__declspec(dllexport)
wire_t int_wire = WIRE_T(WIRE_OUTPUT_PP, &dummy_cb);

void trigger_interrupt(uint8_t int_num, uint8_t active) {
    if(active == 0) {
        regs.triggerded_int &= ~int_num;
        regs.ISR &= ~int_num;
        regs.IRR &= ~int_num;
        int_wire.wire_set_state(WIRE_LOW);
    } else {
        regs.triggerded_int = int_num;
        regs.IRR = int_num;
        if((regs.IMR & int_num) == 0) {
            regs.ISR = int_num;
            int_wire.wire_set_state(WIRE_HIGH);
        }
    }
}

void int0_cb(wire_state_t new_state) {
    uint8_t int_num = 1 << 0;
    if(new_state == WIRE_LOW) {
        trigger_interrupt(int_num, 0);
    } else {
        trigger_interrupt(int_num, 1);
    }
}

void int1_cb(wire_state_t new_state) {
    uint8_t int_num = 1 << 1;
    if(new_state == WIRE_LOW) {
        trigger_interrupt(int_num, 0);
    } else {
        trigger_interrupt(int_num, 1);
    }
}

__declspec(dllexport)   // Timer interrupt
wire_t int0_wire = WIRE_T(WIRE_INPUT, &int0_cb);

__declspec(dllexport)   // Keyboard interrupt
wire_t int1_wire = WIRE_T(WIRE_INPUT, &int1_cb);

void write_byte(uint8_t addr, uint8_t data) {
    static int state;
    if((addr == 0x20) && ((data & 0x10) > 0)) { // ICW1
        regs.ICW1 = data;
        state = 1;  // Waiting for ICW2
    } else if(state == 1) { // ICW2
        regs.ICW2 = data;
        if(regs.ICW1 & 0x01) {
            state = 3;  // Wait for ICW4
        } else {
            state = 2;  // Wait for ICW3
        }
    } else if(state == 2) {     // ICW3
        printf("INTERRUPT CONTROLLER ERROR: Write ICW3 is not implemented!\n");   // Not implemented
        state = 0;
    } else if(state == 3) { // ICW4
        regs.ICW4 = data;
        state = 4;  // Initialization complete, go to the normal mode
    } else if(state == 4) {
        if(addr == 0x21) {
            regs.OCW1 = data;
            regs.IMR = data;
        } else {
            if((data & 0x18) == 0) {
                regs.OCW2 = data;
            } else if((data & 0x18) == 0x08) {
                regs.OCW3 = data;
            }
        }
    }
    
}

__declspec(dllexport)
void data_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(DEVICE_LOG_FILE, "INT_CONTROLLER_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    // regs.int_register = value;
    if(addr == 0xA0) {
        regs.triggerded_int = value;
    } else if((addr == 0x20) || (addr == 0x21) ){
        write_byte(addr & 0xFF, value);
    }
}

__declspec(dllexport)
uint16_t data_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    if(addr == 0xA0) {
        ret_val = regs.triggerded_int;
        if(int_wire.wire_get_state() == WIRE_HIGH) {
            int_wire.wire_set_state(WIRE_LOW);
        }
    } else if(addr == 0x20) {
        if(regs.OCW3 & 0x01) {
            ret_val = regs.IRR;
        } else {
            ret_val = regs.ISR;
        }
    } else if(addr == 0x21) {
        ret_val = regs.IMR;
    }
    mylog(DEVICE_LOG_FILE, "INT_CONTROLLER_READ addr = 0x%04X, width = %d bytes, data = 0x%04X\n", addr, width, ret_val);
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
