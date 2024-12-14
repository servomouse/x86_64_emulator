#include "8253_timer.h"
#include "utils.h"
#include "pins.h"
#include <string.h>

#define DEVICE_NAME         "TIMER"
#define DEVICE_LOG_FILE     "logs/timer.log"
#define DEVICE_DATA_FILE    "data/8253_timer.bin"

typedef struct {
    uint8_t read_load;
    uint8_t read_load_counter;
    uint8_t mode;
    uint8_t bcd;
    uint16_t value;
    uint16_t counter;
    pin_t* output;
    uint8_t counts;
} timer_t;

typedef struct {
    timer_t timer[3];
} device_regs_t;

device_regs_t regs;

size_t ticks_num = 0;

void set_timer_state(timer_t * timer, uint8_t gate_state) {
    if(gate_state == 0) {
        timer->counts = 0;
    } else {
        timer->counts = 1;
        if(timer->value > 0) {
            timer->counter = timer->value;
        }
    }
}

void gate0_cb(uint8_t new_state) {
    set_timer_state(&(regs.timer[0]), new_state);
}

void gate1_cb(uint8_t new_state) {
    set_timer_state(&(regs.timer[1]), new_state);
}

void gate2_cb(uint8_t new_state) {
    set_timer_state(&(regs.timer[2]), new_state);
}

CREATE_PIN(ch0_output_pin, PIN_OUTPUT_PP)
CREATE_PIN(ch1_output_pin, PIN_OUTPUT_PP)
CREATE_PIN(ch2_output_pin, PIN_OUTPUT_PP)

CREATE_PIN(ch0_gate_pin, PIN_INPUT, &gate0_cb)
CREATE_PIN(ch1_gate_pin, PIN_INPUT, &gate1_cb)
CREATE_PIN(ch2_gate_pin, PIN_INPUT, &gate2_cb)

__declspec(dllexport)
void module_save(void) {
    // regs.timer[0].output = NULL;
    // regs.timer[1].output = NULL;
    // regs.timer[2].output = NULL;
    store_data(&regs, sizeof(device_regs_t), DEVICE_DATA_FILE);
}

__declspec(dllexport)
void module_restore(void) {
    device_regs_t data;
    if(EXIT_SUCCESS == restore_data(&data, sizeof(device_regs_t), DEVICE_DATA_FILE)) {
        memcpy(&regs, &data, sizeof(device_regs_t));
    }
    regs.timer[0].output = &ch0_output_pin;
    regs.timer[1].output = &ch1_output_pin;
    regs.timer[2].output = &ch2_output_pin;
}

__declspec(dllexport)
void module_reset(void) {
    for(uint8_t i=0; i<3; i++) {
    regs.timer[i].read_load = 0;
    regs.timer[i].read_load_counter = 0;
    regs.timer[i].mode = 0;
    regs.timer[i].bcd = 0;
    regs.timer[i].value = 0;
    regs.timer[i].counter = 0;
    regs.timer[i].counts = 0;
    }
    regs.timer[0].output = &ch0_output_pin;
    regs.timer[1].output = &ch1_output_pin;
    regs.timer[2].output = &ch2_output_pin;
}

static void set_value(timer_t *timer, uint16_t value) {
    uint8_t lsb = timer->value & 0xFF;
    uint8_t msb = timer->value >> 8;
    if(timer->read_load == 0) {         // Latch value
        ;
    } else if(timer->read_load == 1) {  // Read/Load MSByte only
        lsb = value & 0xFF;
    } else if(timer->read_load == 2) {  // Read/Load LSByte only
        msb = value & 0xFF;
    } else {                            // Read/Load LSByte first, then MSByte
        if(timer->read_load_counter == 0) {
            lsb = value & 0xFF;
            timer->read_load_counter = 1;
            if(timer->mode == 0) {
                timer->counts = 0;
            }
        } else {
            msb = value & 0xFF;
            timer->read_load_counter = 0;
            if((timer->mode == 0) || (timer->mode == 4)) {
                timer->counts = 1;
            }
        }
    }
    timer->value = lsb + ((uint16_t)msb << 8);
    if((timer->mode == 0) || (timer->mode == 4)) {
        timer->counter = timer->value;
    }
    mylog(0, DEVICE_LOG_FILE, "Set timer value to 0x%04X, counter = 0x%04X, load-read = %d\n", timer->value, timer->counter, timer->read_load);
}

static uint8_t get_value(timer_t *timer) {
    uint8_t lsb = timer->counter & 0xFF;
    uint8_t msb = timer->counter >> 8;
    uint8_t ret_val = 0;
    if(timer->read_load == 0) {         // Latch value
        ret_val = lsb;
    } else if(timer->read_load == 1) {  // Read/Load MSByte only
        ret_val = msb;
    } else if(timer->read_load == 2) {  // Read/Load LSByte only
        ret_val = lsb;
    } else {                            // Read/Load LSByte first, then MSByte
        if(timer->read_load_counter == 0) {
            ret_val = lsb;
            timer->read_load_counter = 1;
        } else {
            ret_val = msb;
            timer->read_load_counter = 0;
        }
    }
    return ret_val;
}

__declspec(dllexport)
void data_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(0, DEVICE_LOG_FILE, "TIMER_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    switch(addr) {
        case 0x40:
            set_value(&regs.timer[0], value);
            break;
        case 0x41:
            set_value(&regs.timer[1], value);
            break;
        case 0x42:
            set_value(&regs.timer[2], value);
            break;
        case 0x43: {
            uint8_t timer_idx = (value & 0xFF) >> 6;
            uint8_t mode = (value >> 1) & 0x07;
            uint8_t read_load = (value >> 4) & 0x03;
            uint8_t bcd = value & 0x01;
            if(timer_idx < 3) {
                mylog(0, DEVICE_LOG_FILE, "Set timer %d mode to 0x%02X\n", timer_idx, mode);
                regs.timer[timer_idx].mode = mode;
                regs.timer[timer_idx].read_load = read_load;
                regs.timer[timer_idx].bcd = bcd;
                if((mode == 0) || (mode == 4)) {
                    regs.timer[timer_idx].counter --;
                    regs.timer[timer_idx].counts = 1;
                }
            } else {
                printf("TIMER ERROR: attempt to write to incorrect counter 0x%02X\n", timer_idx);
            }
            break;
        }
        default:
            printf("TIMER ERROR: attempt to write to incorrect port 0x%04X\n", addr);
    }
}

__declspec(dllexport)
uint16_t data_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    switch(addr) {
        case 0x40:
            ret_val = get_value(&(regs.timer[0]));
            break;
        case 0x41:
            ret_val = get_value(&(regs.timer[1]));
            break;
        case 0x42:
            ret_val = get_value(&(regs.timer[2]));
            break;
        case 0x43:
            ret_val = 0xFF;
            break;
        default:
            printf("TIMER ERROR: attempt to read from incorrect port 0x%04x\n", addr);
    }
    mylog(0, DEVICE_LOG_FILE, "TIMER_READ addr = 0x%04X, width = %d bytes, data = 0x%04X\n", addr, width, ret_val);
    return ret_val;
}

void timer_tick(timer_t *timer) {
    if(timer->counts == 0) {
        return;
    }
    switch(timer->mode) {
        case 0:     // Interrupt on terminal count (one-shot)
        case 1: {   // Programmable one-shot (retriggerable)
            if(timer->counter == 0) {
                if(timer->output->get_state() == 0) {
                    timer->output->set_state(1);
                }
                if(timer->mode == 0) {
                    timer->value = 0;
                    timer->counter = 0xFFFF;
                } else {
                    timer->counter = timer->value;
                    timer->counts = 0;
                }
            } else {
                if(timer->output->get_state() == 1) {
                    timer->output->set_state(0);
                }
                timer->counter --;
            }
            break;
        }
        case 2:     // Rate generator (auto reload)
        case 4:     // Software triggered strobe
        case 5:     // Hardware triggered strobe
        case 6: {   // Rate generator (auto reload)
            if(timer->counter == 0) {
                if(timer->output->get_state() == 0) {
                    timer->output->set_state(1);
                }
                if(timer->mode == 4) {
                    timer->value = 0;
                }
                timer->counter = timer->value;
                if((timer->mode == 4) || (timer->mode == 5)) {
                    timer->counts = 0;
                }
            } else {
                if(timer->counter == 1) {    // Low between 1 and 0, the rest of the time is high
                    if(timer->output->get_state() == 1) {
                        timer->output->set_state(0);
                    }
                } else {
                    if(timer->output->get_state() == 0) {
                        timer->output->set_state(1);
                    }
                }
                timer->counter --;
            }
            break;
        }
        case 3:     // Square wave generator
        case 7: {
            if(timer->counter == 0) {
                if(timer->output->get_state() == 0) {
                    timer->output->set_state(1);
                }
                timer->counter = timer->value;
            } else {
                if(timer->counter <= (timer->value >> 1)) {    // First half of the count stay high, then low
                    if(timer->output->get_state() == 1) {
                        timer->output->set_state(0);
                    }
                } else {
                    if(timer->output->get_state() == 0) {
                        timer->output->set_state(1);
                    }
                }
                timer->counter --;
            }
            break;
        }
    }
}

uint8_t tick_divider;

__declspec(dllexport)
int module_tick(uint32_t ticks) {
    if(tick_divider) {
        tick_divider = 0;
        return 0;
    }
    tick_divider ++;
    timer_tick(&(regs.timer[0]));
    timer_tick(&(regs.timer[1]));
    timer_tick(&(regs.timer[2]));
    return 0;
}
