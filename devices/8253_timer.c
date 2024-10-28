#include "8253_timer.h"
// #include "8086.h"
#include "utils.h"
#include "wires.h"
#include <string.h>

#define DEVICE_LOG_FILE "logs/timer.log"
#define DEVICE_DATA_FILE "data/8253_timer.bin"
#define START_ADDR (uint32_t)0x040
#define END_ADDR (uint32_t)0x043

typedef struct {
    uint8_t idx;
    uint16_t value;
    uint16_t inner_value;
    uint8_t mode;
    uint8_t read_counter;
    uint8_t write_counter;
    uint8_t is_counting;
    uint8_t gate;
    void (*output_cb)(uint8_t);
} counter_t;

typedef enum {
    TIMER_WRITE_EVENT = 0,
    TIMER_GATE_RISING,
    TIMER_GATE_FALLING,
} timer_event_t;


typedef struct {
    counter_t counter0;
    counter_t counter1;
    counter_t counter2;
    uint8_t int_time;
} device_regs_t;

device_regs_t regs;

__declspec(dllexport)
uint32_t *module_get_address_range(void) {
    static uint32_t addresses[] = {START_ADDR, END_ADDR};
    return addresses;
}

static void process_event(counter_t *timer, timer_event_t event) {
    uint8_t mode = (timer->mode >> 1) & 0x07;
    mylog(DEVICE_LOG_FILE, "Timer %d mode = %d\n", timer->idx, mode);
    if(event == TIMER_WRITE_EVENT) {
        if((mode != 1) && (mode != 5)) {
            mylog(DEVICE_LOG_FILE, "Timer %d start counting\n", timer->idx);
            timer->is_counting = 1;
        }
        if(mode == 0) {
            mylog(DEVICE_LOG_FILE, "Timer %d stop counting\n", timer->idx);
           (*(timer->output_cb))(0);
        }
    } else if(event == TIMER_GATE_FALLING) {
        if((mode == 0) || (mode == 1) || (mode == 4) || (mode == 5)) {
            mylog(DEVICE_LOG_FILE, "Timer %d stop counting\n", timer->idx);
            timer->is_counting = 0;
        }
    } else if(event == TIMER_GATE_RISING) {
        if((mode == 0) || (mode == 1) || (mode == 4) || (mode == 5)) {
            mylog(DEVICE_LOG_FILE, "Timer %d start counting\n", timer->idx);
            timer->is_counting = 1;
        }
    } else {
        printf("TIMER_EVENT ERROR: invalid event %d\n", event);
    }
}

static void dummy_cb(uint8_t a) {
    return;
}

void wire_dummy_cb(wire_state_t new_state) {
    return;
}

__declspec(dllexport)   // Timer interrupt
wire_t int0_wire = WIRE_T(WIRE_OUTPUT_PP, &wire_dummy_cb);

static void trigger_interrupt(uint8_t a) {
    int0_wire.wire_set_state(WIRE_LOW);
    regs.int_time = 10;
}

__declspec(dllexport)
void module_reset(void) {
    regs.int_time = 0;
    regs.counter0.idx = 0;
    regs.counter1.idx = 1;
    regs.counter2.idx = 2;
    regs.counter0.value = 0;
    regs.counter0.mode = 0;
    regs.counter0.read_counter = 0;
    regs.counter0.write_counter = 0;
    regs.counter0.is_counting = 0;
    regs.counter0.output_cb = &trigger_interrupt;
    regs.counter1.value = 0;
    regs.counter1.mode = 0;
    regs.counter1.read_counter = 0;
    regs.counter1.write_counter = 0;
    regs.counter1.is_counting = 0;
    regs.counter1.output_cb = &dummy_cb;
    regs.counter2.value = 0;
    regs.counter2.mode = 0;
    regs.counter2.read_counter = 0;
    regs.counter2.write_counter = 0;
    regs.counter2.is_counting = 0;
    regs.counter2.output_cb = &dummy_cb;
}

static void set_value(counter_t *timer, uint16_t value) {
    uint8_t submode = (timer->mode >> 4) & 0x03;
    // uint16_t ret_val = orig_value;
    if(submode == 1) {
        timer->value &= 0xFF00;
        timer->value |= value & 0xFF;
        timer->write_counter = 0;
        mylog(DEVICE_LOG_FILE, "Set timer %d value to 0x%04X\n", timer->idx, timer->value);
        process_event(timer, TIMER_WRITE_EVENT);
    } else if(submode == 2) {
        timer->value &= 0x00FF;
        timer->value |= value << 8;
        timer->write_counter = 0;
        mylog(DEVICE_LOG_FILE, "Set timer %d value to 0x%04X\n", timer->idx, timer->value);
        process_event(timer, TIMER_WRITE_EVENT);
    } else {
        if(timer->write_counter == 0) {
            timer->value &= 0xFF00;
            timer->value |= value & 0xFF;
            timer->write_counter += 1;
            mylog(DEVICE_LOG_FILE, "Set timer %d value to 0x%04X\n", timer->idx, timer->value);
        } else {
            timer->value &= 0x00FF;
            timer->value |= value << 8;
            timer->write_counter = 0;
            mylog(DEVICE_LOG_FILE, "Set timer %d value to 0x%04X\n", timer->idx, timer->value);
            process_event(timer, TIMER_WRITE_EVENT);
        }
    }
    timer->inner_value = timer->value;
}

__declspec(dllexport)
void data_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(DEVICE_LOG_FILE, "TIMER_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    switch(addr) {
        case 0x40:
            set_value(&regs.counter0, value);
            break;
        case 0x41:
            set_value(&regs.counter1, value);
            break;
        case 0x42:
            set_value(&regs.counter2, value);
            break;
        case 0x43: {
            uint8_t counter = (value & 0xFF) >> 6;
            if(counter == 0) {
                mylog(DEVICE_LOG_FILE, "Set timer 0 mode to 0x%02X\n", value);
                regs.counter0.mode = value;
            } else if(counter == 1) {
                mylog(DEVICE_LOG_FILE, "Set timer 1 mode to 0x%02X\n", value);
                regs.counter1.mode = value;
            } else if(counter == 2) {
                mylog(DEVICE_LOG_FILE, "Set timer 2 mode to 0x%02X\n", value);
                regs.counter2.mode = value;
            } else {
                printf("TIMER ERROR: attempt to write to incorrect counter 0x%02X\n", value);
            }
            break;
        }
        default:
            printf("TIMER ERROR: attempt to write to incorrect port 0x%04X\n", addr);
    }
}

static uint8_t get_value(uint16_t value, uint8_t mode, uint8_t *counter) {
    uint8_t submode = 0x30 & mode;
    uint8_t ret_val;
    if(submode == 1) {
        ret_val = value & 0xFF;
        *counter = 0;
    } else if(submode == 2) {
        ret_val = value >> 8;
        *counter = 0;
    } else {
        if(*counter == 0) {
            ret_val = value & 0xFF;
            *counter += 1;
        } else {
            ret_val = value >> 8;
            *counter = 0;
        }
    }
    return ret_val;
}

__declspec(dllexport)
uint16_t data_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    switch(addr) {
        case 0x40:
            ret_val = get_value(regs.counter0.inner_value, regs.counter0.mode, &regs.counter0.read_counter);
            break;
        case 0x41:
            ret_val = get_value(regs.counter1.inner_value, regs.counter1.mode, &regs.counter1.read_counter);
            break;
        case 0x42:
            ret_val = get_value(regs.counter2.inner_value, regs.counter2.mode, &regs.counter2.read_counter);
            break;
        case 0x43:
            ret_val = 0xFF;
            break;
        default:
            printf("TIMER ERROR: attempt to read from incorrect port 0x%04x\n", addr);
    }
    mylog(DEVICE_LOG_FILE, "TIMER_READ addr = 0x%04X, width = %d bytes, data = 0x%04X\n", addr, width, ret_val);
    return ret_val;
}

static uint16_t bcd_value(uint16_t value) {
    uint16_t ret_val = value;
    if ((ret_val & 0xF000) == 0xF000) {
        ret_val &= 0x9FFF;
    } else if ((ret_val & 0x0F00) == 0x0F00) {
        ret_val &= 0xF9FF;
    }  else if ((ret_val & 0x00F0) == 0x00F0) {
        ret_val &= 0xFF9F;
    } else if ((ret_val & 0x000F) == 0x000F) {
        ret_val &= 0xFFF9;
    }
    return ret_val;
}

static void counter_tick(counter_t *timer) {
    timer->inner_value -= 1;
    uint8_t mode = (timer->mode >> 1) & 0x07;
    uint8_t bcd_mode = timer->mode & 0x01;
    if(bcd_mode) {
        timer->inner_value = bcd_value(timer->inner_value);
    }
    if(timer->inner_value == 0) {
        if((mode == 0) || (mode == 1)) {
            (*(timer->output_cb))(0);
            timer->is_counting = 0;
        } else if((mode == 2) || (mode == 3) || (mode == 6) || (mode == 7)) {
            (*(timer->output_cb))(0);
        } else if((mode == 4) || (mode == 5)) {
            (*(timer->output_cb))(0);
        }
        timer->inner_value = timer->value;
    } else if(timer->inner_value == 1) {
        if((mode == 2) || (mode == 6)) {
            (*(timer->output_cb))(0);
        }
    } else if(((mode == 3) || (mode == 7)) && (timer->inner_value == ((timer->value) >> 1))) {
            (*(timer->output_cb))(0);
    } else if((timer->inner_value == (timer->value - 1)) && ((mode == 4) || (mode == 5))) {
        (*(timer->output_cb))(1);
    }
    // printf("Timer %d tick: inner_value = 0x%04X\n", timer->idx, timer->inner_value);
}

// __declspec(dllexport)
// void module_tick(void) {
//     // printf("Timer tick function");
//     if(regs.counter0.is_counting == 1) {
//         counter_tick(&regs.counter0);
//     }
//     if(regs.counter1.is_counting == 1) {
//         counter_tick(&regs.counter1);
//     }
//     if(regs.counter2.is_counting == 1) {
//         counter_tick(&regs.counter2);
//     }
// }

void timer_set_gate(uint8_t idx, uint8_t value) {
    timer_event_t event = 0;
    if(value > 0) {
        event = TIMER_GATE_RISING;
    } else {
        event = TIMER_GATE_FALLING;
    }
    if(idx == 0) {
        process_event(&regs.counter0, event);
    } else if(idx == 1) {
        process_event(&regs.counter1, event);
    } else if(idx == 2) {
        process_event(&regs.counter2, event);
    } else {
        printf("TIMER_SET_GATE invalid index: %d\n", idx);
    }
}

void timer_set_output_cb(uint8_t idx, void (*fun_ptr)(uint8_t)) {
    if(idx == 0) {
        regs.counter0.output_cb = fun_ptr;
    } else if(idx == 1) {
        regs.counter1.output_cb = fun_ptr;
    } else if(idx == 2) {
        regs.counter2.output_cb = fun_ptr;
    } else {
        printf("TIMER_SET_OUTPUT invalid index: %d\n", idx);
    }
}

uint8_t tick_counter;

__declspec(dllexport)
int module_tick(void) {
    if(regs.int_time == 0 && WIRE_LOW == int0_wire.wire_get_state()) {
        int0_wire.wire_set_state(WIRE_HIGH);
        regs.int_time = 0;
    }
    if(regs.int_time > 0 && regs.int_time < 0xFF) {
        regs.int_time --;
    }
    if(tick_counter > 0) {
        if(regs.counter0.is_counting == 1) {
            counter_tick(&regs.counter0);
        }
        if(regs.counter1.is_counting == 1) {
            counter_tick(&regs.counter1);
        }
        if(regs.counter2.is_counting == 1) {
            counter_tick(&regs.counter2);
        }
        tick_counter = 0;
    } else {
        tick_counter ++;
    }
    return 0;
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