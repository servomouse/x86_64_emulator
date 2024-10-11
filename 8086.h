#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int init_cpu(uint8_t continue_simulation);
int cpu_tick(void);
void cpu_save_state(void);
void set_int_vector(uint8_t vector);
