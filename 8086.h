#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int init_cpu(uint8_t *memory);
int cpu_tick(void);
