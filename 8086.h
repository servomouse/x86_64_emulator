#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "utils.h"

int init_cpu(uint8_t continue_simulation);
int cpu_tick(void);
void cpu_save_state(void);
void set_int_vector(uint8_t vector);
void set_delayed_int(uint8_t vector, uint16_t dely_ticks);

// API functions:
void connect_address_space(uint8_t space_type, WRITE_FUNC_PTR(write_func), READ_FUNC_PTR(read_func));
void module_reset(void);
void module_save(void);
void module_restore(void);
int module_tick(void);
