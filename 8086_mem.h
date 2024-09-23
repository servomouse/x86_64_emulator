#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


uint8_t * mem_init(uint8_t continue_simulation);
void mem_write(uint32_t addr, uint16_t value, uint8_t width);
uint16_t mem_read(uint32_t addr, uint8_t width);
int store_memory(void);