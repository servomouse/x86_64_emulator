#include "8253_timer.h"
#include "8086.h"
#include "utils.h"

#define TIMER_LOG_FILE "logs/timer.log"

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

counter_t counter0;
counter_t counter1;
counter_t counter2;

static void process_event(counter_t *timer, timer_event_t event) {
    uint8_t mode = (timer->mode >> 1) & 0x07;
    mylog(TIMER_LOG_FILE, "Timer %d mode = %d\n", timer->idx, mode);
    if(event == TIMER_WRITE_EVENT) {
        if((mode != 1) && (mode != 5)) {
            mylog(TIMER_LOG_FILE, "Timer %d start counting\n", timer->idx);
            timer->is_counting = 1;
        }
        if(mode == 0) {
            mylog(TIMER_LOG_FILE, "Timer %d stop counting\n", timer->idx);
           (*(timer->output_cb))(0);
        }
    } else if(event == TIMER_GATE_FALLING) {
        if((mode == 0) || (mode == 1) || (mode == 4) || (mode == 5)) {
            mylog(TIMER_LOG_FILE, "Timer %d stop counting\n", timer->idx);
            timer->is_counting = 0;
        }
    } else if(event == TIMER_GATE_RISING) {
        if((mode == 0) || (mode == 1) || (mode == 4) || (mode == 5)) {
            mylog(TIMER_LOG_FILE, "Timer %d start counting\n", timer->idx);
            timer->is_counting = 1;
        }
    } else {
        printf("TIMER_EVENT ERROR: invalid event %d\n", event);
    }
}

static void dummy_cb(uint8_t a) {
    return;
}

uint8_t timer_init(void) {
    counter0.idx = 0;
    counter1.idx = 1;
    counter2.idx = 2;
    counter0.value = 0;
    counter0.mode = 0;
    counter0.read_counter = 0;
    counter0.write_counter = 0;
    counter0.is_counting = 0;
    counter0.output_cb = &set_int_vector;
    counter1.value = 0;
    counter1.mode = 0;
    counter1.read_counter = 0;
    counter1.write_counter = 0;
    counter1.is_counting = 0;
    counter1.output_cb = &dummy_cb;
    counter2.value = 0;
    counter2.mode = 0;
    counter2.read_counter = 0;
    counter2.write_counter = 0;
    counter2.is_counting = 0;
    counter2.output_cb = &dummy_cb;
    return 0;
}

static void set_value(counter_t *timer, uint16_t value) {
    uint8_t submode = (timer->mode >> 4) & 0x03;
    // uint16_t ret_val = orig_value;
    if(submode == 1) {
        timer->value &= 0xFF00;
        timer->value |= value & 0xFF;
        timer->write_counter = 0;
        mylog(TIMER_LOG_FILE, "Set timer %d value to 0x%04X\n", timer->idx, timer->value);
        process_event(timer, TIMER_WRITE_EVENT);
    } else if(submode == 2) {
        timer->value &= 0x00FF;
        timer->value |= value << 8;
        timer->write_counter = 0;
        mylog(TIMER_LOG_FILE, "Set timer %d value to 0x%04X\n", timer->idx, timer->value);
        process_event(timer, TIMER_WRITE_EVENT);
    } else {
        if(timer->write_counter == 0) {
            timer->value &= 0xFF00;
            timer->value |= value & 0xFF;
            timer->write_counter += 1;
            mylog(TIMER_LOG_FILE, "Set timer %d value to 0x%04X\n", timer->idx, timer->value);
        } else {
            timer->value &= 0x00FF;
            timer->value |= value << 8;
            timer->write_counter = 0;
            mylog(TIMER_LOG_FILE, "Set timer %d value to 0x%04X\n", timer->idx, timer->value);
            process_event(timer, TIMER_WRITE_EVENT);
        }
    }
    timer->inner_value = timer->value;
}

uint8_t timer_write(uint32_t addr, uint16_t value, uint8_t width) {
    mylog(TIMER_LOG_FILE, "TIMER_WRITE addr = 0x%06X, value = 0x%04X, width = %d bytes\n", addr, value, width);
    switch(addr) {
        case 0x40:
            set_value(&counter0, value);
            break;
        case 0x41:
            set_value(&counter1, value);
            break;
        case 0x42:
            set_value(&counter2, value);
            break;
        case 0x43: {
            uint8_t counter = (value & 0xFF) >> 6;
            if(counter == 0) {
                mylog(TIMER_LOG_FILE, "Set timer 0 mode to 0x%02X\n", value);
                counter0.mode = value;
            } else if(counter == 1) {
                mylog(TIMER_LOG_FILE, "Set timer 1 mode to 0x%02X\n", value);
                counter1.mode = value;
            } else if(counter == 2) {
                mylog(TIMER_LOG_FILE, "Set timer 2 mode to 0x%02X\n", value);
                counter2.mode = value;
            } else {
                printf("TIMER ERROR: attempt to write to incorrect counter 0x%02X\n", value);
            }
            break;
        }
        default:
            printf("TIMER ERROR: attempt to write to incorrect port 0x%04X\n", addr);
    }
    return 0;
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

uint16_t timer_read(uint32_t addr, uint8_t width) {
    uint16_t ret_val = 0;
    switch(addr) {
        case 0x40:
            ret_val = get_value(counter0.inner_value, counter0.mode, &counter0.read_counter);
            break;
        case 0x41:
            ret_val = get_value(counter1.inner_value, counter1.mode, &counter1.read_counter);
            break;
        case 0x42:
            ret_val = get_value(counter2.inner_value, counter2.mode, &counter2.read_counter);
            break;
        case 0x43:
            ret_val = 0xFF;
            break;
        default:
            printf("TIMER ERROR: attempt to read from incorrect port 0x%04x\n", addr);
    }
    mylog(TIMER_LOG_FILE, "TIMER_READ addr = 0x%04X, width = %d bytes, data = 0x%04X\n", addr, width, ret_val);
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

void timer_tick(void) {
    // printf("Timer tick function");
    if(counter0.is_counting == 1) {
        counter_tick(&counter0);
    }
    if(counter1.is_counting == 1) {
        counter_tick(&counter1);
    }
    if(counter2.is_counting == 1) {
        counter_tick(&counter2);
    }
}

void timer_set_gate(uint8_t idx, uint8_t value) {
    timer_event_t event = 0;
    if(value > 0) {
        event = TIMER_GATE_RISING;
    } else {
        event = TIMER_GATE_FALLING;
    }
    if(idx == 0) {
        process_event(&counter0, event);
    } else if(idx == 1) {
        process_event(&counter1, event);
    } else if(idx == 2) {
        process_event(&counter2, event);
    } else {
        printf("TIMER_SET_GATE invalid index: %d\n", idx);
    }
}

void timer_set_output_cb(uint8_t idx, void (*fun_ptr)(uint8_t)) {
    if(idx == 0) {
        counter0.output_cb = fun_ptr;
    } else if(idx == 1) {
        counter1.output_cb = fun_ptr;
    } else if(idx == 2) {
        counter2.output_cb = fun_ptr;
    } else {
        printf("TIMER_SET_OUTPUT invalid index: %d\n", idx);
    }
}